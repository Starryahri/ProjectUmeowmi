// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CommonButtonTypes.h"

/**
 * Common UI SCommonButton with no Slate focus rectangle (PLATFORM_UI_NEEDS_FOCUS_OUTLINES).
 * Journal slots use Common Button hover/pressed styling for gamepad focus instead.
 */
class PROJECTUMEOWMI_API SPUJournalSlotButton : public SCommonButton
{
public:
	SLATE_BEGIN_ARGS(SPUJournalSlotButton)
		: _Content()
		, _HAlign(HAlign_Fill)
		, _VAlign(VAlign_Fill)
		, _ClickMethod(EButtonClickMethod::DownAndUp)
		, _TouchMethod(EButtonTouchMethod::DownAndUp)
		, _PressMethod(EButtonPressMethod::DownAndUp)
		, _IsFocusable(true)
		, _IsInteractionEnabled(true)
	{}
	SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_STYLE_ARGUMENT(FButtonStyle, ButtonStyle)
		SLATE_ARGUMENT(EHorizontalAlignment, HAlign)
		SLATE_ARGUMENT(EVerticalAlignment, VAlign)
		SLATE_EVENT(FOnClicked, OnClicked)
		SLATE_EVENT(FOnClicked, OnDoubleClicked)
		SLATE_EVENT(FSimpleDelegate, OnPressed)
		SLATE_EVENT(FSimpleDelegate, OnReleased)
		SLATE_ARGUMENT(EButtonClickMethod::Type, ClickMethod)
		SLATE_ARGUMENT(EButtonTouchMethod::Type, TouchMethod)
		SLATE_ARGUMENT(EButtonPressMethod::Type, PressMethod)
		SLATE_ARGUMENT(bool, IsFocusable)
		SLATE_EVENT(FSimpleDelegate, OnReceivedFocus)
		SLATE_EVENT(FSimpleDelegate, OnLostFocus)
		SLATE_ARGUMENT(bool, IsButtonEnabled)
		SLATE_ARGUMENT(bool, IsInteractionEnabled)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		// FArguments layout matches SCommonButton::FArguments (same SLATE_BEGIN_ARGS); delegate to base implementation.
		SCommonButton::Construct(reinterpret_cast<const SCommonButton::FArguments&>(InArgs));
	}

	virtual const FSlateBrush* GetFocusBrush() const override { return nullptr; }

	/**
	 * SCommonButton only calls SetHover from OnMouseEnter. Gamepad focus does not move the cursor, so NormalHovered
	 * never activates unless we set Slate's hover attribute here (see SWidget::HoveredAttribute / IsHovered).
	 */
	void SetSimulatedHoverFromGame(bool bIn)
	{
		if (bIn)
		{
			SetHover(IsInteractable());
		}
		else if (!IsDirectlyHovered())
		{
			SetHover(false);
		}
	}
};
