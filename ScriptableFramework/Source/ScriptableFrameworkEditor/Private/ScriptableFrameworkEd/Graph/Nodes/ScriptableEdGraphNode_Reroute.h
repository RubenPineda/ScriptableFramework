// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode.h"
#include "ScriptableEdGraphNode_Reroute.generated.h"

/** Ed-graph counterpart of UScriptableNode_Reroute. */
UCLASS()
class UScriptableEdGraphNode_Reroute : public UScriptableEdGraphNode
{
	GENERATED_BODY()

public:
	UScriptableEdGraphNode_Reroute();

	//~ UEdGraphNode interface
	/** Tells SGraphNodeKnot to render us as a control point. The out-params identify pin indices; we have exactly one input (index 0) and one output (index 1) following the order in which AllocateDefaultPins creates them (inputs first, then outputs — same convention the base class uses). */
	virtual bool ShouldDrawNodeAsControlPointOnly(int32& OutInputPinIndex, int32& OutOutputPinIndex) const override
	{
		OutInputPinIndex = 0;
		OutOutputPinIndex = 1;
		return true;
	}
	virtual bool ShouldOverridePinNames() const override { return true; }
	virtual FText GetPinNameOverride(const UEdGraphPin& Pin) const override { return FText::GetEmpty(); }
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override { return FText::GetEmpty(); }
	virtual FText GetTooltipText() const override;
	virtual void GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const override { HoverTextOut.Empty(); }
	//~ End of UEdGraphNode interface
};