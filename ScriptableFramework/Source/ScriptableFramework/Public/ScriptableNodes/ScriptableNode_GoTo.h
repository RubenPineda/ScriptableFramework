// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptableNode_GoTo.generated.h"

/**
 * "Wireless" jump: on activation it asks the runner to fire TargetEvent, waking every matching
 * ReceiveEvent node. Lets a long branch continue elsewhere in the graph without a drawn wire.
 */
UCLASS(DisplayName = "Go To", meta = (NodeCategory = "System|Flow"))
class SCRIPTABLEFRAMEWORK_API UScriptableNode_GoTo : public UScriptableNode
{
	GENERATED_BODY()

public:
	/** Event fired when this node runs. Matches a ReceiveEvent's EventName elsewhere in the graph. */
	UPROPERTY(EditAnywhere, Category = "Go To")
	FName TargetEvent;

	/** Canonical input pin name. */
	static const FName InInputName;

	//~ UScriptableNode interface
	virtual TArray<FName> GetInputPins() const override;
	virtual TArray<FName> GetDeclaredOutputPins() const override;
	//~ End of UScriptableNode interface

#if WITH_EDITOR
	virtual FText GetDisplayTitle() const override;
#endif

protected:
	virtual void ProcessInput(FName InputName) override;
};
