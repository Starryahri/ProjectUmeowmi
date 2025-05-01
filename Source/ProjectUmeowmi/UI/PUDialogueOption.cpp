// Copyright 2025 Century Egg Studios, All rights reserved

#include "PUDialogueOption.h"
#include "DlgSystem/DlgContext.h"
#include "Components/TextBlock.h"

void UPUDialogueOption::Update_Implementation(UDlgContext* ActiveContext)
{
	if (ActiveContext->IsValidOptionIndex(OptionIndex))
	{
		SetVisibility(ESlateVisibility::Visible);
		OptionText->SetText(ActiveContext->GetOptionText(OptionIndex));
	}
	else
	{
		SetVisibility(ESlateVisibility::Hidden);
	}
}