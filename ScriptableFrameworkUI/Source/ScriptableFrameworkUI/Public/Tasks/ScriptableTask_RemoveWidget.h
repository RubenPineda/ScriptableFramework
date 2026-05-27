// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableTasks/ScriptableTask.h"
#include "ScriptableTask_RemoveWidget.generated.h"

class UUserWidget;

/**
 * Removes a UMG widget from its parent (RemoveFromParent). Immediate (finishes the same frame).
 * Wire the Widget input to a sibling task's output (e.g. Create Widget).
 */
UCLASS(DisplayName = "Remove Widget", meta = (TaskCategory = "UI"))
class UScriptableTask_RemoveWidget : public UScriptableTask
{
	GENERATED_BODY()

public:
	/** Widget to remove. Bind this to a sibling task's output (e.g. Create Widget). */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UUserWidget> Widget = nullptr;

	virtual bool IsStoppable() const override { return false; }

protected:
	virtual void BeginTask() override;

#if WITH_EDITOR
	virtual FText GetDisplayTitle() const override;
#endif
};
