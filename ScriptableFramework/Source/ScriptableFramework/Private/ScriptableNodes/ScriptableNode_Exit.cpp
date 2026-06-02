// Copyright 2026 kirzo

#include "ScriptableNodes/ScriptableNode_Exit.h"
#include "ScriptableNodes/ScriptableGraph.h"
#include "ScriptableNodes/ScriptableGraphInstance.h"

const FName UScriptableNode_Exit::FinishedOutputName = TEXT("Finished");
const FName UScriptableNode_Exit::CancelledOutputName = TEXT("Cancelled");

TArray<FName> UScriptableNode_Exit::GetDeclaredOutputPins() const
{
	TArray<FName> Names = { FinishedOutputName, CancelledOutputName };

	// Outer is the graph asset at design time, the runner at runtime (which holds the asset).
	const UScriptableGraph* OwningGraph = nullptr;
	for (const UObject* Cursor = GetOuter(); Cursor; Cursor = Cursor->GetOuter())
	{
		if (const UScriptableGraph* AsGraph = Cast<UScriptableGraph>(Cursor)) { OwningGraph = AsGraph; break; }
		if (const UScriptableGraphInstance* AsInstance = Cast<UScriptableGraphInstance>(Cursor)) { OwningGraph = AsInstance->GetAsset(); break; }
	}

	if (OwningGraph)
	{
		for (const FName& UserOutput : OwningGraph->Outputs)
		{
			if (!UserOutput.IsNone()) Names.AddUnique(UserOutput);
		}
	}

	return Names;
}
