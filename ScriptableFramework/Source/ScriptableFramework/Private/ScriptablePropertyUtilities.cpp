// Copyright 2026 kirzo

#include "ScriptablePropertyUtilities.h"
#include "ScriptableObject.h"
#include "ScriptableObjectAsset.h"
#include "ScriptableRuntimeData.h"
#include "ScriptableContainer.h"
#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptableNodes/ScriptableGraph.h"
#include "Core/KzNamedVariant.h"
#include "UObject/UnrealType.h"
#include "UObject/EnumProperty.h"
#include "StructUtils/PropertyBag.h"
#include "Bindings/ScriptableConversionRegistry.h"

bool FScriptablePropertyUtilities::ArePropertiesCompatible(const FProperty* SourceProp, const FProperty* TargetProp)
{
	if (!SourceProp || !TargetProp) return false;

	// 1. Fast Path: Identical Types
	if (SourceProp->SameType(TargetProp))
	{
		return true;
	}

	// 2. Arrays (Recursive Check)
	if (const FArrayProperty* SourceArray = CastField<FArrayProperty>(SourceProp))
	{
		if (const FArrayProperty* TargetArray = CastField<FArrayProperty>(TargetProp))
		{
			// Recursively check inner properties to support array covariance and conversions
			return ArePropertiesCompatible(SourceArray->Inner, TargetArray->Inner);
		}
		return false;
	}
	// If Source is not an array, but Target is, they are incompatible
	if (TargetProp->IsA<FArrayProperty>()) return false;

	// 3. Objects
	if (const FObjectPropertyBase* SourceObj = CastField<FObjectPropertyBase>(SourceProp))
	{
		// 3A. Object <-> Object (Bidirectional QoL for Blueprints). Mismatches fall through to the converter registry below.
		if (const FObjectPropertyBase* TargetObj = CastField<FObjectPropertyBase>(TargetProp))
		{
			if (SourceObj->PropertyClass->IsChildOf(TargetObj->PropertyClass) ||
				TargetObj->PropertyClass->IsChildOf(SourceObj->PropertyClass))
			{
				return true;
			}
		}

		// 3B. Object -> Bool (IsValid check)
		// Allows binding an Object directly to a Bool (True if valid, False if null)
		if (TargetProp->IsA<FBoolProperty>())
		{
			return true;
		}
	}

	// 4. Structs (exact match only; mismatches fall through to the converter registry below)
	if (const FStructProperty* SourceStruct = CastField<FStructProperty>(SourceProp))
	{
		if (const FStructProperty* TargetStruct = CastField<FStructProperty>(TargetProp))
		{
			if (SourceStruct->Struct == TargetStruct->Struct) return true;
		}
	}

	// 5. Enums & Bytes
	// Special case: Allow cross-binding between Bytes and Enums
	if ((SourceProp->IsA<FByteProperty>() && TargetProp->IsA<FEnumProperty>()) ||
		(SourceProp->IsA<FEnumProperty>() && TargetProp->IsA<FByteProperty>()))
	{
		return true;
	}
	// Strict Enum check
	if (const FEnumProperty* SourceEnum = CastField<FEnumProperty>(SourceProp))
	{
		if (const FEnumProperty* TargetEnum = CastField<FEnumProperty>(TargetProp))
		{
			return SourceEnum->GetEnum() == TargetEnum->GetEnum();
		}
	}

	// 6. Numeric & Bool Conversions
	const bool bSourceNumeric = SourceProp->IsA<FNumericProperty>();
	const bool bTargetNumeric = TargetProp->IsA<FNumericProperty>();
	const bool bSourceBool = SourceProp->IsA<FBoolProperty>();
	const bool bTargetBool = TargetProp->IsA<FBoolProperty>();

	// Numeric <-> Numeric (Int to Float, Float to Int, Byte to Double, etc.)
	if (bSourceNumeric && bTargetNumeric)
	{
		return true;
	}

	// Bool <-> Numeric (0/1 Logic)
	if ((bSourceBool && bTargetNumeric) || (bSourceNumeric && bTargetBool))
	{
		return true;
	}

	// Registered type converters (e.g. AActor* -> FKzTransformSource).
	if (FScriptableConversionRegistry::Get().HasConverter(SourceProp, TargetProp))
	{
		return true;
	}

	return false;
}

