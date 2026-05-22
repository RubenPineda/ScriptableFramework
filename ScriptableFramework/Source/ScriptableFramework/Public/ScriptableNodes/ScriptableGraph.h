// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableObjectAsset.h"
#include "ScriptableNodes/ScriptableGraphConnection.h"
#include "StructUtils/PropertyBag.h"
#include "ScriptableGraph.generated.h"

class UScriptableNode;
class UScriptableGraphInstance;
class UEdGraph;

/**
 * An asset that defines a reusable scriptable graph: a network of UScriptableNode.
 *
 * The asset stores nodes, the centralized connection list, the ID of the (always present) Entry node,
 * and the declared context shape.
 */
UCLASS(BlueprintType, Const)
class SCRIPTABLEFRAMEWORK_API UScriptableGraph final : public UScriptableObjectAsset
{
	GENERATED_BODY()

public:
	UScriptableGraph();

	/**
	 * Fire-and-forget execution of this graph using values from the supplied context.
	 * Constructs a runtime instance under the hood and lets it run until completion or owner death.
	 */
	UFUNCTION(BlueprintCallable, Category = "Scriptable Framework|Graph", meta = (DefaultToSelf = "Owner"))
	static UScriptableGraphInstance* Run(UScriptableGraph* Graph, UObject* Owner, const FScriptableContext& InContext);

	/** All nodes living in this graph. Instanced so editor-created nodes are owned by the asset. */
	UPROPERTY(EditAnywhere, Instanced, Category = "Graph")
	TArray<TObjectPtr<UScriptableNode>> Nodes;

	/** Flat list of all wires in the graph. */
	UPROPERTY()
	TArray<FScriptableGraphConnection> Connections;

	/** Persistent ID of the Entry node. Set automatically on creation and validated/repaired on load. */
	UPROPERTY()
	FGuid EntryNodeID;

	//~ UObject interface
	virtual void PostInitProperties() override;
	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End of UObject interface

protected:
	//~ UScriptableObjectAsset interface
	virtual FInstancedPropertyBag* GetContext() override { return &ContextBag; }

#if WITH_EDITOR
	virtual FName GetContainerName() const override { return NAME_None; }
#endif
	//~ End of UScriptableObjectAsset interface

#if WITH_EDITORONLY_DATA
/** Editor-only visual representation of the graph. Holds node positions, wires, comments, etc. */
	UPROPERTY()
	TObjectPtr<UEdGraph> EdGraph;
#endif

private:
	/** Backing bag holding the declared context shape. Values are not stored at asset level. */
	UPROPERTY(Transient)
	FInstancedPropertyBag ContextBag;

	/** Creates the Entry node if missing and registers its BindingID in EntryNodeID. */
	void EnsureEntryNode();
};