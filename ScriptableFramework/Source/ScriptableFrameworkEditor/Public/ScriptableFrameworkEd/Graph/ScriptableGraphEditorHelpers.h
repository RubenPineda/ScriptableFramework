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
	SCRIPTABLEFRAMEWORKEDITOR_API UEdGraphNode* SpawnTaskNode(UEdGraph* ParentGraph, TSubclassOf<UScriptableTask> TaskClass, const FVector2f& Location, UEdGraphPin* FromPin, bool bSelectNewNode);

	/** Spawns a UScriptableNode subclass directly (Branch, Sequence, etc.) plus its visual ed-node. */
	SCRIPTABLEFRAMEWORKEDITOR_API UEdGraphNode* SpawnNativeNode(UEdGraph* ParentGraph, TSubclassOf<UScriptableNode> NodeClass, const FVector2f& Location, UEdGraphPin* FromPin, bool bSelectNewNode);

	/** Connects FromPin to the first compatible pin on TargetNode after a drag-off spawn (output→first input, input→first output). No-op if incompatible. Routes through the schema so Connections stays in sync and break-others runs. */
	SCRIPTABLEFRAMEWORKEDITOR_API void AutoWireFromPin(UEdGraphPin* FromPin, UEdGraphNode* TargetNode);

	/** Picks the right UScriptableEdGraphNode subclass for the runtime node and spawns it at Location. Does NOT add the runtime to the asset (caller's responsibility). */
	SCRIPTABLEFRAMEWORKEDITOR_API UScriptableEdGraphNode* SpawnEdNodeForRuntime(UEdGraph* ParentGraph, UScriptableNode* RuntimeNode, const FVector2f& Location);
}