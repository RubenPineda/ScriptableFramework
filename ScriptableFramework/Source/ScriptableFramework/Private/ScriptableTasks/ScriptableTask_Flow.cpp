// Copyright 2026 kirzo

#include "ScriptableTasks/ScriptableTask_Flow.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UScriptableTask_Wait::BeginTask()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		// Fallback: If no world (e.g. asset editor preview), finish immediately to avoid getting stuck.
		Finish();
		return;
	}

	float FinalDuration = Duration;

	if (FinalDuration > UE_KINDA_SMALL_NUMBER && RandomDeviation > UE_KINDA_SMALL_NUMBER)
	{
		FinalDuration += FMath::RandRange(-RandomDeviation, RandomDeviation);
	}

	// Clamp to ensure we don't wait for negative time
	FinalDuration = FMath::Max(0.0f, FinalDuration);

	if (FinalDuration <= UE_KINDA_SMALL_NUMBER)
	{
		World->GetTimerManager().SetTimerForNextTick(this, &UScriptableTask_Wait::OnWaitFinished);
	}
	else
	{
		World->GetTimerManager().SetTimer(TimerHandle, this, &UScriptableTask_Wait::OnWaitFinished, FinalDuration, false);
	}
}

void UScriptableTask_Wait::OnWaitFinished()
{
	Finish();
}

void UScriptableTask_Wait::FinishTask()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimerHandle);
	}
}

void UScriptableTask_Wait::StopTask()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimerHandle);
	}
}

#if WITH_EDITOR
FText UScriptableTask_Wait::GetDisplayTitle() const
{
	FNumberFormattingOptions NumberOptions;
	NumberOptions.MaximumFractionalDigits = 2;

	FText DurationText;
	FString BindingName;
	if (GetBindingDisplayText(GET_MEMBER_NAME_CHECKED(UScriptableTask_Wait, Duration), BindingName))
	{
		DurationText = FText::Format(INVTEXT("\"{0}\""), FText::FromString(BindingName));
	}
	else
	{
		DurationText = FText::AsNumber(Duration, &NumberOptions);
	}

	if (BindingName.IsEmpty() && Duration <= UE_KINDA_SMALL_NUMBER)
	{
		return FText::Format(INVTEXT("Wait until next tick"), DurationText, FText::AsNumber(RandomDeviation));
	}

	if (RandomDeviation > 0.0)
	{
		return FText::Format(INVTEXT("Wait {0}s (+/- {1})"), DurationText, FText::AsNumber(RandomDeviation));
	}

	return FText::Format(INVTEXT("Wait {0} s"), DurationText);
}
#endif

#undef LOCTEXT_NAMESPACE