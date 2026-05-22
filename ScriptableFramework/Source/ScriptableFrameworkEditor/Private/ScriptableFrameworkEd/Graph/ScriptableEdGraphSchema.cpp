// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphSchema.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphSchemaActions.h"
#include "ScriptableTasks/ScriptableTask.h"

#include "EdGraph/EdGraph.h"
#include "UObject/UObjectIterator.h"

#define LOCTEXT_NAMESPACE "ScriptableEdGraphSchema"

void UScriptableEdGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	const FText TaskCategory = LOCTEXT("TaskCategory", "Tasks");

	// Enumerate every concrete UScriptableTask subclass and offer one menu entry per class.
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Class = *It;
		if (!Class->IsChildOf(UScriptableTask::StaticClass())) continue;
		if (Class == UScriptableTask::StaticClass()) continue;
		if (Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists)) continue;

		const FString ClassName = Class->GetName();
		if (ClassName.StartsWith(TEXT("SKEL_")) || ClassName.StartsWith(TEXT("REINST_"))) continue;

		const FText MenuDesc = Class->GetDisplayNameText();
		const FText Tooltip = Class->GetToolTipText();

		TSharedPtr<FScriptableEdGraphSchemaAction_NewTaskNode> Action = MakeShared<FScriptableEdGraphSchemaAction_NewTaskNode>(TaskCategory, MenuDesc, Tooltip, 0, Class);
		ContextMenuBuilder.AddAction(Action);
	}
}

#undef LOCTEXT_NAMESPACE