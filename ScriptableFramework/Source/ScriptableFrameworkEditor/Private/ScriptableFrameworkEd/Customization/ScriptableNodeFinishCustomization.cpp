// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Customization/ScriptableNodeFinishCustomization.h"

#include "ScriptableNodes/ScriptableNode_Finish.h"
#include "ScriptableNodes/ScriptableGraph.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "PropertyHandle.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ScriptableNodeFinishCustomization"

void FScriptableNodeFinishCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> Selected;
	DetailBuilder.GetObjectsBeingCustomized(Selected);
	if (Selected.IsEmpty()) return;

	WeakFinish = Cast<UScriptableNode_Finish>(Selected[0].Get());
	if (!WeakFinish.IsValid()) return;

	WeakGraph = WeakFinish->GetTypedOuter<UScriptableGraph>();

	OutputNameHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UScriptableNode_Finish, OutputName));
	if (!OutputNameHandle.IsValid()) return;

	/** Default FName row → hidden; we always replace it (or hide entirely if no Outputs to pick from). */
	DetailBuilder.HideProperty(OutputNameHandle);

	bool bHasAnyOutput = false;
	if (const UScriptableGraph* Graph = WeakGraph.Get())
	{
		for (const FName& Out : Graph->Outputs)
		{
			if (!Out.IsNone()) { bHasAnyOutput = true; break; }
		}
	}
	/** Hide entirely only when there's literally nothing to do: no Outputs declared AND no stale pick to clear. */
	if (!bHasAnyOutput && WeakFinish->OutputName.IsNone()) return;

	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(TEXT("Finish"));
	Category.AddCustomRow(LOCTEXT("OutputNameRow", "Output Name"))
		.NameContent()
		[
			OutputNameHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(180.f)
		[
			SNew(SComboButton)
				.OnGetMenuContent(this, &FScriptableNodeFinishCustomization::BuildOutputMenu)
				.ContentPadding(FMargin(4.f, 0.f))
				.ButtonContent()
				[
					SNew(STextBlock)
						.Font(IDetailLayoutBuilder::GetDetailFont())
						.Text(this, &FScriptableNodeFinishCustomization::GetSelectedOutputText)
				]
		];
}

FText FScriptableNodeFinishCustomization::GetSelectedOutputText() const
{
	const UScriptableNode_Finish* Finish = WeakFinish.Get();
	if (Finish && !Finish->OutputName.IsNone())
	{
		return FText::FromName(Finish->OutputName);
	}
	return LOCTEXT("SelectOutput", "Select output...");
}

TSharedRef<SWidget> FScriptableNodeFinishCustomization::BuildOutputMenu()
{
	FMenuBuilder MenuBuilder(/*bShouldCloseWindowAfterMenuSelection*/ true, nullptr);

	/** Clear-to-default entry: sets OutputName=None so the runtime falls back to Exit's "Finished". */
	MenuBuilder.AddMenuEntry(
		LOCTEXT("DefaultFinished", "Default (Finished)"),
		LOCTEXT("DefaultFinishedTip", "Clear the pick; the runtime falls back to Exit's 'Finished' output."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &FScriptableNodeFinishCustomization::OnOutputPicked, FName(NAME_None))));

	const UScriptableGraph* Graph = WeakGraph.Get();
	if (Graph)
	{
		const bool bHasUserOutputs = Graph->Outputs.ContainsByPredicate([](const FName& Out) { return !Out.IsNone(); });
		if (bHasUserOutputs) MenuBuilder.AddMenuSeparator();

		for (const FName& Output : Graph->Outputs)
		{
			if (Output.IsNone()) continue;
			MenuBuilder.AddMenuEntry(
				FText::FromName(Output),
				FText::GetEmpty(),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &FScriptableNodeFinishCustomization::OnOutputPicked, Output)));
		}
	}

	return MenuBuilder.MakeWidget();
}

void FScriptableNodeFinishCustomization::OnOutputPicked(FName OutputName)
{
	if (OutputNameHandle.IsValid())
	{
		/** SetValue wraps the change in a transaction and broadcasts PropertyChanged across multi-select. */
		OutputNameHandle->SetValue(OutputName);
	}
}

#undef LOCTEXT_NAMESPACE
