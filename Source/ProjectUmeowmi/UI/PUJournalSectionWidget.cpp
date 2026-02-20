// Copyright Epic Games, Inc. All Rights Reserved.

#include "PUJournalSectionWidget.h"

UPUJournalSectionWidget::UPUJournalSectionWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UPUJournalSectionWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	OnSectionActivated();
}

void UPUJournalSectionWidget::NativeOnDeactivated()
{
	OnSectionDeactivated();
	Super::NativeOnDeactivated();
}
