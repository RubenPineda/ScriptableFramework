// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Customization/ScriptableTaskCallFunctionCustomization.h"
#include "ScriptableFrameworkEd/Customization/Widgets/SScriptableNamePicker.h"
#include "ScriptableFrameworkEditorHelpers.h"
#include "ScriptableTasks/ScriptableTask_CallFunction.h"
#include "Core/KzNamedVariant.h"

#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailPropertyRow.h"
#include "PropertyHandle.h"
#include "PropertyCustomizationHelpers.h"
#include "ClassViewerFilter.h"

namespace
{
	// FunctionName and ResultLocal are NoBinding: swap the value widget only, without the binding
	// decoration BindPropertyRow adds.
	void AddPickerRow(IDetailChildrenBuilder& ChildBuilder, TSharedRef<IPropertyHandle> Handle, TSharedRef<SWidget> Picker)
	{
		IDetailPropertyRow& Row = ChildBuilder.AddProperty(Handle);
		TSharedPtr<SWidget> NameWidget, ValueWidget;
		Row.GetDefaultWidgets(NameWidget, ValueWidget);

		Row.CustomWidget()
			.NameContent()
			[
				NameWidget.IsValid() ? NameWidget.ToSharedRef() : Handle->CreatePropertyNameWidget()
			]
			.ValueContent()
			[
				Picker
			];
	}

	// Restricts the TargetClass picker to classes in the bound Target's hierarchy: an unrelated class
	// could never pass the runtime IsA guard, so the call would silently skip. The bound type is resolved
	// once (lazily) so it is not recomputed for every class in the viewer.
	class FTargetClassHierarchyFilter : public IClassViewerFilter
	{
	public:
		explicit FTargetClassHierarchyFilter(TWeakObjectPtr<UScriptableTask_CallFunction> InTask) : Task(InTask) {}

		virtual bool IsClassAllowed(const FClassViewerInitializationOptions&, const UClass* InClass, TSharedRef<FClassViewerFilterFuncs>) override
		{
			const UClass* Bound = GetBound();
			return !Bound || !InClass || InClass->IsChildOf(Bound) || Bound->IsChildOf(InClass);
		}

		virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions&, const TSharedRef<const IUnloadedBlueprintData> InClassData, TSharedRef<FClassViewerFilterFuncs>) override
		{
			const UClass* Bound = GetBound();
			return !Bound || InClassData->IsChildOf(Bound);
		}

	private:
		const UClass* GetBound()
		{
			if (!bResolved)
			{
				const UScriptableTask_CallFunction* TaskPtr = Task.Get();
				BoundClass = TaskPtr ? TaskPtr->GetBoundTargetClass() : nullptr;
				bResolved = true;
			}
			return BoundClass;
		}

		TWeakObjectPtr<UScriptableTask_CallFunction> Task;
		const UClass* BoundClass = nullptr;
		bool bResolved = false;
	};
}

void FScriptableTaskCallFunctionCustomization::ProcessPropertyHandle(TSharedRef<IPropertyHandle> SubPropertyHandle, IDetailChildrenBuilder& ChildBuilder, UScriptableObject* Obj, const TArray<FPropertyBindingBindableStructDescriptor>& AccessibleStructs)
{
	const FProperty* Prop = SubPropertyHandle->GetProperty();
	const FName PropName = Prop ? Prop->GetFName() : NAME_None;
	TWeakObjectPtr<UScriptableTask_CallFunction> Task = Cast<UScriptableTask_CallFunction>(Obj);

	if (Task.IsValid() && PropName == GET_MEMBER_NAME_CHECKED(UScriptableTask_CallFunction, TargetClass))
	{
		AddPickerRow(ChildBuilder, SubPropertyHandle,
			SNew(SClassPropertyEntryBox)
				.MetaClass(UObject::StaticClass())
				.AllowAbstract(true)
				.AllowNone(true)
				.SelectedClass_Lambda([Task]() { return Task.IsValid() ? Task->TargetClass.Get() : nullptr; })
				.OnSetClass_Lambda([Handle = TSharedPtr<IPropertyHandle>(SubPropertyHandle)](const UClass* NewClass)
				{
					Handle->SetValueFromFormattedString(NewClass ? NewClass->GetPathName() : TEXT("None"));
				})
				.ClassViewerFilters({ MakeShared<FTargetClassHierarchyFilter>(Task) }));
		return;
	}

	if (Task.IsValid() && PropName == GET_MEMBER_NAME_CHECKED(UScriptableTask_CallFunction, FunctionName))
	{
		if (!ScriptableFrameworkEditor::IsPropertyVisible(SubPropertyHandle)) return;

		AddPickerRow(ChildBuilder, SubPropertyHandle,
			SNew(SScriptableNamePicker)
				.SectionTitle(INVTEXT("Functions"))
				.CurrentName_Lambda([Task]() { return Task.IsValid() ? Task->FunctionName : NAME_None; })
				.OnGetItems_Lambda([Task]()
				{
					const UClass* Class = Task.IsValid() ? Task->GetEffectiveTargetClass() : nullptr;
					return ScriptableNamePickerItems::FromFunctions(Class, [](const UFunction* Func) { return UScriptableTask_CallFunction::IsFunctionExposable(Func); });
				})
				.OnNamePicked_Lambda([Handle = TSharedPtr<IPropertyHandle>(SubPropertyHandle)](FName Picked) { Handle->SetValue(Picked); }));
		return;
	}

	if (Task.IsValid() && PropName == GET_MEMBER_NAME_CHECKED(UScriptableTask_CallFunction, ResultLocal))
	{
		if (!ScriptableFrameworkEditor::IsPropertyVisible(SubPropertyHandle)) return;

		AddPickerRow(ChildBuilder, SubPropertyHandle,
			SNew(SScriptableNamePicker)
				.SectionTitle(INVTEXT("Locals"))
				.CurrentName_Lambda([Task]() { return Task.IsValid() ? Task->ResultLocal : NAME_None; })
				.OnGetItems_Lambda([Task]() -> TArray<FScriptableNameItem>
				{
					if (!Task.IsValid()) return {};
					const UFunction* Function = Task->ResolveFunction();
					return ScriptableNamePickerItems::FromLocals(Task.Get(), [Function](const FKzNamedVariant& Var)
					{
						return UScriptableTask_CallFunction::CanLocalReceiveReturnValue(Function, Var);
					});
				})
				.OnNamePicked_Lambda([Handle = TSharedPtr<IPropertyHandle>(SubPropertyHandle)](FName Picked) { Handle->SetValue(Picked); }));
		return;
	}

	FScriptableTaskCustomization::ProcessPropertyHandle(SubPropertyHandle, ChildBuilder, Obj, AccessibleStructs);
}