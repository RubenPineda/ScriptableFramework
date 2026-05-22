// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode_Entry.h"

FText UScriptableEdGraphNode_Entry::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return NSLOCTEXT("ScriptableEdGraphNode_Entry", "EntryTitle", "Entry");
}

FLinearColor UScriptableEdGraphNode_Entry::GetNodeTitleColor() const
{
	return FLinearColor(0.2f, 0.55f, 0.3f);
}