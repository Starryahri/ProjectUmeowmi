// Copyright Epic Games, Inc. All Rights Reserved.

#include "PUJournalSlotWidget.h"
#include "PUJournalButtonInternal.h"
#include "CommonButtonBase.h"
#include "Blueprint/WidgetTree.h"

UCommonButtonInternalBase* UPUJournalSlotWidget::ConstructInternalButton()
{
	return WidgetTree->ConstructWidget<UPUJournalButtonInternal>(UPUJournalButtonInternal::StaticClass(), FName(TEXT("InternalRootButtonBase")));
}

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

void UPUJournalSlotWidget::ApplySlateHoverVisuals()
{
	NativeOnHovered();
	if (UPUJournalButtonInternal* Internal = Cast<UPUJournalButtonInternal>(GetRootWidget()))
	{
		Internal->SetSimulatedSlateHover(true);
	}
}

void UPUJournalSlotWidget::ApplySlateHoverForGamepadFocus()
{
	ApplySlateHoverVisuals();
}

FReply UPUJournalSlotWidget::NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent)
{
	FReply Reply = Super::NativeOnFocusReceived(InGeometry, InFocusEvent);
	ApplySlateHoverVisuals();
	return Reply;
}

void UPUJournalSlotWidget::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	if (UPUJournalButtonInternal* Internal = Cast<UPUJournalButtonInternal>(GetRootWidget()))
	{
		Internal->SetSimulatedSlateHover(false);
	}
	Super::NativeOnFocusLost(InFocusEvent);
	if (!IsHovered())
	{
		NativeOnUnhovered();
	}
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
