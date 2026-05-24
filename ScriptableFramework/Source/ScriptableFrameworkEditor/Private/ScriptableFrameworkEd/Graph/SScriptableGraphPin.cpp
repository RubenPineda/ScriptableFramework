// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/SScriptableGraphPin.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Widgets/SNullWidget.h"

void SScriptableGraphPin::Construct(const FArguments& InArgs, UEdGraphPin* InPin)
{
	SGraphPinExec::Construct(SGraphPinExec::FArguments(), InPin);

	if (InPin)
	{
		// Defer to the ed-node: if it says the label should be hidden for this pin name, drop it.
		if (const UScriptableEdGraphNode* OwningNode = Cast<UScriptableEdGraphNode>(InPin->GetOwningNode()))
		{
			if (!OwningNode->ShouldShowPinLabel(InPin->PinName))
			{
				SetShowLabel(false);
			}
		}
	}
}

TSharedRef<SWidget> SScriptableGraphPin::GetDefaultValueWidget()
{
	return SNullWidget::NullWidget;
}

EVisibility SScriptableGraphPin::OnGetLabelVisibility() const
{
	// Kept for completeness, no longer used after the SetShowLabel-based approach.
	return EVisibility::Visible;
}