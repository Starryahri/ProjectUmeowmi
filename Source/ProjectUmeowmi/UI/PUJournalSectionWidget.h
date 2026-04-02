// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "PUJournalSectionWidget.generated.h"

/**
 * Base class for journal section pages (tab content).
 * Extends CommonActivatableWidget for activation when switching tabs.
 */
UCLASS(Abstract, Blueprintable)
class PROJECTUMEOWMI_API UPUJournalSectionWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UPUJournalSectionWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

	/** Override to refresh section content when the tab is activated (e.g. lazy load data) */
	UFUNCTION(BlueprintNativeEvent, Category = "Journal")
	void OnSectionActivated();
	virtual void OnSectionActivated_Implementation() {}

	/** Override to clean up when the tab is deactivated */
	UFUNCTION(BlueprintNativeEvent, Category = "Journal")
	void OnSectionDeactivated();
	virtual void OnSectionDeactivated_Implementation() {}
};
