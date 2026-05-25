// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableObject.h"
#include "ScriptableTask.generated.h"

SCRIPTABLEFRAMEWORK_API DECLARE_LOG_CATEGORY_EXTERN(LogScriptableTask, Log, All);

class UScriptableTask;
class UScriptableCondition;

DECLARE_MULTICAST_DELEGATE_OneParam(FScriptableTaskNativeDelegate, UScriptableTask* /*Task*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FScriptableTaskDelegate, UScriptableTask*, Task);

UENUM()
enum class EScriptableTaskStatus : uint8
{
	None, Begun, Finished, Stopped
};

struct SCRIPTABLEFRAMEWORK_API FScriptableTaskEvents
{
	FScriptableTaskDelegate OnTaskBegin;
	FScriptableTaskDelegate OnTaskFinish;
};

/** Flow control settings for the task. */
USTRUCT(BlueprintType)
struct FScriptableTaskControl
{
	GENERATED_BODY()

	/** If true, the task will loop automatically upon finishing. */
	UPROPERTY(EditAnywhere, Category = "Control")
	uint8 bLoop : 1 = false;

	/** Number of loops. 0 means infinite. */
	UPROPERTY(EditAnywhere, Category = "Control", meta = (ClampMin = 0))
	int32 LoopCount = 0;

	/** If true, this task will execute only once during its lifecycle. */
	UPROPERTY(EditAnywhere, Category = "Control")
	uint8 bDoOnce : 1 = false;
};

UCLASS(Abstract, DefaultToInstanced, EditInlineNew, Blueprintable, BlueprintType, HideCategories = (Hidden), CollapseCategories)
class SCRIPTABLEFRAMEWORK_API UScriptableTask : public UScriptableObject
{
	GENERATED_BODY()

private:
	/** Current status of the task. */
	EScriptableTaskStatus Status = EScriptableTaskStatus::None;

	/** Advanced execution logic (Looping, DoOnce). */
	UPROPERTY(EditAnywhere, Category = Hidden, meta = (NoBinding))
	FScriptableTaskControl Control;

	/** Counter for the current loop iteration. */
	UPROPERTY(Transient)
	int32 CurrentLoopIndex = 0;

	/** Flag to track if a DoOnce task has already executed. */
	UPROPERTY(Transient)
	uint8 bDoOnceFinished : 1 = false;

	/** Name of the output fired by the most recent Finish call. */
	FName LastFiredOutput = NAME_None;

public:
	/** Canonical name of the default completion output. */
	static const FName CompletedOutputName;

	/** Canonical name of the cancellation output, fired by Stop(). */
	static const FName StoppedOutputName;

	EScriptableTaskStatus GetStatus() const { return Status; }

	/** Read-only access to flow-control settings (Loop, DoOnce, LoopCount). */
	const FScriptableTaskControl& GetControl() const { return Control; }

	/** Indicates that BeginTask has been called, but the task has not yet finished or stopped. */
	bool HasBegun() const { return Status == EScriptableTaskStatus::Begun; }
	/** Indicates that the task has finished normally. */
	bool HasFinished() const { return Status == EScriptableTaskStatus::Finished; }
	/** Indicates that the task has been cancelled via Stop(). */
	bool HasStopped() const { return Status == EScriptableTaskStatus::Stopped; }

	/** Returns the name of the output fired by the most recent Finish/Stop call. */
	FName GetLastFiredOutput() const { return LastFiredOutput; }

	/**
	 * Returns the set of named outputs this task can fire on completion.
	 * Default: a single output named CompletedOutputName.
	 * Tasks that expose multiple completion paths (e.g. Started, Completed) override this.
	 * The Stopped output is implicit when IsStoppable() returns true and is NOT included here.
	 */
	virtual TArray<FName> GetOutputPins() const { return { CompletedOutputName }; }

	/**
	 * Returns true if this task supports cancellation via Stop().
	 * Default: true. Override and return false for tasks where cancellation makes no sense.
	 */
	virtual bool IsStoppable() const { return true; }

	virtual bool IsReadyToTick() const override { return HasBegun(); }

	virtual void OnUnregister() override;

	UFUNCTION(BlueprintCallable, Category = ScriptableTask)
	void Reset();

	/**
	 * Begins the execution of this task.
	 * Requires task to be registered.
	 */
	UFUNCTION(BlueprintCallable, Category = ScriptableTask)
	void Begin();

	/**
	 * Finish the execution of this task.
	 * Fires the CompletedOutputName output. For tasks with multiple outputs, use FinishWithOutput.
	 */
	UFUNCTION(BlueprintCallable, Category = ScriptableTask)
	void Finish();

	/**
	 * Cancels the execution of this task. Does nothing if !IsStoppable(), or if the task has not begun
	 * or has already finished/stopped. Fires OnTaskStopped (not OnTaskFinish) and does not honor loop control.
	 */
	UFUNCTION(BlueprintCallable, Category = ScriptableTask)
	void Stop();

	FScriptableTaskNativeDelegate OnTaskBeginNative;
	FScriptableTaskNativeDelegate OnTaskFinishNative;
	FScriptableTaskNativeDelegate OnTaskStoppedNative;

	UPROPERTY(BlueprintAssignable)
	FScriptableTaskDelegate OnTaskBegin;

	UPROPERTY(BlueprintAssignable)
	FScriptableTaskDelegate OnTaskFinish;

	UPROPERTY(BlueprintAssignable)
	FScriptableTaskDelegate OnTaskStopped;

protected:
	/**
	 * Finish variant that fires a specific named output instead of the default CompletedOutputName.
	 * Use this in tasks that declare multiple outputs in GetOutputPins().
	 */
	void FinishWithOutput(FName OutputName);

private:
	virtual void ResetTask();
	virtual void BeginTask();
	virtual void FinishTask();
	virtual void StopTask();

protected:
	/** Blueprint implementable event for when the task resets. */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Reset Task"))
	void ReceiveResetTask();

	/** Blueprint implementable event for when the task begins. */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Begin Task"))
	void ReceiveBeginTask();

	/** Blueprint implementable event for when the task ends. */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Finish Task"))
	void ReceiveFinishTask();

	/** Blueprint implementable event for when the task is cancelled via Stop(). */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Stop Task"))
	void ReceiveStopTask();

#if WITH_EDITOR
public:
	/**
	 * Override to give this task class a custom node title color in the graph editor.
	 * Return an unset TOptional (the default) to use the generic task color.
	 */
	virtual TOptional<FLinearColor> GetNodeTitleColor() const { return TOptional<FLinearColor>(); }
#endif
};