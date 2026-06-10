// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableTaskCustomization.h"

/**
 * Customization for UScriptableTask_CallFunction: swaps the plain FunctionName and ResultLocal
 * dropdowns for type-aware pickers. Functions show the function glyph tinted by their return
 * type's pin color; ResultLocal shows each Local's pin icon and only offers compatible entries.
 */
class FScriptableTaskCallFunctionCustomization : public FScriptableTaskCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance() { return MakeShareable(new FScriptableTaskCallFunctionCustomization); }

	virtual void ProcessPropertyHandle(TSharedRef<IPropertyHandle> SubPropertyHandle, IDetailChildrenBuilder& ChildBuilder, UScriptableObject* Obj, const TArray<FPropertyBindingBindableStructDescriptor>& AccessibleStructs) override;
};