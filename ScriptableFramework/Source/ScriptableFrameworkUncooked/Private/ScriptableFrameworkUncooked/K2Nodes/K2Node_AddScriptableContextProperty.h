// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "K2Node_AddScriptableContextProperty.generated.h"

class FBlueprintActionDatabaseRegistrar;
class FKismetCompilerContext;
class UEdGraph;
class UEdGraphPin;

/**
 * Custom K2 node for UScriptableBlueprintLibrary::AddScriptableContextProperty.
 *
 * Visually mirrors the default UFUNCTION node (Exec / Context Ref / ParameterName / Type), but
 * adds an output Context pin whose net at compile time is the SAME source as the input Context.
 * The library function itself is void and takes the context by UPARAM(Ref); the output pin is a
 * pure editor-side convenience that lets the user chain multiple Add / Set Scriptable Context
 * Property nodes through it without breaking the underlying FInstancedPropertyBag identity.
 *
 * ExpandNode spawns an intermediate UK2Node_CallFunction targeting the real UFUNCTION and rewires
 * the output Context pin's consumers to read directly from the same source pin that fed the input.
 */
UCLASS()
class UK2Node_AddScriptableContextProperty : public UK2Node
{
	GENERATED_BODY()

public:
	static const FName PN_ContextIn;
	static const FName PN_ContextOut;
	static const FName PN_ParameterName;
	static const FName PN_Type;

	//~ Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	//~ End UEdGraphNode interface

	//~ Begin UK2Node interface
	virtual FText GetMenuCategory() const override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual bool IsNodePure() const override { return false; }
	virtual bool ShouldShowNodeProperties() const override { return false; }
	//~ End UK2Node interface

private:
	UEdGraphPin* GetContextInPin() const;
	UEdGraphPin* GetContextOutPin() const;
	UEdGraphPin* GetTypePin() const;
	UEdGraphPin* GetParameterNamePin() const;
};
