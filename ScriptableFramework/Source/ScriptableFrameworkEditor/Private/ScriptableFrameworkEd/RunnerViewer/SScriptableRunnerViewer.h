// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"

class UScriptableGraphInstance;
class UScriptableNode;

/** Snapshot of a recent output-fire on a runner, kept in a short ring buffer. */
struct FScriptableRunnerFire
{
	TWeakObjectPtr<UScriptableNode> Node;
	FName OutputName;
	double Time = 0.0;
};

/** Per-runner debug data the viewer maintains: fire history + bookkeeping. */
struct FScriptableRunnerDebugRow
{
	TWeakObjectPtr<UScriptableGraphInstance> Instance;
	TArray<FScriptableRunnerFire> Fires;
	/** One handle per (live) node we subscribed to, so UntrackInstance can remove cleanly. */
	TArray<TPair<TWeakObjectPtr<UScriptableNode>, FDelegateHandle>> NodeFireHandles;
	/** Slate slots that the poll repopulates each tick so active-node + fires lists stay live. */
	TWeakPtr<class SVerticalBox> ActiveNodesBox;
	TWeakPtr<class SVerticalBox> FiresBox;
	/** Cached counts so the header text can update dynamically without rebuilding the whole row. */
	int32 ActiveCount = 0;

	/** Change-detection so the slot-rebuild only fires when the state actually moved (otherwise mouse hover flickers). */
	TArray<FGuid> LastPaintedActiveIds;
	int32 FiresVersion = 0;
	int32 LastPaintedFiresVersion = -1;
};

/**
 * Global panel that lists every live UScriptableGraphInstance across PIE worlds, with each row's
 * active nodes and the last N fires. Polls subsystems at 100ms via an active timer; subscribes
 * per-runner to OnPinFiredNative so the fire history stays accurate without scanning.
 */
class SScriptableRunnerViewer : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SScriptableRunnerViewer) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SScriptableRunnerViewer();

private:
	using FRowPtr = TSharedPtr<FScriptableRunnerDebugRow>;

	/** Walks every UScriptableGraphSubsystem currently alive (one per world) and folds their runners into Rows. */
	void RefreshFromSubsystems();

	/** Per-runner subscription so we don't have to scan firings; rolling buffer capped at MaxFireHistory. */
	void TrackInstance(FRowPtr Row);
	void UntrackInstance(FRowPtr Row);

	TSharedRef<ITableRow> GenerateRunnerRow(FRowPtr Row, const TSharedRef<STableViewBase>& Owner);
	void GetRowChildren(FRowPtr Row, TArray<FRowPtr>& OutChildren);

	/** Rebuilds the inner SVerticalBox slots of every row so active nodes and recent fires reflect the live state. Called from the poll. */
	void RefreshRowContents();

	void PopulateActiveNodes(FRowPtr Row);
	void PopulateFires(FRowPtr Row);

	EActiveTimerReturnType TickPoll(double InCurrentTime, float InDeltaTime);

	void OnCancelAllClicked();
	void OnCancelRunnerClicked(TWeakObjectPtr<UScriptableGraphInstance> WeakInstance);
	void OnJumpToNode(TWeakObjectPtr<UScriptableNode> WeakNode);

	TArray<FRowPtr> Rows;
	TSharedPtr<STreeView<FRowPtr>> Tree;

	static constexpr int32 MaxFireHistory = 20;
};
