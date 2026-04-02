// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/Image.h"
#include "GameplayTagContainer.h"
#include "PUCommonButton.h"
#include "PUJournalSlotWidget.generated.h"

class UCommonButtonInternalBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPUJournalSlotEntryEvent, FGameplayTag, EntryTag);

/**
 * Reusable journal grid cell: Common UI button + gameplay tag identity + optional icon.
 * Parent grid binds to OnEntryHovered to drive the detail panel — same event fires for hover, focus,
 * click, and Common UI selection (unlocked slots only).
 */
UCLASS(Blueprintable, ClassGroup = UI, meta = (Category = "Project Umeowmi"))
class PROJECTUMEOWMI_API UPUJournalSlotWidget : public UPUCommonButton
{
	GENERATED_BODY()

public:
	UPUJournalSlotWidget(const FObjectInitializer& ObjectInitializer);

	/** Identity for this cell (ingredient tag, dish tag, etc.). */
	UFUNCTION(BlueprintCallable, Category = "Journal|Slot")
	void SetEntryTag(const FGameplayTag& InTag) { EntryTag = InTag; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Journal|Slot")
	FGameplayTag GetEntryTag() const { return EntryTag; }

	/** Sets the slot icon from a texture (e.g. PantryTexture). Clears the image if null. */
	UFUNCTION(BlueprintCallable, Category = "Journal|Slot")
	void SetSlotIconTexture(UTexture2D* Texture);

	/**
	 * Configure grid cell: empty padding, locked (known ingredient not yet unlocked), or unlocked.
	 * Empty/locked slots are non-interactive (no hover/focus detail).
	 */
	UFUNCTION(BlueprintCallable, Category = "Journal|Slot")
	void ConfigureIngredientSlot(const FGameplayTag& InTag, UTexture2D* IconTexture, bool bEmptySlot, bool bLockedIngredient);

	/** After SetUserFocus (e.g. ingredients grid first slot), ensures Slate hover brush + Common UI hover match gamepad focus. */
	UFUNCTION(BlueprintCallable, Category = "Journal|Slot")
	void ApplySlateHoverForGamepadFocus();

	UFUNCTION(BlueprintPure, Category = "Journal|Slot")
	bool IsSlotEmpty() const { return bSlotIsEmpty; }

	UFUNCTION(BlueprintPure, Category = "Journal|Slot")
	bool IsLockedIngredientSlot() const { return bSlotIsLockedIngredient; }

protected:
	virtual UCommonButtonInternalBase* ConstructInternalButton() override;

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	virtual void NativeOnHovered() override;
	virtual void NativeOnUnhovered() override;
	virtual FReply NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnFocusLost(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnClicked() override;
	virtual void NativeOnSelected(bool bBroadcast) override;

	void BroadcastHovered();
	void BroadcastUnhovered();

	void ApplySlateHoverVisuals();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal|Slot")
	FGameplayTag EntryTag;

	/** True when this cell is past the end of the ingredient list (padding). */
	UPROPERTY(BlueprintReadOnly, Category = "Journal|Slot")
	bool bSlotIsEmpty = false;

	/** True when the ingredient exists but is not unlocked yet. */
	UPROPERTY(BlueprintReadOnly, Category = "Journal|Slot")
	bool bSlotIsLockedIngredient = false;

	/** Optional icon; name in Blueprint should match for BindWidget. */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Journal|Slot|UI")
	TObjectPtr<UImage> SlotIcon;

public:
	UPROPERTY(BlueprintAssignable, Category = "Journal|Slot")
	FOnPUJournalSlotEntryEvent OnEntryHovered;

	UPROPERTY(BlueprintAssignable, Category = "Journal|Slot")
	FOnPUJournalSlotEntryEvent OnEntryUnhovered;
};
