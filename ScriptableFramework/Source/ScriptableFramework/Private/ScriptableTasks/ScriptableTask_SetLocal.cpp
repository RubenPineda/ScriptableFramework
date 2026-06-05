// Copyright 2026 kirzo

#include "ScriptableTasks/ScriptableTask_SetLocal.h"
#include "ScriptableContainer.h"
#include "ScriptableNodes/ScriptableGraph.h"
#include "Core/KzNamedVariant.h"
#include "StructUtils/PropertyBag.h"
#include "UObject/UnrealType.h"

namespace
{
	// Returns a view over whichever Locals declaration applies to this task. Two supported authoring contexts:
	// 1) Task lives inside a UScriptableGraph: walk outers, return Graph->Locals.
	// 2) Task lives inside an FScriptableContainer (e.g. FScriptableAction.Tasks): walk outers, reflect each
	//    owner for a container struct member whose Tasks array holds us, return its LocalsDefinitions.
	TConstArrayView<FKzNamedVariant> FindLocalsDeclarationFor(const UObject* TaskNode)
	{
		if (!TaskNode) return {};

		if (const UScriptableGraph* Graph = TaskNode->GetTypedOuter<UScriptableGraph>())
		{
			return TConstArrayView<FKzNamedVariant>(Graph->Locals);
		}

		const UScriptStruct* BaseContainerStruct = FScriptableContainer::StaticStruct();
		for (const UObject* Cursor = TaskNode->GetOuter(); Cursor; Cursor = Cursor->GetOuter())
		{
			for (TFieldIterator<FProperty> It(Cursor->GetClass()); It; ++It)
			{
				const FStructProperty* StructProp = CastField<FStructProperty>(*It);
				if (!StructProp || !StructProp->Struct->IsChildOf(BaseContainerStruct)) continue;

				const FScriptableContainer* Container = StructProp->ContainerPtrToValuePtr<FScriptableContainer>(Cursor);
				if (!Container) continue;

				for (TFieldIterator<FProperty> Inner(StructProp->Struct); Inner; ++Inner)
				{
					const FArrayProperty* ArrayProp = CastField<FArrayProperty>(*Inner);
					if (!ArrayProp) continue;
					const FObjectProperty* ObjProp = CastField<FObjectProperty>(ArrayProp->Inner);
					if (!ObjProp) continue;

					FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(const_cast<FScriptableContainer*>(Container)));
					for (int32 i = 0; i < Helper.Num(); ++i)
					{
						if (ObjProp->GetObjectPropertyValue(Helper.GetRawPtr(i)) == TaskNode)
						{
							return TConstArrayView<FKzNamedVariant>(Container->LocalsDefinitions);
						}
					}
				}
			}
		}

		return {};
	}
}

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

void UScriptableTask_SetLocal::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Force-sync Value's type to the picked local. No feedback loop because we only mutate on real mismatches.
	const FKzNamedVariant* Local = FindOwningLocal();
	if (!Local)
	{
		// VarName empty or pointing at a missing local: clear Value so the editor doesn't show a stale slot.
		if (Value.IsValid()) Value.Reset();
		return;
	}

	const EPropertyBagPropertyType DesiredType = Local->GetValue().GetType();
	const UObject* DesiredTypeObject = Local->GetValue().GetTypeObject();
	if (Value.GetType() != DesiredType || Value.GetTypeObject() != DesiredTypeObject)
	{
		Value.SetType(DesiredType, DesiredTypeObject);
	}
}

TArray<FString> UScriptableTask_SetLocal::GetLocalNames() const
{
	TArray<FString> Names;
	const TConstArrayView<FKzNamedVariant> Locals = FindLocalsDeclarationFor(this);
	Names.Reserve(Locals.Num());
	for (const FKzNamedVariant& Var : Locals)
	{
		if (!Var.GetName().IsNone()) Names.Add(Var.GetName().ToString());
	}
	return Names;
}

const FKzNamedVariant* UScriptableTask_SetLocal::FindOwningLocal() const
{
	if (VarName.IsNone()) return nullptr;

	const TConstArrayView<FKzNamedVariant> Locals = FindLocalsDeclarationFor(this);
	for (const FKzNamedVariant& Var : Locals)
	{
		if (Var.GetName() == VarName) return &Var;
	}
	return nullptr;
}
#endif