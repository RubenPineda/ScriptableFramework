// Copyright 2026 kirzo

#include "ScriptableNodes/ScriptableGraph.h"
#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptableNodes/ScriptableNode_Entry.h"
#include "Core/KzBagOps.h"

UScriptableGraph::UScriptableGraph()
{
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
		ContextBag.Reset();
		for (const FKzParamDef& Param : Context)
		{
			KzBagOps::AddProperty(ContextBag, Param);
		}
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