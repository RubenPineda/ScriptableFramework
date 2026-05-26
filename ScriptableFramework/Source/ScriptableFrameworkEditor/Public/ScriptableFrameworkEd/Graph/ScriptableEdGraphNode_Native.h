// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode.h"
#include "ScriptableEdGraphNode_Native.generated.h"

/**
 * Generic concrete visual node for any UScriptableNode subclass that doesn't have a specialized
 * ed-node yet (Sequence, Branch, AND, ReceiveEvent, ...).
 */
UCLASS()
class SCRIPTABLEFRAMEWORKEDITOR_API UScriptableEdGraphNode_Native : public UScriptableEdGraphNode
{
	GENERATED_BODY()

public:
	//~ UEdGraphNode interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	//~ End of UEdGraphNode interface

	//~ UScriptableEdGraphNode interface
	virtual bool ShouldShowPinLabel(FName PinName) const override;
	//~ End of UScriptableEdGraphNode interface
};