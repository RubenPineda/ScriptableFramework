// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Core/KzValidationTypes.h"

class UObject;

/**
 * Pure binding validation pass for any UObject that may own nested UScriptableObjects.
 * Extracted as a free helper so multiple validators (the global UEditorValidatorBase
 * one and the per-graph UKzAssetValidator one) can share a single implementation.
 */
namespace FScriptableBindingsValidation
{
	/**
	 * Walk every UScriptableObject under Root and emit one issue per "In" property
	 * without a manual binding, plus one per "Context" property without manual or
	 * auto-resolved binding. Each issue carries the offending object's BindingID
	 * as ContextId so UI panels can navigate to the source.
	 */
	SCRIPTABLEFRAMEWORKEDITOR_API void ValidateBindings(const UObject* Root, TArray<FKzValidationIssue>& OutIssues);
}
