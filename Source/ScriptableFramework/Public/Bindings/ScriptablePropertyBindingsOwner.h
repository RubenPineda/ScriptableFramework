// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PropertyBindingPath.h"
#include "ScriptablePropertyBindingsOwner.generated.h"

// Forward declaration
struct FScriptablePropertyBindings;

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UScriptablePropertyBindingsOwner : public UInterface
{
	GENERATED_BODY()
};

class SCRIPTABLEFRAMEWORK_API IScriptablePropertyBindingsOwner
{
	GENERATED_BODY()

public:
	virtual FScriptablePropertyBindings* GetPropertyBindings() = 0;
	virtual void GetAccessibleStructs(const UObject* TargetOuterObject, TArray<FBindableStructDesc>& OutStructDescs) const = 0;
};