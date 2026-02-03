#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "PUPopupData.generated.h"

class UTexture2D;

/**
 * Popup type enum for different popup styles/behaviors
 */
UENUM(BlueprintType)
enum class EPopupType : uint8
{
	Notification	UMETA(DisplayName = "Notification"),		// Simple notification (ingredient unlocks, etc.)
	Tutorial		UMETA(DisplayName = "Tutorial"),			// Tutorial/help popup
	Confirmation	UMETA(DisplayName = "Confirmation"),		// Yes/No confirmation dialog
	Info			UMETA(DisplayName = "Info"),				// Informational popup
	Warning			UMETA(DisplayName = "Warning"),			// Warning message
	Error			UMETA(DisplayName = "Error")				// Error message
};

/**
 * Button data for popup buttons
 */
USTRUCT(BlueprintType)
struct FPopupButtonData
{
	GENERATED_BODY()

	FPopupButtonData()
		: ButtonID(NAME_None)
		, ButtonLabel(FText::GetEmpty())
		, bIsPrimary(false)
	{}

	// Unique identifier for this button (e.g., "YES", "NO", "OK", "CANCEL")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button")
	FName ButtonID;

	// Display text for the button
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button")
	FText ButtonLabel;

	// Whether this is a primary button (for styling purposes)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button")
	bool bIsPrimary;
};

/**
 * Popup data structure for displaying notifications, confirmations, tutorials, etc.
 */
USTRUCT(BlueprintType)
struct FPopupData
{
	GENERATED_BODY()

	FPopupData()
		: PopupType(EPopupType::Notification)
		, Title(FText::GetEmpty())
		, Message(FText::GetEmpty())
		, Icon(nullptr)
		, bModal(false)
		, bAutoDismiss(false)
		, AutoDismissTime(3.0f)
		, bShowCloseButton(true)
	{}

	// Type of popup (affects styling/behavior)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Popup")
	EPopupType PopupType;

	// Title/header text (optional)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Popup")
	FText Title;

	// Main message/body text
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Popup")
	FText Message;

	// Optional icon/image to display
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Popup")
	UTexture2D* Icon;

	// Buttons to display (empty = default "OK" button)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Popup")
	TArray<FPopupButtonData> Buttons;

	// Whether popup blocks input (modal) or allows gameplay to continue
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Popup")
	bool bModal;

	// Whether popup automatically closes after AutoDismissTime
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Popup")
	bool bAutoDismiss;

	// Time in seconds before auto-dismiss (if bAutoDismiss is true)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Popup", meta = (EditCondition = "bAutoDismiss"))
	float AutoDismissTime;

	// Whether to show the X close button in the corner
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Popup")
	bool bShowCloseButton;

	// Additional data for specific popup types (e.g., ingredient tags for unlock popups)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Popup")
	TArray<FGameplayTag> AdditionalData;
};

