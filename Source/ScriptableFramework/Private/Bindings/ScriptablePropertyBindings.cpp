// Copyright 2025 kirzo

#include "Bindings/ScriptablePropertyBindings.h"

UE_DISABLE_OPTIMIZATION

void FScriptablePropertyBindings::PerformCopies(const FPropertyBindingDataView& SrcObjectView, const FPropertyBindingDataView& DestObjectView)
{
	for (const FScriptablePropertyBinding& Binding : Bindings)
	{
		TArray<FPropertyBindingPathIndirection> SourceIndirections;
		if (!Binding.SourcePath.ResolveIndirectionsWithValue(SrcObjectView, SourceIndirections)) continue;

		TArray<FPropertyBindingPathIndirection> TargetIndirections;
		if (!Binding.TargetPath.ResolveIndirectionsWithValue(DestObjectView, TargetIndirections)) continue;

		const FPropertyBindingPathIndirection& SourceLeaf = SourceIndirections.Last();
		const FPropertyBindingPathIndirection& TargetLeaf = TargetIndirections.Last();

		const FProperty* SourceProp = SourceLeaf.GetProperty();
		const FProperty* TargetProp = TargetLeaf.GetProperty();
		const void* SourceAddr = SourceLeaf.GetPropertyAddress();
		void* TargetAddr = TargetLeaf.GetMutablePropertyAddress();

		if (SourceProp && TargetProp && SourceAddr && TargetAddr)
		{
			if (SourceProp->SameType(TargetProp))
			{
				SourceProp->CopyCompleteValue(TargetAddr, SourceAddr);
			}
			else
			{
				// Float <-> Double
				if (SourceProp->IsA<FFloatProperty>() && TargetProp->IsA<FDoubleProperty>())
				{
					const float SrcVal = CastField<FFloatProperty>(SourceProp)->GetFloatingPointPropertyValue(SourceAddr);
					CastField<FDoubleProperty>(TargetProp)->SetFloatingPointPropertyValue(TargetAddr, (double)SrcVal);
				}
				else if (SourceProp->IsA<FDoubleProperty>() && TargetProp->IsA<FFloatProperty>())
				{
					const double SrcVal = CastField<FDoubleProperty>(SourceProp)->GetFloatingPointPropertyValue(SourceAddr);
					CastField<FFloatProperty>(TargetProp)->SetFloatingPointPropertyValue(TargetAddr, (float)SrcVal);
				}
			}
		}
	}
}

void FScriptablePropertyBindings::AddPropertyBinding(const FPropertyBindingPath& SourcePath, const FPropertyBindingPath& TargetPath)
{
	// If a binding already exists for this target, update it
	for (FScriptablePropertyBinding& Binding : Bindings)
	{
		if (Binding.TargetPath == TargetPath)
		{
			Binding.SourcePath = SourcePath;
			return;
		}
	}

	// Otherwise, create a new one
	FScriptablePropertyBinding& NewBinding = Bindings.AddDefaulted_GetRef();
	NewBinding.SourcePath = SourcePath;
	NewBinding.TargetPath = TargetPath;
}

void FScriptablePropertyBindings::RemovePropertyBindings(const FPropertyBindingPath& TargetPath)
{
	Bindings.RemoveAll([&TargetPath](const FScriptablePropertyBinding& Binding)
	{
		return Binding.TargetPath == TargetPath;
	});
}

bool FScriptablePropertyBindings::HasPropertyBinding(const FPropertyBindingPath& TargetPath) const
{
	return Bindings.ContainsByPredicate([&TargetPath](const FScriptablePropertyBinding& Binding)
	{
		return Binding.TargetPath == TargetPath;
	});
}

const FPropertyBindingPath* FScriptablePropertyBindings::GetPropertyBinding(const FPropertyBindingPath& TargetPath) const
{
	const FScriptablePropertyBinding* FoundBinding = Bindings.FindByPredicate([&TargetPath](const FScriptablePropertyBinding& Binding)
	{
		return Binding.TargetPath == TargetPath;
	});

	return FoundBinding ? &FoundBinding->SourcePath : nullptr;
}

bool FScriptablePropertyBindings::ArePropertiesCompatible(const FProperty* SourceProp, const FProperty* TargetProp)
{
	if (!SourceProp || !TargetProp) return false;

	// 1. Base Case: Identical Types
	if (SourceProp->SameType(TargetProp))
	{
		return true;
	}

	// 2. Objects: Allow if source is child of target (Inheritance)
	// (Copied from StateTreePropertyBindings.cpp)
	if (const FObjectPropertyBase* SourceObj = CastField<FObjectPropertyBase>(SourceProp))
	{
		if (const FObjectPropertyBase* TargetObj = CastField<FObjectPropertyBase>(TargetProp))
		{
			return SourceObj->PropertyClass->IsChildOf(TargetObj->PropertyClass);
		}
	}

	// 3. Special Case UE5: Vectors (Struct vs Double)
	// If both have the same C++ type (e.g., "FVector"), they are compatible even if Unreal says no.
	if (SourceProp->GetCPPType() == TargetProp->GetCPPType())
	{
		return true;
	}

	// 4. Numeric Promotions (Simplified from StateTree)
	// Allow connecting Float to Double
	const bool bSourceIsReal = SourceProp->IsA<FFloatProperty>() || SourceProp->IsA<FDoubleProperty>();
	const bool bTargetIsReal = TargetProp->IsA<FFloatProperty>() || TargetProp->IsA<FDoubleProperty>();
	if (bSourceIsReal && bTargetIsReal) return true;

	// Allow connecting Int to Float/Double
	if (SourceProp->IsA<FIntProperty>() && bTargetIsReal) return true;

	// Allow Bool to Numeric
	if (SourceProp->IsA<FBoolProperty>() && TargetProp->IsA<FNumericProperty>()) return true;

	return false;
}

UE_ENABLE_OPTIMIZATION