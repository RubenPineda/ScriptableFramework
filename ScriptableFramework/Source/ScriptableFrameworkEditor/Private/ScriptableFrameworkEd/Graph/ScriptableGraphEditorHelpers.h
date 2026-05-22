// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"

class UEdGraph;
class UEdGraphNode;
class UEdGraphPin;
class UScriptableTask;
class UScriptableNode;

namespace ScriptableGraphEditorHelpers
{
	/** Spawns a UScriptableNode_Task wrapper with the given task class instantiated inside, plus its visual ed-node. */
	UEdGraphNode* SpawnTaskNode(UEdGraph* ParentGraph, TSubclassOf<UScriptableTask> TaskClass, const FVector2f& Location, UEdGraphPin* FromPin, bool bSelectNewNode);

	/** Spawns a UScriptableNode subclass directly (Branch, Sequence, etc.) plus its visual ed-node. */
	UEdGraphNode* SpawnNativeNode(UEdGraph* ParentGraph, TSubclassOf<UScriptableNode> NodeClass, const FVector2f& Location, UEdGraphPin* FromPin, bool bSelectNewNode);
}