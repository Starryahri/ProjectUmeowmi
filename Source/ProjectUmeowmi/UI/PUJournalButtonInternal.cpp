// Copyright Epic Games, Inc. All Rights Reserved.

#include "PUJournalButtonInternal.h"
#include "SPUJournalSlotButton.h"
#include "Components/ButtonSlot.h"
#include "Templates/SharedPointer.h"

void UPUJournalButtonInternal::SetSimulatedSlateHover(bool bHovered)
{
	if (!MyCommonButton.IsValid())
	{
		return;
	}
	if (TSharedPtr<SPUJournalSlotButton> SlateSlot = StaticCastSharedPtr<SPUJournalSlotButton>(MyCommonButton))
	{
		SlateSlot->SetSimulatedHoverFromGame(bHovered);
	}
}

TSharedRef<SWidget> UPUJournalButtonInternal::RebuildWidget()
{
	MyButton = MyCommonButton = SNew(SPUJournalSlotButton)
		.OnClicked(BIND_UOBJECT_DELEGATE(FOnClicked, SlateHandleClickedOverride))
		.OnDoubleClicked(BIND_UOBJECT_DELEGATE(FOnClicked, SlateHandleDoubleClicked))
		.OnPressed(BIND_UOBJECT_DELEGATE(FSimpleDelegate, SlateHandlePressedOverride))
		.OnReleased(BIND_UOBJECT_DELEGATE(FSimpleDelegate, SlateHandleReleasedOverride))
		.ButtonStyle(&GetStyle())
		.ClickMethod(GetClickMethod())
		.TouchMethod(GetTouchMethod())
		.IsFocusable(GetIsFocusable())
		.IsButtonEnabled(bButtonEnabled)
		.IsInteractionEnabled(bInteractionEnabled)
		.OnReceivedFocus(BIND_UOBJECT_DELEGATE(FSimpleDelegate, SlateHandleOnReceivedFocus))
		.OnLostFocus(BIND_UOBJECT_DELEGATE(FSimpleDelegate, SlateHandleOnLostFocus));

	MyBox = SNew(SBox)
		.MinDesiredWidth(MinWidth)
		.MinDesiredHeight(MinHeight)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			MyCommonButton.ToSharedRef()
		];

	if (GetChildrenCount() > 0)
	{
		Cast<UButtonSlot>(GetContentSlot())->BuildSlot(MyCommonButton.ToSharedRef());
	}

	return MyBox.ToSharedRef();
}
