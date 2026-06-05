// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "StructUtils/PropertyBag.h"
#include "ScriptableRuntimeData.generated.h"

class UScriptableObject;

/**
 * Bundle of shared runtime state injected into every UScriptableObject when its owning container launches.
 * Context is read-only shared input, Locals is mutable per-instance state, BindingsMap routes Task-to-Task lookups. 
 */
USTRUCT()
struct SCRIPTABLEFRAMEWORK_API FScriptableRuntimeData
{
	GENERATED_BODY()

	/** Read-only input bag owned by the launching container (e.g. UScriptableGraphInstance::Context). */
	const FInstancedPropertyBag* Context = nullptr;

	/** Mutable per-instance bag for graph-scoped variables. Written by SetLocal-style tasks, read by bindings. */
	FInstancedPropertyBag* Locals = nullptr;

	/** Binding ID -> owning ScriptableObject map so Task-to-Task SourceID lookups resolve at bind time. */
	const TMap<FGuid, TObjectPtr<UScriptableObject>>* BindingsMap = nullptr;
};

namespace ScriptableBindingSources
{
	/** Hardcoded StructID for the Locals bindable source. */
	constexpr FGuid ContextStructID(0, 0, 0, 0);

	/** Hardcoded StructID for the Locals bindable source. */
	constexpr FGuid LocalsStructID(0, 0, 0, 1);
}