void FScriptablePropertyUtilities::CopyPropertyValue(const FProperty* SourceProp, const void* SourceAddr, const FProperty* TargetProp, void* TargetAddr)
{
	if (!SourceProp || !TargetProp || !SourceAddr || !TargetAddr) return;

	// Identical Types (Fast Copy)
	if (SourceProp->SameType(TargetProp))
	{
		SourceProp->CopyCompleteValue(TargetAddr, SourceAddr);
		return;
	}

	// Object Reference Handling (TObjectPtr <-> Raw Ptr, Child -> Parent)
	if (const FObjectPropertyBase* SrcObjProp = CastField<FObjectPropertyBase>(SourceProp))
	{
		// TObjectPtr <-> Raw Ptr (and soft source -> hard target)
		if (const FObjectPropertyBase* TgtObjProp = CastField<FObjectPropertyBase>(TargetProp))
		{
			// This gets the UObject* regardless of whether it's stored as TObjectPtr or raw pointer.
			UObject* SourceObject = SrcObjProp->GetObjectPropertyValue(SourceAddr);

			// A soft source resolves to null until it's loaded; load it so a HARD target receives the
			// asset (binding a TSoftObjectPtr<X> to a TObjectPtr<X>). Skip when the target is itself soft:
			// matching soft->soft is a same-type fast copy above, and we must not force a load there.
			if (!SourceObject && !TgtObjProp->IsA<FSoftObjectProperty>())
			{
				if (const FSoftObjectProperty* SrcSoftProp = CastField<FSoftObjectProperty>(SourceProp))
				{
					const FSoftObjectPtr& SoftValue = SrcSoftProp->GetPropertyValue(SourceAddr);
					if (!SoftValue.IsNull())
					{
						SourceObject = SoftValue.LoadSynchronous();
					}
				}
			}

			if (!SourceObject || SourceObject->IsA(TgtObjProp->PropertyClass))
			{
				TgtObjProp->SetObjectPropertyValue(TargetAddr, SourceObject);
			}
			return;
		}
		// Object -> Bool
		if (const FBoolProperty* TgtBool = CastField<FBoolProperty>(TargetProp))
		{
			// Get the pointer (works for TObjectPtr and raw pointers)
			const UObject* SourceObject = SrcObjProp->GetObjectPropertyValue(SourceAddr);

			// True if not null, False if null
			TgtBool->SetPropertyValue(TargetAddr, SourceObject != nullptr);
			return;
		}
		// Object -> anything else (e.g. a struct): handled by the converter registry below.
	}
	// Numeric <-> Numeric Conversion
	else if (SourceProp->IsA<FNumericProperty>() && TargetProp->IsA<FNumericProperty>())
	{
		const FNumericProperty* SrcNum = CastField<FNumericProperty>(SourceProp);
		const FNumericProperty* TgtNum = CastField<FNumericProperty>(TargetProp);

		if (SrcNum->IsFloatingPoint())
		{
			const double Val = SrcNum->GetFloatingPointPropertyValue(SourceAddr);
			if (TgtNum->IsFloatingPoint()) TgtNum->SetFloatingPointPropertyValue(TargetAddr, Val);
			else TgtNum->SetIntPropertyValue(TargetAddr, (int64)Val);
		}
		else
		{
			const int64 Val = SrcNum->GetSignedIntPropertyValue(SourceAddr);
			if (TgtNum->IsFloatingPoint()) TgtNum->SetFloatingPointPropertyValue(TargetAddr, (double)Val);
			else TgtNum->SetIntPropertyValue(TargetAddr, Val);
		}
		return;
	}
	// Bool -> Numeric (True=1, False=0)
	else if (const FBoolProperty* SrcBool = CastField<FBoolProperty>(SourceProp))
	{
		if (const FNumericProperty* TgtNum = CastField<FNumericProperty>(TargetProp))
		{
			const bool bVal = SrcBool->GetPropertyValue(SourceAddr);
			if (TgtNum->IsFloatingPoint()) TgtNum->SetFloatingPointPropertyValue(TargetAddr, bVal ? 1.0 : 0.0);
			else TgtNum->SetIntPropertyValue(TargetAddr, int64(bVal ? 1 : 0));
			return;
		}
	}
	// Numeric -> Bool (0=False, !=0 True)
	else if (const FNumericProperty* SrcNum = CastField<FNumericProperty>(SourceProp))
	{
		if (const FBoolProperty* TgtBool = CastField<FBoolProperty>(TargetProp))
		{
			bool bResult = false;
			if (SrcNum->IsFloatingPoint()) bResult = !FMath::IsNearlyZero(SrcNum->GetFloatingPointPropertyValue(SourceAddr));
			else bResult = (SrcNum->GetSignedIntPropertyValue(SourceAddr) != 0);

			TgtBool->SetPropertyValue(TargetAddr, bResult);
			return;
		}
	}

	// Fallback: registered type converters (e.g. AActor* -> FKzTransformSource).
	FScriptableConversionRegistry::Get().Convert(SourceProp, SourceAddr, TargetProp, TargetAddr);
}

