// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptableNode_Reroute.generated.h"

/** Visual passthrough used to reorganize wires without changing flow. */
UCLASS(DisplayName = "Reroute", meta = (NodeCategory = "System|Flow", Hidden))
class SCRIPTABLEFRAMEWORK_API UScriptableNode_Reroute final : public UScriptableNode
{
	GENERATED_BODY()

public:
	//~ UScriptableNode interface
	virtual TArray<FName> GetInputPins() const override;
	virtual TArray<FName> GetDeclaredOutputPins() const override;
	virtual void ProcessInput(FName InputName) override;
	//~ End of UScriptableNode interface
};