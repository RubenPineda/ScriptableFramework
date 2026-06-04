// Copyright 2026 kirzo

#include "ScriptableNodes/AsyncRunScriptableGraph.h"
#include "ScriptableNodes/ScriptableGraph.h"
#include "ScriptableNodes/ScriptableGraphInstance.h"

UAsyncRunScriptableGraph* UAsyncRunScriptableGraph::RunScriptableGraph(UObject* Owner, UScriptableGraph* Graph, const FScriptableContext& Context, FName Id)
{
	UAsyncRunScriptableGraph* Node = NewObject<UAsyncRunScriptableGraph>(Owner);

	Node->GraphOwner = Owner;
	Node->GraphAsset = Graph;
	Node->LaunchContext = Context;
	Node->LaunchId = Id;

	if (Owner)
	{
		Node->RegisterWithGameInstance(Owner);
	}

	return Node;
}

void UAsyncRunScriptableGraph::Activate()
{
	Super::Activate();

	if (!GraphOwner || !GraphAsset)
	{
		SetReadyToDestroy();
		return;
	}

	Runner = UScriptableGraph::Run(GraphAsset, GraphOwner, LaunchContext, LaunchId);
	if (!Runner)
	{
		SetReadyToDestroy();
		return;
	}

	// Hand the runner out immediately so callers can drive it (events, context) while it runs.
	Started.Broadcast(Runner);

	// A graph with no latent work finishes synchronously inside Run, before we could subscribe.
	if (Runner->IsFinished())
	{
		HandleGraphFinished();
		return;
	}

	Runner->OnGraphFinishedNative.AddUObject(this, &UAsyncRunScriptableGraph::HandleGraphFinished);
}

void UAsyncRunScriptableGraph::HandleGraphFinished()
{
	if (Runner)
	{
		Runner->OnGraphFinishedNative.RemoveAll(this);
	}

	Finished.Broadcast(Runner);
	SetReadyToDestroy();
}

void UAsyncRunScriptableGraph::SetReadyToDestroy()
{
	if (Runner)
	{
		Runner->OnGraphFinishedNative.RemoveAll(this);
	}

	Runner = nullptr;
	GraphOwner = nullptr;
	GraphAsset = nullptr;

	Super::SetReadyToDestroy();
}
