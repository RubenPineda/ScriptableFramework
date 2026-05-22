// Copyright 2026 kirzo

#include "ScriptableBlueprintLibrary.h"
#include "ScriptablePropertyUtilities.h"
#include "StructUtils/PropertyBag.h"
#include "Core/KzBagOps.h"

// Steps the wildcard value off the BP stack and assigns it into a bag property by name.
// If bAddIfMissing is true and the property does not exist yet, it is defined from the value's
// type (mirrors FScriptableContext::SetProperty<T>). Always finishes the stack frame.
static void AssignStackValueToBag(FFrame& Stack, FInstancedPropertyBag& Bag, FName ParameterName, bool bAddIfMissing, const FString& FunctionName)
{
	const UScriptStruct* BagStruct = Bag.GetPropertyBagStruct();
	const FProperty* BagProp = BagStruct ? BagStruct->FindPropertyByName(ParameterName) : nullptr;

	// The bag has no such property yet (e.g. a context freshly created in Blueprint has a null bag).
	// If allowed, define the property from the incoming value's type, which also initializes the bag.
	if (!BagProp && bAddIfMissing)
	{
		Stack.MostRecentPropertyAddress = nullptr;
		Stack.MostRecentProperty = nullptr;
		Stack.StepCompiledIn<FProperty>(nullptr);

		FProperty* ValueProp = Stack.MostRecentProperty;
		void* ValuePtr = Stack.MostRecentPropertyAddress;

		P_FINISH;

		if (!ValueProp || !ValuePtr)
		{
#if WITH_EDITOR
			FFrame::KismetExecutionMessage(*FString::Printf(TEXT("%s: Could not define parameter '%s' from the supplied value. Define it explicitly with AddScriptableContextProperty first."), *FunctionName, *ParameterName.ToString()), ELogVerbosity::Warning);
#endif
			return;
		}

		// Define the property from the value's type, then copy the value into the freshly created slot.
		Bag.AddProperty(ParameterName, ValueProp);

		const UScriptStruct* NewBagStruct = Bag.GetPropertyBagStruct();
		const FProperty* NewBagProp = NewBagStruct ? NewBagStruct->FindPropertyByName(ParameterName) : nullptr;
		uint8* StructMemory = Bag.GetMutableValue().GetMemory();

		if (NewBagProp && StructMemory)
		{
			uint8* DestPtr = NewBagProp->ContainerPtrToValuePtr<uint8>(StructMemory);
			NewBagProp->CopyCompleteValue(DestPtr, ValuePtr);
		}
		return;
	}

	if (!BagProp)
	{
		// Step to clear the stack
		Stack.StepCompiledIn<FProperty>(nullptr);
		P_FINISH;
#if WITH_EDITOR
		FFrame::KismetExecutionMessage(*FString::Printf(TEXT("%s: Parameter '%s' not found in the context."), *FunctionName, *ParameterName.ToString()), ELogVerbosity::Warning);
#endif
		return;
	}

	// Evaluate Value into a local buffer based on the expected BagProp size.
	// This PREVENTS the "Attempted to reference 'self' as an addressable property" crash
	// by giving R-Values (like 'self' or Math functions) a valid memory space to write into.
	void* LocalValue = FMemory_Alloca(BagProp->GetSize());
	BagProp->InitializeValue(LocalValue);

	Stack.MostRecentPropertyAddress = nullptr;
	Stack.MostRecentProperty = nullptr;

	Stack.StepCompiledIn<FProperty>(LocalValue);

	FProperty* ValueProp = Stack.MostRecentProperty;
	void* ValuePtr = (Stack.MostRecentPropertyAddress != nullptr) ? Stack.MostRecentPropertyAddress : LocalValue;

	P_FINISH;

	FStructView MutableStruct = Bag.GetMutableValue();
	uint8* StructMemory = MutableStruct.GetMemory();

	if (StructMemory)
	{
		uint8* DestPtr = BagProp->ContainerPtrToValuePtr<uint8>(StructMemory);

		// If ValueProp is valid, do the rigorous compatibility check
		if (ValueProp && !FScriptablePropertyUtilities::ArePropertiesCompatible(ValueProp, BagProp))
		{
#if WITH_EDITOR
			FFrame::KismetExecutionMessage(*FString::Printf(TEXT("%s: Type mismatch for parameter '%s'."), *FunctionName, *ParameterName.ToString()), ELogVerbosity::Warning);
#endif
		}
		else
		{
			// --- QoL: Safe Internal Object Casting ---
			if (const FObjectPropertyBase* TargetObjProp = CastField<FObjectPropertyBase>(BagProp))
			{
				UObject* SourceObject = nullptr;

				// Extract UObject from ValuePtr depending on whether ValueProp was valid (Variables) or Null (R-Values like 'self')
				if (const FObjectPropertyBase* SourceObjProp = CastField<FObjectPropertyBase>(ValueProp))
				{
					SourceObject = SourceObjProp->GetObjectPropertyValue(ValuePtr);
				}
				else if (!ValueProp)
				{
					// It's an R-Value (e.g. 'self'). ValuePtr directly points to the memory where EX_Self wrote the UObject*.
					SourceObject = *(UObject**)ValuePtr;
				}

				// Check if the runtime instance is actually a match for the target class
				if (SourceObject && SourceObject->IsA(TargetObjProp->PropertyClass))
				{
					TargetObjProp->SetObjectPropertyValue(DestPtr, SourceObject);
				}
				else
				{
					// Safe failure: assign nullptr and warn
					TargetObjProp->SetObjectPropertyValue(DestPtr, nullptr);

					if (SourceObject)
					{
#if WITH_EDITOR
						FFrame::KismetExecutionMessage(*FString::Printf(TEXT("%s: Runtime cast failed for parameter '%s'. Passed object '%s' is not of type '%s'."), *FunctionName, *ParameterName.ToString(), *SourceObject->GetName(), *TargetObjProp->PropertyClass->GetName()), ELogVerbosity::Warning);
#endif
					}
				}
			}
			else
			{
				// --- Standard Assignment for other types (Primitives, Structs, Arrays) ---
				BagProp->CopyCompleteValue(DestPtr, ValuePtr);
			}
		}
	}

	// Always clean up the local memory to prevent memory leaks with Arrays/Strings
	BagProp->DestroyValue(LocalValue);
}

