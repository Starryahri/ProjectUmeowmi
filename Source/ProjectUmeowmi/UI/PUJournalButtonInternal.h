// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CommonButtonBase.h"
#include "PUJournalButtonInternal.generated.h"

/**
 * Internal root button for PUJournalSlotWidget — same as UCommonButtonInternalBase but builds SPUJournalSlotButton
 * so Slate does not draw the default blue focus rectangle (see SPUJournalSlotButton::GetFocusBrush).
 */
UCLASS()
class PROJECTUMEOWMI_API UPUJournalButtonInternal : public UCommonButtonInternalBase
{
	GENERATED_BODY()

public:
	/** Match Slate hover visuals to gamepad/keyboard focus (see SPUJournalSlotButton::SetSimulatedHoverFromGame). */
	void SetSimulatedSlateHover(bool bHovered);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
};
