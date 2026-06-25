// Copyright 2026 kirzo

#include "ScriptableTasks/ScriptableTask_CallFunction.h"
#include "ScriptablePropertyUtilities.h"
#include "Core/KzNamedVariant.h"
#include "StructUtils/PropertyBag.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

#if WITH_EDITOR
#include "UObject/ObjectSaveContext.h"
#endif

namespace
{
	// Declared input parameters. Excludes the return value and true out params. A const-ref input also
	// carries CPF_OutParm in UE, so it is kept (distinguished by CPF_ConstParm).
	bool IsInputParam(const FProperty* Param)
	{
		if (!Param || !Param->HasAnyPropertyFlags(CPF_Parm) || Param->HasAnyPropertyFlags(CPF_ReturnParm)) return false;
		return !Param->HasAnyPropertyFlags(CPF_OutParm) || Param->HasAnyPropertyFlags(CPF_ConstParm);
	}
}

void UScriptableTask_CallFunction::BeginTask()
{
	UObject* CallTarget = Target;
	if (!CallTarget || FunctionName.IsNone())
	{
		UE_LOG(LogScriptableTask, Warning, TEXT("%s: call skipped (no target object or no function selected)."), *GetName());
		Finish();
		return;
	}

	if (TargetClass && !CallTarget->IsA(TargetClass))
	{
		UE_LOG(LogScriptableTask, Warning, TEXT("%s: call skipped ('%s' is not a '%s')."), *GetName(), *CallTarget->GetName(), *TargetClass->GetName());
		Finish();
		return;
	}

	UFunction* Function = CallTarget->GetClass()->FindFunctionByName(FunctionName);
	if (!Function)
	{
		UE_LOG(LogScriptableTask, Warning, TEXT("%s: call skipped (function '%s' not found on '%s')."), *GetName(), *FunctionName.ToString(), *CallTarget->GetClass()->GetName());
		Finish();
		return;
	}

	FStructOnScope Frame(Function);
	uint8* FrameMemory = Frame.GetStructMemory();

	// Fill input params from the bag. A missing or incompatible member keeps the zero-initialized
	// default; Blueprint pin defaults are call-site metadata and do not apply to reflection calls.
	if (const UPropertyBag* BagStruct = Parameters.GetPropertyBagStruct())
	{
		uint8* BagMemory = Parameters.GetMutableValue().GetMemory();
		for (TFieldIterator<FProperty> It(Function); It; ++It)
		{
			const FProperty* Param = *It;
			if (!IsInputParam(Param)) continue;

			const FPropertyBagPropertyDesc* Desc = BagStruct->FindPropertyDescByName(Param->GetFName());
			if (!Desc || !Desc->CachedProperty) continue;

			FScriptablePropertyUtilities::CopyPropertyValue(Desc->CachedProperty, Desc->CachedProperty->ContainerPtrToValuePtr<void>(BagMemory), Param, Param->ContainerPtrToValuePtr<void>(FrameMemory));
		}
	}

	CallTarget->ProcessEvent(Function, FrameMemory);

	WriteReturnValueToLocal(Function, FrameMemory);

	Finish();
}

void UScriptableTask_CallFunction::WriteReturnValueToLocal(const UFunction* Function, uint8* FrameMemory)
{
	if (ResultLocal.IsNone()) return;

	const FProperty* ReturnProp = Function->GetReturnProperty();
	if (!ReturnProp)
	{
		UE_LOG(LogScriptableTask, Warning, TEXT("%s: ResultLocal '%s' is set but '%s' returns nothing."), *GetName(), *ResultLocal.ToString(), *Function->GetName());
		return;
	}

	FInstancedPropertyBag* Locals = GetMutableLocals();
	const UPropertyBag* LocalsStruct = Locals ? Locals->GetPropertyBagStruct() : nullptr;
	const FPropertyBagPropertyDesc* LocalDesc = LocalsStruct ? LocalsStruct->FindPropertyDescByName(ResultLocal) : nullptr;
	if (!LocalDesc || !LocalDesc->CachedProperty)
	{
		UE_LOG(LogScriptableTask, Warning, TEXT("%s: Local '%s' not found; return value of '%s' discarded."), *GetName(), *ResultLocal.ToString(), *Function->GetName());
		return;
	}

	uint8* LocalsMemory = Locals->GetMutableValue().GetMemory();
	FScriptablePropertyUtilities::CopyPropertyValue(ReturnProp, ReturnProp->ContainerPtrToValuePtr<void>(FrameMemory), LocalDesc->CachedProperty, LocalDesc->CachedProperty->ContainerPtrToValuePtr<void>(LocalsMemory));
}

