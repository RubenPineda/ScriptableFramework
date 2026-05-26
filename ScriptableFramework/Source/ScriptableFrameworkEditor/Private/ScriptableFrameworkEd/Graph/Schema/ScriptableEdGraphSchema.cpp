// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphSchema.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode.h"
#include "ScriptableFrameworkEd/Graph/Schema/ScriptableConnectionDrawingPolicy.h"
#include "ScriptableNodes/ScriptableGraph.h"
#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptableTasks/ScriptableActionAsset.h"
#include "ScriptableNodes/ScriptableNode_Task.h"
#include "ScriptableTasks/ScriptableTask_RunGraph.h"
#include "ScriptableFrameworkEd/Graph/ScriptableGraphEditorHelpers.h"
#include "ScopedTransaction.h"

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

	/** Returns true if the schema can spawn a node from the given asset. Used both for the hover-message decision (green / red icon) and the actual drop dispatch. */
	bool IsScriptableDroppable(const FAssetData& Asset)
	{
		const UClass* AssetClass = Asset.GetClass();
		if (!AssetClass) return false;
		return AssetClass->IsChildOf(UScriptableActionAsset::StaticClass())
			|| AssetClass->IsChildOf(UScriptableGraph::StaticClass());
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

	// An OUTPUT can fan out to at most one consumer.
	// An INPUT can accept fan-in from multiple sources
	const UEdGraphPin* OutputPin = (A->Direction == EGPD_Output) ? A : B;
	const bool bOutputBusy = OutputPin && OutputPin->LinkedTo.Num() > 0;

	if (bOutputBusy)
	{
		// Break only the output side, leave any existing input-side connections intact.
		const ECanCreateConnectionResponse Response = (A->Direction == EGPD_Output) ? CONNECT_RESPONSE_BREAK_OTHERS_A : CONNECT_RESPONSE_BREAK_OTHERS_B;
		return FPinConnectionResponse(Response, LOCTEXT("ReplaceOutput", "Replace existing output connection."));
	}

	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, FText::GetEmpty());
}

bool UScriptableEdGraphSchema::TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const
{
	const FPinConnectionResponse Response = CanCreateConnection(A, B);
	if (Response.Response == CONNECT_RESPONSE_DISALLOW) return false;

	// Default TryCreateConnection's BreakAllPinLinks() for BREAK_OTHERS_* sidesteps BreakSinglePinLink,
	// leaving stale UScriptableGraph::Connections. Route the breaks through this schema instead.
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
	const FScopedTransaction Transaction(NSLOCTEXT("ScriptableEdGraphSchema", "BreakSingleLink", "Break Single Link"));

	UEdGraphPin* FromPin = nullptr;
	UEdGraphPin* ToPin = nullptr;
	if (!SortPinsByDirection(SourcePin, TargetPin, FromPin, ToPin)) { Super::BreakSinglePinLink(SourcePin, TargetPin); return; }

	UScriptableGraph* GraphAsset = GetOwningGraphAsset(FromPin);

	const UScriptableEdGraphNode* FromEdNode = Cast<UScriptableEdGraphNode>(FromPin->GetOwningNode());
	const UScriptableEdGraphNode* ToEdNode = Cast<UScriptableEdGraphNode>(ToPin->GetOwningNode());

	if (GraphAsset) GraphAsset->Modify();
	if (FromPin->GetOwningNode()) FromPin->GetOwningNode()->Modify();
	if (ToPin->GetOwningNode()) ToPin->GetOwningNode()->Modify();

	Super::BreakSinglePinLink(SourcePin, TargetPin);

	if (!GraphAsset || !FromEdNode || !ToEdNode || !FromEdNode->GetRuntimeNode() || !ToEdNode->GetRuntimeNode()) return;

	const FGuid FromID = FromEdNode->GetRuntimeNode()->GetBindingID();
	const FGuid ToID = ToEdNode->GetRuntimeNode()->GetBindingID();
	const FName FromPinName = FromPin->PinName;
	const FName ToPinName = ToPin->PinName;

	GraphAsset->Connections.RemoveAll([&](const FScriptableGraphConnection& C)
		{
			return C.From.NodeID == FromID && C.From.PinName == FromPinName
				&& C.To.NodeID == ToID && C.To.PinName == ToPinName;
		});
}

