// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "ScriptableContext.h"
#include "AsyncRunScriptableGraph.generated.h"

class UScriptableGraph;
class UScriptableGraphInstance;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAsyncScriptableGraphEvent, UScriptableGraphInstance*, Runner);

/** Async node that runs a UScriptableGraph and exposes its live runner. */
UCLASS(MinimalAPI)
class UAsyncRunScriptableGraph : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/**
	 * Runs a UScriptableGraph asset. Started fires immediately with the live runner (use it to send
	 * events or mutate context while it runs); Finished fires when the graph completes, with the same runner.
	 */
	UFUNCTION(BlueprintCallable, Category = "Scriptable Framework|Graph", meta = (DefaultToSelf = "Owner", BlueprintInternalUseOnly = "true", DisplayName = "Run Scriptable Graph", AutoCreateRefTerm = "Context", AdvancedDisplay = "Id"))
	static UAsyncRunScriptableGraph* RunScriptableGraph(UObject* Owner, UScriptableGraph* Graph, const FScriptableContext& Context, FName Id);

	/** Fired right after the graph launches. Carries the live runner. */
	UPROPERTY(BlueprintAssignable)
	FAsyncScriptableGraphEvent Started;

	/** Fired when the graph finishes (naturally or cancelled). Carries the runner. */
	UPROPERTY(BlueprintAssignable)
	FAsyncScriptableGraphEvent Finished;

	virtual void Activate() override;
	virtual void SetReadyToDestroy() override;

private:
	void HandleGraphFinished();

protected:
	UPROPERTY(Transient)
	TObjectPtr<UObject> GraphOwner;

	UPROPERTY(Transient)
	TObjectPtr<UScriptableGraph> GraphAsset;

	UPROPERTY(Transient)
	TObjectPtr<UScriptableGraphInstance> Runner;

	/** Values to seed the graph context with at launch. */
	UPROPERTY()
	FScriptableContext LaunchContext;

	/** Optional caller-supplied identifier for this run; surfaced by debug UI and queried by CancelRunnersById. */
	UPROPERTY()
	FName LaunchId;
};
