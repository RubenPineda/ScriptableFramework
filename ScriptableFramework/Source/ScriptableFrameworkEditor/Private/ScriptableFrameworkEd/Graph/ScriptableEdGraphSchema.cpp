// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphSchema.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode.h"
#include "ScriptableNodes/ScriptableGraph.h"
#include "ScriptableNodes/ScriptableNode.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "Framework/Commands/GenericCommands.h"
#include "ToolMenu.h"
#include "ToolMenuSection.h"

#define LOCTEXT_NAMESPACE "ScriptableEdGraphSchema"

namespace
{
	/** Returns the UScriptableGraph that owns the EdGraph of this pin, or null if anything is missing. */
	UScriptableGraph* GetOwningGraphAsset(const UEdGraphPin* Pin)
	{
		if (!Pin || !Pin->GetOwningNode()) return nullptr;
		const UEdGraph* EdGraph = Pin->GetOwningNode()->GetGraph();
		return EdGraph ? Cast<UScriptableGraph>(EdGraph->GetOuter()) : nullptr;
	}

	/** Reorders the pair so the first returned is the output side. Returns false if directions are not opposite. */
	bool SortPinsByDirection(UEdGraphPin* A, UEdGraphPin* B, UEdGraphPin*& OutFrom, UEdGraphPin*& OutTo)
	{
		if (!A || !B || A->Direction == B->Direction) return false;
		OutFrom = (A->Direction == EGPD_Output) ? A : B;
		OutTo = (A->Direction == EGPD_Output) ? B : A;
		return true;
	}
}

const FPinConnectionResponse UScriptableEdGraphSchema::CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const
{
	if (!A || !B)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("InvalidPin", "Invalid pin."));
	}

	if (A->GetOwningNode() == B->GetOwningNode())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("SameNode", "Cannot connect a node to itself."));
	}

	if (A->Direction == B->Direction)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("SameDirection", "Cannot connect pins of the same direction."));
	}

	if (A->PinType.PinCategory != UScriptableEdGraphNode::ScriptableExecPinCategory ||
		B->PinType.PinCategory != UScriptableEdGraphNode::ScriptableExecPinCategory)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("IncompatibleCategory", "Incompatible pin category."));
	}

	// Each pin holds at most one connection. If either side is already wired, the engine breaks
	// the existing link(s) before making the new one (we route those breaks through this schema
	// in TryCreateConnection so the asset's Connections list stays in sync).
	const bool bABusy = A->LinkedTo.Num() > 0;
	const bool bBBusy = B->LinkedTo.Num() > 0;

	if (bABusy && bBBusy) return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_AB, LOCTEXT("ReplaceBoth", "Replace existing connections."));
	if (bABusy)           return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_A, LOCTEXT("ReplaceA", "Replace existing connection."));
	if (bBBusy)           return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_B, LOCTEXT("ReplaceB", "Replace existing connection."));

	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, FText::GetEmpty());
}

bool UScriptableEdGraphSchema::TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const
{
	const FPinConnectionResponse Response = CanCreateConnection(A, B);
	if (Response.Response == CONNECT_RESPONSE_DISALLOW) return false;

	// The default UEdGraphSchema::TryCreateConnection calls PinX->BreakAllPinLinks() for the
	// BREAK_OTHERS_* responses, which sidesteps BreakSinglePinLink and leaves stale entries in
	// UScriptableGraph::Connections. Route the breaks through this schema instead.
	auto BreakAllThroughSchema = [this](UEdGraphPin* Pin)
		{
			const TArray<UEdGraphPin*> Snapshot = Pin->LinkedTo;
			for (UEdGraphPin* Other : Snapshot)
			{
				BreakSinglePinLink(Pin, Other);
			}
		};

	if (Response.Response == CONNECT_RESPONSE_BREAK_OTHERS_A || Response.Response == CONNECT_RESPONSE_BREAK_OTHERS_AB)
	{
		BreakAllThroughSchema(A);
	}
	if (Response.Response == CONNECT_RESPONSE_BREAK_OTHERS_B || Response.Response == CONNECT_RESPONSE_BREAK_OTHERS_AB)
	{
		BreakAllThroughSchema(B);
	}

	A->MakeLinkTo(B);
	PersistConnection(A, B);

	return true;
}

