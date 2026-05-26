// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "EdGraphUtilities.h"

/**
 * Slate node factory mapping ed-nodes to custom SGraphNode widgets (e.g. Task -> SScriptableGraphNode_Task).
 * Returns null for unmapped types so they fall through to the default factory.
 */
class SCRIPTABLEFRAMEWORKEDITOR_API FScriptableGraphNodeFactory : public FGraphPanelNodeFactory
{
public:
	virtual TSharedPtr<class SGraphNode> CreateNode(class UEdGraphNode* Node) const override;
};