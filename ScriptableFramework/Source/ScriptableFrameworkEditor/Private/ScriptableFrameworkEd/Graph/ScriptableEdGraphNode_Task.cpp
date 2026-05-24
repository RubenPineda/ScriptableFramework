// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode_Task.h"
#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptableNodes/ScriptableNode_Task.h"
#include "ScriptableTasks/ScriptableTask.h"
#include "ScriptableFrameworkEditorStyle.h"

FText UScriptableEdGraphNode_Task::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (UScriptableNode_Task* TaskNode = Cast<UScriptableNode_Task>(GetRuntimeNode()))
	{
		if (TaskNode->Task)
		{
			return TaskNode->Task->GetDisplayTitle();
		}
	}
	return NSLOCTEXT("ScriptableEdGraphNode_Task", "EmptyTaskTitle", "Task (empty)");
}

FLinearColor UScriptableEdGraphNode_Task::GetNodeTitleColor() const
{
	if (const UScriptableNode_Task* TaskNode = Cast<UScriptableNode_Task>(GetRuntimeNode()))
	{
		if (const UScriptableTask* Task = TaskNode->Task)
		{
			if (TOptional<FLinearColor> Custom = Task->GetNodeTitleColor()) return *Custom;
		}
	}

	return FScriptableFrameworkEditorStyle::ScriptableTaskColor;
}

bool UScriptableEdGraphNode_Task::ShouldShowPinLabel(FName PinName) const
{
	if (PinName == UScriptableNode_Task::StartInputName) return false;
	if (PinName == UScriptableTask::CompletedOutputName) return false;
	return true;
}