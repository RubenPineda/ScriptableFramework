// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/ScriptableGraphPinFactory.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode.h"
#include "ScriptableFrameworkEd/Graph/SScriptableGraphPin.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphNode.h"

TSharedPtr<SGraphPin> FScriptableGraphPinFactory::CreatePin(UEdGraphPin* InPin) const
{
	if (!InPin) return nullptr;

	// Match only pins from our schema (any other graph using the same editor's K2 schema will
	// fall through to the default factory).
	if (InPin->PinType.PinCategory != UScriptableEdGraphNode::ScriptableExecPinCategory) return nullptr;

	return SNew(SScriptableGraphPin, InPin);
}