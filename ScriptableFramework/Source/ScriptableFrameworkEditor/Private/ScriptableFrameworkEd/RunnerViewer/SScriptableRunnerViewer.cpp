// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/RunnerViewer/SScriptableRunnerViewer.h"

#include "ScriptableNodes/ScriptableGraph.h"
#include "ScriptableNodes/ScriptableGraphInstance.h"
#include "ScriptableNodes/ScriptableGraphSubsystem.h"
#include "ScriptableNodes/ScriptableNode.h"
#include "ScriptableFrameworkEd/Graph/Editor/ScriptableGraphEditor.h"
#include "ScriptableFrameworkEd/Debug/ScriptableDebugRegistry.h"

#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "EdGraph/EdGraph.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode.h"

#include "Framework/Application/SlateApplication.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "ScriptableRunnerViewer"

void SScriptableRunnerViewer::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			.Padding(4.f)
			[
				SNew(SVerticalBox)

					/** Header: Cancel All + summary line. */
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)
					[
						SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 4, 0)
							[
								SNew(SButton)
									.ButtonStyle(FAppStyle::Get(), "Button")
									.OnClicked_Lambda([this]() { OnCancelAllClicked(); return FReply::Handled(); })
									.ToolTipText(LOCTEXT("CancelAllTip", "Cancel every active scriptable runner across all PIE worlds."))
									[
										SNew(STextBlock).Text(LOCTEXT("CancelAll", "Cancel All"))
									]
							]
							+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
							[
								SNew(STextBlock)
									.ColorAndOpacity(FSlateColor::UseSubduedForeground())
									.Text_Lambda([this]()
										{
											return Rows.Num() == 0
												? LOCTEXT("NoneRunning", "No active runners.")
												: FText::Format(LOCTEXT("RunnerCount", "{0} runner(s)"), FText::AsNumber(Rows.Num()));
										})
							]
					]

					/** Tree of runners. */
					+ SVerticalBox::Slot().FillHeight(1.f)
					[
						SAssignNew(Tree, STreeView<FRowPtr>)
							.TreeItemsSource(&Rows)
							.OnGenerateRow(this, &SScriptableRunnerViewer::GenerateRunnerRow)
							.OnGetChildren(this, &SScriptableRunnerViewer::GetRowChildren)
							.SelectionMode(ESelectionMode::Single)
					]
			]
	];

	RegisterActiveTimer(0.1f, FWidgetActiveTimerDelegate::CreateSP(this, &SScriptableRunnerViewer::TickPoll));
	RefreshFromSubsystems();
}

SScriptableRunnerViewer::~SScriptableRunnerViewer()
{
	for (FRowPtr& Row : Rows)
	{
		UntrackInstance(Row);
	}
}

EActiveTimerReturnType SScriptableRunnerViewer::TickPoll(double InCurrentTime, float InDeltaTime)
{
	RefreshFromSubsystems();
	RefreshRowContents();
	return EActiveTimerReturnType::Continue;
}

void SScriptableRunnerViewer::RefreshRowContents()
{
	for (FRowPtr& Row : Rows)
	{
		if (!Row.IsValid()) continue;
		PopulateActiveNodes(Row);
		PopulateFires(Row);
	}
}

void SScriptableRunnerViewer::RefreshFromSubsystems()
{
	/** Collect every active runner across every UWorld currently alive. */
	TArray<UScriptableGraphInstance*> Live;
	if (GEngine)
	{
		for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
		{
			UWorld* World = Ctx.World();
			if (!World) continue;
			if (UScriptableGraphSubsystem* Sub = World->GetSubsystem<UScriptableGraphSubsystem>())
			{
				for (UScriptableGraphInstance* Inst : Sub->GetActiveRunners())
				{
					if (Inst) Live.Add(Inst);
				}
			}
		}
	}

	/** Drop rows whose instance died. */
	for (int32 i = Rows.Num() - 1; i >= 0; --i)
	{
		const bool bStillAlive = Live.Contains(Rows[i]->Instance.Get());
		if (!bStillAlive)
		{
			UntrackInstance(Rows[i]);
			Rows.RemoveAt(i);
		}
	}

	/** Add rows for newly-seen runners + subscribe. */
	for (UScriptableGraphInstance* Inst : Live)
	{
		const bool bAlreadyTracked = Rows.ContainsByPredicate([Inst](const FRowPtr& Row) { return Row->Instance.Get() == Inst; });
		if (bAlreadyTracked) continue;

		FRowPtr NewRow = MakeShared<FScriptableRunnerDebugRow>();
		NewRow->Instance = Inst;
		TrackInstance(NewRow);
		Rows.Add(NewRow);
	}

	if (Tree.IsValid()) Tree->RequestTreeRefresh();
}

