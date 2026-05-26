// Copyright 2026 kirzo

#include "ScriptableNodes/ScriptableNode_Sequence.h"
#include "ScriptableNodes/ScriptableGraph.h"

const FName UScriptableNode_Sequence::InInputName = TEXT("In");

FName UScriptableNode_Sequence::MakeOutputName(int32 BranchIndex)
{
	return FName(*FString::Printf(TEXT("%d"), BranchIndex));
}

TArray<FName> UScriptableNode_Sequence::GetInputPins() const
{
	return { InInputName };
}

TArray<FName> UScriptableNode_Sequence::GetDeclaredOutputPins() const
{
	const int32 Count = FMath::Max(1, OutputCount);

	TArray<FName> Outputs;
	Outputs.Reserve(Count);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		Outputs.Add(MakeOutputName(Index));
	}
	return Outputs;
}

void UScriptableNode_Sequence::ProcessInput(FName InputName)
{
	if (InputName != InInputName) return;

	const int32 Count = FMath::Max(1, OutputCount);

	// Arm every output up-front.
	for (int32 Index = 0; Index < Count; ++Index)
	{
		MarkOutputActive(MakeOutputName(Index));
	}

	// Consume the input before firing.
	MarkInputInactive(InInputName);

	for (int32 Index = 0; Index < Count; ++Index)
	{
		FireOutput(MakeOutputName(Index));
	}
}

#if WITH_EDITOR
void UScriptableNode_Sequence::AddOutputPin()
{
	Modify();
	OutputCount += 1;

	FProperty* OutputCountProperty = FindFProperty<FProperty>(GetClass(), GET_MEMBER_NAME_CHECKED(UScriptableNode_Sequence, OutputCount));
	FPropertyChangedEvent ChangeEvent(OutputCountProperty, EPropertyChangeType::ValueSet);
	PostEditChangeProperty(ChangeEvent);
}

void UScriptableNode_Sequence::RemoveOutputPinAt(int32 BranchIndex)
{
	if (OutputCount <= 1) return;
	if (BranchIndex < 0 || BranchIndex >= OutputCount) return;

	UScriptableGraph* OwningGraph = Cast<UScriptableGraph>(GetOuter());

	Modify();
	if (OwningGraph) OwningGraph->Modify();

	// Rewrite outgoing connections in two phases for clarity:
	//   1. Drop wires originating from the victim pin (BranchIndex).
	//   2. Shift wires originating from any higher-indexed pin (BranchIndex+1..OutputCount-1)
	//      down by one. Iterate ascending so each rename targets a free slot.
	// Asset->Connections is the source of truth.
	if (OwningGraph)
	{
		const FGuid SelfID = GetBindingID();
		const FName VictimPinName = MakeOutputName(BranchIndex);

		OwningGraph->Connections.RemoveAll([&SelfID, &VictimPinName](const FScriptableGraphConnection& Conn)
			{
				return Conn.From.NodeID == SelfID && Conn.From.PinName == VictimPinName;
			});

		for (int32 OldIdx = BranchIndex + 1; OldIdx < OutputCount; ++OldIdx)
		{
			const FName OldName = MakeOutputName(OldIdx);
			const FName NewName = MakeOutputName(OldIdx - 1);

			for (FScriptableGraphConnection& Conn : OwningGraph->Connections)
			{
				if (Conn.From.NodeID == SelfID && Conn.From.PinName == OldName)
				{
					Conn.From.PinName = NewName;
				}
			}
		}
	}

	OutputCount -= 1;

	FProperty* OutputCountProperty = FindFProperty<FProperty>(GetClass(), GET_MEMBER_NAME_CHECKED(UScriptableNode_Sequence, OutputCount));
	FPropertyChangedEvent ChangeEvent(OutputCountProperty, EPropertyChangeType::ValueSet);
	PostEditChangeProperty(ChangeEvent);
}
#endif