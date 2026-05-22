// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "ScriptableEdGraphNode.generated.h"

class UScriptableNode;

/**
 * Base UEdGraphNode wrapping a runtime UScriptableNode.
 * Mirrors the runtime node's input/output pins onto the visual graph.
 */
UCLASS(Abstract)
class UScriptableEdGraphNode : public UEdGraphNode
{
	GENERATED_BODY()

public:
	/** Pin category used for all scriptable-graph pins. Single category for now: any pin connects to any compatible pin. */
	static const FName ScriptableExecPinCategory;

	/** The runtime node this editor node wraps. Persisted so the link survives save/load. */
	UPROPERTY()
	TObjectPtr<UScriptableNode> RuntimeNode;

	/** Binds this editor node to its runtime counterpart. Call before AllocateDefaultPins. */
	void SetRuntimeNode(UScriptableNode* InNode) { RuntimeNode = InNode; }

	/** Returns the runtime node this editor node wraps. */
	UScriptableNode* GetRuntimeNode() const { return RuntimeNode; }

	//~ UEdGraphNode interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual void DestroyNode() override;
	//~ End of UEdGraphNode interface
};