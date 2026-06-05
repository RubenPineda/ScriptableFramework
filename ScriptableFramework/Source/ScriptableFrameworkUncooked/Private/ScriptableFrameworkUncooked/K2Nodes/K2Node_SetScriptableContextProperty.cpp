// Copyright 2026 kirzo

#include "ScriptableFrameworkUncooked/K2Nodes/K2Node_SetScriptableContextProperty.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "KismetCompiler.h"
#include "ScriptableBlueprintLibrary.h"
#include "ScriptableContext.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "K2Node_SetScriptableContextProperty"

const FName UK2Node_SetScriptableContextProperty::PN_ContextIn(TEXT("Context"));
const FName UK2Node_SetScriptableContextProperty::PN_ContextOut(TEXT("ContextOut"));
const FName UK2Node_SetScriptableContextProperty::PN_ParameterName(TEXT("ParameterName"));
const FName UK2Node_SetScriptableContextProperty::PN_Value(TEXT("Value"));

void UK2Node_SetScriptableContextProperty::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	// Context (Ref) input. Struct of FScriptableContext, passed by reference.
	{
		UEdGraphNode::FCreatePinParams ContextInParams;
		ContextInParams.bIsReference = true;
		UEdGraphPin* ContextInPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct, FScriptableContext::StaticStruct(), PN_ContextIn, ContextInParams);
		ContextInPin->PinFriendlyName = LOCTEXT("ContextPinFriendlyName", "Context");
	}

	// Parameter Name input (FName).
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Name, PN_ParameterName);

	// Value input. Wildcard; the concrete type is inferred from whatever the user connects.
	{
		UEdGraphPin* ValuePin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, PN_Value);
		ResetValuePinToWildcard(ValuePin);
	}

	// Context (Ref) output. Same struct type, identity-shared with the input at compile time.
	{
		UEdGraphNode::FCreatePinParams ContextOutParams;
		ContextOutParams.bIsReference = true;
		UEdGraphPin* ContextOutPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Struct, FScriptableContext::StaticStruct(), PN_ContextOut, ContextOutParams);
		ContextOutPin->PinFriendlyName = LOCTEXT("ContextOutPinFriendlyName", "Context");
	}

	Super::AllocateDefaultPins();
}

FText UK2Node_SetScriptableContextProperty::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("NodeTitle", "Set Scriptable Context Property");
}

FText UK2Node_SetScriptableContextProperty::GetTooltipText() const
{
	return LOCTEXT("NodeTooltip",
		"Sets a named property on a Scriptable Context. The output Context pin reads from the same source as the input pin, so chaining multiple Set Scriptable Context Property nodes through it accumulates edits on the same underlying property bag.");
}

FSlateIcon UK2Node_SetScriptableContextProperty::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = FLinearColor::White;
	static FSlateIcon Icon(FAppStyle::GetAppStyleSetName(), TEXT("Kismet.AllClasses.FunctionIcon"));
	return Icon;
}

FText UK2Node_SetScriptableContextProperty::GetMenuCategory() const
{
	return LOCTEXT("MenuCategory", "Scriptable Framework|Context");
}

void UK2Node_SetScriptableContextProperty::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(ActionKey);
		check(Spawner);
		ActionRegistrar.AddBlueprintAction(ActionKey, Spawner);
	}
}

void UK2Node_SetScriptableContextProperty::PinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::PinConnectionListChanged(Pin);

	if (!Pin || Pin->GetOwningNode() != this)
	{
		return;
	}

	// Mirror K2Node_CallFunction's CustomStructureParam helper: when Value gains a link, copy the
	// far-side pin's type onto our Value pin; when disconnected, reset to wildcard.
	if (Pin->PinName == PN_Value)
	{
		if (Pin->LinkedTo.Num() > 0)
		{
			PropagateValuePinTypeFromConnection(Pin);
		}
		else
		{
			ResetValuePinToWildcard(Pin);
		}
	}
}

void UK2Node_SetScriptableContextProperty::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	Super::ReallocatePinsDuringReconstruction(OldPins);

	// Restore the resolved wildcard type from the saved-out pin so a reloaded BP keeps any
	// Value-pin connection intact instead of failing schema validation on a wildcard mismatch.
	UEdGraphPin* NewValuePin = GetValuePin();
	if (!NewValuePin)
	{
		return;
	}
	for (UEdGraphPin* OldPin : OldPins)
	{
		if (OldPin && OldPin->PinName == PN_Value && OldPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Wildcard)
		{
			NewValuePin->PinType = OldPin->PinType;
			break;
		}
	}
}

void UK2Node_SetScriptableContextProperty::PostReconstructNode()
{
	Super::PostReconstructNode();

	// If Value ended up wildcard but has a live link, infer the type from the link (same path as
	// PinConnectionListChanged), so paste / duplicate land on the right concrete type.
	UEdGraphPin* ValuePin = GetValuePin();
	if (ValuePin && ValuePin->LinkedTo.Num() > 0 && ValuePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Wildcard)
	{
		PropagateValuePinTypeFromConnection(ValuePin);
	}
}

