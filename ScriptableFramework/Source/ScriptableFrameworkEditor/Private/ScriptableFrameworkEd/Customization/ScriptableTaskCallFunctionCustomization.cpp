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
}

void FScriptableTaskCallFunctionCustomization::ProcessPropertyHandle(TSharedRef<IPropertyHandle> SubPropertyHandle, IDetailChildrenBuilder& ChildBuilder, UScriptableObject* Obj, const TArray<FPropertyBindingBindableStructDescriptor>& AccessibleStructs)
{
	const FProperty* Prop = SubPropertyHandle->GetProperty();
	const FName PropName = Prop ? Prop->GetFName() : NAME_None;
	TWeakObjectPtr<UScriptableTask_CallFunction> Task = Cast<UScriptableTask_CallFunction>(Obj);

	if (Task.IsValid() && PropName == GET_MEMBER_NAME_CHECKED(UScriptableTask_CallFunction, FunctionName))
	{
		if (!ScriptableFrameworkEditor::IsPropertyVisible(SubPropertyHandle)) return;

		AddPickerRow(ChildBuilder, SubPropertyHandle,
			SNew(SScriptableNamePicker)
				.SectionTitle(INVTEXT("Functions"))
				.CurrentName_Lambda([Task]() { return Task.IsValid() ? Task->FunctionName : NAME_None; })
				.OnGetItems_Lambda([Task]()
				{
					const UClass* Class = Task.IsValid() ? Task->TargetClass.Get() : nullptr;
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
					const UFunction* Function = Task->TargetClass ? Task->TargetClass->FindFunctionByName(Task->FunctionName) : nullptr;
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