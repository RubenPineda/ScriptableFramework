// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "K2Node_SetScriptableContextProperty.generated.h"

class FBlueprintActionDatabaseRegistrar;
class FKismetCompilerContext;
class UEdGraph;
class UEdGraphPin;

/**
 * Custom K2 node for UScriptableBlueprintLibrary::SetScriptableContextProperty.
 *
 * Visually mirrors the default UFUNCTION node (Exec / Context Ref / ParameterName / Wildcard Value),
 * but adds an output Context pin whose net at compile time is the SAME source as the input Context.
 * The library function itself is void and takes the context by UPARAM(Ref); the output pin is a
 * pure editor-side convenience that lets the user chain multiple Set Scriptable Context Property
 * nodes through it without breaking the underlying FInstancedPropertyBag identity.
 *
 * ExpandNode spawns an intermediate UK2Node_CallFunction targeting the real UFUNCTION and rewires
 * the output Context pin's consumers to read directly from the same source pin that fed the input
 * Context. The wildcard is then propagated via the intermediate node's CustomStructureParam helper.
 */
UCLASS()
class UK2Node_SetScriptableContextProperty : public UK2Node
{
	GENERATED_BODY()

public:
	static const FName PN_ContextIn;
	static const FName PN_ContextOut;
	static const FName PN_ParameterName;
	static const FName PN_Value;

	//~ Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual void PinConnectionListChanged(UEdGraphPin* Pin) override;
	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) override;
	virtual void PostReconstructNode() override;
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
	UEdGraphPin* GetValuePin() const;
	UEdGraphPin* GetParameterNamePin() const;

	/** Resets the wildcard Value pin back to PC_Wildcard when nothing is connected. */
	void ResetValuePinToWildcard(UEdGraphPin* InValuePin) const;

	/** Propagates the connected pin's type onto the wildcard Value pin (mirrors CustomStructureParam behaviour). */
	void PropagateValuePinTypeFromConnection(UEdGraphPin* InValuePin) const;
};