// Resolves the scriptable container from the BP stack and assigns the wildcard value into its context.
static void AssignContextParameterToContainer(FFrame& Stack, const UScriptStruct* ExpectedStructType, const FString& FunctionName)
{
	// 1. Retrieve the strongly typed container struct.
	Stack.StepCompiledIn<FStructProperty>(nullptr);
	FStructProperty* ContainerProp = CastField<FStructProperty>(Stack.MostRecentProperty);
	void* ContainerPtr = Stack.MostRecentPropertyAddress;

	// 2. Retrieve the parameter name.
	P_GET_PROPERTY(FNameProperty, ParameterName);

	if (!ContainerProp || !ContainerPtr || !ContainerProp->Struct->IsChildOf(ExpectedStructType))
	{
		// Step to clear the stack if we abort early
		Stack.StepCompiledIn<FProperty>(nullptr);
		P_FINISH;
		return;
	}

	// Safely cast to the base container since both actions and requirements inherit from it.
	FScriptableContainer* Container = static_cast<FScriptableContainer*>(ContainerPtr);

	// Construct the dynamic property bag on the actual instance memory if it's empty.
	if (!Container->Context.IsValid())
	{
		Container->ConstructContext();
	}

	// Containers define their shape via ContextDefinitions, so the property must already exist.
	AssignStackValueToBag(Stack, Container->Context, ParameterName, /*bAddIfMissing=*/false, FunctionName);
}

// Resolves the scriptable context from the BP stack and assigns the wildcard value into its bag.
static void AssignContextParameterToContext(FFrame& Stack, const FString& FunctionName)
{
	// 1. Retrieve the context struct.
	Stack.StepCompiledIn<FStructProperty>(nullptr);
	FStructProperty* ContextProp = CastField<FStructProperty>(Stack.MostRecentProperty);
	void* ContextPtr = Stack.MostRecentPropertyAddress;

	// 2. Retrieve the parameter name.
	P_GET_PROPERTY(FNameProperty, ParameterName);

	if (!ContextProp || !ContextPtr || !ContextProp->Struct->IsChildOf(FScriptableContext::StaticStruct()))
	{
		Stack.StepCompiledIn<FProperty>(nullptr);
		P_FINISH;
		return;
	}

	FScriptableContext* Context = static_cast<FScriptableContext*>(ContextPtr);

	// A context carries its own shape, so define the property on the fly if it isn't there yet.
	AssignStackValueToBag(Stack, Context->GetBag(), ParameterName, true, FunctionName);
}

// --- Thunk Implementations ---
DEFINE_FUNCTION(UScriptableBlueprintLibrary::execSetScriptableContextProperty)
{
	AssignContextParameterToContext(Stack, TEXT("SetScriptableContextProperty"));
}

DEFINE_FUNCTION(UScriptableBlueprintLibrary::execSetActionContextParameter)
{
	AssignContextParameterToContainer(Stack, FScriptableAction::StaticStruct(), TEXT("SetActionContextParameter"));
}

DEFINE_FUNCTION(UScriptableBlueprintLibrary::execSetRequirementContextParameter)
{
	AssignContextParameterToContainer(Stack, FScriptableRequirement::StaticStruct(), TEXT("SetRequirementContextParameter"));
}

void UScriptableBlueprintLibrary::AddScriptableContextProperty(UPARAM(Ref)FScriptableContext& Context, FName ParameterName, const FKzTypeDef& Type)
{
	KzBagOps::AddProperty(Context.GetBag(), FKzParamDef(ParameterName, Type));
}

void UScriptableBlueprintLibrary::SetActionContext(UPARAM(Ref) FScriptableAction& Action, const FScriptableContext& Context)
{
	Action.SetContext(Context);
}

void UScriptableBlueprintLibrary::SetRequirementContext(UPARAM(Ref) FScriptableRequirement& Requirement, const FScriptableContext& Context)
{
	Requirement.SetContext(Context);
}

bool UScriptableBlueprintLibrary::EvaluateRequirement(UObject* Owner, const FScriptableRequirement& Requirement)
{
	return FScriptableRequirement::EvaluateRequirement(Owner, Requirement);
}

bool UScriptableBlueprintLibrary::EvaluateRequirementWithContext(UObject* Owner, const FScriptableRequirement& Requirement, const FScriptableContext& Context)
{
	return FScriptableRequirement::EvaluateRequirement(Owner, Requirement, Context);
}