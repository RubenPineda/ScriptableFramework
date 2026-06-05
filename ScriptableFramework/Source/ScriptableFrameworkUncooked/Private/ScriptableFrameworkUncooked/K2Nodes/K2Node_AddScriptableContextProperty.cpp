// Copyright 2026 kirzo

#include "ScriptableFrameworkUncooked/K2Nodes/K2Node_AddScriptableContextProperty.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "Core/KzTypeDef.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "KismetCompiler.h"
#include "ScriptableBlueprintLibrary.h"
#include "ScriptableContext.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "K2Node_AddScriptableContextProperty"

const FName UK2Node_AddScriptableContextProperty::PN_ContextIn(TEXT("Context"));
const FName UK2Node_AddScriptableContextProperty::PN_ContextOut(TEXT("ContextOut"));
const FName UK2Node_AddScriptableContextProperty::PN_ParameterName(TEXT("ParameterName"));
const FName UK2Node_AddScriptableContextProperty::PN_Type(TEXT("Type"));

void UK2Node_AddScriptableContextProperty::AllocateDefaultPins()
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

	// Type input. FKzTypeDef struct; the customization provides the picker UI on the pin.
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct, FKzTypeDef::StaticStruct(), PN_Type);

	// Context (Ref) output. Same struct type, identity-shared with the input at compile time.
	{
		UEdGraphNode::FCreatePinParams ContextOutParams;
		ContextOutParams.bIsReference = true;
		UEdGraphPin* ContextOutPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Struct, FScriptableContext::StaticStruct(), PN_ContextOut, ContextOutParams);
		ContextOutPin->PinFriendlyName = LOCTEXT("ContextOutPinFriendlyName", "Context");
	}

	Super::AllocateDefaultPins();
}

FText UK2Node_AddScriptableContextProperty::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("NodeTitle", "Add Scriptable Context Property");
}

FText UK2Node_AddScriptableContextProperty::GetTooltipText() const
{
	return LOCTEXT("NodeTooltip",
		"Adds a typed property to a Scriptable Context by name. The output Context pin reads from the same source as the input pin, so chaining Add / Set Scriptable Context Property nodes through it accumulates edits on the same underlying property bag.");
}

FSlateIcon UK2Node_AddScriptableContextProperty::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = FLinearColor::White;
	static FSlateIcon Icon(FAppStyle::GetAppStyleSetName(), TEXT("Kismet.AllClasses.FunctionIcon"));
	return Icon;
}

FText UK2Node_AddScriptableContextProperty::GetMenuCategory() const
{
	return LOCTEXT("MenuCategory", "Scriptable Framework|Context");
}

void UK2Node_AddScriptableContextProperty::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(ActionKey);
		check(Spawner);
		ActionRegistrar.AddBlueprintAction(ActionKey, Spawner);
	}
}

void UK2Node_AddScriptableContextProperty::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	UEdGraphPin* MyExecIn = FindPin(UEdGraphSchema_K2::PN_Execute, EGPD_Input);
	UEdGraphPin* MyExecOut = FindPin(UEdGraphSchema_K2::PN_Then, EGPD_Output);
	UEdGraphPin* MyContextIn = GetContextInPin();
	UEdGraphPin* MyContextOut = GetContextOutPin();
	UEdGraphPin* MyParameterName = GetParameterNamePin();
	UEdGraphPin* MyType = GetTypePin();

	if (!MyExecIn || !MyExecOut || !MyContextIn || !MyContextOut || !MyParameterName || !MyType)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("MissingPins", "Add Scriptable Context Property @@ is missing one of its required pins.").ToString(), this);
		BreakAllNodeLinks();
		return;
	}

	const FName TargetFunctionName = GET_FUNCTION_NAME_CHECKED(UScriptableBlueprintLibrary, AddScriptableContextProperty);

	UK2Node_CallFunction* InnerCall = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	InnerCall->FunctionReference.SetExternalMember(TargetFunctionName, UScriptableBlueprintLibrary::StaticClass());
	InnerCall->AllocateDefaultPins();

	UEdGraphPin* InnerExecIn = InnerCall->FindPinChecked(UEdGraphSchema_K2::PN_Execute, EGPD_Input);
	UEdGraphPin* InnerExecOut = InnerCall->FindPinChecked(UEdGraphSchema_K2::PN_Then, EGPD_Output);
	UEdGraphPin* InnerContext = InnerCall->FindPinChecked(PN_ContextIn);
	UEdGraphPin* InnerParameterName = InnerCall->FindPinChecked(PN_ParameterName);
	UEdGraphPin* InnerType = InnerCall->FindPinChecked(PN_Type);

	CompilerContext.MovePinLinksToIntermediate(*MyExecIn, *InnerExecIn);
	CompilerContext.MovePinLinksToIntermediate(*MyExecOut, *InnerExecOut);
	CompilerContext.MovePinLinksToIntermediate(*MyContextIn, *InnerContext);
	CompilerContext.MovePinLinksToIntermediate(*MyParameterName, *InnerParameterName);
	CompilerContext.MovePinLinksToIntermediate(*MyType, *InnerType);

	// Self-passthrough on Context: re-link every downstream consumer of MyContextOut to the same
	// source pin that drives InnerContext's by-ref read. Mirrors the Set Scriptable Context
	// Property K2Node; see that file for the longer explanation.
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
		CompilerContext.MessageLog.Warning(*LOCTEXT("ContextOutWithoutSource", "Add Scriptable Context Property @@ has nothing wired into Context In; the Context Out chain has no source to share.").ToString(), this);
	}

	BreakAllNodeLinks();

	Super::ExpandNode(CompilerContext, SourceGraph);
}

UEdGraphPin* UK2Node_AddScriptableContextProperty::GetContextInPin() const
{
	return FindPin(PN_ContextIn, EGPD_Input);
}

UEdGraphPin* UK2Node_AddScriptableContextProperty::GetContextOutPin() const
{
	return FindPin(PN_ContextOut, EGPD_Output);
}

UEdGraphPin* UK2Node_AddScriptableContextProperty::GetTypePin() const
{
	return FindPin(PN_Type, EGPD_Input);
}

UEdGraphPin* UK2Node_AddScriptableContextProperty::GetParameterNamePin() const
{
	return FindPin(PN_ParameterName, EGPD_Input);
}

#undef LOCTEXT_NAMESPACE
