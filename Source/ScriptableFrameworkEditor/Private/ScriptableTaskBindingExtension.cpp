// Copyright 2026 kirzo

#include "ScriptableTaskBindingExtension.h"
#include "ScriptableObject.h"
#include "Bindings/ScriptablePropertyBindingsOwner.h"
#include "Bindings/ScriptablePropertyBindings.h"
#include "PropertyBindingPath.h"

#include "IPropertyAccessEditor.h"
#include "Features/IModularFeatures.h"
#include "PropertyHandle.h"
#include "DetailWidgetRow.h"
#include "IDetailPropertyRow.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "EdGraphSchema_K2.h"
#include "StructUtils/InstancedStruct.h"
#include "Misc/Crc.h"

#define LOCTEXT_NAMESPACE "ScriptableTaskBindingExtension"

UE_DISABLE_OPTIMIZATION

namespace ScriptableObject::PropertyBinding
{
	/**
	 * Helper to find the owning ScriptableObject.
	 */
	UScriptableObject* GetOuterScriptableObject(const TSharedPtr<const IPropertyHandle>& InPropertyHandle)
	{
		TArray<UObject*> OuterObjects;
		InPropertyHandle->GetOuterObjects(OuterObjects);
		for (UObject* OuterObject : OuterObjects)
		{
			if (OuterObject)
			{
				if (UScriptableObject* ScriptableObject = Cast<UScriptableObject>(OuterObject))
				{
					return ScriptableObject;
				}
				if (UScriptableObject* OuterScriptableObject = OuterObject->GetTypedOuter<UScriptableObject>())
				{
					return OuterScriptableObject;
				}
			}
		}
		return nullptr;
	}

	/**
	 * Generates a deterministic ID based on the owning object.
	 * This avoids using FGuid() (zeros), which can be interpreted as invalid by the binding system.
	 */
	FGuid GetScriptableObjectDataID(UScriptableObject* Owner)
	{
		if (Owner)
		{
			// We use PathName to ensure stability between editor sessions for the same object/asset.
			// Note: If the object is renamed, the ID changes and bindings might break,
			// but for dynamic PropertyBags in the editor, this is usually acceptable.
			const uint32 Hash = GetTypeHash(Owner->GetPathName());
			return FGuid(Hash, Hash, Hash, Hash);
		}
		return FGuid();
	}

