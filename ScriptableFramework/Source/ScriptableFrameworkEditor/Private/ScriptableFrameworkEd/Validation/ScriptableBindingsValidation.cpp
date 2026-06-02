// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Validation/ScriptableBindingsValidation.h"

#include "ScriptableObject.h"
#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptablePropertyUtilities.h"
#include "ScriptableFrameworkEditorHelpers.h"
#include "Bindings/ScriptablePropertyBindings.h"
#include "Engine/Blueprint.h"
#include "UObject/UnrealType.h"

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
		if (!Obj) continue;

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
					FText::FromString(Obj->GetName()),
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
						FText::FromString(Obj->GetName()),
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

			const FProperty* TargetProp = nullptr;
			const bool bTargetValid = ScriptableFrameworkEditor::ValidateBindingPath(Obj->GetClass(), &Binding.TargetPath, TargetProp);
			if (!bTargetValid || !TargetProp)
			{
				const FText Msg = FText::Format(
					LOCTEXT("ObsoleteBindingTargetGone", "'{0}': Binding targets property '{1}' that no longer exists."),
					FText::FromString(Obj->GetName()),
					TargetName);
				OutIssues.Add(FKzValidationIssue::WithContextId(EKzValidationSeverity::Error, Msg, GBindingValidatorId, NavId));
				continue;
			}

			const FPropertyBindingBindableStructDescriptor* SourceDesc = AccessibleStructs.FindByPredicate(
				[&](const FPropertyBindingBindableStructDescriptor& Desc) { return Desc.ID == Binding.SourcePath.GetStructID(); });

			if (!SourceDesc)
			{
				const FText Msg = FText::Format(
					LOCTEXT("ObsoleteBindingSourceContext", "'{0}': Binding on '{1}' points to a source context that no longer exists."),
					FText::FromString(Obj->GetName()),
					TargetName);
				OutIssues.Add(FKzValidationIssue::WithContextId(EKzValidationSeverity::Error, Msg, GBindingValidatorId, NavId));
				continue;
			}

			const FProperty* SourceProp = nullptr;
			const bool bSourceValid = ScriptableFrameworkEditor::ValidateBindingPath(SourceDesc->Struct.Get(), &Binding.SourcePath, SourceProp);
			if (!bSourceValid || !SourceProp)
			{
				const FText Msg = FText::Format(
					LOCTEXT("ObsoleteBindingSourceProp", "'{0}': Binding on '{1}' points to a source property that no longer exists."),
					FText::FromString(Obj->GetName()),
					TargetName);
				OutIssues.Add(FKzValidationIssue::WithContextId(EKzValidationSeverity::Error, Msg, GBindingValidatorId, NavId));
				continue;
			}

			if (!FScriptablePropertyUtilities::ArePropertiesCompatible(SourceProp, TargetProp))
			{
				const FText Msg = FText::Format(
					LOCTEXT("ObsoleteBindingTypeMismatch", "'{0}': Binding on '{1}' has an incompatible source type."),
					FText::FromString(Obj->GetName()),
					TargetName);
				OutIssues.Add(FKzValidationIssue::WithContextId(EKzValidationSeverity::Error, Msg, GBindingValidatorId, NavId));
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
