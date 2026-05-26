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

	/** Runtime class this ed-node visualizes. Concrete subclasses set this in their constructor; the framework uses it to build the runtime-class -> ed-node-class lookup table during module startup. Left null on the abstract base (and on the generic fallback) so the registry skips them. */
	UClass* RuntimeNodeClass = nullptr;

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
	virtual void ReconstructNode() override;
	//~ End of UEdGraphNode interface

	/** Hook for subclasses to contribute right-click menu entries when the user clicks a specific pin. Called by UScriptableEdGraphSchema::GetContextMenuActions on the pin route. Base does nothing; specialized ed-nodes (e.g. Sequence's "Remove pin") override and append entries. */
	virtual void AppendPinContextActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const {}

	/** Decides whether a pin's label is rendered. Default shows every label; override to hide. */
	virtual bool ShouldShowPinLabel(FName PinName) const { return true; }
};