void UScriptableEdGraphSchema::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotification) const
{
	const FScopedTransaction Transaction(NSLOCTEXT("ScriptableEdGraphSchema", "BreakPinLinks", "Break Pin Links"));

	const UScriptableEdGraphNode* SfNode = Cast<UScriptableEdGraphNode>(TargetPin.GetOwningNode());
	const UEdGraph* OwningGraph = TargetPin.GetOwningNode() ? TargetPin.GetOwningNode()->GetGraph() : nullptr;
	UScriptableGraph* GraphAsset = OwningGraph ? Cast<UScriptableGraph>(OwningGraph->GetOuter()) : nullptr;

	if (GraphAsset) GraphAsset->Modify();
	if (TargetPin.GetOwningNode()) TargetPin.GetOwningNode()->Modify();
	for (UEdGraphPin* LinkedPin : TargetPin.LinkedTo)
	{
		if (LinkedPin && LinkedPin->GetOwningNode()) LinkedPin->GetOwningNode()->Modify();
	}

	Super::BreakPinLinks(TargetPin, bSendsNodeNotification);

	if (!SfNode || !SfNode->GetRuntimeNode() || !GraphAsset) return;

	const FGuid NodeID = SfNode->GetRuntimeNode()->GetBindingID();
	const FName PinName = TargetPin.PinName;
	const EEdGraphPinDirection Direction = TargetPin.Direction;

	GraphAsset->Connections.RemoveAll([&](const FScriptableGraphConnection& C)
		{
			if (Direction == EGPD_Output)
			{
				return C.From.NodeID == NodeID && C.From.PinName == PinName;
			}
			return C.To.NodeID == NodeID && C.To.PinName == PinName;
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
	if (!Menu || !Context || !Context->Node) return;

	// Two routes: pin context vs empty-graph context. FGraphEditorCommands entries are wired by the
	// SGraphEditor widget; our FScriptableGraphCommands entries are wired in the toolkit's BindGraphCommands.
	if (Context->Pin)
	{
		// The engine adds the standard PIN ACTIONS (Break, Straighten, Select...) before this
		// callback, so adding them here would duplicate the section. Contribute only node-type-
		// specific entries (e.g. Sequence's Remove pin).
		if (const UScriptableEdGraphNode* SfEdNode = Cast<UScriptableEdGraphNode>(Context->Node))
		{
			SfEdNode->AppendPinContextActions(Menu, Context);
		}
		return;
	}

	// The SGraphEditor attaches its FUICommandList (the one we filled in BindGraphCommands) to the
	// menu, so referencing FGenericCommands entries resolves to our handlers without further wiring.
	FToolMenuSection& Section = Menu->AddSection(TEXT("ScriptableNodeEdit"), LOCTEXT("NodeEditSection", "Edit"));
	Section.AddMenuEntry(FGenericCommands::Get().Cut);
	Section.AddMenuEntry(FGenericCommands::Get().Copy);
	Section.AddMenuEntry(FGenericCommands::Get().Paste);
	Section.AddMenuEntry(FGenericCommands::Get().Duplicate);
	Section.AddSeparator(TEXT("ScriptableNodeEditDeleteSep"));
	Section.AddMenuEntry(FGenericCommands::Get().Delete);

	Super::GetContextMenuActions(Menu, Context);
}

FConnectionDrawingPolicy* UScriptableEdGraphSchema::CreateConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj) const
{
	return new FScriptableConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements);
}

