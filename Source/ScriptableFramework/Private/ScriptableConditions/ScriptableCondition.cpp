// Copyright 2025 kirzo

#include "ScriptableConditions/ScriptableCondition.h"

bool UScriptableCondition::CheckCondition()
{
	ResolveBindings();
	const bool bResult = Evaluate();
	return IsNegated() ? !bResult : bResult;
}