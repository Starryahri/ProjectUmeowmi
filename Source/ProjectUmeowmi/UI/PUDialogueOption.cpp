// Copyright 2025 Century Egg Studios, All rights reserved

#include "PUDialogueOption.h"
#include "DlgSystem/DlgContext.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "PUDialogueBox.h"

void UPUDialogueOption::NativeConstruct()
{
	Super::NativeConstruct();

	// Set up button click handler
	if (OptionButton)
	{
		OptionButton->OnClicked.AddDynamic(this, &UPUDialogueOption::OnOptionClicked);
		//UE_LOG(LogTemp,Log, TEXT("PUDialogueOption::NativeConstruct - Button click handler set up"));
	}
	else
	{
		//UE_LOG(LogTemp,Warning, TEXT("PUDialogueOption::NativeConstruct - No OptionButton found!"));
	}
}

void UPUDialogueOption::SetParentDialogueBox(UPUDialogueBox* DialogueBox)
{
	ParentDialogueBox = DialogueBox;
	//UE_LOG(LogTemp,Log, TEXT("PUDialogueOption::SetParentDialogueBox - Set parent dialogue box to %p"), DialogueBox);
}

void UPUDialogueOption::Update_Implementation(UDlgContext* ActiveContext)
{
	CurrentContext = ActiveContext;

	if (IsValid(ActiveContext) && ActiveContext->IsValidOptionIndex(OptionIndex))
	{
		SetVisibility(ESlateVisibility::Visible);
		if (OptionText)
		{
			OptionText->SetText(ActiveContext->GetOptionText(OptionIndex));
		}
		//UE_LOG(LogTemp,Log, TEXT("PUDialogueOption::Update - Updated option %d with text: %s"), 
		//	OptionIndex, *ActiveContext->GetOptionText(OptionIndex).ToString());
	}
	else
	{
		SetVisibility(ESlateVisibility::Hidden);
		//UE_LOG(LogTemp,Warning, TEXT("PUDialogueOption::Update - Invalid context or option index %d"), OptionIndex);
	}
}

void UPUDialogueOption::SelectOption()
{
	//UE_LOG(LogTemp,Log, TEXT("PUDialogueOption::SelectOption - Attempting to select option %d"), OptionIndex);

	if (!IsValid(CurrentContext))
	{
		//UE_LOG(LogTemp,Warning, TEXT("PUDialogueOption::SelectOption - Invalid dialogue context"));
		return;
	}

	if (!CurrentContext->IsValidOptionIndex(OptionIndex))
	{
		//UE_LOG(LogTemp,Warning, TEXT("PUDialogueOption::SelectOption - Invalid option index %d"), OptionIndex);
		return;
	}

	// Select the option and move to the next node
	bool bSuccess = CurrentContext->ChooseOption(OptionIndex);
	//UE_LOG(LogTemp,Log, TEXT("PUDialogueOption::SelectOption - ChooseOption returned %s"), bSuccess ? TEXT("true") : TEXT("false"));

	// Update the parent dialogue box if we have one
	if (IsValid(ParentDialogueBox))
	{
		//UE_LOG(LogTemp,Log, TEXT("PUDialogueOption::SelectOption - Updating parent dialogue box"));
		ParentDialogueBox->Update(CurrentContext);
	}
	else
	{
		//UE_LOG(LogTemp,Warning, TEXT("PUDialogueOption::SelectOption - No parent dialogue box set"));
	}
}

void UPUDialogueOption::OnOptionClicked()
{
	//UE_LOG(LogTemp,Log, TEXT("PUDialogueOption::OnOptionClicked - Button clicked for option %d"), OptionIndex);
	SelectOption();
}