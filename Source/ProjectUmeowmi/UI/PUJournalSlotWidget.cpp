// Copyright Epic Games, Inc. All Rights Reserved.

#include "PUJournalSlotWidget.h"

UPUJournalSlotWidget::UPUJournalSlotWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UPUJournalSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UPUJournalSlotWidget::NativeDestruct()
{
	Super::NativeDestruct();
}

void UPUJournalSlotWidget::SetSlotIconTexture(UTexture2D* Texture)
{
	if (!SlotIcon)
	{
		return;
	}
	if (Texture)
	{
		SlotIcon->SetBrushFromTexture(Texture);
		SlotIcon->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		SlotIcon->SetBrush(FSlateBrush());
		SlotIcon->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UPUJournalSlotWidget::ConfigureIngredientSlot(const FGameplayTag& InTag, UTexture2D* IconTexture, bool bEmptySlot, bool bLockedIngredient)
{
	bSlotIsEmpty = bEmptySlot;
	bSlotIsLockedIngredient = !bEmptySlot && bLockedIngredient;

	if (bEmptySlot)
	{
		SetEntryTag(FGameplayTag());
		SetSlotIconTexture(nullptr);
	}
	else
	{
		SetEntryTag(InTag);
		if (bSlotIsLockedIngredient)
		{
			SetSlotIconTexture(nullptr);
		}
		else
		{
			SetSlotIconTexture(IconTexture);
		}
	}

	SetIsEnabled(!bEmptySlot && !bSlotIsLockedIngredient);
}

void UPUJournalSlotWidget::NativeOnHovered()
{
	Super::NativeOnHovered();
	BroadcastHovered();
}

void UPUJournalSlotWidget::NativeOnUnhovered()
{
	Super::NativeOnUnhovered();
	BroadcastUnhovered();
}

FReply UPUJournalSlotWidget::NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent)
{
	FReply Reply = Super::NativeOnFocusReceived(InGeometry, InFocusEvent);
	BroadcastHovered();
	return Reply;
}

void UPUJournalSlotWidget::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnFocusLost(InFocusEvent);
	BroadcastUnhovered();
}

void UPUJournalSlotWidget::NativeOnClicked()
{
	Super::NativeOnClicked();
	BroadcastHovered();
}

void UPUJournalSlotWidget::NativeOnSelected(bool bBroadcast)
{
	Super::NativeOnSelected(bBroadcast);
	BroadcastHovered();
}

void UPUJournalSlotWidget::BroadcastHovered()
{
	if (bSlotIsEmpty || bSlotIsLockedIngredient)
	{
		return;
	}
	OnEntryHovered.Broadcast(EntryTag);
}

void UPUJournalSlotWidget::BroadcastUnhovered()
{
	OnEntryUnhovered.Broadcast(EntryTag);
}