void UK2Node_SetScriptableContextProperty::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	UEdGraphPin* MyExecIn = FindPin(UEdGraphSchema_K2::PN_Execute, EGPD_Input);
	UEdGraphPin* MyExecOut = FindPin(UEdGraphSchema_K2::PN_Then, EGPD_Output);
	UEdGraphPin* MyContextIn = GetContextInPin();
	UEdGraphPin* MyContextOut = GetContextOutPin();
	UEdGraphPin* MyParameterName = GetParameterNamePin();
	UEdGraphPin* MyValue = GetValuePin();

	if (!MyExecIn || !MyExecOut || !MyContextIn || !MyContextOut || !MyParameterName || !MyValue)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("MissingPins", "Set Scriptable Context Property @@ is missing one of its required pins.").ToString(), this);
		BreakAllNodeLinks();
		return;
	}

	const FName TargetFunctionName = GET_FUNCTION_NAME_CHECKED(UScriptableBlueprintLibrary, SetScriptableContextProperty);

	UK2Node_CallFunction* InnerCall = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	InnerCall->FunctionReference.SetExternalMember(TargetFunctionName, UScriptableBlueprintLibrary::StaticClass());
	InnerCall->AllocateDefaultPins();

	UEdGraphPin* InnerExecIn = InnerCall->FindPinChecked(UEdGraphSchema_K2::PN_Execute, EGPD_Input);
	UEdGraphPin* InnerExecOut = InnerCall->FindPinChecked(UEdGraphSchema_K2::PN_Then, EGPD_Output);
	UEdGraphPin* InnerContext = InnerCall->FindPinChecked(PN_ContextIn);
	UEdGraphPin* InnerParameterName = InnerCall->FindPinChecked(PN_ParameterName);
	UEdGraphPin* InnerValue = InnerCall->FindPinChecked(PN_Value);

	CompilerContext.MovePinLinksToIntermediate(*MyExecIn, *InnerExecIn);
	CompilerContext.MovePinLinksToIntermediate(*MyExecOut, *InnerExecOut);
	CompilerContext.MovePinLinksToIntermediate(*MyContextIn, *InnerContext);
	CompilerContext.MovePinLinksToIntermediate(*MyParameterName, *InnerParameterName);
	CompilerContext.MovePinLinksToIntermediate(*MyValue, *InnerValue);

	// Trigger the inner CallFunction's CustomStructureParam helper so it stamps the concrete type
	// from LinkedTo[0]->PinType onto InnerValue. Without this the BP backend pushes the function's
	// declared int32 type and the runtime CustomThunk reads garbage off the stack.
	InnerCall->PinConnectionListChanged(InnerValue);

	// Self-passthrough on Context: every downstream consumer of MyContextOut must read the SAME
	// source pin that drives InnerContext's by-ref read. We can't MovePinLinksToIntermediate the
	// outer output pin onto InnerContext directly (that would create input-to-input links, which
	// the schema rejects). Instead, look at what InnerContext is now connected to (the source
	// variable getter the user wired into MyContextIn) and re-link each MyContextOut consumer to
	// that same source. After this the chain reads from one shared FInstancedPropertyBag identity.
	if (InnerContext->LinkedTo.Num() > 0)
	{
		UEdGraphPin* SourcePin = InnerContext->LinkedTo[0];
		if (SourcePin)
		{
			const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
			const TArray<UEdGraphPin*> Consumers = MyContextOut->LinkedTo;
			for (UEdGraphPin* Consumer : Consumers)
			{
				if (Consumer)
				{
					K2Schema->TryCreateConnection(SourcePin, Consumer);
				}
			}
		}
	}
	else if (MyContextOut->LinkedTo.Num() > 0)
	{
		// ContextIn has no source but the user wired ContextOut downstream. The UFUNCTION is
		// UPARAM(Ref) with no AutoCreateRefTerm, so the BP compiler will already raise a
		// missing-ref error on InnerContext. Emit a note pointing at the right node so the user
		// knows the chain has nothing to share.
		CompilerContext.MessageLog.Warning(*LOCTEXT("ContextOutWithoutSource", "Set Scriptable Context Property @@ has nothing wired into Context In; the Context Out chain has no source to share.").ToString(), this);
	}

	BreakAllNodeLinks();

	Super::ExpandNode(CompilerContext, SourceGraph);
}

UEdGraphPin* UK2Node_SetScriptableContextProperty::GetContextInPin() const
{
	return FindPin(PN_ContextIn, EGPD_Input);
}

UEdGraphPin* UK2Node_SetScriptableContextProperty::GetContextOutPin() const
{
	return FindPin(PN_ContextOut, EGPD_Output);
}

UEdGraphPin* UK2Node_SetScriptableContextProperty::GetValuePin() const
{
	return FindPin(PN_Value, EGPD_Input);
}

UEdGraphPin* UK2Node_SetScriptableContextProperty::GetParameterNamePin() const
{
	return FindPin(PN_ParameterName, EGPD_Input);
}

void UK2Node_SetScriptableContextProperty::ResetValuePinToWildcard(UEdGraphPin* InValuePin) const
{
	if (!InValuePin)
	{
		return;
	}
	// Preserve the ref/const flags (mirrors FCustomStructureParamHelper::HandleSinglePin in
	// K2Node_CallFunction). Today the Value pin has neither flag, but keeping the engine pattern
	// avoids silently stripping them if a future signature change adds one.
	const bool bWasRef = InValuePin->PinType.bIsReference;
	const bool bWasConst = InValuePin->PinType.bIsConst;
	InValuePin->PinType.ResetToDefaults();
	InValuePin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
	InValuePin->PinType.PinSubCategory = NAME_None;
	InValuePin->PinType.PinSubCategoryObject = nullptr;
	InValuePin->PinType.bIsReference = bWasRef;
	InValuePin->PinType.bIsConst = bWasConst;
}

void UK2Node_SetScriptableContextProperty::PropagateValuePinTypeFromConnection(UEdGraphPin* InValuePin) const
{
	if (!InValuePin || InValuePin->LinkedTo.Num() == 0 || !InValuePin->LinkedTo[0])
	{
		return;
	}
	const UEdGraphPin* Source = InValuePin->LinkedTo[0];
	InValuePin->PinType = Source->PinType;
}

#undef LOCTEXT_NAMESPACE