void SScriptableRunnerViewer::TrackInstance(FRowPtr Row)
{
	UScriptableGraphInstance* Inst = Row->Instance.Get();
	if (!Inst) return;

	/**
	 * Hook every node's OnPinFiredNative once. Lambda captures are weak so the binding stays safe even if
	 * the viewer or the row dies before the runner does. Handles stored on the row for clean removal.
	 */
	TWeakPtr<SScriptableRunnerViewer> WeakSelf = StaticCastSharedRef<SScriptableRunnerViewer>(AsShared());
	TWeakPtr<FScriptableRunnerDebugRow> WeakRow(Row);

	for (UScriptableNode* Node : Inst->GetNodes())
	{
		if (!Node) continue;
		const FDelegateHandle Handle = Node->OnPinFiredNative.AddLambda([WeakSelf, WeakRow](UScriptableNode* FiredNode, FName OutputName)
			{
				if (!WeakSelf.IsValid()) return;
				TSharedPtr<FScriptableRunnerDebugRow> RowPin = WeakRow.Pin();
				if (!RowPin.IsValid()) return;

				FScriptableRunnerFire Fire;
				Fire.Node = FiredNode;
				Fire.OutputName = OutputName;
				Fire.Time = FApp::GetCurrentTime();
				RowPin->Fires.Insert(Fire, 0);
				if (RowPin->Fires.Num() > MaxFireHistory) RowPin->Fires.SetNum(MaxFireHistory);
				++RowPin->FiresVersion;
			});
		Row->NodeFireHandles.Emplace(Node, Handle);
	}
}

void SScriptableRunnerViewer::UntrackInstance(FRowPtr Row)
{
	if (!Row.IsValid()) return;
	for (const TPair<TWeakObjectPtr<UScriptableNode>, FDelegateHandle>& Pair : Row->NodeFireHandles)
	{
		if (UScriptableNode* Node = Pair.Key.Get())
		{
			Node->OnPinFiredNative.Remove(Pair.Value);
		}
	}
	Row->NodeFireHandles.Reset();
}

void SScriptableRunnerViewer::GetRowChildren(FRowPtr Row, TArray<FRowPtr>& OutChildren)
{
	// Detail rendered inline; tree is flat.
}

namespace
{
	FString GetOwnerLabel(const UObject* Owner)
	{
		if (!Owner) return TEXT("<no owner>");
		return Owner->GetName();
	}

	FString GetAssetLabel(const UScriptableGraph* Asset)
	{
		if (!Asset) return TEXT("<unknown>");
		return Asset->GetName();
	}

	FString GetNodeLabel(const UScriptableNode* Node)
	{
		if (!Node) return TEXT("<gone>");
		return Node->GetTraceLabel();
	}
}

