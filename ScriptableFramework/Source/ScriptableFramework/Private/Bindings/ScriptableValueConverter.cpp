// Copyright 2026 kirzo

#include "Bindings/ScriptableValueConverter.h"
#include "StructUtils/PropertyBag.h"

FScriptableConversionType::FScriptableConversionType() : Type(EPropertyBagPropertyType::None) {}

FScriptableConversionType FScriptableConversionType::Object(const UClass* Class) { return FScriptableConversionType(EPropertyBagPropertyType::Object, Class); }
FScriptableConversionType FScriptableConversionType::Struct(const UScriptStruct* ScriptStruct) { return FScriptableConversionType(EPropertyBagPropertyType::Struct, ScriptStruct); }
FScriptableConversionType FScriptableConversionType::Enum(const UEnum* EnumType) { return FScriptableConversionType(EPropertyBagPropertyType::Enum, EnumType); }

FScriptableConversionType FScriptableConversionType::FromProperty(const FProperty* Property)
{
	if (!Property) return FScriptableConversionType();

	const FPropertyBagPropertyDesc Desc(NAME_None, Property);
	if (Desc.ContainerTypes.Num() > 0) return FScriptableConversionType();

	return FScriptableConversionType(Desc.ValueType, Desc.ValueTypeObject);
}

bool FScriptableConversionType::IsValid() const { return Type != EPropertyBagPropertyType::None; }