// Copyright 2026 kirzo

#pragma once

#include "KzLibEditorModule_Base.h"

struct FScriptableTypeCache;
class UScriptableGraph;

class FScriptableFrameworkEditorModule : public FKzLibEditorModule_Base
{
private:
	TSharedPtr<FScriptableTypeCache> ScriptableTypeCache;
	EAssetTypeCategories::Type ScriptableAssetCategoryBit = EAssetTypeCategories::None;

	/** Assets whose Launch was refused during the current PIE session. Surfaced as a dialog on EndPIE. */
	TArray<TWeakObjectPtr<UScriptableGraph>> CompileBlockedDuringPIE;

	FDelegateHandle LaunchBlockedHandle;
	FDelegateHandle PostLoadedHandle;
	FDelegateHandle BeginPIEHandle;
	FDelegateHandle EndPIEHandle;

	void HandleLaunchBlocked(UScriptableGraph* Asset);
	void HandleGraphPostLoaded(UScriptableGraph* Asset);
	void HandleBeginPIE(const bool bSimulating);
	void HandleEndPIE(const bool bSimulating);

protected:
	virtual void OnStartupModule() override;
	virtual void OnShutdownModule() override;

public:
	TSharedPtr<FScriptableTypeCache> GetScriptableTypeCache();
};