#if WITH_EDITOR
namespace
{
	using FContainerVisitor = TFunctionRef<bool(FScriptableContainer&, const UScriptStruct*, const FProperty*)>;

	/**
	 * Visits every FScriptableContainer reachable from a struct's properties, descending through plain
	 * structs and arrays. Containers are authored wherever it is convenient (a member, an array of them,
	 * a member of a struct inside an array), so the search cannot enumerate shapes: it has to recurse.
	 * Returns true as soon as the visitor claims a container, which stops the walk.
	 */
	bool VisitContainers(const UStruct* Struct, void* Memory, const FContainerVisitor& Visitor, const FProperty* RootProperty = nullptr)
	{
		if (!Struct || !Memory) return false;

		const UScriptStruct* BaseContainerStruct = FScriptableContainer::StaticStruct();

		for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			// The outermost property is the one carrying the edit flags, so it is what gets reported
			const FProperty* Root = RootProperty ? RootProperty : *It;

			if (const FStructProperty* StructProp = CastField<FStructProperty>(*It))
			{
				void* StructMemory = StructProp->ContainerPtrToValuePtr<void>(Memory);

				if (StructProp->Struct->IsChildOf(BaseContainerStruct))
				{
					if (Visitor(*static_cast<FScriptableContainer*>(StructMemory), StructProp->Struct, Root)) return true;
				}
				else if (VisitContainers(StructProp->Struct, StructMemory, Visitor, Root))
				{
					return true;
				}
			}
			else if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(*It))
			{
				const FStructProperty* InnerStructProp = CastField<FStructProperty>(ArrayProp->Inner);
				if (!InnerStructProp) continue;

				FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Memory));
				for (int32 Index = 0; Index < Helper.Num(); ++Index)
				{
					void* ElementMemory = Helper.GetRawPtr(Index);

					if (InnerStructProp->Struct->IsChildOf(BaseContainerStruct))
					{
						if (Visitor(*reinterpret_cast<FScriptableContainer*>(ElementMemory), InnerStructProp->Struct, Root)) return true;
					}
					else if (VisitContainers(InnerStructProp->Struct, ElementMemory, Visitor, Root))
					{
						return true;
					}
				}
			}
		}

		return false;
	}

	/** Index of Node inside one of the container's object arrays, or INDEX_NONE if it holds it elsewhere. */
	int32 FindNodeInContainer(const UScriptStruct* ContainerStruct, void* ContainerMemory, const UObject* Node, const FArrayProperty*& OutArrayProp)
	{
		OutArrayProp = nullptr;

		for (TFieldIterator<FProperty> It(ContainerStruct); It; ++It)
		{
			if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(*It))
			{
				const FObjectProperty* ObjProp = CastField<FObjectProperty>(ArrayProp->Inner);
				if (!ObjProp) continue;

				FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(ContainerMemory));
				for (int32 i = 0; i < Helper.Num(); ++i)
				{
					if (ObjProp->GetObjectPropertyValue(Helper.GetRawPtr(i)) == Node)
					{
						OutArrayProp = ArrayProp;
						return i;
					}
				}
			}
			else if (const FObjectProperty* ObjProp = CastField<FObjectProperty>(*It))
			{
				if (ObjProp->GetObjectPropertyValue_InContainer(ContainerMemory) == Node) return 0;
			}
		}

		return INDEX_NONE;
	}
}

