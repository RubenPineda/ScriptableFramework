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

	/** Target local. UE renders this as a dropdown driven by GetLocalNames over the owning asset's Locals. */
	UPROPERTY(EditAnywhere, Category = "Set Local", meta = (NoBinding, GetOptions = "GetLocalNames"))
	FName VarName = NAME_None;

	/** Value to write. FixedType meta hides FKzVariant's type selector; PostEditChangeProperty force-syncs Type to the picked local. */
	UPROPERTY(EditAnywhere, Category = "Set Local", meta = (NoBinding, FixedType))
	FKzVariant Value;

	virtual bool IsStoppable() const override { return false; }

#if WITH_EDITOR
	virtual FText GetDisplayTitle() const override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	virtual void BeginTask() override;

private:
#if WITH_EDITOR
	/** Backing function for VarName's GetOptions meta. Returns the names of all valid Locals on the owning UScriptableObjectAsset. */
	UFUNCTION()
	TArray<FString> GetLocalNames() const;

	/** Looks up the FKzNamedVariant matching the current VarName on the owning asset's Locals. Null if none. */
	const struct FKzNamedVariant* FindOwningLocal() const;
#endif
};