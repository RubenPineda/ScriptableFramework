// Copyright 2026 kirzo

#include "ScriptableConditions/ScriptableCondition_Compare.h"

#if WITH_EDITOR
FText UScriptableCondition_CompareNumbers::GetDisplayTitle() const
{
	FString OpStr;
	switch (Operation)
	{
		case EScriptableComparisonOp::Equal:          OpStr = TEXT("=="); break;
		case EScriptableComparisonOp::NotEqual:       OpStr = TEXT("!="); break;
		case EScriptableComparisonOp::Less:           OpStr = TEXT("<"); break;
		case EScriptableComparisonOp::LessOrEqual:    OpStr = TEXT("<="); break;
		case EScriptableComparisonOp::Greater:        OpStr = TEXT(">"); break;
		case EScriptableComparisonOp::GreaterOrEqual: OpStr = TEXT(">="); break;
	}

	// Helper lambda to determine if we show the literal value or the binding name
	auto GetValueText = [this](FName PropName, double CurrentValue) -> FText
	{
		FString BindingName;
		if (GetBindingDisplayText(PropName, BindingName))
		{
			return FText::FromString(BindingName);
		}

		FNumberFormattingOptions Format;
		Format.SetMaximumFractionalDigits(2);
		return FText::AsNumber(CurrentValue, &Format);
	};

	// Format: [Health] [>] [50.0]
	return FText::Format(INVTEXT("{0} {1} {2}"),
											 GetValueText(GET_MEMBER_NAME_CHECKED(UScriptableCondition_CompareNumbers, A), A),
											 FText::FromString(OpStr),
											 GetValueText(GET_MEMBER_NAME_CHECKED(UScriptableCondition_CompareNumbers, B), B)
	);
}
#endif

bool UScriptableCondition_CompareNumbers::Evaluate_Implementation() const
{
	switch (Operation)
	{
		case EScriptableComparisonOp::Equal:
		return FMath::IsNearlyEqual(A, B, ErrorTolerance);

		case EScriptableComparisonOp::NotEqual:
		return !FMath::IsNearlyEqual(A, B, ErrorTolerance);

		case EScriptableComparisonOp::Less:
		return A < B;

		case EScriptableComparisonOp::LessOrEqual:
		return A <= B;

		case EScriptableComparisonOp::Greater:
		return A > B;

		case EScriptableComparisonOp::GreaterOrEqual:
		return A >= B;
	}

	return false;
}

#if WITH_EDITOR
FText UScriptableCondition_Distance::GetDisplayTitle() const
{
	FString OpStr;
	switch (Operation)
	{
		case EScriptableComparisonOp::Equal:          OpStr = TEXT("=="); break;
		case EScriptableComparisonOp::NotEqual:       OpStr = TEXT("!="); break;
		case EScriptableComparisonOp::Less:           OpStr = TEXT("<"); break;
		case EScriptableComparisonOp::LessOrEqual:    OpStr = TEXT("<="); break;
		case EScriptableComparisonOp::Greater:        OpStr = TEXT(">"); break;
		case EScriptableComparisonOp::GreaterOrEqual: OpStr = TEXT(">="); break;
	}

	// Helper to get text for actors
	auto GetActorText = [this](FName PropName, AActor* Actor) -> FText
	{
		FString BindingName;
		if (GetBindingDisplayText(PropName, BindingName))
		{
			return FText::FromString(BindingName);
		}
		return Actor ? FText::FromString(Actor->GetActorLabel()) : INVTEXT("None");
	};

	// Helper for distance value
	auto GetDistanceText = [this](double Val) -> FText
	{
		FString BindingName;
		if (GetBindingDisplayText(GET_MEMBER_NAME_CHECKED(UScriptableCondition_Distance, Distance), BindingName))
		{
			return FText::FromString(BindingName);
		}
		return FText::AsNumber(Val);
	};

	// Format: Distance(Self, Target) < 500
	return FText::Format(INVTEXT("Distance({0}, {1}) {2} {3}"),
											 GetActorText(GET_MEMBER_NAME_CHECKED(UScriptableCondition_Distance, Origin), Origin),
											 GetActorText(GET_MEMBER_NAME_CHECKED(UScriptableCondition_Distance, Target), Target),
											 FText::FromString(OpStr),
											 GetDistanceText(Distance)
	);
}
#endif

bool UScriptableCondition_Distance::Evaluate_Implementation() const
{
	if (!Origin || !Target)
	{
		return false;
	}

	const float ActualDistanceSq = Origin->GetSquaredDistanceTo(Target);
	const float ThresholdSq = FMath::Square(Distance);

	switch (Operation)
	{
		case EScriptableComparisonOp::Equal:          return FMath::IsNearlyEqual(ActualDistanceSq, ThresholdSq, 1.e-4);
		case EScriptableComparisonOp::NotEqual:       return !FMath::IsNearlyEqual(ActualDistanceSq, ThresholdSq, 1.e-4);
		case EScriptableComparisonOp::Less:           return ActualDistanceSq < ThresholdSq;
		case EScriptableComparisonOp::LessOrEqual:    return ActualDistanceSq <= ThresholdSq;
		case EScriptableComparisonOp::Greater:        return ActualDistanceSq > ThresholdSq;
		case EScriptableComparisonOp::GreaterOrEqual: return ActualDistanceSq >= ThresholdSq;
	}

	return false;
}