void UScriptableEdGraphSchema::GetAssetsGraphHoverMessage(const TArray<FAssetData>& Assets, const UEdGraph* HoverGraph, FString& OutTooltipText, bool& OutOkIcon) const
{
	// Called while dragging Content-Browser assets over the canvas: green icon + tooltip if at least
	// one asset is droppable (action or graph), red icon + explanation otherwise.
	int32 NumDroppable = 0;
	for (const FAssetData& Asset : Assets)
	{
		if (IsScriptableDroppable(Asset)) ++NumDroppable;
	}

	if (NumDroppable == 0)
	{
		OutOkIcon = false;
		OutTooltipText = TEXT("Drop a Scriptable Action or Scriptable Graph asset to add it as a node.");
		return;
	}

	OutOkIcon = true;
	OutTooltipText = FString::Printf(TEXT("Drop %d Scriptable asset(s) here"), NumDroppable);
}

void UScriptableEdGraphSchema::DroppedAssetsOnGraph(const TArray<FAssetData>& Assets, const FVector2f& GraphPosition, UEdGraph* Graph) const
{
	if (!Graph) return;

	const FScopedTransaction Transaction(NSLOCTEXT("ScriptableEdGraphSchema", "DropAssetsTx", "Drop Assets on Graph"));

	// Stagger the spawn positions so dropping multiple assets at once doesn't pile them on top of
	// each other. Same vertical offset BP uses when handling multi-asset drops.
	FVector2f Cursor = GraphPosition;
	constexpr float VerticalStep = 120.0f;

	for (const FAssetData& Asset : Assets)
	{
		const UClass* AssetClass = Asset.GetClass();
		if (!AssetClass) continue;

		UEdGraphNode* SpawnedNode = nullptr;

		if (AssetClass->IsChildOf(UScriptableActionAsset::StaticClass()))
		{
			// Action asset: spawn a Task wrapper around UScriptableTask_RunAsset, then point its
			// Asset field at the dropped action.
			UScriptableActionAsset* ActionAsset = Cast<UScriptableActionAsset>(Asset.GetAsset());
			if (!ActionAsset) continue;

			SpawnedNode = ScriptableGraphEditorHelpers::SpawnTaskNode(Graph, UScriptableTask_RunAsset::StaticClass(), Cursor, /*FromPin*/ nullptr, /*bSelectNewNode*/ true);
			if (UScriptableEdGraphNode* SfEdNode = Cast<UScriptableEdGraphNode>(SpawnedNode))
			{
				if (UScriptableNode_Task* Wrapper = Cast<UScriptableNode_Task>(SfEdNode->GetRuntimeNode()))
				{
					if (UScriptableTask_RunAsset* RunAssetTask = Cast<UScriptableTask_RunAsset>(Wrapper->Task))
					{
						RunAssetTask->Modify();
						RunAssetTask->Asset = ActionAsset;
						SfEdNode->ReconstructNode();
					}
				}
			}
		}
		else if (AssetClass->IsChildOf(UScriptableGraph::StaticClass()))
		{
			// Graph asset: spawn a Task wrapper around UScriptableTask_RunGraph, set the asset.
			UScriptableGraph* GraphAsset = Cast<UScriptableGraph>(Asset.GetAsset());
			if (!GraphAsset) continue;

			SpawnedNode = ScriptableGraphEditorHelpers::SpawnTaskNode(Graph, UScriptableTask_RunGraph::StaticClass(), Cursor, /*FromPin*/ nullptr, /*bSelectNewNode*/ true);
			if (UScriptableEdGraphNode* SfEdNode = Cast<UScriptableEdGraphNode>(SpawnedNode))
			{
				if (UScriptableNode_Task* Wrapper = Cast<UScriptableNode_Task>(SfEdNode->GetRuntimeNode()))
				{
					if (UScriptableTask_RunGraph* RunGraphTask = Cast<UScriptableTask_RunGraph>(Wrapper->Task))
					{
						RunGraphTask->Modify();
						RunGraphTask->GraphAsset = GraphAsset;
						SfEdNode->ReconstructNode();
					}
				}
			}
		}

		if (SpawnedNode)
		{
			Cursor.Y += VerticalStep;
		}
	}
}

#undef LOCTEXT_NAMESPACE