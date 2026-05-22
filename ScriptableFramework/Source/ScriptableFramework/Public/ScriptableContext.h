// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "StructUtils/PropertyBag.h"
#include "Core/KzPropertyBagHelpers.h"
#include "ScriptableContext.generated.h"

/**
 * A reusable property bag wrapper that carries both shape definitions and values.
 * It can be applied to a FScriptableContainer as its shape (via AddContext) and passed
 * to Run / EvaluateRequirement to supply the values used during a single execution.
 */
USTRUCT(BlueprintType)
struct SCRIPTABLEFRAMEWORK_API FScriptableContext
{
	GENERATED_BODY()

public:
	/** Returns the internal property bag (used to copy values between bags). */
	FInstancedPropertyBag& GetBag() { return Bag; }

	/** Returns the internal property bag (used to copy values between bags). */
	const FInstancedPropertyBag& GetBag() const { return Bag; }

	/** Returns true if a property with the given name exists. */
	bool HasProperty(const FName& Name) const { return Bag.FindPropertyDescByName(Name) != nullptr; }

	/** Defines a property and its default value. Replaces any existing property with the same name. */
	template <typename T>
	void AddProperty(const FName& Name)
	{
		KzPropertyBag::Add<T>(Bag, Name);
	}

	/** Sets a property value by name. Defines the property if it does not exist yet. */
	template <typename T>
	void SetProperty(const FName& Name, const T& Value)
	{
		KzPropertyBag::Set(Bag, Name, Value);
	}

	/** Gets a property value by name. Returns a default-constructed value if missing or type-mismatched. */
	template <typename T>
	T GetProperty(const FName& Name) const
	{
		auto Result = KzPropertyBag::Get<T>(Bag, Name);
		return Result.HasValue() ? Result.GetValue() : T();
	}

private:
	/** Backing property bag holding both the shape and the values. */
	UPROPERTY(EditAnywhere, Category = "Context")
	FInstancedPropertyBag Bag;
};
