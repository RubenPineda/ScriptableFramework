// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptableNode_Task.generated.h"

class UScriptableTask;

/**
 * Wrapper node hosting a single UScriptableTask. Inputs: Start (always) and Stop (if stoppable).
 * Outputs come from the task's GetOutputPins(); the base auto-appends Stopped when Stop is an input.
 */
UCLASS(meta = (Hidden))
class SCRIPTABLEFRAMEWORK_API UScriptableNode_Task : public UScriptableNode
{
	GENERATED_BODY()

public:
	/** Canonical name of the Start input pin. */
	static const FName StartInputName;

	/** The task this node hosts. */
	UPROPERTY(EditAnywhere, Instanced, Category = "Task")
	TObjectPtr<UScriptableTask> Task;

	//~ UScriptableNode interface
	virtual TArray<FName> GetInputPins() const override;
	virtual TArray<FName> GetDeclaredOutputPins() const override;
	virtual void Teardown() override;
	//~ End of UScriptableNode interface

	//~ UScriptableObject interface
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	//~ End of UScriptableObject interface

protected:
	virtual void ProcessInput(FName InputName) override;

private:
	void HandleTaskFinished(UScriptableTask* InTask);
	void HandleTaskStopped(UScriptableTask* InTask);
};