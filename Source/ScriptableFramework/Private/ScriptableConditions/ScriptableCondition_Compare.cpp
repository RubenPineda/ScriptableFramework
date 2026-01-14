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
		// Build the path to the local property (e.g., "A")
		FPropertyBindingPath TargetPath;
		TargetPath.SetStructID(GetBindingID());
		TargetPath.AddPathSegment(PropName);

		// Check if there is a binding for this property
		if (const FPropertyBindingPath* SourcePath = GetPropertyBindings().GetPropertyBinding(TargetPath))
		{
			//  Clean up the path to show only the leaf variable name 
			FString FullPath = SourcePath->ToString();

			int32 LastDotIndex;
			if (FullPath.FindLastChar('.', LastDotIndex))
			{
				// Return the substring after the last dot
				return FText::FromString(FullPath.RightChop(LastDotIndex + 1));
			}

			// If no dots found (top-level variable), return the full name
			return FText::FromString(FullPath);
		}

		FNumberFormattingOptions Format;
		Format.SetMaximumFractionalDigits(2);

		// If not bound, return the formatted literal value
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