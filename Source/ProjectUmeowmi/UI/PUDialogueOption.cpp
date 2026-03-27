// Copyright 2025 Century Egg Studios, All rights reserved

#include "PUDialogueOption.h"
#include "DlgSystem/DlgContext.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "PUDialogueBox.h"
#include "../PUProjectUmeowmiGameInstance.h"
#include "ProjectUmeowmi/ProjectUmeowmiCharacter.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace PUDialogueScoringLog
{
	static constexpr const TCHAR* Tag = TEXT("[PUDialogueScoring]");
}

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
		// Hide the option text - button will be used as "Next" button instead
		if (OptionText)
		{
			OptionText->SetVisibility(ESlateVisibility::Collapsed);
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

	// If typewriter is active, handle skip-on-input: complete text but do NOT advance yet
	if (IsValid(ParentDialogueBox) && ParentDialogueBox->IsTypewriterActive())
	{
		UPUProjectUmeowmiGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance<UPUProjectUmeowmiGameInstance>() : nullptr;
		const bool bSkipOnInput = GI ? GI->GetDialogueTypewriterSkipOnInput() : true;

		if (bSkipOnInput)
		{
			ParentDialogueBox->CompleteTypewriter();
			// Don't advance - user must press again to continue to next line
			return;
		}
		else
		{
			// Skip-on-input disabled: ignore click while typing
			return;
		}
	}

	// Select the option and move to the next node
	const bool bEndedBefore = CurrentContext->HasDialogueEnded();
	UE_LOG(LogTemp, Display, TEXT("%s [Option/SelectOption] idx=%d ctx=%p HasEnded(before)=%d OptionsNum=%d parentBox=%p"),
		PUDialogueScoringLog::Tag, OptionIndex, CurrentContext, bEndedBefore ? 1 : 0, CurrentContext->GetOptionsNum(), ParentDialogueBox);
	bool bSuccess = CurrentContext->ChooseOption(OptionIndex);
	UE_LOG(LogTemp, Display, TEXT("%s [Option/SelectOption] ChooseOption(%d) => %s HasEnded(after)=%d"),
		PUDialogueScoringLog::Tag, OptionIndex, bSuccess ? TEXT("true") : TEXT("false"), CurrentContext->HasDialogueEnded() ? 1 : 0);

	// ChooseOption can run Dlg enter events (e.g. BeginDishScoring) that SwapToScoringDialogueBox on the character.
	// ParentDialogueBox still points at the old widget — Update must target the character's current DialogueBox.
	UPUDialogueBox* BoxToUpdate = ParentDialogueBox;
	APlayerController* PC = GetOwningPlayer();
	if (!PC && IsValid(ParentDialogueBox))
	{
		PC = ParentDialogueBox->GetOwningPlayer();
	}
	if (!PC)
	{
		if (UWorld* World = GetWorld())
		{
			PC = World->GetFirstPlayerController();
		}
	}
	if (PC)
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			if (AProjectUmeowmiCharacter* PlayerChar = Cast<AProjectUmeowmiCharacter>(Pawn))
			{
				if (UPUDialogueBox* CharBox = PlayerChar->GetDialogueBox())
				{
					BoxToUpdate = CharBox;
				}
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%s [Option/SelectOption] no PlayerController (this=%p parentBox=%p) — cannot resolve character DialogueBox"),
			PUDialogueScoringLog::Tag, this, ParentDialogueBox);
	}
	if (BoxToUpdate != ParentDialogueBox)
	{
		UE_LOG(LogTemp, Display, TEXT("%s [Option/SelectOption] post-ChooseOption: dialogue box replaced (parent=%p -> character=%p %s) — updating character box"),
			PUDialogueScoringLog::Tag, ParentDialogueBox, BoxToUpdate, BoxToUpdate ? *BoxToUpdate->GetClass()->GetName() : TEXT("null"));
		SetParentDialogueBox(BoxToUpdate);
	}
	if (IsValid(BoxToUpdate))
	{
		BoxToUpdate->Update(CurrentContext);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%s [Option/SelectOption] No dialogue box to Update after ChooseOption"), PUDialogueScoringLog::Tag);
	}
}

void UPUDialogueOption::OnOptionClicked()
{
	//UE_LOG(LogTemp,Log, TEXT("PUDialogueOption::OnOptionClicked - Button clicked for option %d"), OptionIndex);
	SelectOption();
}