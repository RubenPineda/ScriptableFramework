// Copyright 2026 kirzo

#include "ScriptableConditions/ScriptableCondition_NestedRequirement.h"

void UScriptableCondition_NestedRequirement::OnRegister()
{
	Super::OnRegister();

	// Inherit the parent scope's context by copying its bag into our local one. This is what makes
	// the inner conditions resolve context bindings: Requirement.Register passes the wrapper's owner,
	// and AddBindingSource needs a locally-valid Context to inject — relying on the UScriptableObject
	// fallback is fragile when the wrapper's own ContextRef hasn't propagated. Mirrors
	// UScriptableCondition_Asset.
	if (const FInstancedPropertyBag* ParentContext = GetContext())
	{
		Requirement.Context = *ParentContext;
	}

	Requirement.Register(GetOwner());
}

void UScriptableCondition_NestedRequirement::OnUnregister()
{
	Super::OnUnregister();

	Requirement.Unregister();
}

bool UScriptableCondition_NestedRequirement::Evaluate_Implementation() const
{
	return Requirement.Evaluate();
}