TSharedRef<ITableRow> SScriptableRunnerViewer::GenerateRunnerRow(FRowPtr Row, const TSharedRef<STableViewBase>& Owner)
{
	UScriptableGraphInstance* Inst = Row.IsValid() ? Row->Instance.Get() : nullptr;

	const FString AssetName = Inst ? GetAssetLabel(Inst->GetAsset()) : TEXT("?");
	const FString OwnerName = Inst ? GetOwnerLabel(Inst->GetOwner()) : TEXT("?");
	const FName RunnerId   = Inst ? Inst->GetId() : NAME_None;

	TSharedRef<SVerticalBox> ActiveBox = SNew(SVerticalBox);
	TSharedRef<SVerticalBox> FiresBox  = SNew(SVerticalBox);

	if (Row.IsValid())
	{
		Row->ActiveNodesBox = ActiveBox;
		Row->FiresBox = FiresBox;
		PopulateActiveNodes(Row);
		PopulateFires(Row);
	}

	TWeakObjectPtr<UScriptableGraphInstance> WeakInst(Inst);
	TWeakPtr<FScriptableRunnerDebugRow> WeakRow(Row);

	return SNew(STableRow<FRowPtr>, Owner)
		.Padding(FMargin(4, 6))
		[
			SNew(SVerticalBox)

				/** Header: asset name, owner, cancel button. */
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
						[
							SNew(STextBlock)
								.Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
								.Text(FText::FromString(RunnerId.IsNone()
									? FString::Printf(TEXT("%s  (owner: %s)"), *AssetName, *OwnerName)
									: FString::Printf(TEXT("%s  [%s]  (owner: %s)"), *AssetName, *RunnerId.ToString(), *OwnerName)))
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0, 0, 0).VAlign(VAlign_Center)
						[
							SNew(SButton)
								.ButtonStyle(FAppStyle::Get(), "Button")
								.OnClicked_Lambda([this, WeakInst]() { OnCancelRunnerClicked(WeakInst); return FReply::Handled(); })
								.ToolTipText(LOCTEXT("CancelRunnerTip", "Cancel this runner. Graph nodes run their Exit cleanup if any."))
								[
									SNew(STextBlock).Text(LOCTEXT("CancelRunner", "Cancel"))
								]
						]
				]

				/** Active nodes section — header text refreshes via attribute so the count stays live. */
				+ SVerticalBox::Slot().AutoHeight().Padding(8, 4, 0, 0)
				[
					SNew(STextBlock)
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						.Text_Lambda([WeakRow]()
							{
								TSharedPtr<FScriptableRunnerDebugRow> Pin = WeakRow.Pin();
								return FText::Format(LOCTEXT("ActiveNodesLabel", "Active nodes ({0}):"),
									FText::AsNumber(Pin.IsValid() ? Pin->ActiveCount : 0));
							})
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(8, 0, 0, 0)
				[
					ActiveBox
				]

				/** Recent fires section. */
				+ SVerticalBox::Slot().AutoHeight().Padding(8, 4, 0, 0)
				[
					SNew(STextBlock)
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						.Text(LOCTEXT("RecentFiresLabel", "Recent fires (newest first):"))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(8, 0, 0, 0)
				[
					FiresBox
				]
		];
}

void SScriptableRunnerViewer::PopulateActiveNodes(FRowPtr Row)
{
	if (!Row.IsValid()) return;
	TSharedPtr<SVerticalBox> Box = Row->ActiveNodesBox.Pin();
	if (!Box.IsValid()) return;

	UScriptableGraphInstance* Inst = Row->Instance.Get();

	/** Snapshot the current set as sorted FGuid list. Sort so the comparison is order-independent. */
	TArray<FGuid> CurrentIds;
	if (Inst)
	{
		for (UScriptableNode* Node : Inst->GetActiveNodes())
		{
			if (Node) CurrentIds.Add(Node->GetBindingID());
		}
		CurrentIds.Sort();
	}

	/** Skip rebuild when the set hasn't moved (most ticks). Keeps the hover/click state intact. */
	if (CurrentIds == Row->LastPaintedActiveIds) return;

	Row->LastPaintedActiveIds = MoveTemp(CurrentIds);
	Row->ActiveCount = Row->LastPaintedActiveIds.Num();

	Box->ClearChildren();
	if (!Inst) return;

	TWeakObjectPtr<UScriptableGraphInstance> WeakInst(Inst);
	for (UScriptableNode* Node : Inst->GetActiveNodes())
	{
		if (!Node) continue;
		TWeakObjectPtr<UScriptableNode> WeakNode(Node);
		Box->AddSlot().AutoHeight().Padding(0, 1)
		[
			SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
				.ContentPadding(FMargin(2, 0))
				.OnClicked_Lambda([this, WeakNode, WeakInst]() { OnJumpToNode(WeakNode, WeakInst); return FReply::Handled(); })
				[
					SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("  - %s"), *GetNodeLabel(Node))))
				]
		];
	}
}

