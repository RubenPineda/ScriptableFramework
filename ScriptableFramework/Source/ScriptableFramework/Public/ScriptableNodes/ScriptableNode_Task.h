// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptableNode_Task.generated.h"

class UScriptableTask;

/**
 * Wrapper node that hosts a single UScriptableTask inside a graph.
 *
 * Declares inputs Start (always) and Stop (if the task is stoppable). Outputs are taken from the
 * task's GetOutputPins(); the base class auto-appends Stopped whenever Stop is among the inputs.
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