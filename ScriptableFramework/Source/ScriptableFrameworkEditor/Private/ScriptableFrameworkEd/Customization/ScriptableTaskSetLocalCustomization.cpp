// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Customization/ScriptableTaskSetLocalCustomization.h"
#include "ScriptableFrameworkEd/Customization/Widgets/SScriptableNamePicker.h"
#include "ScriptableFrameworkEditorHelpers.h"
#include "ScriptableTasks/ScriptableTask_SetLocal.h"
#include "Core/KzNamedVariant.h"

#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailPropertyRow.h"
#include "PropertyHandle.h"

void FScriptableTaskSetLocalCustomization::ProcessPropertyHandle(TSharedRef<IPropertyHandle> SubPropertyHandle, IDetailChildrenBuilder& ChildBuilder, UScriptableObject* Obj, const TArray<FPropertyBindingBindableStructDescriptor>& AccessibleStructs)
{
	const FProperty* Prop = SubPropertyHandle->GetProperty();
	TWeakObjectPtr<UScriptableTask_SetLocal> Task = Cast<UScriptableTask_SetLocal>(Obj);

	if (Task.IsValid() && Prop && Prop->GetFName() == GET_MEMBER_NAME_CHECKED(UScriptableTask_SetLocal, VarName))
	{
		if (!ScriptableFrameworkEditor::IsPropertyVisible(SubPropertyHandle)) return;

		// VarName is NoBinding: swap the value widget only, without the binding decoration BindPropertyRow adds.
		IDetailPropertyRow& Row = ChildBuilder.AddProperty(SubPropertyHandle);
		TSharedPtr<SWidget> NameWidget, ValueWidget;
		Row.GetDefaultWidgets(NameWidget, ValueWidget);

		Row.CustomWidget()
			.NameContent()
			[
				NameWidget.IsValid() ? NameWidget.ToSharedRef() : SubPropertyHandle->CreatePropertyNameWidget()
			]
			.ValueContent()
			[
				SNew(SScriptableNamePicker)
					.SectionTitle(INVTEXT("Locals"))
					.CurrentName_Lambda([Task]() { return Task.IsValid() ? Task->VarName : NAME_None; })
					.OnGetItems_Lambda([Task]()
					{
						return ScriptableNamePickerItems::FromLocals(Task.Get(), [](const FKzNamedVariant&) { return true; });
					})
					.OnNamePicked_Lambda([Handle = TSharedPtr<IPropertyHandle>(SubPropertyHandle)](FName Picked) { Handle->SetValue(Picked); })
			];
		return;
	}

	FScriptableTaskCustomization::ProcessPropertyHandle(SubPropertyHandle, ChildBuilder, Obj, AccessibleStructs);
}