#if WITH_EDITOR
FText UScriptableTask_CallFunction::GetDisplayTitle() const
{
	if (FunctionName.IsNone()) return INVTEXT("Call Function");

	// Show it code-style as "Target.Function()" when the Target is bound (e.g. "Quiz.ShowCurrentQuestion()").
	FString TargetName;
	if (GetBindingDisplayText(GET_MEMBER_NAME_CHECKED(UScriptableTask_CallFunction, Target), TargetName))
	{
		return FText::Format(INVTEXT("{0}.{1}()"), FText::FromString(TargetName), FText::FromName(FunctionName));
	}

	return FText::Format(INVTEXT("Call Function: {0}()"), FText::FromName(FunctionName));
}

void UScriptableTask_CallFunction::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropName = PropertyChangedEvent.GetPropertyName();
	if (PropName == GET_MEMBER_NAME_CHECKED(UScriptableTask_CallFunction, TargetClass))
	{
		// Keep the selection only if the new class still offers the same function.
		if (!IsFunctionExposable(ResolveFunction()))
		{
			FunctionName = NAME_None;
		}
		SyncParametersBag();
		SanitizeResultLocal();
	}
	else if (PropName == GET_MEMBER_NAME_CHECKED(UScriptableTask_CallFunction, FunctionName))
	{
		SyncParametersBag();
		SanitizeResultLocal();
	}
}

void UScriptableTask_CallFunction::PostBindingChanged(FName TargetPropertyName)
{
	Super::PostBindingChanged(TargetPropertyName);

	if (TargetPropertyName != GET_MEMBER_NAME_CHECKED(UScriptableTask_CallFunction, Target))
	{
		return;
	}

	// Drop an explicit TargetClass that no longer narrows the bound Target type (binding cleared or
	// rebound to an unrelated type).
	if (TargetClass)
	{
		const UClass* Bound = GetBoundTargetClass();
		if (!Bound || (!TargetClass->IsChildOf(Bound) && !Bound->IsChildOf(TargetClass)))
		{
			TargetClass = nullptr;
		}
	}

	// The effective class may have changed; drop a FunctionName that is no longer callable on it.
	if (!FunctionName.IsNone() && !IsFunctionExposable(ResolveFunction()))
	{
		FunctionName = NAME_None;
	}

	SyncParametersBag();
	SanitizeResultLocal();
}

void UScriptableTask_CallFunction::PreSave(FObjectPreSaveContext SaveContext)
{
	// Absorb signature drift (e.g. a recompiled Blueprint) before bindings are baked and validated.
	SyncParametersBag();
	Super::PreSave(SaveContext);
}

