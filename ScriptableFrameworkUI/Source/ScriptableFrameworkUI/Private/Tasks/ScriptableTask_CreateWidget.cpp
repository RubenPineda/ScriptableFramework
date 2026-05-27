// Copyright 2026 kirzo

#include "Tasks/ScriptableTask_CreateWidget.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

void UScriptableTask_CreateWidget::BeginTask()
{
	// Synchronous: creating a widget is instant.
	if (WidgetClass)
	{
		if (UWorld* World = GetWorld())
		{
			// Owning player: the explicit one, else the first local player controller.
			APlayerController* PC = OwningPlayer ? OwningPlayer.Get() : UGameplayStatics::GetPlayerController(World, 0);

			// Create with a player owner when we have one (required for PlayerScreen), else the world.
			Widget = PC ? CreateWidget<UUserWidget>(PC, WidgetClass) : CreateWidget<UUserWidget>(World, WidgetClass);

			if (Widget)
			{
				switch (AddTarget)
				{
				case EScriptableWidgetAddTarget::Viewport:
					Widget->AddToViewport(ZOrder);
					break;
				case EScriptableWidgetAddTarget::PlayerScreen:
					Widget->AddToPlayerScreen(ZOrder);
					break;
				case EScriptableWidgetAddTarget::None:
				default:
					break;
				}
			}
		}
	}

	Finish();
}

void UScriptableTask_CreateWidget::ResetTask()
{
	// Remove the widget so loops / re-runs don't stack or leak widgets.
	if (Widget)
	{
		Widget->RemoveFromParent();
	}
	Widget = nullptr;
}

#if WITH_EDITOR
FText UScriptableTask_CreateWidget::GetDisplayTitle() const
{
	const FString ClassName = WidgetClass ? WidgetClass->GetName() : TEXT("None");
	return FText::Format(INVTEXT("Create Widget [{0}]"), FText::FromString(ClassName));
}
#endif
