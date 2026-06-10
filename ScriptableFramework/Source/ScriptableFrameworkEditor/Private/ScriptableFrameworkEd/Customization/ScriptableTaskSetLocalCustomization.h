// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableTaskCustomization.h"

/**
 * Customization for UScriptableTask_SetLocal: swaps the plain VarName dropdown for a
 * type-aware picker that shows each Local's pin icon and color.
 */
class FScriptableTaskSetLocalCustomization : public FScriptableTaskCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance() { return MakeShareable(new FScriptableTaskSetLocalCustomization); }

	virtual void ProcessPropertyHandle(TSharedRef<IPropertyHandle> SubPropertyHandle, IDetailChildrenBuilder& ChildBuilder, UScriptableObject* Obj, const TArray<FPropertyBindingBindableStructDescriptor>& AccessibleStructs) override;
};