TConstArrayView<FKzNamedVariant> FScriptablePropertyUtilities::FindLocalsDeclarationFor(const UObject* Node)
{
	if (!Node) return {};

	if (const UScriptableGraph* Graph = Node->GetTypedOuter<UScriptableGraph>())
	{
		return TConstArrayView<FKzNamedVariant>(Graph->Locals);
	}

	for (UObject* Cursor = const_cast<UObject*>(Node->GetOuter()); Cursor; Cursor = Cursor->GetOuter())
	{
		bool bFound = false;
		TConstArrayView<FKzNamedVariant> Locals;
		VisitContainers(Cursor->GetClass(), Cursor, [Node, &bFound, &Locals](FScriptableContainer& Container, const UScriptStruct* ContainerStruct, const FProperty*)
			{
				const FArrayProperty* HoldingArray = nullptr;
				if (FindNodeInContainer(ContainerStruct, &Container, Node, HoldingArray) == INDEX_NONE) return false;

				bFound = true;
				Locals = TConstArrayView<FKzNamedVariant>(Container.LocalsDefinitions);
				return true;
			});

		// The nearest container owns the answer, even when it declares no locals at all
		if (bFound) return Locals;
	}
	return {};
}

bool FScriptablePropertyUtilities::IsOwningContainerInstanceEditable(const UObject* Node)
{
	if (!Node) return false;

	for (UObject* Cursor = const_cast<UObject*>(Node->GetOuter()); Cursor; Cursor = Cursor->GetOuter())
	{
		bool bFound = false;
		bool bEditable = false;

		VisitContainers(Cursor->GetClass(), Cursor, [Node, &bFound, &bEditable](FScriptableContainer& Container, const UScriptStruct* ContainerStruct, const FProperty* RootProperty)
			{
				const FArrayProperty* HoldingArray = nullptr;
				if (FindNodeInContainer(ContainerStruct, &Container, Node, HoldingArray) == INDEX_NONE) return false;

				bFound = true;

				// EditAnywhere / EditInstanceOnly -> editable per instance; EditDefaultsOnly is not.
				bEditable = RootProperty && RootProperty->HasAnyPropertyFlags(CPF_Edit) && !RootProperty->HasAnyPropertyFlags(CPF_DisableEditOnInstance);
				return true;
			});

		if (bFound) return bEditable;
	}
	return false;
}

bool FScriptablePropertyUtilities::IsPropertyBindableInput(const FProperty* Property)
{
	if (!Property) return false;
	if (Property->HasMetaData(TEXT("ScriptableInput"))) return true;
	const FString Category = Property->GetMetaData(TEXT("Category"));
	return Category.Contains(TEXT("Input"));
}

bool FScriptablePropertyUtilities::IsPropertyBindableOutput(const FProperty* Property)
{
	if (!Property) return false;
	if (Property->HasMetaData(TEXT("ScriptableOutput"))) return true;
	const FString Category = Property->GetMetaData(TEXT("Category"));
	return Category.Contains(TEXT("Output"));
}

bool FScriptablePropertyUtilities::IsPropertyBindableContext(const FProperty* Property)
{
	if (!Property) return false;
	if (Property->HasMetaData(TEXT("ScriptableContext"))) return true;
	const FString Category = Property->GetMetaData(TEXT("Category"));
	return Category.Contains(TEXT("Context"));
}

bool FScriptablePropertyUtilities::AreSiblingBindingsAllowed(const UObject* ParentObject)
{
	if (!ParentObject) return false;
	return !ParentObject->GetClass()->HasMetaData(TEXT("BlockSiblingBindings"));
}

