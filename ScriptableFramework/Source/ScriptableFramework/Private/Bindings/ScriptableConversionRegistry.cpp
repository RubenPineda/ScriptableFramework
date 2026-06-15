// Copyright 2026 kirzo

#include "Bindings/ScriptableConversionRegistry.h"
#include "StructUtils/PropertyBag.h"
#include "UObject/UObjectHash.h"
#include "UObject/UObjectGlobals.h"

FScriptableConversionRegistry& FScriptableConversionRegistry::Get()
{
	static FScriptableConversionRegistry Instance;
	return Instance;
}

FScriptableConversionRegistry::FScriptableConversionRegistry()
{
#if WITH_EDITOR
	// Live Coding / hot reload can add or change converter classes; rebuild on the next query.
	FCoreUObjectDelegates::ReloadCompleteDelegate.AddLambda([](EReloadCompleteReason) { FScriptableConversionRegistry::Get().Invalidate(); });
#endif
}

void FScriptableConversionRegistry::EnsureBuilt()
{
	if (bBuilt) return;
	bBuilt = true;
	ConvertersByFrom.Reset();

	TArray<UClass*> ConverterClasses;
	GetDerivedClasses(UScriptableValueConverter::StaticClass(), ConverterClasses, true);
	for (const UClass* ConverterClass : ConverterClasses)
	{
		if (ConverterClass->HasAnyClassFlags(CLASS_Abstract)) continue;

		const UScriptableValueConverter* Converter = Cast<UScriptableValueConverter>(ConverterClass->GetDefaultObject());
		if (!Converter) continue;

		TArray<FScriptableValueConversion> Conversions;
		Converter->GetConversions(Conversions);
		for (const FScriptableValueConversion& Conversion : Conversions)
		{
			if (!Conversion.From.IsValid() || !Conversion.To.IsValid()) continue;
			ConvertersByFrom.FindOrAdd(Conversion.From).Emplace(Conversion.To, Converter);
		}
	}
}

const UScriptableValueConverter* FScriptableConversionRegistry::FindConverter(const FProperty* SourceProp, const FProperty* TargetProp)
{
	EnsureBuilt();

	const FScriptableConversionType To = FScriptableConversionType::FromProperty(TargetProp);
	if (!To.IsValid()) return nullptr;

	FScriptableConversionType From = FScriptableConversionType::FromProperty(SourceProp);
	while (From.IsValid())
	{
		if (const TArray<TPair<FScriptableConversionType, const UScriptableValueConverter*>>* Candidates = ConvertersByFrom.Find(From))
		{
			for (const TPair<FScriptableConversionType, const UScriptableValueConverter*>& Candidate : *Candidates)
			{
				if (Candidate.Key == To) return Candidate.Value;
			}
		}

		// Object sources also match a converter registered against any of their base classes.
		if (From.Type != EPropertyBagPropertyType::Object) break;
		const UClass* SourceClass = Cast<UClass>(From.TypeObject);
		const UClass* SuperClass = SourceClass ? SourceClass->GetSuperClass() : nullptr;
		if (!SuperClass) break;
		From = FScriptableConversionType::Object(SuperClass);
	}
	return nullptr;
}

bool FScriptableConversionRegistry::HasConverter(const FProperty* SourceProp, const FProperty* TargetProp)
{
	return FindConverter(SourceProp, TargetProp) != nullptr;
}

bool FScriptableConversionRegistry::Convert(const FProperty* SourceProp, const void* SourceAddr, const FProperty* TargetProp, void* TargetAddr)
{
	if (const UScriptableValueConverter* Converter = FindConverter(SourceProp, TargetProp))
	{
		return Converter->Convert(SourceProp, SourceAddr, TargetProp, TargetAddr);
	}
	return false;
}