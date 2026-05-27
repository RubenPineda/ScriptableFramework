// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableTasks/ScriptableTask.h"
#include "Templates/SubclassOf.h"
#include "ScriptableTask_CreateWidget.generated.h"

class UUserWidget;
class APlayerController;

/** Where a freshly created widget should be placed. */
UENUM(BlueprintType)
enum class EScriptableWidgetAddTarget : uint8
{
	/** Create the widget but leave it unattached (a later task adds it). */
	None,
	/** Add to the game viewport. */
	Viewport,
	/** Add to the owning player's screen (split-screen aware). Requires an owning player. */
	PlayerScreen
};

/**
 * Creates a UMG widget and optionally adds it to the viewport or the player screen. Immediate (finishes
 * the same frame). Exposes the created widget as an Output so sibling tasks can bind to it.
 */
UCLASS(DisplayName = "Create Widget", meta = (TaskCategory = "UI"))
class UScriptableTask_CreateWidget : public UScriptableTask
{
	GENERATED_BODY()

public:
	/** Widget class to instantiate. Null = the task is a no-op. */
	UPROPERTY(EditAnywhere, Category = "Config")
	TSubclassOf<UUserWidget> WidgetClass;

	/** Where to place the created widget (or None to leave it unattached). */
	UPROPERTY(EditAnywhere, Category = "Config")
	EScriptableWidgetAddTarget AddTarget = EScriptableWidgetAddTarget::Viewport;

	/** Draw order when added; higher draws on top. */
	UPROPERTY(EditAnywhere, Category = "Config", meta = (EditCondition = "AddTarget != EScriptableWidgetAddTarget::None"))
	int32 ZOrder = 0;

	/** Owning player for the widget. Required for PlayerScreen; if unset, falls back to the first local player. */
	UPROPERTY(EditAnywhere, Category = "Config")
	TObjectPtr<APlayerController> OwningPlayer;

	/** The created widget. Bind subsequent tasks to this. */
	UPROPERTY(VisibleAnywhere, Transient, BlueprintReadOnly, Category = "Output")
	TObjectPtr<UUserWidget> Widget = nullptr;

	virtual bool IsStoppable() const override { return false; }

protected:
	virtual void BeginTask() override;
	virtual void ResetTask() override;

#if WITH_EDITOR
	virtual FText GetDisplayTitle() const override;
#endif
};