void FScriptablePropertyUtilities::CollectPreviousSiblings(const UObject* ParentObject, const UObject* CurrentChild, TArray<const UScriptableObject*>& OutObjects)
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


void FScriptablePropertyUtilities::GatherAccessibleStructs(const UScriptableObject* TargetObject, TArray<FPropertyBindingBindableStructDescriptor>& OutStructDescs)
{
	OutStructDescs.Reset();
	if (!TargetObject) return;
	const UScriptableObject* RootObject = TargetObject->GetRoot();
	if (!RootObject) return;

	// =======================================================================================
	// LAMBDA 1: Reflection Helper to find the FScriptableContainer holding a specific Node
	// =======================================================================================
	auto FindContainerForNode = [](const UObject* Node) -> FScriptableContainer*
		{
			if (!Node) return nullptr;
			UObject* Owner = const_cast<UObject*>(Node->GetOuter());
			if (!Owner) return nullptr;

			FScriptableContainer* Found = nullptr;
			VisitContainers(Owner->GetClass(), Owner, [Node, &Found](FScriptableContainer& Container, const UScriptStruct* ContainerStruct, const FProperty*)
				{
					const FArrayProperty* HoldingArray = nullptr;
					if (FindNodeInContainer(ContainerStruct, &Container, Node, HoldingArray) != INDEX_NONE)
					{
						Found = &Container;
						return true;
					}
					return false;
				});

			return Found;
		};

	// =======================================================================================
	// LAMBDA 2: Reflection helper to collect previous siblings purely from memory arrays
	// =======================================================================================
	auto CollectSiblingsHeadless = [](const UObject* Node, TArray<const UScriptableObject*>& OutSiblings)
		{
			if (!Node) return;
			UObject* Owner = const_cast<UObject*>(Node->GetOuter());
			if (!Owner) return;

			// Siblings are the entries authored before Node in whichever array holds it
			auto CollectBefore = [&OutSiblings](const FArrayProperty* ArrayProp, void* ArrayOwner, int32 TargetIndex)
				{
					const FObjectProperty* ObjProp = CastField<FObjectProperty>(ArrayProp->Inner);
					if (!ObjProp) return;

					FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(ArrayOwner));
					for (int32 i = 0; i < TargetIndex && i < Helper.Num(); ++i)
					{
						if (const UScriptableObject* PrevSibling = Cast<UScriptableObject>(ObjProp->GetObjectPropertyValue(Helper.GetRawPtr(i))))
						{
							OutSiblings.Add(PrevSibling);
						}
					}
				};

			bool bFound = false;
			VisitContainers(Owner->GetClass(), Owner, [Node, &CollectBefore, &bFound](FScriptableContainer& Container, const UScriptStruct* ContainerStruct, const FProperty*)
				{
					const FArrayProperty* HoldingArray = nullptr;
					const int32 Index = FindNodeInContainer(ContainerStruct, &Container, Node, HoldingArray);
					if (Index == INDEX_NONE) return false;

					if (HoldingArray)
					{
						CollectBefore(HoldingArray, &Container, Index);
					}

					bFound = true;
					return true;
				});

			if (bFound) return;

			// Not held by a container: a plain TArray<UObject*> on the owner, as graph nodes are
			for (TFieldIterator<FProperty> It(Owner->GetClass()); It; ++It)
			{
				const FArrayProperty* ArrayProp = CastField<FArrayProperty>(*It);
				const FObjectProperty* ObjProp = ArrayProp ? CastField<FObjectProperty>(ArrayProp->Inner) : nullptr;
				if (!ObjProp) continue;

				FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Owner));
				for (int32 i = 0; i < Helper.Num(); ++i)
				{
					if (ObjProp->GetObjectPropertyValue(Helper.GetRawPtr(i)) == Node)
					{
						CollectBefore(ArrayProp, Owner, i);
						return;
					}
				}
			}
		};

	// -------------------------------------------------------------------------------
	// 1. Context Hierarchy (Single Effective Scope via Reflection)
	// -------------------------------------------------------------------------------
	const UObject* CurrentNode = TargetObject;
	bool bFoundContext = false;

	while (CurrentNode && !bFoundContext)
	{
		bool bFoundLocals = false;

		// Try to find the FScriptableContainer structurally wrapping the CurrentNode
		if (FScriptableContainer* Container = FindContainerForNode(CurrentNode))
		{
			// GetContext() rebuilds the transient bag from ContextDefinitions on the fly when needed
			// (after load it's empty until something hydrates it); without this the bake at PreSave
			// would see no context and wipe the task's auto-bindings.
			FInstancedPropertyBag& Bag = Container->GetContext();
			if (Bag.IsValid() && Bag.GetNumPropertiesInBag() > 0)
			{
				FPropertyBindingBindableStructDescriptor& ContextDesc = OutStructDescs.AddDefaulted_GetRef();
				ContextDesc.Name = FName(TEXT("Context"));
				ContextDesc.Struct = Bag.GetPropertyBagStruct();
				ContextDesc.ID = ScriptableBindingSources::ContextStructID;

				bFoundContext = true;
			}

			// GetLocals() rebuilds the runtime bag's shape from LocalsDefinitions on demand, mirroring Context.
			FInstancedPropertyBag& LocalsBag = Container->GetLocals();
			if (LocalsBag.IsValid() && LocalsBag.GetNumPropertiesInBag() > 0)
			{
				FPropertyBindingBindableStructDescriptor& LocalsDesc = OutStructDescs.AddDefaulted_GetRef();
				LocalsDesc.Name = FName(TEXT("Locals"));
				LocalsDesc.Struct = LocalsBag.GetPropertyBagStruct();
				LocalsDesc.ID = ScriptableBindingSources::LocalsStructID;
				bFoundLocals = true;
			}

			if (bFoundContext || bFoundLocals) break;
		}

		// Graph fallback: UScriptableGraph holds Context as a flat field and exposes Locals via GetLocalsShape.
		if (const UScriptableGraph* ConstGraph = Cast<UScriptableGraph>(CurrentNode))
		{
			UScriptableGraph* Graph = const_cast<UScriptableGraph*>(ConstGraph);
			if (const FInstancedPropertyBag* AssetContext = Graph->GetContext())
			{
				if (AssetContext->IsValid() && AssetContext->GetNumPropertiesInBag() > 0)
				{
					FPropertyBindingBindableStructDescriptor& ContextDesc = OutStructDescs.AddDefaulted_GetRef();
					ContextDesc.Name = FName(TEXT("Context"));
					ContextDesc.Struct = AssetContext->GetPropertyBagStruct();
					ContextDesc.ID = ScriptableBindingSources::ContextStructID;

					bFoundContext = true;
				}
			}

			if (const FInstancedPropertyBag* LocalsShape = Graph->GetLocalsShape())
			{
				if (LocalsShape->IsValid() && LocalsShape->GetNumPropertiesInBag() > 0)
				{
					FPropertyBindingBindableStructDescriptor& LocalsDesc = OutStructDescs.AddDefaulted_GetRef();
					LocalsDesc.Name = FName(TEXT("Locals"));
					LocalsDesc.Struct = LocalsShape->GetPropertyBagStruct();
					LocalsDesc.ID = ScriptableBindingSources::LocalsStructID;
					bFoundLocals = true;
				}
			}

			if (bFoundContext || bFoundLocals) break;
		}
		// Non-graph asset fallback: still publishes Context. Locals only exist on UScriptableGraph and on
		// FScriptableContainer, handled above.
		else if (const UScriptableObjectAsset* ConstAsset = Cast<UScriptableObjectAsset>(CurrentNode))
		{
			UScriptableObjectAsset* Asset = const_cast<UScriptableObjectAsset*>(ConstAsset);
			if (const FInstancedPropertyBag* AssetContext = Asset->GetContext())
			{
				if (AssetContext->IsValid() && AssetContext->GetNumPropertiesInBag() > 0)
				{
					FPropertyBindingBindableStructDescriptor& ContextDesc = OutStructDescs.AddDefaulted_GetRef();
					ContextDesc.Name = FName(TEXT("Context"));
					ContextDesc.Struct = AssetContext->GetPropertyBagStruct();
					ContextDesc.ID = ScriptableBindingSources::ContextStructID;

					bFoundContext = true;
					break;
				}
			}
		}

		// Move up to the next parent in the hierarchy
		CurrentNode = CurrentNode->GetOuter();
		if (CurrentNode && CurrentNode->IsA<UPackage>()) break;
	}

	// -------------------------------------------------------------------------------
	// 2. Siblings (Via Memory Traversal)
	// -------------------------------------------------------------------------------
	TArray<const UScriptableObject*> AccessibleObjects;
	CollectSiblingsHeadless(TargetObject, AccessibleObjects);

	// -------------------------------------------------------------------------------
	// 3. Traversal (Hierarchical Parents & Siblings of Parents)
	// -------------------------------------------------------------------------------
	const UObject* IteratorNode = TargetObject;
	while (IteratorNode)
	{
		const UObject* ParentNode = IteratorNode->GetOuter();
		if (!ParentNode || ParentNode == RootObject->GetOuter()) break;

		// A graph-node wrapper is not itself a bindable source: a task reads the graph Context and
		// other nodes' Outputs (handled in section 3.5 below), never its own hosting node. Skipping
		// it stops a task from listing its hosting node (i.e. "itself") as a source.
		if (const UScriptableObject* ParentScriptableObject = Cast<UScriptableObject>(ParentNode))
		{
			if (!ParentNode->IsA<UScriptableNode>())
			{
				AccessibleObjects.Add(ParentScriptableObject); // Parent
				if (AreSiblingBindingsAllowed(ParentScriptableObject))
				{
					CollectPreviousSiblings(ParentScriptableObject, IteratorNode, AccessibleObjects);
				}
			}
		}
		IteratorNode = ParentNode;
	}

	// -------------------------------------------------------------------------------
	// 3.5 Graph cross-node Outputs
	// In a graph each task lives in its own node, so tasks are never array-siblings. Expose every
	// OTHER node's proxy that carries an Output, so a node's Input can read it. Ordering across
	// event-driven paths is the author's responsibility (the runtime no-ops on an unset value).
	// -------------------------------------------------------------------------------
	{
		const UScriptableNode* OwningNode = nullptr;
		const UScriptableGraph* OwningGraph = nullptr;
		for (const UObject* Outer = TargetObject->GetOuter(); Outer; Outer = Outer->GetOuter())
		{
			if (!OwningNode) OwningNode = Cast<UScriptableNode>(Outer);
			if (const UScriptableGraph* Graph = Cast<UScriptableGraph>(Outer)) { OwningGraph = Graph; break; }
		}

		if (OwningGraph)
		{
			for (const TObjectPtr<UScriptableNode>& Node : OwningGraph->Nodes)
			{
				if (!Node || Node == OwningNode) continue;

				UScriptableObject* Proxy = Node->GetBindingProxy();
				if (!Proxy) continue;

				// Only expose proxies that actually carry an Output (skip pure inputs/internals).
				bool bHasOutput = false;
				for (TFieldIterator<FProperty> PropIt(Proxy->GetClass()); PropIt && !bHasOutput; ++PropIt)
				{
					bHasOutput = IsPropertyBindableOutput(*PropIt);
				}

				if (bHasOutput)
				{
					AccessibleObjects.Add(Proxy);
				}
			}
		}
	}

	// -------------------------------------------------------------------------------
	// 4. Convert to Output
	// -------------------------------------------------------------------------------
	for (const UScriptableObject* Obj : AccessibleObjects)
	{
		if (Obj->GetBindingID().IsValid())
		{
			FPropertyBindingBindableStructDescriptor& Desc = OutStructDescs.AddDefaulted_GetRef();
			FString DisplayName = Obj->GetName();
			Desc.Name = FName(*DisplayName);
			Desc.Struct = Obj->GetClass();
			Desc.ID = Obj->GetBindingID();
		}
	}
}

