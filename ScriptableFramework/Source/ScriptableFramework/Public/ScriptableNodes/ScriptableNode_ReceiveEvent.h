// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptableNode_ReceiveEvent.generated.h"

/**
 * External-trigger entry point: no inputs, single "Out". Unlike Entry, fires not on launch but when
 * gameplay calls UScriptableGraphInstance::FireEvent(EventName); all matching ReceiveEvent nodes fire
 * in parallel (intentional fan-out across branches).
 */
UCLASS(DisplayName = "Event", meta = (NodeCategory = "System|Flow"))
class SCRIPTABLEFRAMEWORK_API UScriptableNode_ReceiveEvent : public UScriptableNode
{
	GENERATED_BODY()

public:
	/** Symbolic event key the gameplay caller uses to address this node. Empty = inert. */
	UPROPERTY(EditAnywhere, Category = "Receive Event")
	FName EventName;

	/** Canonical output pin name. */
	static const FName OutOutputName;

	//~ UScriptableNode interface
	virtual TArray<FName> GetInputPins() const override;
	virtual TArray<FName> GetDeclaredOutputPins() const override;
	//~ End of UScriptableNode interface

#if WITH_EDITOR
	virtual FText GetDisplayTitle() const override;

	//~ UObject interface
	virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End of UObject interface

	/** Walks Graph->Nodes and rewrites every UScriptableNode_GoTo whose TargetEvent equals OldName. Shared by F2 rename and PostEditChange so both paths refactor consistently. */
	static void ApplyTargetReferenceRename(class UScriptableGraph* Graph, FName OldName, FName NewName);
#endif

	/** Called by FireEvent when EventName matches: arms and fires the single output. Mirrors UScriptableNode_Entry::Activate. */
	void Trigger();

#if WITH_EDITORONLY_DATA
private:
	/** Snapshot taken in PreEditChange so PostEditChangeProperty can see the prior value and refactor GoTos. */
	FName PreviousEventName_ForRename;
#endif
};