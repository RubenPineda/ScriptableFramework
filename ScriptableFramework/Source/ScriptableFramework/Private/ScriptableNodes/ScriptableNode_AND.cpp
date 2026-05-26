// Copyright 2026 kirzo

#include "ScriptableNodes/ScriptableNode_AND.h"
#include "ScriptableNodes/ScriptableGraph.h"

const FName UScriptableNode_AND::OutOutputName = TEXT("Out");

FName UScriptableNode_AND::MakeInputName(int32 BranchIndex)
{
	return FName(*FString::Printf(TEXT("%d"), BranchIndex));
}

TArray<FName> UScriptableNode_AND::GetInputPins() const
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

TArray<FName> UScriptableNode_AND::GetDeclaredOutputPins() const
{
	return { OutOutputName };
}

void UScriptableNode_AND::ProcessInput(FName InputName)
{
	const TArray<FName> AllInputs = GetInputPins();
	if (!AllInputs.Contains(InputName)) return;

	// Mark this input consumed (it's "in" until we fire).
	SeenInputs.Add(InputName);
	MarkInputInactive(InputName);

	// Not yet all inputs pulsed → stay quiet and keep waiting.
	if (SeenInputs.Num() < AllInputs.Num()) return;

	// All inputs pulsed at least once. Fire and rearm.
	SeenInputs.Reset();
	MarkOutputActive(OutOutputName);
	FireOutput(OutOutputName);
}

#if WITH_EDITOR
void UScriptableNode_AND::PostEditUndo()
{
	Super::PostEditUndo();

	FProperty* InputCountProperty = FindFProperty<FProperty>(GetClass(), GET_MEMBER_NAME_CHECKED(UScriptableNode_AND, InputCount));
	FPropertyChangedEvent ChangeEvent(InputCountProperty, EPropertyChangeType::ValueSet);
	PostEditChangeProperty(ChangeEvent);
}

FText UScriptableNode_AND::GetDisplayTitle() const
{
	return INVTEXT("AND");
}

void UScriptableNode_AND::AddInputPin()
{
	Modify();
	InputCount += 1;

	FProperty* InputCountProperty = FindFProperty<FProperty>(GetClass(), GET_MEMBER_NAME_CHECKED(UScriptableNode_AND, InputCount));
	FPropertyChangedEvent ChangeEvent(InputCountProperty, EPropertyChangeType::ValueSet);
	PostEditChangeProperty(ChangeEvent);
}

void UScriptableNode_AND::RemoveInputPinAt(int32 BranchIndex)
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

	// Same connection-rewriting as Sequence, but for *incoming* wires (To.PinName instead of
	// From.PinName). Drop entries that arrive at the victim pin; shift everything above it down.
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

	// Defensive: forget any pending pulse on the victim pin so the gate doesn't carry stale state
	// across a structural change. SeenInputs is transient and only matters at runtime, but if a
	// graph is being edited during a paused-runtime debug session this avoids a deadlock on
	// the next pulse.
	SeenInputs.Remove(MakeInputName(BranchIndex));

	InputCount -= 1;

	FProperty* InputCountProperty = FindFProperty<FProperty>(GetClass(), GET_MEMBER_NAME_CHECKED(UScriptableNode_AND, InputCount));
	FPropertyChangedEvent ChangeEvent(InputCountProperty, EPropertyChangeType::ValueSet);
	PostEditChangeProperty(ChangeEvent);
}
#endif