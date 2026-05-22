// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode_Task.h"
#include "ScriptableNodes/ScriptableNode_Task.h"
#include "ScriptableTasks/ScriptableTask.h"

FText UScriptableEdGraphNode_Task::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (UScriptableNode_Task* TaskNode = Cast<UScriptableNode_Task>(GetRuntimeNode()))
	{
		if (TaskNode->Task)
		{
			return TaskNode->Task->GetClass()->GetDisplayNameText();
		}
	}
	return NSLOCTEXT("ScriptableEdGraphNode_Task", "EmptyTaskTitle", "Task (empty)");
}

FLinearColor UScriptableEdGraphNode_Task::GetNodeTitleColor() const
{
	return FLinearColor(0.25f, 0.45f, 0.65f);
}