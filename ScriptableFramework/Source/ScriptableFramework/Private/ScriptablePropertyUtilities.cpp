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
		// 3A. Object <-> Object (Bidirectional QoL for Blueprints)
		if (const FObjectPropertyBase* TargetObj = CastField<FObjectPropertyBase>(TargetProp))
		{
			return SourceObj->PropertyClass->IsChildOf(TargetObj->PropertyClass) ||
				TargetObj->PropertyClass->IsChildOf(SourceObj->PropertyClass);
		}

		// 3B. Object -> Bool (IsValid check)
		// Allows binding an Object directly to a Bool (True if valid, False if null)
		if (TargetProp->IsA<FBoolProperty>())
		{
			return true;
		}
	}

	// 4. Structs
	if (const FStructProperty* SourceStruct = CastField<FStructProperty>(SourceProp))
	{
		if (const FStructProperty* TargetStruct = CastField<FStructProperty>(TargetProp))
		{
			return SourceStruct->Struct == TargetStruct->Struct;
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
		// TObjectPtr <-> Raw Ptr
		if (const FObjectPropertyBase* TgtObjProp = CastField<FObjectPropertyBase>(TargetProp))
		{
			// This gets the UObject* regardless of whether it's stored as TObjectPtr or raw pointer
			UObject* SourceObject = SrcObjProp->GetObjectPropertyValue(SourceAddr);

			if (!SourceObject || SourceObject->IsA(TgtObjProp->PropertyClass))
			{
				TgtObjProp->SetObjectPropertyValue(TargetAddr, SourceObject);
			}
		}
		// Object -> Bool
		else if (const FBoolProperty* TgtBool = CastField<FBoolProperty>(TargetProp))
		{
			// Get the pointer (works for TObjectPtr and raw pointers)
			const UObject* SourceObject = SrcObjProp->GetObjectPropertyValue(SourceAddr);

			// True if not null, False if null
			TgtBool->SetPropertyValue(TargetAddr, SourceObject != nullptr);
		}
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
	}
	// Bool -> Numeric (True=1, False=0)
	else if (const FBoolProperty* SrcBool = CastField<FBoolProperty>(SourceProp))
	{
		if (const FNumericProperty* TgtNum = CastField<FNumericProperty>(TargetProp))
		{
			const bool bVal = SrcBool->GetPropertyValue(SourceAddr);
			if (TgtNum->IsFloatingPoint()) TgtNum->SetFloatingPointPropertyValue(TargetAddr, bVal ? 1.0 : 0.0);
			else TgtNum->SetIntPropertyValue(TargetAddr, int64(bVal ? 1 : 0));
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
		}
	}
}

