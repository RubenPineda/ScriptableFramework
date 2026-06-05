// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Core/KzParamDef.h"
#include "Core/KzNamedVariant.h"
#include "ScriptableObjectAsset.generated.h"

/**
 * Base class for all Scriptable Assets (Actions, Requirements, etc).
 * Centralizes editor-only logic like Menu Categories.
 */
UCLASS(Abstract, BlueprintType, Const, meta = (PrioritizeCategories = "Config"))
class SCRIPTABLEFRAMEWORK_API UScriptableObjectAsset : public UObject
{
	GENERATED_BODY()

public:
#if WITH_EDITORONLY_DATA
	/** Category used for organization in the editor picker (e.g. "Combat|Melee"). */
	UPROPERTY(EditDefaultsOnly, Category = "Config", meta = (AssetRegistrySearchable))
	FString MenuCategory;
#endif

#if WITH_EDITOR
	virtual void GetAssetRegistryTags(FAssetRegistryTagsContext Context) const override;

	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif

	/** Defines the context this asset expects. */
	UPROPERTY(EditAnywhere, Category = "Config")
	TArray<FKzParamDef> Context;

	/** Per-instance mutable state. Graph-scoped variables seeded with each entry's default value at Launch. */
	UPROPERTY(EditAnywhere, Category = "Local Variables")
	TArray<FKzNamedVariant> Locals;

	virtual FInstancedPropertyBag* GetContext() PURE_VIRTUAL(UScriptableObjectAsset::GetContext(), return nullptr;)

#if WITH_EDITOR
	/** Returns the cached design-time shape of the Locals bag, or null when the subclass doesn't expose Locals yet. */
	virtual const FInstancedPropertyBag* GetLocalsShape() const { return nullptr; }
#endif

protected:
#if WITH_EDITOR
	virtual FName GetContainerName() const PURE_VIRTUAL(UScriptableObjectAsset::GetContainerName(), return NAME_None;)
#endif

private:
#if WITH_EDITOR
	void RefreshContext();
#endif
};