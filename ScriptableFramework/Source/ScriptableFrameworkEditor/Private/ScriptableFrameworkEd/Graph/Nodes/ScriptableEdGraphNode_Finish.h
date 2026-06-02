// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode_Native.h"
#include "ScriptableEdGraphNode_Finish.generated.h"

/** Ed-node for UScriptableNode_Finish: red Exit-family title color and a dynamic "Finish (X)" title. */
UCLASS()
class UScriptableEdGraphNode_Finish : public UScriptableEdGraphNode_Native
{
	GENERATED_BODY()

public:
	UScriptableEdGraphNode_Finish();

	//~ UEdGraphNode interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual bool ShowPaletteIconOnNode() const override { return true; }
	//~ End of UEdGraphNode interface
};
