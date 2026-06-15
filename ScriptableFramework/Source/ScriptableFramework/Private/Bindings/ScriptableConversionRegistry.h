// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Bindings/ScriptableValueConverter.h"

/**
 * Process-wide registry of UScriptableValueConverter subclasses, keyed by source type. Built lazily by
 * discovering converter classes and reading their declared conversions; consulted by the binding copy
 * path (FScriptablePropertyUtilities) as a fallback when source and target types do not match directly.
 */
class FScriptableConversionRegistry
{
public:
	static FScriptableConversionRegistry& Get();

	/** True if a converter can turn SourceProp's value into TargetProp's type (base classes of object sources match). */
	bool HasConverter(const FProperty* SourceProp, const FProperty* TargetProp);

	/** Runs the matching converter, copying SourceAddr -> TargetAddr. Returns false when none applies. */
	bool Convert(const FProperty* SourceProp, const void* SourceAddr, const FProperty* TargetProp, void* TargetAddr);

	/** Drops the cache so the next query rediscovers converters (e.g. after a hot reload). */
	void Invalidate() { bBuilt = false; }

private:
	FScriptableConversionRegistry();

	void EnsureBuilt();
	const UScriptableValueConverter* FindConverter(const FProperty* SourceProp, const FProperty* TargetProp);

	TMap<FScriptableConversionType, TArray<TPair<FScriptableConversionType, const UScriptableValueConverter*>>> ConvertersByFrom;
	bool bBuilt = false;
};