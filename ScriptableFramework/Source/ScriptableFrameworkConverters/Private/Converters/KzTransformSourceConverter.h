// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Bindings/ScriptableValueConverter.h"
#include "KzTransformSourceConverter.generated.h"

/** Builds an FKzTransformSource binding target from an actor, scene component, vector, rotator, quat or transform source. */
UCLASS()
class UKzTransformSourceConverter : public UScriptableValueConverter
{
	GENERATED_BODY()

public:
	virtual void GetConversions(TArray<FScriptableValueConversion>& OutConversions) const override;
	virtual bool Convert(const FProperty* SourceProp, const void* SourceAddr, const FProperty* TargetProp, void* TargetAddr) const override;
};