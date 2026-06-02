// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode_Native.h"
#include "ScriptableEdGraphNode_SubGraph.generated.h"

/** Ed-node for UScriptableNode_SubGraph: title color matches the UScriptableGraph asset's color. */
UCLASS()
class UScriptableEdGraphNode_SubGraph : public UScriptableEdGraphNode_Native
{
	GENERATED_BODY()

public:
	UScriptableEdGraphNode_SubGraph();

	//~ UEdGraphNode interface
	virtual FLinearColor GetNodeTitleColor() const override;
	//~ End of UEdGraphNode interface
};
