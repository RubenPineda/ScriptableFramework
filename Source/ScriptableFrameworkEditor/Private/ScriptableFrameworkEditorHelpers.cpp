// Copyright 2026 kirzo

#include "ScriptableFrameworkEditorHelpers.h"
#include "PropertyHandle.h"
#include "ScriptableObject.h"
#include "ScriptableTasks/ScriptableTask.h"
#include "ScriptableConditions/ScriptableCondition.h"
#include "StructUtils/InstancedStruct.h"
#include "IPropertyAccessEditor.h"

namespace ScriptableObjectTraversal
{
	/** Checks if the Parent class allows data binding between its children (Siblings). */
	static bool AreSiblingBindingsAllowed(const UObject* ParentObject)
	{
		if (!ParentObject) return false;
		return !ParentObject->GetClass()->HasMetaData(TEXT("BlockSiblingBindings"));
	}

	/** Scans the ParentObject for any Array Property that contains the CurrentChild. */
	static void CollectPreviousSiblings(const UObject* ParentObject, const UObject* CurrentChild, TArray<const UScriptableObject*>& OutObjects)
	{
		if (!ParentObject || !CurrentChild) return;

		for (TFieldIterator<FArrayProperty> PropIt(ParentObject->GetClass()); PropIt; ++PropIt)
		{
			FArrayProperty* ArrayProp = *PropIt;
			FObjectProperty* InnerProp = CastField<FObjectProperty>(ArrayProp->Inner);

			if (InnerProp && InnerProp->PropertyClass->IsChildOf(UScriptableObject::StaticClass()))
			{
				FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(ParentObject));
				bool bFoundCurrentChildInArray = false;
				TArray<const UScriptableObject*> PotentialSiblings;

				for (int32 i = 0; i < Helper.Num(); ++i)
				{
					UObject* Item = InnerProp->GetObjectPropertyValue(Helper.GetRawPtr(i));
					if (Item == CurrentChild)
					{
						bFoundCurrentChildInArray = true;
						break;
					}
					if (const UScriptableObject* ScriptableItem = Cast<UScriptableObject>(Item))
					{
						PotentialSiblings.Add(ScriptableItem);
					}
				}

				if (bFoundCurrentChildInArray)
				{
					OutObjects.Append(PotentialSiblings);
					return;
				}
			}
		}
	}
}

namespace ScriptableFrameworkEditor
{
	bool IsPropertyVisible(TSharedRef<IPropertyHandle> PropertyHandle)
	{
		if (FProperty* Property = PropertyHandle->GetProperty())
		{
			if (Property->GetMetaData(TEXT("Category")) == TEXT("Hidden")) return false;
			if (Property->HasAllPropertyFlags(CPF_Edit | CPF_DisableEditOnInstance))
			{
				if (UObject* Object = Property->GetOwnerUObject())
				{
					return Object->IsTemplate();
				}
			}
		}
		return true;
	}

	void GetScriptableCategory(const UClass* ScriptableClass, FName& ClassCategoryMeta, FName& PropertyCategoryMeta)
	{
		if (ScriptableClass->IsChildOf(UScriptableTask::StaticClass()))
		{
			ClassCategoryMeta = MD_TaskCategory;
			PropertyCategoryMeta = MD_TaskCategories;
		}
		else if (ScriptableClass->IsChildOf(UScriptableCondition::StaticClass()))
		{
			ClassCategoryMeta = MD_ConditionCategory;
			PropertyCategoryMeta = MD_ConditionCategories;
		}
	}

	bool IsPropertyBindableOutput(const FProperty* Property)
	{
		if (!Property) return false;
		if (Property->HasMetaData(TEXT("ScriptableOutput"))) return true;
		const FString Category = Property->GetMetaData(TEXT("Category"));
		return Category.StartsWith(TEXT("Output"));
	}