void UScriptableTask_CallFunction::ValidateObject(TArray<FKzValidationIssue>& OutIssues) const
{
	static const FName GCallFunctionValidatorId(TEXT("ScriptableCallFunction"));

	if (FunctionName.IsNone())
	{
		OutIssues.Add(FKzValidationIssue(EKzValidationSeverity::Warning,
			FText::Format(INVTEXT("'{0}': no function selected; the task will do nothing."), FText::FromString(GetName())),
			GCallFunctionValidatorId));
		return;
	}

	const UClass* EffectiveClass = GetEffectiveTargetClass();
	if (!EffectiveClass)
	{
		// No explicit TargetClass and Target's bound type is unknown; the function is resolved on the
		// actual Target at runtime, so it cannot be verified here.
		return;
	}

	const UFunction* Function = EffectiveClass->FindFunctionByName(FunctionName);
	if (!Function || !IsFunctionExposable(Function))
	{
		OutIssues.Add(FKzValidationIssue(EKzValidationSeverity::Error,
			FText::Format(INVTEXT("'{0}': function '{1}' is missing on '{2}' or is no longer callable from here."),
				FText::FromString(GetName()), FText::FromName(FunctionName),
				EffectiveClass->GetDisplayNameText()),
			GCallFunctionValidatorId));
		return;
	}

	// An explicit TargetClass the bound Target can never satisfy makes the runtime IsA guard skip the call.
	if (TargetClass)
	{
		const UClass* BoundClass = GetBoundTargetClass();
		if (BoundClass && !TargetClass->IsChildOf(BoundClass) && !BoundClass->IsChildOf(TargetClass))
		{
			OutIssues.Add(FKzValidationIssue(EKzValidationSeverity::Error,
				FText::Format(INVTEXT("'{0}': TargetClass '{1}' is unrelated to the bound Target type '{2}'; the call will be skipped at runtime."),
					FText::FromString(GetName()), TargetClass->GetDisplayNameText(), BoundClass->GetDisplayNameText()),
				GCallFunctionValidatorId));
		}
	}

	// Parameters drift vs the live signature. PreSave resyncs on save, but validation also runs live.
	if (!AreParametersInSync(Parameters.GetPropertyBagStruct(), Function))
	{
		FKzValidationIssue Issue(EKzValidationSeverity::Error,
			FText::Format(INVTEXT("'{0}': Parameters are out of sync with the signature of '{1}'."),
				FText::FromString(GetName()), FText::FromName(FunctionName)),
			GCallFunctionValidatorId);
		Issue.QuickFixLabel = INVTEXT("Sync");
		TWeakObjectPtr<UScriptableTask_CallFunction> WeakThis(const_cast<UScriptableTask_CallFunction*>(this));
		Issue.QuickFix = [WeakThis]()
		{
			UScriptableTask_CallFunction* Task = WeakThis.Get();
			if (!Task) return;
			Task->Modify();
			Task->SyncParametersBag();
			Task->PostEditChange();
		};
		OutIssues.Add(MoveTemp(Issue));
	}

	if (!ResultLocal.IsNone())
	{
		if (!Function->GetReturnProperty())
		{
			OutIssues.Add(FKzValidationIssue(EKzValidationSeverity::Warning,
				FText::Format(INVTEXT("'{0}': Result Local '{1}' is set but '{2}' returns nothing."),
					FText::FromString(GetName()), FText::FromName(ResultLocal), FText::FromName(FunctionName)),
				GCallFunctionValidatorId));
			return;
		}

		const FKzNamedVariant* Local = nullptr;
		for (const FKzNamedVariant& Var : FScriptablePropertyUtilities::FindLocalsDeclarationFor(this))
		{
			if (Var.GetName() == ResultLocal) { Local = &Var; break; }
		}

		if (!Local)
		{
			OutIssues.Add(FKzValidationIssue(EKzValidationSeverity::Error,
				FText::Format(INVTEXT("'{0}': Result Local '{1}' does not exist."),
					FText::FromString(GetName()), FText::FromName(ResultLocal)),
				GCallFunctionValidatorId));
			return;
		}

		// The dropdown only offers compatible Locals, but the Local's type can change after selection.
		if (!CanLocalReceiveReturnValue(Function, *Local))
		{
			OutIssues.Add(FKzValidationIssue(EKzValidationSeverity::Warning,
				FText::Format(INVTEXT("'{0}': return type of '{1}' does not match Result Local '{2}'; the value will be discarded at runtime."),
					FText::FromString(GetName()), FText::FromName(FunctionName), FText::FromName(ResultLocal)),
				GCallFunctionValidatorId));
		}
	}
}

