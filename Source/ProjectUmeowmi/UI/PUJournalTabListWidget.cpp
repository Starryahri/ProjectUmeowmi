// Copyright Epic Games, Inc. All Rights Reserved.

#include "PUJournalTabListWidget.h"
#include "PUJournalWidget.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "CommonButtonBase.h"
#include "Layout/Margin.h"

UPUJournalTabListWidget::UPUJournalTabListWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UPUJournalTabListWidget::SetTabButtonsContainer(UVerticalBox* InContainer)
{
	TabButtonsContainer = InContainer;
}

void UPUJournalTabListWidget::HandleTabCreation_Implementation(FName TabNameID, UCommonButtonBase* TabButton)
{
	Super::HandleTabCreation_Implementation(TabNameID, TabButton);

	if (TabButtonsContainer && TabButton)
	{
		const bool bIsFirstTab = (TabButtonsContainer->GetChildrenCount() == 0);
		UVerticalBoxSlot* ButtonSlot = TabButtonsContainer->AddChildToVerticalBox(TabButton);
		if (ButtonSlot)
		{
			// First tab gets no padding; rest get padding from Journal
			FMargin SlotPadding(0.f, 0.f, 0.f, 0.f);
			if (!bIsFirstTab && OwningJournal.IsValid())
			{
				SlotPadding = OwningJournal->GetTabSlotPadding();
			}
			ButtonSlot->SetPadding(SlotPadding);
		}
	}
}

void UPUJournalTabListWidget::HandleTabRemoval_Implementation(FName TabNameID, UCommonButtonBase* TabButton)
{
	// Base class removes from parent before calling this - no additional cleanup needed
	Super::HandleTabRemoval_Implementation(TabNameID, TabButton);
}
