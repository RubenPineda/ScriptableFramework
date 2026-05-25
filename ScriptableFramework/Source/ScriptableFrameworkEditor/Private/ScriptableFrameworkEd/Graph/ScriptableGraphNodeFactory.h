// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "EdGraphUtilities.h"

/**
 * Slate node factory that supplies custom SGraphNode subclasses for our editor graph nodes.
 * Currently maps UScriptableEdGraphNode_Task -> SScriptableGraphNode_Task; all other node
 * types fall through to the default factory by returning null.
 */
class FScriptableGraphNodeFactory : public FGraphPanelNodeFactory
{
public:
	virtual TSharedPtr<class SGraphNode> CreateNode(class UEdGraphNode* Node) const override;
};