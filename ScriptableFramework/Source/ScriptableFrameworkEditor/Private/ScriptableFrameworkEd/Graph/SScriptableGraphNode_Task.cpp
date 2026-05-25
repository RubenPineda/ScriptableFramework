// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/SScriptableGraphNode_Task.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode_Task.h"
#include "ScriptableFrameworkEditorStyle.h"
#include "ScriptableNodes/ScriptableNode_Task.h"
#include "ScriptableTasks/ScriptableTask.h"

#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "SScriptableGraphNode_Task"

void SScriptableGraphNode_Task::Construct(const FArguments& InArgs, UScriptableEdGraphNode_Task* InNode)
{
	EdTaskNode = InNode;
	GraphNode = InNode;
	SetCursor(EMouseCursor::CardinalCross);
	UpdateGraphNode();
}

TSharedRef<SWidget> SScriptableGraphNode_Task::MakeBadge(const FText& Label, const FLinearColor& TintColor) const
{
	return SNew(SBorder)
		.BorderImage(FScriptableFrameworkEditorStyle::Get().GetBrush("ScriptableFramework.Param.Background"))
		.BorderBackgroundColor(TintColor)
		.Padding(FMargin(5.f, 1.f))
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
				.Text(Label)
				.TextStyle(FScriptableFrameworkEditorStyle::Get(), "ScriptableFramework.Param.Label")
				.ColorAndOpacity(FLinearColor::White)
		];
}

TArray<FOverlayWidgetInfo> SScriptableGraphNode_Task::GetOverlayWidgets(bool bSelected, const FVector2f& WidgetSize) const
{
	TArray<FOverlayWidgetInfo> Widgets = SGraphNode::GetOverlayWidgets(bSelected, WidgetSize);

	UScriptableEdGraphNode_Task* EdNode = EdTaskNode.Get();
	if (!EdNode) return Widgets;

	const UScriptableNode_Task* Wrapper = Cast<UScriptableNode_Task>(EdNode->GetRuntimeNode());
	if (!Wrapper || !Wrapper->Task) return Widgets;

	const FScriptableTaskControl& Control = Wrapper->Task->GetControl();

	// DoOnce badge: top-left corner.
	if (Control.bDoOnce)
	{
		FOverlayWidgetInfo Info;
		Info.OverlayOffset = FVector2f(-8.f, -8.f);
		Info.Widget = MakeBadge(INVTEXT("Once"), FLinearColor(0.85f, 0.65f, 0.1f));
		Widgets.Add(Info);
	}

	// Loop badge: top-right corner. LoopCount = 0 means "infinite" in the model; surface that
	// with the infinity glyph rather than the literal zero, which would otherwise read as
	// "loops zero times".
	if (Control.bLoop)
	{
		const FText LoopLabel = (Control.LoopCount <= 0)
			? INVTEXT("\u00D7\u221E")
			: FText::Format(INVTEXT("\u00D7{0}"), FText::AsNumber(Control.LoopCount));

		FOverlayWidgetInfo Info;
		Info.OverlayOffset = FVector2f(WidgetSize.X - 24.f, -8.f);
		Info.Widget = MakeBadge(LoopLabel, FLinearColor(0.1f, 0.4f, 0.8f));
		Widgets.Add(Info);
	}

	return Widgets;
}

#undef LOCTEXT_NAMESPACE