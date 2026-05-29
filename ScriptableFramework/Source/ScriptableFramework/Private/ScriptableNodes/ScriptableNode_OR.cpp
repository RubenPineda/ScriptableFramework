// Copyright 2026 kirzo

#include "ScriptableNodes/ScriptableNode_OR.h"
#include "ScriptableNodes/ScriptableGraph.h"

const FName UScriptableNode_OR::OutOutputName = TEXT("Out");

FName UScriptableNode_OR::MakeInputName(int32 BranchIndex)
{
	return FName(*FString::Printf(TEXT("%d"), BranchIndex));
}

TArray<FName> UScriptableNode_OR::GetInputPins() const
{
	const int32 Count = FMath::Max(MinInputCount, InputCount);

	TArray<FName> Inputs;
	Inputs.Reserve(Count);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		Inputs.Add(MakeInputName(Index));
	}
	return Inputs;
}

TArray<FName> UScriptableNode_OR::GetDeclaredOutputPins() const
{
	return { OutOutputName };
}

void UScriptableNode_OR::ProcessInput(FName InputName)
{
	const TArray<FName> AllInputs = GetInputPins();
	if (!AllInputs.Contains(InputName)) return;

	// Always drop the input from the active set: pulses are consumed instantly, regardless of
	// whether this is the one that fires the output or a no-op post-fire pulse. The wrapping
	// InactiveNotificationsSuppressed counter (set by ActivateInput) keeps the transient
	// add-then-remove invisible to observers.
	MarkInputInactive(InputName);

	if (bFired) return;

	bFired = true;
	MarkOutputActive(OutOutputName);
	FireOutput(OutOutputName);
}

#if WITH_EDITOR
FText UScriptableNode_OR::GetDisplayTitle() const
{
	return INVTEXT("OR");
}

void UScriptableNode_OR::AddInputPin()
{
	Modify();
	InputCount += 1;
}

void UScriptableNode_OR::RemoveInputPinAt(int32 BranchIndex)
{
	if (InputCount <= MinInputCount) return;
	if (BranchIndex < 0 || BranchIndex >= InputCount) return;

	UScriptableGraph* OwningGraph = Cast<UScriptableGraph>(GetOuter());

	Modify();
	if (OwningGraph)
	{
		OwningGraph->Modify();
		if (OwningGraph->EdGraph) OwningGraph->EdGraph->Modify();
	}

	// Same rewiring strategy as AND: drop wires landing on the victim pin, then shift higher-indexed
	// pins down by one so connections stay intact after the removal.
	if (OwningGraph)
	{
		const FGuid SelfID = GetBindingID();
		const FName VictimPinName = MakeInputName(BranchIndex);

		OwningGraph->Connections.RemoveAll([&SelfID, &VictimPinName](const FScriptableGraphConnection& Conn)
			{
				return Conn.To.NodeID == SelfID && Conn.To.PinName == VictimPinName;
			});

		for (int32 OldIdx = BranchIndex + 1; OldIdx < InputCount; ++OldIdx)
		{
			const FName OldName = MakeInputName(OldIdx);
			const FName NewName = MakeInputName(OldIdx - 1);

			for (FScriptableGraphConnection& Conn : OwningGraph->Connections)
			{
				if (Conn.To.NodeID == SelfID && Conn.To.PinName == OldName)
				{
					Conn.To.PinName = NewName;
				}
			}
		}
	}

	InputCount -= 1;
}
#endif