void SScriptableRunnerViewer::PopulateFires(FRowPtr Row)
{
	if (!Row.IsValid()) return;
	TSharedPtr<SVerticalBox> Box = Row->FiresBox.Pin();
	if (!Box.IsValid()) return;

	/** Monotonic counter incremented inside the fire-tracking lambda; mismatch means new fires since last paint. */
	if (Row->FiresVersion == Row->LastPaintedFiresVersion) return;
	Row->LastPaintedFiresVersion = Row->FiresVersion;

	Box->ClearChildren();

	TWeakObjectPtr<UScriptableGraphInstance> WeakInst(Row->Instance);
	for (const FScriptableRunnerFire& Fire : Row->Fires)
	{
		TWeakObjectPtr<UScriptableNode> WeakFireNode = Fire.Node;
		const FName CapturedOutputName = Fire.OutputName;
		Box->AddSlot().AutoHeight().Padding(0, 1)
		[
			SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
				.ContentPadding(FMargin(2, 0))
				.OnClicked_Lambda([this, WeakFireNode, WeakInst]() { OnJumpToNode(WeakFireNode, WeakInst); return FReply::Handled(); })
				[
					SNew(STextBlock)
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						.Text(FText::FromString(FString::Printf(TEXT("  %s . %s"),
							*GetNodeLabel(WeakFireNode.Get()),
							*CapturedOutputName.ToString())))
				]
		];
	}
}

void SScriptableRunnerViewer::OnCancelAllClicked()
{
	if (!GEngine) return;
	for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
	{
		UWorld* World = Ctx.World();
		if (!World) continue;
		if (UScriptableGraphSubsystem* Sub = World->GetSubsystem<UScriptableGraphSubsystem>())
		{
			Sub->CancelAllRunners();
		}
	}
}

void SScriptableRunnerViewer::OnCancelRunnerClicked(TWeakObjectPtr<UScriptableGraphInstance> WeakInstance)
{
	UScriptableGraphInstance* Inst = WeakInstance.Get();
	if (!Inst) return;
	UWorld* World = Inst->GetWorld();
	if (!World) return;
	if (UScriptableGraphSubsystem* Sub = World->GetSubsystem<UScriptableGraphSubsystem>())
	{
		Sub->CancelRunner(Inst);
	}
}

void SScriptableRunnerViewer::OnJumpToNode(TWeakObjectPtr<UScriptableNode> WeakNode, TWeakObjectPtr<UScriptableGraphInstance> WeakInstance)
{
	UScriptableNode* Node = WeakNode.Get();
	if (!Node || !GEditor) return;
	const UScriptableGraph* Asset = Node->FindOwningAsset();
	if (!Asset || !Asset->EdGraph) return;

	/** Set the clicked runner as this asset's Debug Object so the canvas halos light up. */
	if (UScriptableGraphInstance* Inst = WeakInstance.Get())
	{
		FScriptableDebugRegistry::SetDebugInstance(Asset, Inst);
	}

	UEdGraphNode* MatchingEdNode = nullptr;
	const FGuid TargetId = Node->GetBindingID();
	for (UEdGraphNode* EdNode : Asset->EdGraph->Nodes)
	{
		const UScriptableEdGraphNode* SfEd = Cast<UScriptableEdGraphNode>(EdNode);
		if (SfEd && SfEd->GetRuntimeNode() && SfEd->GetRuntimeNode()->GetBindingID() == TargetId)
		{
			MatchingEdNode = EdNode;
			break;
		}
	}

	UAssetEditorSubsystem* Sub = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if (!Sub) return;
	UScriptableGraph* MutableAsset = const_cast<UScriptableGraph*>(Asset);
	Sub->OpenEditorForAsset(MutableAsset);
	if (IAssetEditorInstance* EditorInst = Sub->FindEditorForAsset(MutableAsset, /*bFocusIfOpen*/ true))
	{
		if (FScriptableGraphEditor* GraphEditor = static_cast<FScriptableGraphEditor*>(EditorInst))
		{
			GraphEditor->JumpToNode(MatchingEdNode);
		}
	}
}

#undef LOCTEXT_NAMESPACE
