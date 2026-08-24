// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Validation/ScriptableBindingsValidation.h"

#include "ScriptableObject.h"
#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptablePropertyUtilities.h"
#include "ScriptableFrameworkEditorHelpers.h"
#include "Bindings/ScriptablePropertyBindings.h"
#include "GameFramework/Actor.h"
#include "PropertyBindingDataView.h"
#include "Engine/Blueprint.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "ScriptableBindingsValidation"

namespace
{
	/** Stable tag attached to every issue emitted by this pass. */
	const FName GBindingValidatorId(TEXT("ScriptableBindings"));

	/**
	 * Returns the BindingID the issue should navigate to. UI panels (e.g. the graph editor) can only
	 * resolve UScriptableNode IDs, so for inner objects (a task wrapped by UScriptableNode_Task)
	 * we walk up the outer chain to the enclosing node. Falls back to the object's own ID otherwise.
	 */
	FGuid GetNavigationContextId(const UScriptableObject* Obj)
	{
		if (!Obj) return FGuid();
		if (const UScriptableNode* OuterNode = Obj->GetTypedOuter<UScriptableNode>())
		{
			return OuterNode->GetBindingID();
		}
		return Obj->GetBindingID();
	}

	/**
	 * Where the object lives, for issue messages: the owning actor's outliner label plus the subobject
	 * chain below it (falls back to the package-relative path for non-actor roots). Without this, a
	 * save-time error only names the inner task ("ScriptableTask_X_0") and gives no clue which
	 * Blueprint or placed actor to open.
	 */
	FText GetOwnerContextText(const UScriptableObject* Obj)
	{
		if (!Obj) return LOCTEXT("UnknownOwner", "<unknown>");
		if (const AActor* OwnerActor = Obj->GetTypedOuter<AActor>())
		{
			return FText::FromString(FString::Printf(TEXT("%s: %s"), *OwnerActor->GetActorNameOrLabel(), *Obj->GetPathName(OwnerActor)));
		}
		return FText::FromString(Obj->GetPathName(Obj->GetPackage()));
	}

	/**
	 * True for leftovers of a Blueprint recompile. Trashed components keep hanging off the actor until the
	 * next GC, carrying their old bindings, and validating them reports problems about objects that no
	 * longer run and are never saved.
	 */
	bool IsStaleSubobject(const UObject* Object)
	{
		for (const UObject* Current = Object; Current; Current = Current->GetOuter())
		{
			if (!IsValid(Current) || Current->HasAnyFlags(RF_NewerVersionExists))
			{
				return true;
			}

			const FString Name = Current->GetName();
			if (Name.StartsWith(TEXT("TRASH_")) || Name.StartsWith(TEXT("REINST_")))
			{
				return true;
			}
		}

		return false;
	}

	/** Returns the leaf property name from a target path, or the path's last segment if it cannot be resolved. */
	FText GetTargetDisplayName(const UScriptableObject* Obj, const FPropertyBindingPath& TargetPath)
	{
		const FProperty* TargetProp = nullptr;
		if (Obj && ScriptableFrameworkEditor::ValidateBindingPath(Obj->GetClass(), &TargetPath, TargetProp) && TargetProp)
		{
			return TargetProp->GetDisplayNameText();
		}
		if (TargetPath.NumSegments() > 0)
		{
			return FText::FromName(TargetPath.GetSegment(TargetPath.NumSegments() - 1).GetName());
		}
		return LOCTEXT("UnknownProperty", "<unknown>");
	}
}

