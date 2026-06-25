// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"

struct FKzNamedVariant;

class SCRIPTABLEFRAMEWORK_API FScriptablePropertyUtilities
{
public:
	/**
	 * Type checking logic including Bidirectional QoL for Objects.
	 * Used by Blueprint Thunks at runtime and Binding systems in the Editor.
	 */
	static bool ArePropertiesCompatible(const FProperty* InputProp, const FProperty* TargetProp);

	/**
	 * Copies a single value between two resolved property addresses, applying the framework's
	 * conversions (TObjectPtr <-> raw pointer, Object -> Bool, Numeric <-> Numeric, Bool <-> Numeric).
	 * Pair it with ArePropertiesCompatible; incompatible pairs are silently skipped.
	 */
	static void CopyPropertyValue(const FProperty* SourceProp, const void* SourceAddr, const FProperty* TargetProp, void* TargetAddr);

#if WITH_EDITOR
	/**
	 * Returns a view over whichever Locals declaration applies to this object. Two supported authoring
	 * contexts: a UScriptableGraph in the outer chain (Graph->Locals), or an FScriptableContainer member
	 * found by reflecting each owner for a container whose arrays hold the object (LocalsDefinitions).
	 */
	static TConstArrayView<FKzNamedVariant> FindLocalsDeclarationFor(const UObject* Node);

	/** True if the FScriptableContainer holding Node is editable per-instance (EditAnywhere / EditInstanceOnly). False if not found or EditDefaultsOnly. */
	static bool IsOwningContainerInstanceEditable(const UObject* Node);

	static bool IsPropertyBindableInput(const FProperty* Property);
	static bool IsPropertyBindableOutput(const FProperty* Property);
	static bool IsPropertyBindableContext(const FProperty* Property);

	/** Checks if the Parent class allows data binding between its children (Siblings). */
	static bool AreSiblingBindingsAllowed(const UObject* ParentObject);

	/** Scans the ParentObject for any Array Property that contains the CurrentChild, and collects all previous siblings. */
	static void CollectPreviousSiblings(const UObject* ParentObject, const UObject* CurrentChild, TArray<const class UScriptableObject*>& OutObjects);

	/** Gathers all external Context Structs (e.g., Global Contexts, Owner Contexts) accessible by this object. */
	static void GatherAccessibleStructs(const class UScriptableObject* TargetObject, TArray<struct FPropertyBindingBindableStructDescriptor>& OutStructs);

	/** Attempts to automatically discover a compatible binding path for a Context property. */
	static bool FindAutoBindingPath(const FProperty* TargetProperty, const TArray<struct FPropertyBindingBindableStructDescriptor>& AccessibleStructs, struct FPropertyBindingPath& OutPath);

	/** Resolves the object class a bindable object property is bound to (via its binding's source leaf), or null when unbound or not an object. */
	static UClass* ResolveBoundSourceClass(const class UScriptableObject* Object, const FProperty* TargetProperty);
#endif
};