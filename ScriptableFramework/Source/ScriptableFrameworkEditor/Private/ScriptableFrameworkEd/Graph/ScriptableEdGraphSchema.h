// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphSchema.h"
#include "ScriptableEdGraphSchema.generated.h"

/**
 * Schema for the scriptable graph editor. Defines which pins can connect, what the
 * context menu offers, and other editor-only graph semantics.
 */
UCLASS()
class UScriptableEdGraphSchema : public UEdGraphSchema
{
	GENERATED_BODY()

};