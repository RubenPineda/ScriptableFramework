// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableTasks/ScriptableTask.h"
#include "Core/KzVariant.h"
#include "ScriptableTask_SetLocal.generated.h"

/**
 * Writes a value into one of the graph instance's Locals slots and fires Completed immediately.
 * VarName picks the destination from the owning asset's Locals; Value is type-constrained to that local
 * in the details customization. Non-stoppable (the write is atomic, no in-flight work to cancel).
 */
UCLASS(DisplayName = "Set Local", meta = (NodeCategory = "System|Variables"))
class SCRIPTABLEFRAMEWORK_API UScriptableTask_SetLocal : public UScriptableTask
{
	GENERATED_BODY()

public:
	UScriptableTask_SetLocal();

	/** Target local. Rendered in the details panel as a dropdown over the owning asset's Locals names. */
	UPROPERTY(EditAnywhere, Category = "Set Local", meta = (NoBinding))
	FName VarName = NAME_None;

	/** Value to write. The customization forces this to the type of the selected local; disabled when none is picked. */
	UPROPERTY(EditAnywhere, Category = "Set Local", meta = (NoBinding))
	FKzVariant Value;

	virtual bool IsStoppable() const override { return false; }

#if WITH_EDITOR
	virtual FText GetDisplayTitle() const override;
#endif

protected:
	virtual void BeginTask() override;
};