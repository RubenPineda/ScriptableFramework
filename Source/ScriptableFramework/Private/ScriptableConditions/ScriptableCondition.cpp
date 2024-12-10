// Copyright Kirzo. All Rights Reserved.

#include "ScriptableConditions/ScriptableCondition.h"

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

	TGuardValue<decltype(Condition->OwnerPrivate)> OwnerGuard(Condition->OwnerPrivate, Owner);
	TGuardValue<decltype(Condition->WorldPrivate)> WorldGuard(Condition->WorldPrivate, Owner ? Owner->GetWorld() : nullptr);

	const bool bResult = Condition->Evaluate();
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