// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptableNode_ReceiveEvent.generated.h"

/**
 * External-trigger entry point. Like UScriptableNode_Entry, has no inputs and a single "Out"
 * output — but unlike Entry, doesn't fire on graph launch. Instead, it waits for gameplay code
 * to call UScriptableGraphInstance::FireEvent(EventName), at which point every ReceiveEvent in
 * the graph whose EventName matches is triggered in parallel (fan-out is intentional, lets the
 * user split response logic across multiple visual branches).
 */
UCLASS(DisplayName = "Event", meta = (NodeCategory = "System|Flow"))
class SCRIPTABLEFRAMEWORK_API UScriptableNode_ReceiveEvent : public UScriptableNode
{
	GENERATED_BODY()

public:
	/** Symbolic event key the gameplay caller uses to address this node. Empty = inert. */
	UPROPERTY(EditAnywhere, Category = "Receive Event")
	FName EventName;

	/** Canonical output pin name. */
	static const FName OutOutputName;

	//~ UScriptableNode interface
	virtual TArray<FName> GetInputPins() const override;
	virtual TArray<FName> GetDeclaredOutputPins() const override;
	//~ End of UScriptableNode interface

#if WITH_EDITOR
	virtual FText GetDisplayTitle() const override;
#endif

	/** Origin-of-flow entry point. Called by UScriptableGraphInstance::FireEvent when EventName matches. Arms and fires the single output. Mirrors UScriptableNode_Entry::Activate. */
	void Trigger();
};