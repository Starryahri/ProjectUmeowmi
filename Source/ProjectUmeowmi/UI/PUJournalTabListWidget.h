// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonTabListWidgetBase.h"
#include "PUJournalTabListWidget.generated.h"

class UVerticalBox;

/**
 * Tab list widget for the journal - displays tabs vertically on the right edge of the book.
 * Overrides HandleTabCreation/HandleTabRemoval to add tab buttons to the container.
 * TabButtonsContainer lives in the parent Journal widget - call SetTabButtonsContainer from the Journal.
 */
UCLASS(Blueprintable)
class PROJECTUMEOWMI_API UPUJournalTabListWidget : public UCommonTabListWidgetBase
{
	GENERATED_BODY()

public:
	UPUJournalTabListWidget(const FObjectInitializer& ObjectInitializer);

	/** Set the container and owning journal. Call from Journal's NativeConstruct. */
	UFUNCTION(BlueprintCallable, Category = "Journal|Tabs")
	void SetTabButtonsContainer(UVerticalBox* InContainer);

	/** Set the owning journal - required so we read padding from the live instance (not cached). */
	UFUNCTION(BlueprintCallable, Category = "Journal|Tabs")
	void SetOwningJournal(class UPUJournalWidget* InJournal) { OwningJournal = InJournal; }

protected:
	virtual void HandleTabCreation_Implementation(FName TabNameID, UCommonButtonBase* TabButton) override;
	virtual void HandleTabRemoval_Implementation(FName TabNameID, UCommonButtonBase* TabButton) override;

	/** Container for tab buttons */
	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> TabButtonsContainer;

	/** Owning journal - we read padding from here at add-time so Blueprint edits apply immediately */
	UPROPERTY(Transient)
	TWeakObjectPtr<class UPUJournalWidget> OwningJournal;
};
