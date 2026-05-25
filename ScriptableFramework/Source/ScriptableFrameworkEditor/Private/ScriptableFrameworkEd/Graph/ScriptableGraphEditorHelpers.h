// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"

class UEdGraph;
class UEdGraphNode;
class UEdGraphPin;
class UScriptableTask;
class UScriptableNode;
class UScriptableEdGraphNode;

namespace ScriptableGraphEditorHelpers
{
	/** Spawns a UScriptableNode_Task wrapper with the given task class instantiated inside, plus its visual ed-node. */
	UEdGraphNode* SpawnTaskNode(UEdGraph* ParentGraph, TSubclassOf<UScriptableTask> TaskClass, const FVector2f& Location, UEdGraphPin* FromPin, bool bSelectNewNode);

	/** Spawns a UScriptableNode subclass directly (Branch, Sequence, etc.) plus its visual ed-node. */
	UEdGraphNode* SpawnNativeNode(UEdGraph* ParentGraph, TSubclassOf<UScriptableNode> NodeClass, const FVector2f& Location, UEdGraphPin* FromPin, bool bSelectNewNode);

	/** Connects FromPin to the first compatible pin on TargetNode after a drag-off spawn. Drag from an output picks the new node's first input; drag from an input picks the first output. Silent no-op if FromPin is null, pin categories don't match, or no compatible pin exists. Goes through the schema so Asset->Connections stays in sync and break-others logic runs. */
	void AutoWireFromPin(UEdGraphPin* FromPin, UEdGraphNode* TargetNode);

	/** Picks the right UScriptableEdGraphNode subclass for the runtime node and spawns it at Location. Does NOT add the runtime to the asset (caller's responsibility). */
	UScriptableEdGraphNode* SpawnEdNodeForRuntime(UEdGraph* ParentGraph, UScriptableNode* RuntimeNode, const FVector2f& Location);
}