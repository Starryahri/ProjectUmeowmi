#pragma once

#include "CoreMinimal.h"
#include "PUCommonUserWidget.h"
#include "PUPopupData.h"
#include "PUPopupWidget.generated.h"

class UTextBlock;
class UImage;
class UButton;
class UHorizontalBox;
class UVerticalBox;
class UBorder;
class UUserWidget;

/**
 * Generic popup widget for notifications, confirmations, tutorials, etc.
 * Inherits from PUCommonUserWidget for consistent styling.
 */
UCLASS(BlueprintType, Blueprintable)
class PROJECTUMEOWMI_API UPUPopupWidget : public UPUCommonUserWidget
{
	GENERATED_BODY()

public:
	UPUPopupWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/**
	 * Set the popup data and update the UI
	 * @param InPopupData - The popup configuration data
	 */
	UFUNCTION(BlueprintCallable, Category = "Popup")
	void SetPopupData(const FPopupData& InPopupData);

	/**
	 * Close the popup (called by buttons or auto-dismiss)
	 * @param ButtonID - The ID of the button that closed it (or NAME_None if closed via other means)
	 */
	UFUNCTION(BlueprintCallable, Category = "Popup")
	void Close(FName ButtonID = NAME_None);

	/**
	 * Get the current popup data
	 */
	UFUNCTION(BlueprintCallable, Category = "Popup")
	const FPopupData& GetPopupData() const { return CurrentPopupData; }

protected:
	// UI Elements (use BindWidget meta to auto-bind from Blueprint)
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "Popup|UI")
	UTextBlock* TitleText;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "Popup|UI")
	UTextBlock* MessageText;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "Popup|UI")
	UImage* IconImage;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "Popup|UI")
	UHorizontalBox* ButtonsContainer;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "Popup|UI")
	UButton* CloseButton;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "Popup|UI")
	UBorder* PopupBorder;

	// Button widget class to spawn dynamically
	// Can be a UButton or a custom User Widget (like WBP_PopupButton)
	// If using a custom widget, it should have a function to set button text/data
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Popup|Settings")
	TSubclassOf<UUserWidget> ButtonWidgetClass;

	// Optional: Name of the TextBlock in the button widget that displays the label.
	// If your button shows "OK" instead of the ButtonLabel, set this to your TextBlock's name (e.g. "ButtonText", "LabelText").
	// Leave empty to auto-detect (searches Button content first, then any TextBlock).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Popup|Settings", meta = (DisplayName = "Button Label TextBlock Name"))
	FName ButtonLabelWidgetName;

	// Current popup data
	UPROPERTY(BlueprintReadOnly, Category = "Popup")
	FPopupData CurrentPopupData;

	// Auto-dismiss timer
	FTimerHandle AutoDismissTimer;

	// Helper function for auto-dismiss timer (no parameters)
	void OnAutoDismissTimer();

	// Track spawned button widgets for cleanup
	UPROPERTY()
	TArray<UButton*> SpawnedButtons;

	// Track spawned custom button widgets for cleanup
	UPROPERTY()
	TArray<UUserWidget*> SpawnedButtonWidgets;

	// Internal functions
	void CreateButtons();
	void ClearButtons();
	void StartAutoDismissTimer();
	void StopAutoDismissTimer();
	void OnButtonClicked(FName ButtonID);
	void UpdatePopupStyle();

	// Button click handler (called by button delegates)
	// Note: Since we can't easily determine which button called this with dynamic delegates,
	// buttons should call HandleButtonClickWithID instead
	UFUNCTION()
	void HandleButtonClick();

	// Blueprint-callable function for buttons to call with themselves
	// This allows buttons to pass their own reference so we can look up the button ID
	UFUNCTION(BlueprintCallable, Category = "Popup")
	void HandleButtonClickWithID(UButton* ClickedButton);

	// Blueprint-callable function for custom button widgets to call with themselves
	// Pass the button ID directly (custom widgets should store their ButtonID)
	UFUNCTION(BlueprintCallable, Category = "Popup")
	void HandleButtonClickWithIDDirect(FName ButtonID);

	// Map to track which button corresponds to which ID
	UPROPERTY()
	TMap<UButton*, FName> ButtonIDMap;

	// Map to track custom button widgets and their IDs
	UPROPERTY()
	TMap<UUserWidget*, FName> ButtonWidgetIDMap;

	// Helper to get button ID from button reference (for Blueprint use)
	UFUNCTION(BlueprintCallable, Category = "Popup")
	FName GetButtonID(UButton* Button) const;
};

