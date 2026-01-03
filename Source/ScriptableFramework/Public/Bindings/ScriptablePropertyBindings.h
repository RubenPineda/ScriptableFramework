// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "PropertyBindingPath.h"
#include "ScriptablePropertyBindings.generated.h"

/**
 * Defines a single binding: Copy from SourcePath -> TargetPath
 */
USTRUCT()
struct SCRIPTABLEFRAMEWORK_API FScriptablePropertyBinding
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = Binding)
	FPropertyBindingPath SourcePath;

	UPROPERTY(EditDefaultsOnly, Category = Binding)
	FPropertyBindingPath TargetPath;
};

/**
 * Container for all property bindings of an object.
 */
USTRUCT()
struct SCRIPTABLEFRAMEWORK_API FScriptablePropertyBindings
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = Bindings)
	TArray<FScriptablePropertyBinding> Bindings;

	/**
	 * Copies data from the source view to the destination view using the defined paths.
	 */
	void PerformCopies(const FPropertyBindingDataView& SrcObjectView, const FPropertyBindingDataView& DestObjectView);

	// Editor Helpers
	void AddPropertyBinding(const FPropertyBindingPath& SourcePath, const FPropertyBindingPath& TargetPath);
	void RemovePropertyBindings(const FPropertyBindingPath& TargetPath);
	bool HasPropertyBinding(const FPropertyBindingPath& TargetPath) const;

	/**
	 * Retrieves the source path bound to the specified target path, if any.
	 * @param TargetPath The path of the target property to find the binding for.
	 * @return Pointer to the source path if found, nullptr otherwise.
	 */
	const FPropertyBindingPath* GetPropertyBinding(const FPropertyBindingPath& TargetPath) const;

	// Type Compatibility (Delegates to Unreal's property system)
	static bool ArePropertiesCompatible(const FProperty* SourceProp, const FProperty* TargetProp);
};