void FScriptableBindingsValidation::ValidateBindings(const UObject* Root, TArray<FKzValidationIssue>& OutIssues)
{
	if (!Root) return;

	/** Blueprints store their data on the GeneratedClass CDO; skip until compiled. */
	const UObject* TargetObject = Root;
	if (const UBlueprint* Blueprint = Cast<UBlueprint>(Root))
	{
		if (!Blueprint->GeneratedClass) return;
		TargetObject = Blueprint->GeneratedClass->GetDefaultObject();
	}

	TArray<UObject*> NestedObjects;
	GetObjectsWithOuter(TargetObject, NestedObjects, /*bIncludeNestedObjects*/ true);

	for (UObject* RawObj : NestedObjects)
	{
		UScriptableObject* Obj = Cast<UScriptableObject>(RawObj);
		if (!Obj || IsStaleSubobject(Obj)) continue;

		/**
		 * Read-only pass. SanitizeObsoleteBindings would wipe valid bindings without re-baking,
		 * so it belongs to BakeAutoBindings (PreSave/edit), not here.
		 */

		TArray<FPropertyBindingBindableStructDescriptor> AccessibleStructs;
		FScriptablePropertyUtilities::GatherAccessibleStructs(Obj, AccessibleStructs);

		const FScriptablePropertyBindings& Bindings = Obj->GetPropertyBindings();
		const FGuid NavId = GetNavigationContextId(Obj);

		/** (A) Unbound-required checks: only properties classified as Input/Context. */
		for (TFieldIterator<FProperty> It(Obj->GetClass()); It; ++It)
		{
			const FProperty* Prop = *It;
			const bool bIsInput = FScriptablePropertyUtilities::IsPropertyBindableInput(Prop);
			const bool bIsContext = FScriptablePropertyUtilities::IsPropertyBindableContext(Prop);
			if (!bIsInput && !bIsContext) continue;

			FPropertyBindingPath TargetPath;
			TargetPath.SetStructID(Obj->GetBindingID());
			TargetPath.AddPathSegment(Prop->GetFName());

			if (Bindings.HasManualPropertyBinding(TargetPath)) continue;

			if (bIsInput)
			{
				const FText Msg = FText::Format(
					LOCTEXT("MissingInputBindingError", "'{0}': Input '{1}' must be connected to a value."),
					GetOwnerContextText(Obj),
					Prop->GetDisplayNameText());
				OutIssues.Add(FKzValidationIssue::WithContextId(EKzValidationSeverity::Error, Msg, GBindingValidatorId, NavId));
			}
			else
			{
				FPropertyBindingPath AutoBindingPath;
				if (!FScriptablePropertyUtilities::FindAutoBindingPath(Prop, AccessibleStructs, AutoBindingPath))
				{
					const FText Msg = FText::Format(
						LOCTEXT("MissingContextBindingError", "'{0}': Context '{1}' could not be auto-resolved and requires a manual wire."),
						GetOwnerContextText(Obj),
						Prop->GetDisplayNameText());
					OutIssues.Add(FKzValidationIssue::WithContextId(EKzValidationSeverity::Error, Msg, GBindingValidatorId, NavId));
				}
			}
		}

		/**
		 * (B) Obsolete-binding checks: iterate the binding table directly. Any property may carry a manual
		 * binding (Config/Hidden/etc. — not only Input/Context), so the binding table is the authoritative
		 * source. Auto-bindings are skipped: they are baked from current state, so cannot be obsolete.
		 */
		for (const FScriptablePropertyBinding& Binding : Bindings.Bindings)
		{
			if (Binding.bIsAutoBinding) continue;

			const FText TargetName = GetTargetDisplayName(Obj, Binding.TargetPath);

			/** Shared quick-fix for every obsolete-binding variant below: drop this binding from the owning object. */
			TWeakObjectPtr<UScriptableObject> WeakObj(Obj);
			FPropertyBindingPath CapturedTarget = Binding.TargetPath;
			auto BuildIssue = [&](const FText& Msg)
			{
				FKzValidationIssue Issue = FKzValidationIssue::WithContextId(EKzValidationSeverity::Error, Msg, GBindingValidatorId, NavId);
				Issue.QuickFixLabel = LOCTEXT("ClearStaleBinding", "Clear");
				Issue.QuickFix = [WeakObj, CapturedTarget]()
				{
					UScriptableObject* SObj = WeakObj.Get();
					if (!SObj) return;
					const FScopedTransaction Tx(LOCTEXT("QF_ClearStaleBinding", "Clear stale binding"));
					SObj->Modify();
					SObj->GetPropertyBindings().RemovePropertyBindings(CapturedTarget);
					/** Broadcast OnObjectPropertyChanged so the editor marks dirty, reconstructs the slate node, and refreshes details. */
					SObj->PostEditChange();
				};
				return Issue;
			};

			const FProperty* TargetProp = nullptr;
			bool bTargetValid = ScriptableFrameworkEditor::ValidateBindingPath(Obj->GetClass(), &Binding.TargetPath, TargetProp);
			if (!bTargetValid)
			{
				/** The static walk cannot see into instance property bags (dynamic layout); retry against live memory. */
				void* TargetAddr = nullptr;
				TArray<TSharedPtr<FStructOnScope>> TempMemory;
				bTargetValid = FScriptablePropertyBindings::ResolvePath(Binding.TargetPath, FPropertyBindingDataView(Obj), TargetProp, TargetAddr, TempMemory);
			}
			if (!bTargetValid || !TargetProp)
			{
				OutIssues.Add(BuildIssue(FText::Format(
					LOCTEXT("ObsoleteBindingTargetGone", "'{0}': Binding targets property '{1}' that no longer exists."),
					GetOwnerContextText(Obj),
					TargetName)));
				continue;
			}

			const FPropertyBindingBindableStructDescriptor* SourceDesc = AccessibleStructs.FindByPredicate(
				[&](const FPropertyBindingBindableStructDescriptor& Desc) { return Desc.ID == Binding.SourcePath.GetStructID(); });

			if (!SourceDesc)
			{
				OutIssues.Add(BuildIssue(FText::Format(
					LOCTEXT("ObsoleteBindingSourceContext", "'{0}': Binding on '{1}' points to a source context that no longer exists."),
					GetOwnerContextText(Obj),
					TargetName)));
				continue;
			}

			const FProperty* SourceProp = nullptr;
			const bool bSourceValid = ScriptableFrameworkEditor::ValidateBindingPath(SourceDesc->Struct.Get(), &Binding.SourcePath, SourceProp);
			if (!bSourceValid || !SourceProp)
			{
				OutIssues.Add(BuildIssue(FText::Format(
					LOCTEXT("ObsoleteBindingSourceProp", "'{0}': Binding on '{1}' points to a source property that no longer exists."),
					GetOwnerContextText(Obj),
					TargetName)));
				continue;
			}

			if (!FScriptablePropertyUtilities::ArePropertiesCompatible(SourceProp, TargetProp))
			{
				OutIssues.Add(BuildIssue(FText::Format(
					LOCTEXT("ObsoleteBindingTypeMismatch", "'{0}': Binding on '{1}' has an incompatible source type."),
					GetOwnerContextText(Obj),
					TargetName)));
			}
		}

		/** (C) Per-object semantic validation (task-specific checks the generic passes cannot know). */
		{
			TArray<FKzValidationIssue> ObjectIssues;
			Obj->ValidateObject(ObjectIssues);
			for (FKzValidationIssue& Issue : ObjectIssues)
			{
				if (!Issue.ContextId.IsValid()) Issue.ContextId = NavId;
			}
			OutIssues.Append(MoveTemp(ObjectIssues));
		}
	}
}

#undef LOCTEXT_NAMESPACE
