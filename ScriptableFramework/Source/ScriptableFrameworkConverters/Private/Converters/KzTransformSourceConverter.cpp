// Copyright 2026 kirzo

#include "Converters/KzTransformSourceConverter.h"
#include "Misc/KzTransformSource.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "UObject/UnrealType.h"
#include "UObject/Class.h"

void UKzTransformSourceConverter::GetConversions(TArray<FScriptableValueConversion>& OutConversions) const
{
	const FScriptableConversionType To = FScriptableConversionType::Struct(FKzTransformSource::StaticStruct());

	OutConversions.Emplace(FScriptableConversionType::Object(AActor::StaticClass()), To);
	OutConversions.Emplace(FScriptableConversionType::Object(USceneComponent::StaticClass()), To);
	OutConversions.Emplace(FScriptableConversionType::Struct(TBaseStructure<FVector>::Get()), To);
	OutConversions.Emplace(FScriptableConversionType::Struct(TBaseStructure<FRotator>::Get()), To);
	OutConversions.Emplace(FScriptableConversionType::Struct(TBaseStructure<FQuat>::Get()), To);
	OutConversions.Emplace(FScriptableConversionType::Struct(TBaseStructure<FTransform>::Get()), To);
}

bool UKzTransformSourceConverter::Convert(const FProperty* SourceProp, const void* SourceAddr, const FProperty* TargetProp, void* TargetAddr) const
{
	FKzTransformSource& Out = *static_cast<FKzTransformSource*>(TargetAddr);

	if (const FObjectPropertyBase* ObjectProp = CastField<FObjectPropertyBase>(SourceProp))
	{
		UObject* Object = ObjectProp->GetObjectPropertyValue(SourceAddr);
		if (const AActor* Actor = Cast<AActor>(Object))
		{
			Out = FKzTransformSource(Actor);
			return true;
		}
		if (const USceneComponent* Component = Cast<USceneComponent>(Object))
		{
			Out = FKzTransformSource(Component);
			return true;
		}
		return false;
	}

	if (const FStructProperty* StructProp = CastField<FStructProperty>(SourceProp))
	{
		if (StructProp->Struct == TBaseStructure<FVector>::Get())
		{
			Out = FKzTransformSource(*static_cast<const FVector*>(SourceAddr));
			return true;
		}
		if (StructProp->Struct == TBaseStructure<FRotator>::Get())
		{
			Out = FKzTransformSource(*static_cast<const FRotator*>(SourceAddr));
			return true;
		}
		if (StructProp->Struct == TBaseStructure<FQuat>::Get())
		{
			Out = FKzTransformSource(*static_cast<const FQuat*>(SourceAddr));
			return true;
		}
		if (StructProp->Struct == TBaseStructure<FTransform>::Get())
		{
			Out = FKzTransformSource(*static_cast<const FTransform*>(SourceAddr));
			return true;
		}
	}

	return false;
}