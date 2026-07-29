// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "ScriptableContainer.h"
#include "ScriptableRequirement.generated.h"

class UScriptableCondition;
class UScriptableObject;
struct FScriptableContext;

/** Logical operation for the requirement group. */
UENUM(BlueprintType)
enum class EScriptableRequirementMode : uint8
{
	And,
	Or
};

/** A container for a list of conditions with a logic operation (AND/OR). */
USTRUCT(BlueprintType)
struct SCRIPTABLEFRAMEWORK_API FScriptableRequirement : public FScriptableContainer
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Logic")
	EScriptableRequirementMode Mode = EScriptableRequirementMode::And;

	/** If true, the result of the entire group evaluation is inverted. */
	UPROPERTY(EditAnywhere, Category = "Logic")
	uint8 bNegate : 1 = false;

	UPROPERTY(EditAnywhere, Instanced, Category = "Conditions")
	TArray<TObjectPtr<UScriptableCondition>> Conditions;

private:
	UPROPERTY(Transient)
	uint8 bIsRegistered : 1 = false;

	// -------------------------------------------------------------------
	// API
	// -------------------------------------------------------------------
public:
	/** Registers all conditions and sets up the Binding Context. */
	void Register(UObject* InOwner);

	void Unregister();

	bool Evaluate() const;

	bool IsEmpty() const { return Conditions.IsEmpty(); }

	/**
	 * Creates a deep copy of this requirement with its Instanced conditions re-owned by NewOuter. A plain struct copy
	 * only duplicates the condition POINTERS (still owned by this requirement's outer), so to store a requirement in
	 * different, cook-safe memory — e.g. baking an editor-graph node's condition into a runtime data asset — copy it
	 * via Clone. Returns an un-registered copy ready to evaluate.
	 */
	FScriptableRequirement Clone(UObject* NewOuter) const;

public:
	/** Static entry point to evaluate a requirement. */
	static bool EvaluateRequirement(UObject* Owner, const FScriptableRequirement& Requirement);

	/** Static evaluation entry point using an external context. */
	static bool EvaluateRequirement(UObject* Owner, const FScriptableRequirement& Requirement, const FScriptableContext& Context);
};