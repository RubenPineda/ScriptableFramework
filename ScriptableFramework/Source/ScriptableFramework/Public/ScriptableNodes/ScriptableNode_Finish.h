// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptableNode_Finish.generated.h"

/**
 * Terminator node. Stops in-flight work (no pin propagation) and fires Exit's OutputName so the
 * matching cleanup runs. Multiple Finish nodes allowed; first to activate wins. None → "Finished".
 */
UCLASS(DisplayName = "Finish", meta = (NodeCategory = "System|Flow"))
class SCRIPTABLEFRAMEWORK_API UScriptableNode_Finish : public UScriptableNode
{
	GENERATED_BODY()

public:
	static const FName InInputName;

	/** One of UScriptableGraph::Outputs or Exit's "Finished"/"Cancelled". */
	UPROPERTY(EditAnywhere, Category = "Finish")
	FName OutputName;

	//~ UScriptableNode interface
	virtual TArray<FName> GetInputPins() const override;
	virtual TArray<FName> GetDeclaredOutputPins() const override { return {}; }
	//~ End of UScriptableNode interface

#if WITH_EDITOR
	virtual FText GetDisplayTitle() const override;
#endif

protected:
	//~ UScriptableNode interface
	virtual void ProcessInput(FName InputName) override;
	//~ End of UScriptableNode interface
};
