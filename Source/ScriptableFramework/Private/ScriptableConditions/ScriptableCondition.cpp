// Copyright 2025 kirzo

#include "ScriptableConditions/ScriptableCondition.h"
#include "ScriptableConditions/ScriptableRequirementAsset.h"
#include "ScriptableConditions/ScriptableCondition_Group.h"

#if WITH_EDITOR
FText UScriptableCondition::GetDescription() const
{
	return GetClass()->GetDisplayNameText();
}
#endif

bool UScriptableCondition::CheckCondition()
{
	ResolveBindings();
	const bool bResult = Evaluate();
	return IsNegated() ? !bResult : bResult;
}

void UScriptableCondition_Asset::OnRegister()
{
	Super::OnRegister();

	if (Asset)
	{
		// 1. Create a transient Group to act as the runtime container.
		// We use a Group because it already encapsulates the logic for FScriptableRequirement (Evaluation + Bindings).
		UScriptableCondition_Group* Group = NewObject<UScriptableCondition_Group>(this, NAME_None, RF_Transient);

		if (Group)
		{
			// 2. Copy struct properties (Mode, Negate, Context)
			// This performs a copy of the PropertyBag, preserving default values from the Asset.
			Group->Requirement.Mode = Asset->Requirement.Mode;
			Group->Requirement.bNegate = Asset->Requirement.bNegate;
			Group->Requirement.Context = Asset->Requirement.Context;

			// 3. Deep Copy Conditions
			// We MUST duplicate the conditions. If we just copy the pointers, registering them 
			// would modify the objects inside the Asset (changing their Outer/Bindings), which is bad.
			const int32 NumConds = Asset->Requirement.Conditions.Num();
			Group->Requirement.Conditions.Reset(NumConds);

			for (const UScriptableCondition* SourceCond : Asset->Requirement.Conditions)
			{
				if (SourceCond)
				{
					// Duplicate using 'Group' as the new outer.
					UScriptableCondition* NewCond = DuplicateObject<UScriptableCondition>(SourceCond, Group);
					Group->Requirement.Conditions.Add(NewCond);
				}
			}

			// 4. Assign to our internal pointer
			Condition = Group;

			// 5. Inject Runtime Data & Register
			// This passes the Stack down to the Group, which passes it to its children + the new Asset Context.
			PropagateRuntimeData(Condition);
			Condition->Register(GetOwner());
		}
	}
}

void UScriptableCondition_Asset::OnUnregister()
{
	if (Condition)
	{
		Condition->Unregister();
		Condition = nullptr; // Release the transient group
	}

	Super::OnUnregister();
}

void UScriptableCondition_Asset::InitRuntimeData(const FInstancedPropertyBag* InContext, const TMap<FGuid, TObjectPtr<UScriptableObject>>* InBindingMap)
{
	Super::InitRuntimeData(InContext, InBindingMap);

	if (Condition)
	{
		Condition->InitRuntimeData(InContext, InBindingMap);
	}
}

bool UScriptableCondition_Asset::Evaluate_Implementation() const
{
	if (Condition)
	{
		return Condition->CheckCondition();
	}
	return false;
}

#if WITH_EDITOR
FText UScriptableCondition_Asset::GetDescription() const
{
	return Asset ? FText::FromString(Asset->GetName()) : INVTEXT("None");
}
#endif