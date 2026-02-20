// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "PUJournalTypes.h"
#include "PUJournalSectionWidget.generated.h"

/**
 * Base class for all journal section content (Recipes, Ingredients, People, Town, Settings).
 * Extends CommonActivatableWidget for proper activation/deactivation when switching tabs.
 * Override NativeOnActivated to refresh data when the section becomes visible.
 */
UCLASS(Abstract, Blueprintable)
class PROJECTUMEOWMI_API UPUJournalSectionWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UPUJournalSectionWidget(const FObjectInitializer& ObjectInitializer);

	/** The section type this widget represents */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Journal")
	EJournalSectionType GetSectionType() const { return SectionType; }

	/** Set the section type (typically set in Blueprint subclass defaults) */
	UFUNCTION(BlueprintCallable, Category = "Journal")
	void SetSectionType(EJournalSectionType InSectionType) { SectionType = InSectionType; }

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

	/** The section type - set in Blueprint defaults for each subclass */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Journal")
	EJournalSectionType SectionType = EJournalSectionType::Recipes;
};
