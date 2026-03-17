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

	/**
	 * Horizontal alignment in viewport (0=left, 0.5=center, 1=right).
	 * Useful for tutorials - e.g. 0.5 for center, 0 for left-aligned near a UI element.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Popup|Layout", meta = (ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1"))
	float HorizontalAlignment = 0.5f;

	/**
	 * Vertical alignment in viewport (0=top, 0.5=center, 1=bottom).
	 * Useful for tutorials - e.g. 0.25 for upper area, 0.75 for lower area.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Popup|Layout", meta = (ClampMin = "0", ClampMax = "1", UIMin = "0", UIMax = "1"))
	float VerticalAlignment = 0.5f;

	/**
	 * Optional position offset from the aligned point (in pixels).
	 * Use for fine-tuning - e.g. nudge a centered popup up or down.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Popup|Layout")
	FVector2D PositionOffset = FVector2D::ZeroVector;

	/**
	 * Optional size override. If both X and Y are > 0, overrides the popup's desired size.
	 * Leave at (0,0) to use the Blueprint's default size.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Popup|Layout")
	FVector2D SizeOverride = FVector2D::ZeroVector;

	// Additional data for specific popup types (e.g., ingredient tags for unlock popups)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Popup")
	TArray<FGameplayTag> AdditionalData;
};

