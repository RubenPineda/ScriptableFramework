// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "EdGraphUtilities.h"

/**
 * Creates SScriptableGraphPin widgets for ScriptableExec-category pins.
 * Registered with FEdGraphUtilities at editor module startup.
 */
class SCRIPTABLEFRAMEWORKEDITOR_API FScriptableGraphPinFactory : public FGraphPanelPinFactory
{
public:
	virtual TSharedPtr<class SGraphPin> CreatePin(class UEdGraphPin* InPin) const override;
};