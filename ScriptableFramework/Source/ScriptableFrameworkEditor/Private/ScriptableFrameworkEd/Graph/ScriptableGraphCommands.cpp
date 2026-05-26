// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/ScriptableGraphCommands.h"
#include "Framework/Commands/InputChord.h"

#define LOCTEXT_NAMESPACE "ScriptableGraphCommands"

void FScriptableGraphCommands::RegisterCommands()
{
	UI_COMMAND(
		RemoveSequencePin,
		"Remove pin",
		"Remove this branch from the Sequence node. Higher-indexed branches shift down to fill the gap and keep their downstream connections.",
		EUserInterfaceActionType::Button,
		FInputChord()
	);
}

#undef LOCTEXT_NAMESPACE