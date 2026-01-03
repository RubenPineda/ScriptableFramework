// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "IDetailPropertyExtensionHandler.h"

class FScriptableTaskBindingExtension : public IDetailPropertyExtensionHandler
{
public:
	virtual bool IsPropertyExtendable(const UClass* InObjectClass, const IPropertyHandle& PropertyHandle) const override;
	virtual void ExtendWidgetRow(FDetailWidgetRow& InWidgetRow, const IDetailLayoutBuilder& InDetailBuilder, const UClass* InObjectClass, TSharedPtr<IPropertyHandle> PropertyHandle) override;

	static void ProcessPropertyRow(IDetailPropertyRow& Row, const IDetailLayoutBuilder& DetailBuilder, UObject* ObjectToModify, TSharedPtr<IPropertyHandle> PropertyHandle);

private:
	static TSharedRef<SWidget> GenerateBindingWidget(const IDetailLayoutBuilder& DetailBuilder, UObject* ObjectToModify, TSharedPtr<IPropertyHandle> PropertyHandle);
};