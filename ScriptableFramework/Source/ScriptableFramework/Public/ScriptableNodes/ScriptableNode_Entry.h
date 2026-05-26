// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptableNode_Entry.generated.h"

/**
 * Entry node: the unique starting point of every UScriptableGraph.
 * Declares a single output ("Out") and no inputs. Activated externally by the graph runner.
 */
UCLASS(meta = (NodeCategory = "System", Hidden))
class SCRIPTABLEFRAMEWORK_API UScriptableNode_Entry : public UScriptableNode
{
	GENERATED_BODY()

public:
	/** Canonical name of the entry node's only output pin. */
	static const FName OutOutputName;

	virtual TArray<FName> GetDeclaredOutputPins() const override { return { OutOutputName }; }

	/** Called by the runner on graph start: arms and fires "Out", propagating downstream. */
	void Activate();
};