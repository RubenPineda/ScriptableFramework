// Copyright 2026 kirzo

#include "Tasks/ScriptableTask_RemoveWidget.h"
#include "Blueprint/UserWidget.h"

void UScriptableTask_RemoveWidget::BeginTask()
{
	// Immediate: detach the widget if we have one.
	if (Widget)
	{
		Widget->RemoveFromParent();
	}

	Finish();
}

#if WITH_EDITOR
FText UScriptableTask_RemoveWidget::GetDisplayTitle() const
{
	FString WidgetName;
	if (!GetBindingDisplayText(GET_MEMBER_NAME_CHECKED(UScriptableTask_RemoveWidget, Widget), WidgetName))
	{
		WidgetName = Widget ? Widget->GetName() : TEXT("None");
	}
	return FText::Format(INVTEXT("Remove {0}"), FText::FromString(WidgetName));
}
#endif
