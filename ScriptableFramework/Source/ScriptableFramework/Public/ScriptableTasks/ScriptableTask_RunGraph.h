// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableTasks/ScriptableTask.h"
#include "ScriptableTask_RunGraph.generated.h"

class UScriptableGraph;
class UScriptableGraphInstance;

/** Runs a UScriptableGraph asset as a sub-execution inside the owning runtime. */
UCLASS(EditInlineNew, BlueprintType, NotBlueprintable, meta = (DisplayName = "Run Graph"))
class SCRIPTABLEFRAMEWORK_API UScriptableTask_RunGraph final : public UScriptableTask
{
	GENERATED_BODY()

public:
	/** Graph asset to instantiate and run when this task starts. Null = the task starts and finishes immediately as a no-op. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scriptable Task")
	TObjectPtr<UScriptableGraph> GraphAsset;

	//~ UScriptableTask interface
	virtual void BeginTask() override;
	virtual void FinishTask() override;
	virtual void StopTask() override;
	virtual void ResetTask() override;
	//~ End of UScriptableTask interface

#if WITH_EDITOR
	virtual FText GetDisplayTitle() const override;
#endif

protected:
	/** Live runner. Held strongly so it doesn't GC mid-execution. */
	UPROPERTY(Transient)
	TObjectPtr<UScriptableGraphInstance> SubRunner;

	/** Forwarded from SubRunner->OnFinishedNative. When the sub-graph terminates (naturally or via Cancel), this fires and we close ourselves. */
	void OnSubGraphFinished();

private:
	/** Shared cleanup used by Finish / Stop / Reset: unsubscribes from the sub-runner's delegate and cancels it if still live, then drops our handle. Idempotent. */
	void CancelSubRunnerIfAny();
};