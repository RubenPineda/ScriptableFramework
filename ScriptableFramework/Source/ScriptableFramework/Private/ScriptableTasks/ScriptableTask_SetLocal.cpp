// Copyright 2026 kirzo

#include "ScriptableTasks/ScriptableTask_SetLocal.h"
#include "Core/KzNamedVariant.h"
#include "StructUtils/PropertyBag.h"

UScriptableTask_SetLocal::UScriptableTask_SetLocal()
{
	bStoppable = false;
}

void UScriptableTask_SetLocal::BeginTask()
{
	FInstancedPropertyBag* Locals = GetMutableLocals();
	if (!Locals || VarName.IsNone())
	{
		Finish();
		return;
	}

	// WriteToBag handles type checks against the bag's declared descriptor and skips the write on mismatch.
	const FKzNamedVariant Tmp(VarName, Value);
	Tmp.WriteToBag(*Locals);
	Finish();
}

#if WITH_EDITOR
FText UScriptableTask_SetLocal::GetDisplayTitle() const
{
	if (VarName.IsNone()) return INVTEXT("Set Local");
	return FText::Format(INVTEXT("Set Local: {0}"), FText::FromName(VarName));
}
#endif