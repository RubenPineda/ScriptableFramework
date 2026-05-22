// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphSchema.h"
#include "ScriptableEdGraphSchemaActions.generated.h"

class UScriptableTask;
class UEdGraph;
class UEdGraphPin;
class UEdGraphNode;

/**
 * Context-menu action that spawns a UScriptableNode_Task (with a specific UScriptableTask class
 * instantiated inside) plus its visual UScriptableEdGraphNode_Task wrapper.
 */
USTRUCT()
struct FScriptableEdGraphSchemaAction_NewTaskNode : public FEdGraphSchemaAction
{
	GENERATED_BODY()

	/** Concrete UScriptableTask subclass to instantiate inside the wrapper. */
	UPROPERTY()
	TSubclassOf<UScriptableTask> TaskClass;

	FScriptableEdGraphSchemaAction_NewTaskNode() = default;

	FScriptableEdGraphSchemaAction_NewTaskNode(FText InCategory, FText InMenuDesc, FText InTooltip, int32 InGrouping, TSubclassOf<UScriptableTask> InTaskClass)
		: FEdGraphSchemaAction(MoveTemp(InCategory), MoveTemp(InMenuDesc), MoveTemp(InTooltip), InGrouping)
		, TaskClass(InTaskClass)
	{
	}

	//~ FEdGraphSchemaAction
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2f& Location, bool bSelectNewNode = true) override;
	//~ End of FEdGraphSchemaAction
};