// Copyright 2026 kirzo

#include "ScriptableNodes/ScriptableGraph.h"
#include "ScriptableNodes/ScriptableGraphInstance.h"
#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptableNodes/ScriptableNode_Entry.h"
#include "Core/KzBagOps.h"

UScriptableGraph::UScriptableGraph()
{
}

UScriptableGraphInstance* UScriptableGraph::Run(UScriptableGraph* Graph, UObject* Owner, const FScriptableContext& InContext)
{
	if (!Graph || !Owner) return nullptr;

	UScriptableGraphInstance* Instance = NewObject<UScriptableGraphInstance>(Owner);
	if (!Instance) return nullptr;

	Instance->Launch(Graph, Owner, InContext);
	return Instance;
}

void UScriptableGraph::PostInitProperties()
{
	Super::PostInitProperties();

	// Skip CDOs, archetypes and objects being loaded. Only freshly constructed live assets get an Entry here.
	if (HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad | RF_NeedPostLoad)) return;

	EnsureEntryNode();
}

void UScriptableGraph::PostLoad()
{
	Super::PostLoad();

	// Heal assets that lost their entry node (legacy data, external edits, etc.).
	EnsureEntryNode();
	RebuildContextBag();
	PruneOrphanConnections();
}

#if WITH_EDITOR
void UScriptableGraph::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	// The base class only refreshes the bag when the property name matches GetContainerName(). For the graph,
	// the declared context lives directly on the inherited Context array, so refresh whenever it is edited.
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UScriptableObjectAsset, Context))
	{
		RebuildContextBag();
	}
}
#endif

void UScriptableGraph::EnsureEntryNode()
{
	// Validate the cached EntryNodeID: it must point to a real Entry node still living in Nodes.
	const bool bEntryIsValid = EntryNodeID.IsValid() && Nodes.ContainsByPredicate([this](const TObjectPtr<UScriptableNode>& Node)
		{
			return Node && Node->GetBindingID() == EntryNodeID && Node->IsA<UScriptableNode_Entry>();
		});

	if (bEntryIsValid) return;

	// Create a fresh Entry, owned by this asset.
	UScriptableNode_Entry* Entry = NewObject<UScriptableNode_Entry>(this, NAME_None, RF_Transactional);
	if (!Entry) return;

	Nodes.Add(Entry);
	EntryNodeID = Entry->GetBindingID();

#if WITH_EDITOR
	// Flag the asset dirty so the auto-repair persists.
	Modify();
#endif
}

void UScriptableGraph::RebuildContextBag()
{
	ContextBag.Reset();
	for (const FKzParamDef& Param : Context)
	{
		KzBagOps::AddProperty(ContextBag, Param);
	}
}

void UScriptableGraph::PruneOrphanConnections()
{
	if (Connections.IsEmpty()) return;

	// Build a quick lookup of GUID -> node for the cross-checks. Node IDs are stable across runs;
	// pin names are not (a task's GetOutputPins can return different sets after edits), so we
	// validate both the endpoint node existence AND the endpoint pin name's current presence.
	TMap<FGuid, UScriptableNode*> NodesByGuid;
	NodesByGuid.Reserve(Nodes.Num());
	for (const TObjectPtr<UScriptableNode>& Node : Nodes)
	{
		if (Node) NodesByGuid.Add(Node->GetBindingID(), Node);
	}

	auto IsValidEndpoint = [&NodesByGuid](const FScriptableGraphPinRef& Ref, bool bExpectOutput) -> bool
		{
			UScriptableNode* const* Found = NodesByGuid.Find(Ref.NodeID);
			if (!Found || !*Found) return false;
			const UScriptableNode* Node = *Found;
			const TArray<FName> Pins = bExpectOutput ? Node->GetOutputPins() : Node->GetInputPins();
			return Pins.Contains(Ref.PinName);
		};

	const int32 OriginalCount = Connections.Num();
	Connections.RemoveAll([&IsValidEndpoint](const FScriptableGraphConnection& Conn)
		{
			return !IsValidEndpoint(Conn.From, /*bExpectOutput*/ true) || !IsValidEndpoint(Conn.To, /*bExpectOutput*/ false);
		});

	if (Connections.Num() != OriginalCount)
	{
		// Asset was effectively modified; mark dirty so the next save persists the cleanup.
		MarkPackageDirty();
	}
}