bool FScriptablePropertyUtilities::FindAutoBindingPath(const FProperty* TargetProperty, const TArray<FPropertyBindingBindableStructDescriptor>& AccessibleStructs, FPropertyBindingPath& OutPath)
{
	if (!TargetProperty) return false;

	// Check if the property is actually marked for context binding (meta or category)
	if (!IsPropertyBindableContext(TargetProperty))
	{
		return false;
	}

	// Search through all available contexts (Global, Local, Owner, etc.)
	for (const FPropertyBindingBindableStructDescriptor& ContextDesc : AccessibleStructs)
	{
		// Auto-binding only ever targets the Context (empty ID). Sibling/cross-node sources carry a
		// valid ID and must be wired manually; auto-matching them by name/type would silently re-route
		// a context binding to another node, which then fails to resolve at runtime.
		if (ContextDesc.ID.IsValid())
		{
			continue;
		}

		const UStruct* Struct = ContextDesc.Struct.Get();
		if (!Struct)
		{
			continue;
		}

		// This handles cases where both the ScriptableObject and Context have a property with the same name
		if (const FProperty* ExactMatchProp = Struct->FindPropertyByName(TargetProperty->GetFName()))
		{
			if (ArePropertiesCompatible(ExactMatchProp, TargetProperty))
			{
				OutPath.Reset();
				OutPath.SetStructID(ContextDesc.ID);
				OutPath.AddPathSegment(ExactMatchProp->GetFName());
				return true;
			}
		}

		// This handles your edge case: Context has "Owner" (AActor*) and Task asks for "TargetActor" (AActor*)
		for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			const FProperty* SourceProp = *It;

			// Skip the exact name match since we already tested it above
			if (SourceProp->GetFName() == TargetProperty->GetFName())
			{
				continue;
			}

			// First property that satisfies the strict type compatibility wins
			if (ArePropertiesCompatible(SourceProp, TargetProperty))
			{
				OutPath.Reset();
				OutPath.SetStructID(ContextDesc.ID);
				OutPath.AddPathSegment(SourceProp->GetFName());
				return true;
			}
		}
	}

	// No compatible property was found in any of the accessible contexts
	return false;
}