#if WITH_EDITOR
TConstArrayView<FKzNamedVariant> FScriptablePropertyUtilities::FindLocalsDeclarationFor(const UObject* Node)
{
	if (!Node) return {};

	if (const UScriptableGraph* Graph = Node->GetTypedOuter<UScriptableGraph>())
	{
		return TConstArrayView<FKzNamedVariant>(Graph->Locals);
	}

	const UScriptStruct* BaseContainerStruct = FScriptableContainer::StaticStruct();
	for (const UObject* Cursor = Node->GetOuter(); Cursor; Cursor = Cursor->GetOuter())
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
					if (ObjProp->GetObjectPropertyValue(Helper.GetRawPtr(i)) == Node)
					{
						return TConstArrayView<FKzNamedVariant>(Container->LocalsDefinitions);
					}
				}
			}
		}
	}

	return {};
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

			const UScriptStruct* BaseContainerStruct = FScriptableContainer::StaticStruct();

			// Iterate through all properties of the Owner
			for (TFieldIterator<FProperty> It(Owner->GetClass()); It; ++It)
			{
				// CASE A: Direct Struct Property (e.g., FScriptableAction MyAction;)
				if (const FStructProperty* StructProp = CastField<FStructProperty>(*It))
				{
					if (StructProp->Struct->IsChildOf(BaseContainerStruct))
					{
						FScriptableContainer* Container = StructProp->ContainerPtrToValuePtr<FScriptableContainer>(Owner);

						// Reflect inside the container to see if it holds our Node
						for (TFieldIterator<FProperty> InnerIt(StructProp->Struct); InnerIt; ++InnerIt)
						{
							if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(*InnerIt))
							{
								if (const FObjectProperty* ObjProp = CastField<FObjectProperty>(ArrayProp->Inner))
								{
									FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Container));
									for (int32 i = 0; i < Helper.Num(); ++i)
									{
										if (ObjProp->GetObjectPropertyValue(Helper.GetRawPtr(i)) == Node) return Container;
									}
								}
							}
							else if (const FObjectProperty* ObjProp = CastField<FObjectProperty>(*InnerIt))
							{
								if (ObjProp->GetObjectPropertyValue_InContainer(Container) == Node) return Container;
							}
						}
					}
				}
				// CASE B: Array of Structs (e.g., TArray<FScriptableRequirement> Requirements;)
				else if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(*It))
				{
					if (const FStructProperty* InnerStructProp = CastField<FStructProperty>(ArrayProp->Inner))
					{
						if (InnerStructProp->Struct->IsChildOf(BaseContainerStruct))
						{
							FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Owner));
							for (int32 Index = 0; Index < Helper.Num(); ++Index)
							{
								FScriptableContainer* Container = reinterpret_cast<FScriptableContainer*>(Helper.GetRawPtr(Index));

								for (TFieldIterator<FProperty> InnerIt(InnerStructProp->Struct); InnerIt; ++InnerIt)
								{
									if (const FArrayProperty* InnerArrayProp = CastField<FArrayProperty>(*InnerIt))
									{
										if (const FObjectProperty* ObjProp = CastField<FObjectProperty>(InnerArrayProp->Inner))
										{
											FScriptArrayHelper InnerHelper(InnerArrayProp, InnerArrayProp->ContainerPtrToValuePtr<void>(Container));
											for (int32 i = 0; i < InnerHelper.Num(); ++i)
											{
												if (ObjProp->GetObjectPropertyValue(InnerHelper.GetRawPtr(i)) == Node) return Container;
											}
										}
									}
									else if (const FObjectProperty* ObjProp = CastField<FObjectProperty>(*InnerIt))
									{
										if (ObjProp->GetObjectPropertyValue_InContainer(Container) == Node) return Container;
									}
								}
							}
						}
					}
				}
			}
			return nullptr;
		};

	// =======================================================================================
	// LAMBDA 2: Reflection helper to collect previous siblings purely from memory arrays
	// =======================================================================================
	auto CollectSiblingsHeadless = [](const UObject* Node, TArray<const UScriptableObject*>& OutSiblings)
		{
			if (!Node) return;
			const UObject* Owner = Node->GetOuter();
			if (!Owner) return;

			// Inner helper: given the memory of a FScriptableContainer (Action / Requirement / ...),
			// find the array holding Node and append its previous entries to OutSiblings.
			// Returns true if Node was found inside this container.
			auto CollectFromContainerMemory = [Node, &OutSiblings](const UStruct* ContainerStruct, const void* ContainerMemory) -> bool
				{
					for (TFieldIterator<FProperty> InnerIt(ContainerStruct); InnerIt; ++InnerIt)
					{
						const FArrayProperty* InnerArrayProp = CastField<FArrayProperty>(*InnerIt);
						if (!InnerArrayProp) continue;

						const FObjectProperty* InnerObjProp = CastField<FObjectProperty>(InnerArrayProp->Inner);
						if (!InnerObjProp) continue;

						FScriptArrayHelper InnerHelper(InnerArrayProp, InnerArrayProp->ContainerPtrToValuePtr<void>(ContainerMemory));

						int32 TargetIndex = INDEX_NONE;
						for (int32 i = 0; i < InnerHelper.Num(); ++i)
						{
							if (InnerObjProp->GetObjectPropertyValue(InnerHelper.GetRawPtr(i)) == Node)
							{
								TargetIndex = i;
								break;
							}
						}

						if (TargetIndex != INDEX_NONE)
						{
							for (int32 i = 0; i < TargetIndex; ++i)
							{
								if (const UScriptableObject* PrevSibling = Cast<UScriptableObject>(InnerObjProp->GetObjectPropertyValue(InnerHelper.GetRawPtr(i))))
								{
									OutSiblings.Add(PrevSibling);
								}
							}
							return true;
						}
					}
					return false;
				};

			const UScriptStruct* BaseContainerStruct = FScriptableContainer::StaticStruct();

			for (TFieldIterator<FProperty> It(Owner->GetClass()); It; ++It)
			{
				// Case A: Single FScriptableContainer struct member on the Owner
				// (e.g. AActor with FScriptableAction OnShootAction; or UScriptableActionAsset::Action).
				if (const FStructProperty* StructProp = CastField<FStructProperty>(*It))
				{
					if (StructProp->Struct->IsChildOf(BaseContainerStruct))
					{
						const void* ContainerMemory = StructProp->ContainerPtrToValuePtr<void>(Owner);
						if (CollectFromContainerMemory(StructProp->Struct, ContainerMemory))
						{
							return;
						}
					}
				}
				else if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(*It))
				{
					// Case B: TArray<UObject*> directly on the Owner.
					if (const FObjectProperty* ObjProp = CastField<FObjectProperty>(ArrayProp->Inner))
					{
						FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Owner));

						int32 TargetIndex = INDEX_NONE;
						for (int32 i = 0; i < Helper.Num(); ++i)
						{
							if (ObjProp->GetObjectPropertyValue(Helper.GetRawPtr(i)) == Node)
							{
								TargetIndex = i;
								break;
							}
						}

						if (TargetIndex != INDEX_NONE)
						{
							for (int32 i = 0; i < TargetIndex; ++i)
							{
								if (const UScriptableObject* PrevSibling = Cast<UScriptableObject>(ObjProp->GetObjectPropertyValue(Helper.GetRawPtr(i))))
								{
									OutSiblings.Add(PrevSibling);
								}
							}
							return;
						}
					}
					// Case C: TArray<FScriptableContainer> on the Owner.
					else if (const FStructProperty* InnerStructProp = CastField<FStructProperty>(ArrayProp->Inner))
					{
						if (InnerStructProp->Struct->IsChildOf(BaseContainerStruct))
						{
							FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Owner));
							for (int32 Index = 0; Index < Helper.Num(); ++Index)
							{
								const void* ContainerMemory = Helper.GetRawPtr(Index);
								if (CollectFromContainerMemory(InnerStructProp->Struct, ContainerMemory))
								{
									return;
								}
							}
						}
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

#endif