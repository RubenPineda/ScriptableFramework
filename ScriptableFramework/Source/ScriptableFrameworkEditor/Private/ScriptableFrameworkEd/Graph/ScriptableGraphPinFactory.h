// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "EdGraphUtilities.h"

/**
 * Pin factory that creates SScriptableGraphPin widgets for any pin whose category is our
 * ScriptableExec category. Registered with FEdGraphUtilities during editor module startup.
 */
class FScriptableGraphPinFactory : public FGraphPanelPinFactory
{
public:
	virtual TSharedPtr<class SGraphPin> CreatePin(class UEdGraphPin* InPin) const override;
};