	bool ArePropertiesCompatible(const FProperty* SourceProp, const FProperty* TargetProp)
	{
		if (!SourceProp || !TargetProp) return false;

		// 1. Base Case: Identical Types
		if (SourceProp->SameType(TargetProp))
		{
			return true;
		}

		// Arrays must be handled strictly because CopyCompleteValue cannot convert element types (e.g. Int to Float).
		// If SameType (Step 1) failed, it means they are not identical.
		if (const FArrayProperty* SourceArray = CastField<FArrayProperty>(SourceProp))
		{
			if (const FArrayProperty* TargetArray = CastField<FArrayProperty>(TargetProp))
			{
				// Allow Covariance for Arrays of Objects (e.g. Array<MyActor> -> Array<AActor>)
				// Since they are pointers, the memory layout is compatible.
				const FObjectPropertyBase* SourceInner = CastField<FObjectPropertyBase>(SourceArray->Inner);
				const FObjectPropertyBase* TargetInner = CastField<FObjectPropertyBase>(TargetArray->Inner);

				if (SourceInner && TargetInner)
				{
					return SourceInner->PropertyClass->IsChildOf(TargetInner->PropertyClass);
				}

				// For any other array type (Int, Float, Struct), we require strict equality (Step 1).
				// If we are here, they didn't match, so we reject them.
				return false;
			}

			// Source is Array, Target is NOT Array -> Incompatible
			return false;
		}

		// Target is Array, Source is NOT Array -> Incompatible
		if (TargetProp->IsA<FArrayProperty>())
		{
			return false;
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

	UScriptableObject* GetOuterScriptableObject(const TSharedPtr<const IPropertyHandle>& InPropertyHandle)
	{
		TArray<UObject*> OuterObjects;
		InPropertyHandle->GetOuterObjects(OuterObjects);
		for (UObject* OuterObject : OuterObjects)
		{
			if (OuterObject)
			{
				if (UScriptableObject* ScriptableObject = Cast<UScriptableObject>(OuterObject)) return ScriptableObject;
				if (UScriptableObject* OuterScriptableObject = OuterObject->GetTypedOuter<UScriptableObject>()) return OuterScriptableObject;
			}
		}
		return nullptr;
	}

	FGuid GetScriptableObjectDataID(UScriptableObject* Owner)
	{
		return Owner ? Owner->GetBindingID() : FGuid();
	}

	void ScriptableFrameworkEditor::GetAccessibleStructs(const UScriptableObject* TargetObject, TArray<FBindableStructDesc>& OutStructDescs)
	{
		if (!TargetObject) return;
		const UScriptableObject* RootObject = TargetObject->GetRoot();
		if (!RootObject) return;

		// 1. Context
		const FInstancedPropertyBag& Bag = RootObject->GetContext();
		if (Bag.IsValid())
		{
			FBindableStructDesc& ContextDesc = OutStructDescs.AddDefaulted_GetRef();
			ContextDesc.Name = FName(TEXT("Context"));
			ContextDesc.Struct = Bag.GetPropertyBagStruct();
			ContextDesc.ID = FGuid();
		}

		// 2. Traversal
		TArray<const UScriptableObject*> AccessibleObjects;
		const UObject* IteratorNode = TargetObject;

		while (IteratorNode)
		{
			const UObject* ParentNode = IteratorNode->GetOuter();
			if (!ParentNode || ParentNode == RootObject->GetOuter()) break;

			if (const UScriptableObject* ParentScriptableObject = Cast<UScriptableObject>(ParentNode))
			{
				AccessibleObjects.Add(ParentScriptableObject); // Parent
				if (ScriptableObjectTraversal::AreSiblingBindingsAllowed(ParentScriptableObject))
				{
					ScriptableObjectTraversal::CollectPreviousSiblings(ParentScriptableObject, IteratorNode, AccessibleObjects); // Siblings
				}
			}
			IteratorNode = ParentNode;
		}

		// 3. Convert
		for (const UScriptableObject* Obj : AccessibleObjects)
		{
			FBindableStructDesc& Desc = OutStructDescs.AddDefaulted_GetRef();
			Desc.Name = FName(*Obj->GetName());
			Desc.Struct = Obj->GetClass();
			Desc.ID = Obj->GetBindingID();
		}
	}

	void MakeStructPropertyPathFromPropertyHandle(UScriptableObject* ScriptableObject, TSharedPtr<const IPropertyHandle> InPropertyHandle, FPropertyBindingPath& OutPath)
	{
		OutPath.Reset();
		if (!ScriptableObject) return;

		TArray<FPropertyBindingPathSegment> PathSegments;
		TSharedPtr<const IPropertyHandle> CurrentPropertyHandle = InPropertyHandle;

		while (CurrentPropertyHandle.IsValid())
		{
			const FProperty* Property = CurrentPropertyHandle->GetProperty();
			if (Property)
			{
				if (const UClass* PropertyOwnerClass = Cast<UClass>(Property->GetOwnerStruct()))
				{
					if (!ScriptableObject->GetClass()->IsChildOf(PropertyOwnerClass)) break;
				}

				FPropertyBindingPathSegment& Segment = PathSegments.InsertDefaulted_GetRef(0);
				Segment.SetName(Property->GetFName());
				Segment.SetArrayIndex(CurrentPropertyHandle->GetIndexInArray());

				if (const FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
				{
					if (ObjectProperty->HasAnyPropertyFlags(CPF_PersistentInstance | CPF_InstancedReference))
					{
						const UObject* Object = nullptr;
						if (CurrentPropertyHandle->GetValue(Object) == FPropertyAccess::Success && Object)
						{
							Segment.SetInstanceStruct(Object->GetClass());
						}
					}
				}
				else if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
				{
					if (StructProperty->Struct == TBaseStructure<FInstancedStruct>::Get())
					{
						void* Address = nullptr;
						if (CurrentPropertyHandle->GetValueData(Address) == FPropertyAccess::Success && Address)
						{
							const FInstancedStruct& Struct = *static_cast<const FInstancedStruct*>(Address);
							Segment.SetInstanceStruct(Struct.GetScriptStruct());
						}
					}
				}

				if (Segment.GetArrayIndex() != INDEX_NONE)
				{
					TSharedPtr<const IPropertyHandle> ParentPropertyHandle = CurrentPropertyHandle->GetParentHandle();
					if (ParentPropertyHandle.IsValid())
					{
						const FProperty* ParentProperty = ParentPropertyHandle->GetProperty();
						if (ParentProperty && ParentProperty->IsA<FArrayProperty>() && Property->GetFName() == ParentProperty->GetFName())
						{
							CurrentPropertyHandle = ParentPropertyHandle;
						}
					}
				}
			}
			CurrentPropertyHandle = CurrentPropertyHandle->GetParentHandle();
		}

		if (PathSegments.Num() > 0)
		{
			FGuid OwnerID = GetScriptableObjectDataID(ScriptableObject);
			OutPath = FPropertyBindingPath(OwnerID, PathSegments);
		}
	}

	void MakeStructPropertyPathFromBindingChain(const FGuid& StructID, const TArray<FBindingChainElement>& InBindingChain, FPropertyBindingPath& OutPath)
	{
		OutPath.Reset();
		OutPath.SetStructID(StructID);

		for (const FBindingChainElement& Element : InBindingChain)
		{
			if (const FProperty* Property = Element.Field.Get<FProperty>())
			{
				OutPath.AddPathSegment(Property->GetFName(), Element.ArrayIndex);
			}
		}
	}

	void SetWrapperAssetProperty(TSharedPtr<IPropertyHandle> Handle, UObject* Asset)
	{
		if (!Handle.IsValid()) return;
		UObject* NewObj = nullptr;
		if (Handle->GetValue(NewObj) == FPropertyAccess::Success && NewObj)
		{
			static const FName CandidateProps[] = { FName("AssetToRun"), FName("AssetToEvaluate") };
			for (const FName& P : CandidateProps)
			{
				if (FObjectProperty* AssetProp = CastField<FObjectProperty>(NewObj->GetClass()->FindPropertyByName(P)))
				{
					NewObj->Modify();
					void* ValuePtr = AssetProp->ContainerPtrToValuePtr<void>(NewObj);
					AssetProp->SetObjectPropertyValue(ValuePtr, Asset);
					return;
				}
			}
		}
	}
}