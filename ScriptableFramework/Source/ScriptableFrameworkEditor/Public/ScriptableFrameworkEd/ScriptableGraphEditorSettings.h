// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ScriptableGraphEditorSettings.generated.h"

UENUM()
enum class EScriptableSaveOnCompile : uint8
{
	Never UMETA(DisplayName = "Never"),
	OnSuccessOnly UMETA(DisplayName = "On Success Only"),
	Always UMETA(DisplayName = "Always"),
};

/** Per-user editor preferences for the Scriptable Graph compile flow. Mirrors the BP compile dropdown. */
UCLASS(Config = EditorPerProjectUserSettings, DefaultConfig, meta = (DisplayName = "Scriptable Graph Editor"))
class SCRIPTABLEFRAMEWORKEDITOR_API UScriptableGraphEditorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** Whether the asset is saved automatically after a Compile pass. */
	UPROPERTY(EditAnywhere, Config, Category = "Compile")
	EScriptableSaveOnCompile SaveOnCompile = EScriptableSaveOnCompile::Never;

	/** When the compile fails, pan and select the first ed-node carrying the error. */
	UPROPERTY(EditAnywhere, Config, Category = "Compile")
	bool bJumpToErrorNode = false;

	//~ UDeveloperSettings interface
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
	//~ End of UDeveloperSettings interface
};
