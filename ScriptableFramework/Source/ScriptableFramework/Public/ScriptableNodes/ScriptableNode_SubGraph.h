// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptableNode_SubGraph.generated.h"

class UScriptableGraph;
class UScriptableGraphInstance;

/**
 * Runs another UScriptableGraph. Output pins mirror the asset's Exit pin set
 * ("Finished" + "Cancelled" + UScriptableGraph::Outputs). On completion fires the matching pin.
 * Pins rebuild on SubGraphAsset change via the editor's OnRuntimeNodePropertyChanged hook.
 */
UCLASS(DisplayName = "Sub-Graph", meta = (NodeCategory = "System|Flow"))
class SCRIPTABLEFRAMEWORK_API UScriptableNode_SubGraph : public UScriptableNode
{
	GENERATED_BODY()

public:
	static const FName InInputName;

	/** Cancels the live sub-runner; fires the "Cancelled" output via the runner's teardown path. */
	static const FName CancelInputName;

	/** Hidden "running" output that keeps the node in ActiveNodes while the sub-runner is alive. */
	static const FName PendingOutputName;

	UPROPERTY(EditAnywhere, Category = "Sub Graph")
	TObjectPtr<UScriptableGraph> SubGraphAsset;

	//~ UScriptableNode interface
	virtual TArray<FName> GetInputPins() const override;
	virtual TArray<FName> GetDeclaredOutputPins() const override;
	virtual void Teardown() override;
	//~ End of UScriptableNode interface

#if WITH_EDITOR
	virtual FText GetDisplayTitle() const override;
#endif

protected:
	//~ UScriptableNode interface
	virtual void ProcessInput(FName InputName) override;
	//~ End of UScriptableNode interface

private:
	UPROPERTY(Transient)
	TObjectPtr<UScriptableGraphInstance> RuntimeSubInstance;

	void HandleSubGraphFinished();

	/** Bail-out helper for ProcessInput when the sub-runner can't launch. */
	void FinishImmediately(FName OutputName);
};