UClass* UScriptableTask_CallFunction::GetBoundTargetClass() const
{
	const FProperty* TargetProperty = GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UScriptableTask_CallFunction, Target));
	return FScriptablePropertyUtilities::ResolveBoundSourceClass(this, TargetProperty);
}

UClass* UScriptableTask_CallFunction::GetEffectiveTargetClass() const
{
	return TargetClass ? TargetClass.Get() : GetBoundTargetClass();
}

const UFunction* UScriptableTask_CallFunction::ResolveFunction() const
{
	const UClass* Class = GetEffectiveTargetClass();
	return Class ? Class->FindFunctionByName(FunctionName) : nullptr;
}

TArray<FString> UScriptableTask_CallFunction::GetFunctionNames() const
{
	TArray<FString> Names;
	const UClass* Class = GetEffectiveTargetClass();
	if (!Class) return Names;

	// IncludeSuper iteration revisits overridden functions on parent classes; AddUnique collapses them.
	for (TFieldIterator<UFunction> It(Class); It; ++It)
	{
		if (IsFunctionExposable(*It))
		{
			Names.AddUnique(It->GetName());
		}
	}
	Names.Sort();
	return Names;
}

TArray<FString> UScriptableTask_CallFunction::GetLocalNames() const
{
	TArray<FString> Names;
	const UFunction* Function = ResolveFunction();
	for (const FKzNamedVariant& Var : FScriptablePropertyUtilities::FindLocalsDeclarationFor(this))
	{
		if (!Var.GetName().IsNone() && CanLocalReceiveReturnValue(Function, Var)) Names.Add(Var.GetName().ToString());
	}
	return Names;
}

bool UScriptableTask_CallFunction::IsFunctionExposable(const UFunction* Func)
{
	if (!Func) return false;
	if (!Func->HasAnyFunctionFlags(FUNC_BlueprintCallable)) return false;
	if (Func->HasAnyFunctionFlags(FUNC_Static | FUNC_EditorOnly)) return false;
	if (Func->HasMetaData(TEXT("DeprecatedFunction"))) return false;

	for (TFieldIterator<FProperty> It(Func); It; ++It)
	{
		const FProperty* Param = *It;
		if (!Param->HasAnyPropertyFlags(CPF_Parm) || Param->HasAnyPropertyFlags(CPF_ReturnParm)) continue;

		// True out params (non-const by-ref) have no place to write back to. A const-ref input also
		// carries CPF_OutParm in UE, so allow it (distinguished by CPF_ConstParm).
		if (Param->HasAnyPropertyFlags(CPF_OutParm) && !Param->HasAnyPropertyFlags(CPF_ConstParm)) return false;

		// Every input must be representable as a property bag member.
		if (FPropertyBagPropertyDesc(Param->GetFName(), Param).ValueType == EPropertyBagPropertyType::None) return false;
	}
	return true;
}

