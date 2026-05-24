// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode_Entry.h"
#include "ScriptableFrameworkEditorStyle.h"

FText UScriptableEdGraphNode_Entry::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return NSLOCTEXT("ScriptableEdGraphNode_Entry", "EntryTitle", "Entry");
}

FLinearColor UScriptableEdGraphNode_Entry::GetNodeTitleColor() const
{
	return FScriptableFrameworkEditorStyle::ScriptableEntryNodeColor;
}