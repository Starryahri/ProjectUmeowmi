// Copyright Epic Games, Inc. All Rights Reserved.

#include "PUSettingsSectionWidget.h"

UPUSettingsSectionWidget::UPUSettingsSectionWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SectionType = EJournalSectionType::Settings;
}