void UScriptableEdGraphSchema::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const
{
	Super::BreakSinglePinLink(SourcePin, TargetPin);

	UEdGraphPin* FromPin = nullptr;
	UEdGraphPin* ToPin = nullptr;
	if (!SortPinsByDirection(SourcePin, TargetPin, FromPin, ToPin)) return;

	UScriptableGraph* GraphAsset = GetOwningGraphAsset(FromPin);
	if (!GraphAsset) return;

	const UScriptableEdGraphNode* FromEdNode = Cast<UScriptableEdGraphNode>(FromPin->GetOwningNode());
	const UScriptableEdGraphNode* ToEdNode = Cast<UScriptableEdGraphNode>(ToPin->GetOwningNode());
	if (!FromEdNode || !ToEdNode || !FromEdNode->GetRuntimeNode() || !ToEdNode->GetRuntimeNode()) return;

	const FGuid FromID = FromEdNode->GetRuntimeNode()->GetBindingID();
	const FGuid ToID = ToEdNode->GetRuntimeNode()->GetBindingID();
	const FName FromPinName = FromPin->PinName;
	const FName ToPinName = ToPin->PinName;

	GraphAsset->Modify();
	GraphAsset->Connections.RemoveAll([&](const FScriptableGraphConnection& C)
		{
			return C.From.NodeID == FromID && C.From.PinName == FromPinName
				&& C.To.NodeID == ToID && C.To.PinName == ToPinName;
		});
}

void UScriptableEdGraphSchema::BreakNodeLinks(UEdGraphNode& TargetNode) const
{
	Super::BreakNodeLinks(TargetNode);

	const UScriptableEdGraphNode* SfNode = Cast<UScriptableEdGraphNode>(&TargetNode);
	if (!SfNode || !SfNode->GetRuntimeNode()) return;

	const UEdGraph* OwningGraph = TargetNode.GetGraph();
	UScriptableGraph* GraphAsset = OwningGraph ? Cast<UScriptableGraph>(OwningGraph->GetOuter()) : nullptr;
	if (!GraphAsset) return;

	const FGuid NodeID = SfNode->GetRuntimeNode()->GetBindingID();

	GraphAsset->Modify();
	GraphAsset->Connections.RemoveAll([&](const FScriptableGraphConnection& C)
		{
			return C.From.NodeID == NodeID || C.To.NodeID == NodeID;
		});
}

void UScriptableEdGraphSchema::PersistConnection(UEdGraphPin* PinA, UEdGraphPin* PinB) const
{
	UEdGraphPin* FromPin = nullptr;
	UEdGraphPin* ToPin = nullptr;
	if (!SortPinsByDirection(PinA, PinB, FromPin, ToPin)) return;

	UScriptableGraph* GraphAsset = GetOwningGraphAsset(FromPin);
	if (!GraphAsset) return;

	const UScriptableEdGraphNode* FromEdNode = Cast<UScriptableEdGraphNode>(FromPin->GetOwningNode());
	const UScriptableEdGraphNode* ToEdNode = Cast<UScriptableEdGraphNode>(ToPin->GetOwningNode());
	if (!FromEdNode || !ToEdNode || !FromEdNode->GetRuntimeNode() || !ToEdNode->GetRuntimeNode()) return;

	FScriptableGraphConnection NewConn;
	NewConn.From.NodeID = FromEdNode->GetRuntimeNode()->GetBindingID();
	NewConn.From.PinName = FromPin->PinName;
	NewConn.To.NodeID = ToEdNode->GetRuntimeNode()->GetBindingID();
	NewConn.To.PinName = ToPin->PinName;

	if (GraphAsset->Connections.Contains(NewConn)) return;

	GraphAsset->Modify();
	GraphAsset->Connections.Add(NewConn);
}

void UScriptableEdGraphSchema::GetContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	Super::GetContextMenuActions(Menu, Context);

	if (!Menu || !Context || !Context->Node) return;

	// Pin or canvas right-clicks land here too; only contribute the node-edit block when the user
	// actually right-clicked on a node.
	if (Context->Pin) return;

	// The SGraphEditor attaches its FUICommandList (the one we filled in BindGraphCommands) to the
	// menu, so referencing FGenericCommands entries resolves to our handlers without further wiring.
	FToolMenuSection& Section = Menu->AddSection(TEXT("ScriptableNodeEdit"), LOCTEXT("NodeEditSection", "Edit"));
	Section.AddMenuEntry(FGenericCommands::Get().Cut);
	Section.AddMenuEntry(FGenericCommands::Get().Copy);
	Section.AddMenuEntry(FGenericCommands::Get().Paste);
	Section.AddMenuEntry(FGenericCommands::Get().Duplicate);
	Section.AddSeparator(TEXT("ScriptableNodeEditDeleteSep"));
	Section.AddMenuEntry(FGenericCommands::Get().Delete);
}

#undef LOCTEXT_NAMESPACE