// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ScriptableValueConverter.generated.h"

enum class EPropertyBagPropertyType : uint8;

/**
 * Identifies a bindable value type: a property-bag value category plus its concrete type object
 * (UClass for objects, UScriptStruct for structs, UEnum for enums) when one applies.
 */
struct SCRIPTABLEFRAMEWORK_API FScriptableConversionType
{
	EPropertyBagPropertyType Type;
	const UObject* TypeObject = nullptr;

	FScriptableConversionType();
	FScriptableConversionType(EPropertyBagPropertyType InType, const UObject* InTypeObject) : Type(InType), TypeObject(InTypeObject) {}

	static FScriptableConversionType Object(const UClass* Class);
	static FScriptableConversionType Struct(const UScriptStruct* ScriptStruct);
	static FScriptableConversionType Enum(const UEnum* EnumType);

	/** Resolves the type id of a property. Container properties (arrays/sets/maps) yield an invalid id. */
	static FScriptableConversionType FromProperty(const FProperty* Property);

	bool IsValid() const;
	bool operator==(const FScriptableConversionType& Other) const { return Type == Other.Type && TypeObject == Other.TypeObject; }
	friend uint32 GetTypeHash(const FScriptableConversionType& Id) { return HashCombine(GetTypeHash(static_cast<uint8>(Id.Type)), GetTypeHash(static_cast<const void*>(Id.TypeObject))); }
};

/** A single (source -> target) conversion that a converter declares it can perform. */
struct FScriptableValueConversion
{
	FScriptableConversionType From;
	FScriptableConversionType To;

	FScriptableValueConversion() = default;
	FScriptableValueConversion(const FScriptableConversionType& InFrom, const FScriptableConversionType& InTo) : From(InFrom), To(InTo) {}
};

/**
 * Base class for value converters used by the binding system to copy between non-identical property
 * types (e.g. an AActor* source into an FKzTransformSource target). Subclass it in any module that
 * knows both types, list the pairs in GetConversions, and perform the copy in Convert. Subclasses are
 * discovered automatically; the CDO does the work, so converters must be stateless.
 */
UCLASS(Abstract)
class SCRIPTABLEFRAMEWORK_API UScriptableValueConverter : public UObject
{
	GENERATED_BODY()

public:
	/** Lists the (source -> target) type pairs this converter handles. */
	virtual void GetConversions(TArray<FScriptableValueConversion>& OutConversions) const PURE_VIRTUAL(UScriptableValueConverter::GetConversions, );

	/** Copies the resolved source value into the target address. Returns false to decline. */
	virtual bool Convert(const FProperty* SourceProp, const void* SourceAddr, const FProperty* TargetProp, void* TargetAddr) const PURE_VIRTUAL(UScriptableValueConverter::Convert, return false;);
};