bool UScriptableTask_CallFunction::CanLocalReceiveReturnValue(const UFunction* Function, const FKzNamedVariant& Local)
{
	const FProperty* ReturnProp = Function ? Function->GetReturnProperty() : nullptr;
	if (!ReturnProp) return false;

	// Mirror CopyPropertyValue's tolerance: exact type, numeric/bool conversions, or related object classes.
	const FPropertyBagPropertyDesc ReturnDesc(ReturnProp->GetFName(), ReturnProp);
	const EPropertyBagPropertyType LocalType = Local.GetValue().GetType();
	const UObject* LocalTypeObject = Local.GetValue().GetTypeObject();

	if (ReturnDesc.ValueType == LocalType && ReturnDesc.ValueTypeObject == LocalTypeObject) return true;

	auto IsNumericish = [](EPropertyBagPropertyType Type)
	{
		return Type == EPropertyBagPropertyType::Bool || Type == EPropertyBagPropertyType::Byte
			|| Type == EPropertyBagPropertyType::Int32 || Type == EPropertyBagPropertyType::Int64
			|| Type == EPropertyBagPropertyType::Float || Type == EPropertyBagPropertyType::Double;
	};
	if (IsNumericish(ReturnDesc.ValueType) && IsNumericish(LocalType)) return true;

	if (ReturnDesc.ValueType == EPropertyBagPropertyType::Object && LocalType == EPropertyBagPropertyType::Object)
	{
		const UClass* ReturnClass = Cast<UClass>(ReturnDesc.ValueTypeObject);
		const UClass* LocalClass = Cast<UClass>(LocalTypeObject);
		return ReturnClass && LocalClass && (ReturnClass->IsChildOf(LocalClass) || LocalClass->IsChildOf(ReturnClass));
	}
	return false;
}

const UPropertyBag* UScriptableTask_CallFunction::BuildParametersBagStruct(const UFunction* Function)
{
	if (!Function) return nullptr;

	TArray<FPropertyBagPropertyDesc> Descs;
	for (TFieldIterator<FProperty> It(Function); It; ++It)
	{
		if (IsInputParam(*It))
		{
			Descs.Emplace((*It)->GetFName(), *It);
		}
	}
	return Descs.IsEmpty() ? nullptr : UPropertyBag::GetOrCreateFromDescs(Descs);
}

bool UScriptableTask_CallFunction::AreParametersInSync(const UPropertyBag* BagStruct, const UFunction* Function)
{
	TArray<FPropertyBagPropertyDesc> Expected;
	if (Function)
	{
		for (TFieldIterator<FProperty> It(Function); It; ++It)
		{
			if (IsInputParam(*It))
			{
				Expected.Emplace((*It)->GetFName(), *It);
			}
		}
	}

	const TConstArrayView<FPropertyBagPropertyDesc> Current = BagStruct ? BagStruct->GetPropertyDescs() : TConstArrayView<FPropertyBagPropertyDesc>();
	if (Current.Num() != Expected.Num()) return false;

	for (int32 i = 0; i < Expected.Num(); ++i)
	{
		// CompatibleType alone accepts related object classes; require the exact type object so a
		// param type change (e.g. AActor -> APawn) still counts as drift.
		if (Current[i].Name != Expected[i].Name || !Current[i].CompatibleType(Expected[i]) || Current[i].ValueTypeObject != Expected[i].ValueTypeObject) return false;
	}
	return true;
}

void UScriptableTask_CallFunction::SanitizeResultLocal()
{
	if (ResultLocal.IsNone()) return;

	const UFunction* Function = ResolveFunction();
	for (const FKzNamedVariant& Var : FScriptablePropertyUtilities::FindLocalsDeclarationFor(this))
	{
		if (Var.GetName() == ResultLocal)
		{
			if (CanLocalReceiveReturnValue(Function, Var)) return;
			break;
		}
	}
	ResultLocal = NAME_None;
}

void UScriptableTask_CallFunction::SyncParametersBag()
{
	const UFunction* Function = ResolveFunction();
	if (!Function || !IsFunctionExposable(Function))
	{
		Parameters.Reset();
		return;
	}

	// Shape-compare before migrating: migration rebuilds the bag struct with fresh desc IDs, which
	// would churn the saved asset on every save even when nothing changed.
	if (AreParametersInSync(Parameters.GetPropertyBagStruct(), Function))
	{
		return;
	}

	const UPropertyBag* NewBagStruct = BuildParametersBagStruct(Function);
	if (!NewBagStruct)
	{
		Parameters.Reset();
	}
	else
	{
		Parameters.MigrateToNewBagStruct(NewBagStruct);
	}
}
#endif