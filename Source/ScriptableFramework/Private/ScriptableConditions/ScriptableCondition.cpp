// Copyright 2025 kirzo

#include "ScriptableConditions/ScriptableCondition.h"
#include "ScriptableConditions/ScriptableConditionAsset.h"

#include "Algo/AllOf.h"
#include "Algo/AnyOf.h"

bool UScriptableCondition::EvaluateCondition(UObject* Owner, UScriptableCondition* Condition)
{
	if (!IsValid(Condition))
	{
		return false;
	}

	if (!Condition->IsEnabled())
	{
		return true;
	}

	Condition->Register(Owner);
	const bool bResult = Condition->Evaluate();
	Condition->Unregister();

	return Condition->IsNegated() ? !bResult : bResult;
}

bool UScriptableCondition_AND::Evaluate_Implementation() const
{
	if (Conditions.IsEmpty())
	{
		return false;
	}

	return Algo::AllOf(Conditions, [MyOwner = GetOwner()](UScriptableCondition* Condition) { return UScriptableCondition::EvaluateCondition(MyOwner, Condition); });
}

bool UScriptableCondition_OR::Evaluate_Implementation() const
{
	return Algo::AnyOf(Conditions, [MyOwner = GetOwner()](UScriptableCondition* Condition) { return UScriptableCondition::EvaluateCondition(MyOwner, Condition); });
}

void UScriptableCondition_Asset::OnRegister()
{
	Super::OnRegister();

	if (!IsValid(Condition))
	{
		CreateConditionInstance();
	}
}

void UScriptableCondition_Asset::OnUnregister()
{
	Super::OnUnregister();

	ClearConditionInstance();
}

bool UScriptableCondition_Asset::Evaluate_Implementation() const
{
	return UScriptableCondition::EvaluateCondition(GetOwner(), Condition);
}

void UScriptableCondition_Asset::CreateConditionInstance()
{
	ClearConditionInstance();

	if (IsValid(AssetToEvaluate) && IsValid(AssetToEvaluate->Condition))
	{
		// Duplicate the template to create a unique instance for this context.
		Condition = DuplicateObject<UScriptableCondition>(AssetToEvaluate->Condition, this);

		if (IsValid(Condition))
		{
			Condition->Register(GetOwner());
		}
	}
}

void UScriptableCondition_Asset::ClearConditionInstance()
{
	if (IsValid(Condition))
	{
		Condition->Unregister();
		Condition = nullptr;
	}
}