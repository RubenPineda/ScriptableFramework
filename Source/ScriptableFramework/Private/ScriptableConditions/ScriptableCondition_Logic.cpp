// Copyright 2026 kirzo

#include "ScriptableConditions/ScriptableCondition_Logic.h"

// ------------------------------------------------------------------------------------------------
// Compare Booleans
// ------------------------------------------------------------------------------------------------

#if WITH_EDITOR
FText UScriptableCondition_Bool::GetDisplayTitle() const
{
	FString BindingName;
	const bool bIsBound = GetBindingDisplayText(GET_MEMBER_NAME_CHECKED(UScriptableCondition_Bool, bValue), BindingName);

	// Determine prefix based on the base class negation flag
	const FString Prefix = IsNegated() ? TEXT("!") : TEXT("");

	if (bIsBound)
	{
		// Case: Bound Variable -> "!IsAlive" or "IsAlive"
		return FText::Format(INVTEXT("{0}{1}"), FText::FromString(Prefix), FText::FromString(BindingName));
	}
	else
	{
		// Case: Static Value.
		// We manually calculate the visual result to be clear:
		// If bValue is true and bNegate is true -> Display "Is False"
		const bool bFinalResult = IsNegated() ? !bValue : bValue;
		return bFinalResult ? INVTEXT("Is True") : INVTEXT("Is False");
	}
}
#endif

bool UScriptableCondition_Bool::Evaluate_Implementation() const
{
	// Simply return the value. 
	// The base class handles the 'bNegate' logic on the result of this function.
	return bValue;
}

#if WITH_EDITOR
FText UScriptableCondition_CompareBooleans::GetDisplayTitle() const
{
	FString OpStr;
	switch (Operation)
	{
		case EScriptableBoolOp::And:      OpStr = TEXT("AND"); break;
		case EScriptableBoolOp::Or:       OpStr = TEXT("OR"); break;
		case EScriptableBoolOp::Xor:      OpStr = TEXT("XOR"); break;
		case EScriptableBoolOp::Nand:     OpStr = TEXT("NAND"); break;
		case EScriptableBoolOp::Equal:    OpStr = TEXT("=="); break;
		case EScriptableBoolOp::NotEqual: OpStr = TEXT("!="); break;
	}

	// Helper lambda to get the binding name or the literal value
	auto GetValueText = [this](FName PropName, bool CurrentValue) -> FText
	{
		FString BindingName;
		if (GetBindingDisplayText(PropName, BindingName))
		{
			return FText::FromString(BindingName);
		}
		return CurrentValue ? INVTEXT("True") : INVTEXT("False");
	};

	// Format: [IsActive] [AND] [IsAlive]
	return FText::Format(INVTEXT("{0} {1} {2}"),
											 GetValueText(GET_MEMBER_NAME_CHECKED(UScriptableCondition_CompareBooleans, bA), bA),
											 FText::FromString(OpStr),
											 GetValueText(GET_MEMBER_NAME_CHECKED(UScriptableCondition_CompareBooleans, bB), bB)
	);
}
#endif

bool UScriptableCondition_CompareBooleans::Evaluate_Implementation() const
{
	switch (Operation)
	{
		case EScriptableBoolOp::And:      return bA && bB;
		case EScriptableBoolOp::Or:       return bA || bB;
		case EScriptableBoolOp::Xor:      return bA ^ bB;
		case EScriptableBoolOp::Nand:     return !(bA && bB);
		case EScriptableBoolOp::Equal:    return bA == bB;
		case EScriptableBoolOp::NotEqual: return bA != bB;
	}
	return false;
}

// ------------------------------------------------------------------------------------------------
// Probability
// ------------------------------------------------------------------------------------------------

#if WITH_EDITOR
FText UScriptableCondition_Probability::GetDisplayTitle() const
{
	FString BindingName;
	if (GetBindingDisplayText(GET_MEMBER_NAME_CHECKED(UScriptableCondition_Probability, Chance), BindingName))
	{
		return FText::Format(INVTEXT("{0} Chance"), FText::FromString(BindingName));
	}

	// Format: "35% Chance"
	const int32 Percent = FMath::RoundToInt(Chance * 100.0f);
	return FText::Format(INVTEXT("{0}% Chance"), FText::AsNumber(Percent));
}
#endif

bool UScriptableCondition_Probability::Evaluate_Implementation() const
{
	return FMath::FRand() < Chance;
}

#if WITH_EDITOR
FText UScriptableCondition_IsValid::GetDisplayTitle() const
{
	FString Name = TEXT("...");
	GetBindingDisplayText(GET_MEMBER_NAME_CHECKED(UScriptableCondition_IsValid, TargetObject), Name);

	const FText ValidText = IsNegated() ? INVTEXT("Is Not Valid") : INVTEXT("Is Valid");
	return FText::Format(INVTEXT("{0} ({1})"), ValidText, FText::FromString(Name));
}
#endif

bool UScriptableCondition_IsValid::Evaluate_Implementation() const
{
	return IsValid(TargetObject);
}