UClass* FScriptablePropertyUtilities::ResolveBoundSourceClass(const UScriptableObject* Object, const FProperty* TargetProperty)
{
	if (!Object || !TargetProperty) return nullptr;

	TArray<FPropertyBindingBindableStructDescriptor> AccessibleStructs;
	GatherAccessibleStructs(Object, AccessibleStructs);

	for (const FScriptablePropertyBinding& Binding : Object->GetPropertyBindings().Bindings)
	{
		// Match the binding whose target is TargetProperty (a top-level object property: single leaf segment).
		const int32 NumTargetSegments = Binding.TargetPath.NumSegments();
		if (NumTargetSegments == 0 || Binding.TargetPath.GetSegment(NumTargetSegments - 1).GetName() != TargetProperty->GetFName())
		{
			continue;
		}

		const FPropertyBindingBindableStructDescriptor* SourceDesc = AccessibleStructs.FindByPredicate(
			[&](const FPropertyBindingBindableStructDescriptor& Desc) { return Desc.ID == Binding.SourcePath.GetStructID(); });
		if (!SourceDesc) return nullptr;

		// Walk the source path to its leaf property, descending through nested structs.
		const UStruct* CurrentStruct = SourceDesc->Struct.Get();
		const FProperty* LeafProp = nullptr;
		for (int32 i = 0; i < Binding.SourcePath.NumSegments(); ++i)
		{
			if (!CurrentStruct) return nullptr;
			LeafProp = CurrentStruct->FindPropertyByName(Binding.SourcePath.GetSegment(i).GetName());
			if (!LeafProp) return nullptr;
			CurrentStruct = nullptr;
			if (const FStructProperty* StructProp = CastField<FStructProperty>(LeafProp)) CurrentStruct = StructProp->Struct;
		}

		if (const FObjectPropertyBase* ObjProp = CastField<FObjectPropertyBase>(LeafProp)) return ObjProp->PropertyClass;
		return nullptr;
	}
	return nullptr;
}

#endif