// Copyright 2026 kirzo

#include "ScriptableContainer.h"
#include "ScriptableContext.h"
#include "ScriptableObject.h"
#include "ScriptableRuntimeData.h"
#include "ScriptablePropertyUtilities.h"
#include "Core/KzBagOps.h"
#include "UObject/UnrealType.h"

void FScriptableContainer::ConstructContext()
{
	ResetContext();
	KzBagOps::AddProperties(Context, ContextDefinitions);
}

bool FScriptableContainer::IsContextBagOutOfSync() const
{
	const UPropertyBag* BagStruct = Context.GetPropertyBagStruct();
	const int32 BagPropertyCount = BagStruct ? BagStruct->GetPropertyDescs().Num() : 0;
	if (BagPropertyCount != ContextDefinitions.Num()) return true;

	if (BagStruct)
	{
		for (const FKzParamDef& Def : ContextDefinitions)
		{
			if (!BagStruct->FindPropertyDescByName(Def.Name)) return true;
		}
	}
	return false;
}

void FScriptableContainer::AddContext(const FScriptableContext& InContext)
{
	const UPropertyBag* BagStruct = InContext.GetBag().GetPropertyBagStruct();
	if (!BagStruct)
	{
		return;
	}

	for (const FPropertyBagPropertyDesc& Desc : BagStruct->GetPropertyDescs())
	{
		ContextDefinitions.Add(FKzParamDef(Desc.Name, Desc.ContainerTypes.GetFirstContainerType(), Desc.ValueType, Desc.ValueTypeObject));
	}

	ConstructContext();
}

void FScriptableContainer::SetContext(const FScriptableContext& InContext)
{
	const FInstancedPropertyBag& SourceBag = InContext.GetBag();
	const UPropertyBag* SourceStruct = SourceBag.GetPropertyBagStruct();
	if (!SourceStruct) return;

	const UPropertyBag* TargetStruct = Context.GetPropertyBagStruct();
	if (!TargetStruct)
	{
		// Local bag is empty (no ContextDefinitions on this container). Adopt the source bag
		// wholesale — there's no authored shape to preserve.
		Context = SourceBag;
		return;
	}

	const uint8* SourceMemory = SourceBag.GetValue().GetMemory();
	uint8* TargetMemory = Context.GetMutableValue().GetMemory();
	if (!SourceMemory || !TargetMemory) return;

	for (const FPropertyBagPropertyDesc& SourceDesc : SourceStruct->GetPropertyDescs())
	{
		if (!SourceDesc.CachedProperty) continue;
		const FPropertyBagPropertyDesc* TargetDesc = TargetStruct->FindPropertyDescByName(SourceDesc.Name);
		if (!TargetDesc || !TargetDesc->CachedProperty) continue;
		if (!TargetDesc->CachedProperty->SameType(SourceDesc.CachedProperty)) continue;

		void* TargetAddr = TargetDesc->CachedProperty->ContainerPtrToValuePtr<void>(TargetMemory);
		const void* SourceAddr = SourceDesc.CachedProperty->ContainerPtrToValuePtr<void>(SourceMemory);
		TargetDesc->CachedProperty->CopyCompleteValue(TargetAddr, SourceAddr);
	}
}

UScriptableObject* FScriptableContainer::FindBindingSource(const FGuid& InID) const
{
	if (const TObjectPtr<UScriptableObject>* Found = BindingSourceMap.Find(InID))
	{
		return Found->Get();
	}
	return nullptr;
}

void FScriptableContainer::AddBindingSource(UScriptableObject* InSource)
{
	if (InSource)
	{
		const FInstancedPropertyBag* ContextToUse = nullptr;

		// 1. Determine which Context to pass down.
		// If our local context is valid and has properties, use it (it acts as the top-most scope).
		if (Context.IsValid() && Context.GetNumPropertiesInBag() > 0)
		{
			ContextToUse = &Context;
		}
		// Otherwise, try to inherit the context from the Owner (Parent Scope).
		else if (UScriptableObject* ScriptableOwner = Cast<UScriptableObject>(Owner))
		{
			ContextToUse = ScriptableOwner->GetContext();
		}

		// 2. Inject Data. FScriptableAction-style containers don't expose Locals yet, so Locals stays null.
		FScriptableRuntimeData Data;
		Data.Context = ContextToUse;
		Data.BindingsMap = &BindingSourceMap;
		InSource->InitRuntimeData(Data);

		FGuid ID = InSource->GetBindingID();
		if (ID.IsValid())
		{
			BindingSourceMap.Add(ID, InSource);
		}
	}
}

void FScriptableContainer::Register(UObject* InOwner)
{
	Owner = InOwner;
	BindingSourceMap.Reset(); // Clean slate
}

void FScriptableContainer::Unregister()
{
	BindingSourceMap.Empty();
	Owner = nullptr;
}