	/**
	 * Generates a property path for the property being edited (The Target of the binding).
	 */
	void MakeStructPropertyPathFromPropertyHandle(TSharedPtr<const IPropertyHandle> InPropertyHandle, FPropertyBindingPath& OutPath)
	{
		OutPath.Reset();

		UScriptableObject* ScriptableObject = GetOuterScriptableObject(InPropertyHandle);
		if (!ScriptableObject)
		{
			return;
		}

		TArray<FPropertyBindingPathSegment> PathSegments;
		TSharedPtr<const IPropertyHandle> CurrentPropertyHandle = InPropertyHandle;

		while (CurrentPropertyHandle.IsValid())
		{
			const FProperty* Property = CurrentPropertyHandle->GetProperty();
			if (Property)
			{
				// If we are editing an Instanced property on an Actor, the hierarchy eventually reaches the Actor's property.
				// We must stop there because runtime binding is relative to the ScriptableObject, not the Actor.
				if (const UClass* PropertyOwnerClass = Cast<UClass>(Property->GetOwnerStruct()))
				{
					// If the property belongs to a class that is NOT the ScriptableObject or one of its parents,
					// it means we have reached the container (e.g., the Actor). Stop adding segments.
					if (!ScriptableObject->GetClass()->IsChildOf(PropertyOwnerClass))
					{
						break;
					}
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

	/**
	 * Resolves the Source Path from the Binding Chain provided by the Widget.
	 */
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

	/**
	 * Struct to cache binding data and handle UI updates (Text, Color, Tooltip).
	 */
	struct FCachedBindingData : public TSharedFromThis<FCachedBindingData>
	{
		TWeakObjectPtr<UScriptableObject> WeakScriptableObject;
		FPropertyBindingPath TargetPath;
		TSharedPtr<const IPropertyHandle> PropertyHandle;
		TArray<FBindableStructDesc> AccessibleStructs;

		// Cached UI data
		FText Text;
		FText TooltipText;
		FLinearColor Color = FLinearColor::White;
		const FSlateBrush* Image = nullptr;
		bool bIsDataCached = false;

		FCachedBindingData(UScriptableObject* InObject, const FPropertyBindingPath& InTargetPath, const TSharedPtr<const IPropertyHandle>& InHandle, const TArray<FBindableStructDesc>& InStructs)
			: WeakScriptableObject(InObject)
			, TargetPath(InTargetPath)
			, PropertyHandle(InHandle)
			, AccessibleStructs(InStructs)
		{
		}

		void UpdateData()
		{
			static const FName PropertyIcon(TEXT("Kismet.Tabs.Variables"));
			Text = FText::GetEmpty();
			TooltipText = FText::GetEmpty();
			Color = FLinearColor::White;
			Image = nullptr;

			if (!PropertyHandle.IsValid()) return;
			UScriptableObject* ScriptableObject = WeakScriptableObject.Get();
			if (!ScriptableObject) return;

			IScriptablePropertyBindingsOwner* BindingOwner = Cast<IScriptablePropertyBindingsOwner>(ScriptableObject);
			FScriptablePropertyBindings* Bindings = BindingOwner ? BindingOwner->GetPropertyBindings() : nullptr;

			const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
			FEdGraphPinType PinType;
			Schema->ConvertPropertyToPinType(PropertyHandle->GetProperty(), PinType);

			if (Bindings && Bindings->HasPropertyBinding(TargetPath))
			{
				// Property IS Bound
				const FPropertyBindingPath* SourcePath = Bindings->GetPropertyBinding(TargetPath);
				if (SourcePath)
				{
					const FBindableStructDesc* SourceDesc = AccessibleStructs.FindByPredicate([&](const FBindableStructDesc& Desc) { return Desc.ID == SourcePath->GetStructID(); });

					FString DisplayString = SourceDesc ? SourceDesc->Name.ToString() : TEXT("Unknown");
					if (!SourcePath->IsPathEmpty())
					{
						DisplayString += TEXT(".") + SourcePath->ToString();
					}

					Text = FText::FromString(DisplayString);
					TooltipText = FText::Format(LOCTEXT("BindingTooltip", "Bound to {0}"), Text);
					Image = FAppStyle::GetBrush(PropertyIcon);
					Color = Schema->GetPinTypeColor(PinType);
				}
			}
			else
			{
				// Property is NOT Bound
				Text = LOCTEXT("Bind", "Bind");
				TooltipText = LOCTEXT("BindActionTooltip", "Bind this property to a value from the Context.");
				Image = FAppStyle::GetBrush(PropertyIcon);

				// Use a neutral default color (White) instead of the specific PinType color when unbound
				Color = FLinearColor::White;
			}

			bIsDataCached = true;
		}

		void AddBinding(const TArray<FBindingChainElement>& InBindingChain)
		{
			if (InBindingChain.IsEmpty()) return;

			UScriptableObject* ScriptableObject = WeakScriptableObject.Get();
			if (!ScriptableObject) return;
			IScriptablePropertyBindingsOwner* BindingOwner = Cast<IScriptablePropertyBindingsOwner>(ScriptableObject);
			if (!BindingOwner) return;

			// Index 0 is the struct context index
			const int32 ContextIndex = InBindingChain[0].ArrayIndex;
			if (!AccessibleStructs.IsValidIndex(ContextIndex)) return;

			FScopedTransaction Transaction(LOCTEXT("AddBinding", "Add Property Binding"));
			ScriptableObject->Modify();

			const FBindableStructDesc& SelectedContext = AccessibleStructs[ContextIndex];

			// Remove context index from chain to process properties
			TArray<FBindingChainElement> PropertyChain = InBindingChain;
			PropertyChain.RemoveAt(0);

			FPropertyBindingPath SourcePath;
			MakeStructPropertyPathFromBindingChain(SelectedContext.ID, PropertyChain, SourcePath);

			BindingOwner->GetPropertyBindings()->AddPropertyBinding(SourcePath, TargetPath);
			UpdateData();
		}

		void RemoveBinding()
		{
			UScriptableObject* ScriptableObject = WeakScriptableObject.Get();
			if (!ScriptableObject) return;
			IScriptablePropertyBindingsOwner* BindingOwner = Cast<IScriptablePropertyBindingsOwner>(ScriptableObject);
			if (!BindingOwner) return;

			FScopedTransaction Transaction(LOCTEXT("RemoveBinding", "Remove Property Binding"));
			ScriptableObject->Modify();

			BindingOwner->GetPropertyBindings()->RemovePropertyBindings(TargetPath);
			UpdateData();
		}

		bool CanRemoveBinding() const
		{
			if (UScriptableObject* ScriptableObject = WeakScriptableObject.Get())
			{
				if (IScriptablePropertyBindingsOwner* BindingOwner = Cast<IScriptablePropertyBindingsOwner>(ScriptableObject))
				{
					return BindingOwner->GetPropertyBindings()->HasPropertyBinding(TargetPath);
				}
			}
			return false;
		}

		// UI Accessors
		FText GetText() { if (!bIsDataCached) UpdateData(); return Text; }
		FText GetTooltipText() { if (!bIsDataCached) UpdateData(); return TooltipText; }
		FLinearColor GetColor() { if (!bIsDataCached) UpdateData(); return Color; }
		const FSlateBrush* GetImage() { if (!bIsDataCached) UpdateData(); return Image; }
	};
}

bool FScriptableTaskBindingExtension::IsPropertyExtendable(const UClass* InObjectClass, const IPropertyHandle& PropertyHandle) const
{
	const FProperty* Property = PropertyHandle.GetProperty();
	// Avoid standard properties that shouldn't be bound
	if (!Property || Property->HasAnyPropertyFlags(CPF_PersistentInstance | CPF_EditorOnly | CPF_Config | CPF_Deprecated))
	{
		return false;
	}

	// Check metadata
	if (PropertyHandle.HasMetaData(TEXT("NoBinding")))
	{
		return false;
	}

	// Ensure we are inside a ScriptableObject
	return ScriptableObject::PropertyBinding::GetOuterScriptableObject(PropertyHandle.AsShared()) != nullptr;
}

void FScriptableTaskBindingExtension::ExtendWidgetRow(FDetailWidgetRow& InWidgetRow, const IDetailLayoutBuilder& InDetailBuilder, const UClass* InObjectClass, TSharedPtr<IPropertyHandle> InPropertyHandle)
{
	// Not used for customizations that call ProcessPropertyRow manually
}

void FScriptableTaskBindingExtension::ProcessPropertyRow(IDetailPropertyRow& Row, const IDetailLayoutBuilder& DetailBuilder, UObject* ObjectToModify, TSharedPtr<IPropertyHandle> PropertyHandle)
{
	// Only add extension if we successfully generated the binding widget
	TSharedPtr<SWidget> BindingWidget = GenerateBindingWidget(DetailBuilder, ObjectToModify, PropertyHandle);

	if (BindingWidget.IsValid() && BindingWidget != SNullWidget::NullWidget)
	{
		Row.CustomWidget()
			.NameContent()
			[
				PropertyHandle->CreatePropertyNameWidget()
			]
			.ValueContent()
			[
				PropertyHandle->CreatePropertyValueWidget()
			]
			.ExtensionContent()
			[
				SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(2.0f, 0.0f)
					[
						BindingWidget.ToSharedRef()
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						PropertyHandle->CreateDefaultPropertyButtonWidgets()
					]
			];
	}
}

TSharedRef<SWidget> FScriptableTaskBindingExtension::GenerateBindingWidget(const IDetailLayoutBuilder& DetailBuilder, UObject* ObjectToModify, TSharedPtr<IPropertyHandle> PropertyHandle)
{
	if (!IModularFeatures::Get().IsModularFeatureAvailable("PropertyAccessEditor"))
	{
		return SNullWidget::NullWidget;
	}

	// 1. Resolve Owner and Bindings
	UScriptableObject* ScriptableObject = ScriptableObject::PropertyBinding::GetOuterScriptableObject(PropertyHandle);
	IScriptablePropertyBindingsOwner* BindingOwner = Cast<IScriptablePropertyBindingsOwner>(ScriptableObject);

	if (!ScriptableObject || !BindingOwner)
	{
		return SNullWidget::NullWidget;
	}

	// 2. Prepare Contexts (Sources)
	TArray<FBindableStructDesc> AccessibleStructs;
	BindingOwner->GetAccessibleStructs(ScriptableObject, AccessibleStructs);

	TArray<FBindingContextStruct> Contexts;
	for (const FBindableStructDesc& Desc : AccessibleStructs)
	{
		if (Desc.IsValid())
		{
			FBindingContextStruct& ContextStruct = Contexts.AddDefaulted_GetRef();
			ContextStruct.DisplayText = FText::FromName(Desc.Name);
			ContextStruct.Struct = const_cast<UStruct*>(Desc.Struct.Get());
		}
	}

	// 3. Prepare Target Path
	FPropertyBindingPath TargetPath;
	ScriptableObject::PropertyBinding::MakeStructPropertyPathFromPropertyHandle(PropertyHandle, TargetPath);

	// 4. Create Cache Data for UI
	TSharedPtr<ScriptableObject::PropertyBinding::FCachedBindingData> CachedData =
		MakeShared<ScriptableObject::PropertyBinding::FCachedBindingData>(ScriptableObject, TargetPath, PropertyHandle, AccessibleStructs);

	// 5. Setup Arguments
	FPropertyBindingWidgetArgs Args;
	Args.Property = PropertyHandle->GetProperty();

	// Ensure these flags are enabled, otherwise the binding menu might be empty
	Args.bAllowPropertyBindings = true;
	Args.bGeneratePureBindings = true;
	Args.bAllowStructMemberBindings = true;
	Args.bAllowUObjectFunctions = true;

	// Allow binding to classes/structs (fixes menu not opening/filtering everything out)
	Args.OnCanBindToClass = FOnCanBindToClass::CreateLambda([](UClass* InClass) { return true; });

	// Allow binding to the root of the context struct (index in Contexts array)
	// Only allow binding to the struct itself if the target property is also a struct of the same type.
	Args.OnCanBindToContextStructWithIndex = FOnCanBindToContextStructWithIndex::CreateLambda([PropertyHandle, AccessibleStructs](const UStruct* InStruct, int32 Index)
	{
		if (const FStructProperty* StructProp = CastField<FStructProperty>(PropertyHandle->GetProperty()))
		{
			return StructProp->Struct == InStruct;
		}
		return false;
	});

	// Filter compatible properties
	Args.OnCanBindPropertyWithBindingChain = FOnCanBindPropertyWithBindingChain::CreateLambda([PropertyHandle](FProperty* InProperty, TArrayView<const FBindingChainElement> BindingChain)
	{
		// Use ScriptableFramework's compatibility check
		return FScriptablePropertyBindings::ArePropertiesCompatible(InProperty, PropertyHandle->GetProperty());
	});

	// Filter visibility/traversal in menu.
	// We accept everything to ensure PropertyBag properties (which might be dynamic) are traversed.
	Args.OnCanAcceptPropertyOrChildrenWithBindingChain = FOnCanAcceptPropertyOrChildrenWithBindingChain::CreateLambda([](FProperty* InProperty, TArrayView<const FBindingChainElement>)
	{
		return true;
	});

	// Action: Add Binding
	Args.OnAddBinding = FOnAddBinding::CreateLambda([CachedData](FName InPropertyName, const TArray<FBindingChainElement>& InBindingChain)
	{
		if (CachedData) CachedData->AddBinding(InBindingChain);
	});

	// Action: Remove Binding
	Args.OnRemoveBinding = FOnRemoveBinding::CreateLambda([CachedData](FName)
	{
		if (CachedData) CachedData->RemoveBinding();
	});

	// Check: Can Remove
	Args.OnCanRemoveBinding = FOnCanRemoveBinding::CreateLambda([CachedData](FName)
	{
		return CachedData ? CachedData->CanRemoveBinding() : false;
	});

	// Visual Feedback
	Args.CurrentBindingText = MakeAttributeLambda([CachedData]() { return CachedData->GetText(); });
	Args.CurrentBindingToolTipText = MakeAttributeLambda([CachedData]() { return CachedData->GetTooltipText(); });
	Args.CurrentBindingColor = MakeAttributeLambda([CachedData]() { return CachedData->GetColor(); });
	Args.CurrentBindingImage = MakeAttributeLambda([CachedData]() { return CachedData->GetImage(); });

	// Styles
	Args.BindButtonStyle = &FAppStyle::Get().GetWidgetStyle<FButtonStyle>("HoverHintOnly");

	IPropertyAccessEditor& PropertyAccessEditor = IModularFeatures::Get().GetModularFeature<IPropertyAccessEditor>("PropertyAccessEditor");
	return PropertyAccessEditor.MakePropertyBindingWidget(Contexts, Args);
}

UE_ENABLE_OPTIMIZATION

#undef LOCTEXT_NAMESPACE