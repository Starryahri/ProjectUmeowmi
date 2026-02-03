#include "PUPopupWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Components/Border.h"
#include "../PUProjectUmeowmiGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Blueprint/WidgetTree.h"
#include "UObject/StructOnScope.h"

UPUPopupWidget::UPUPopupWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Default to UButton, but can be overridden in Blueprint
	ButtonWidgetClass = nullptr; // Will be set in Blueprint
}

void UPUPopupWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Bind close button if it exists
	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(this, &UPUPopupWidget::HandleButtonClick);
	}
}

void UPUPopupWidget::NativeDestruct()
{
	// Stop auto-dismiss timer
	StopAutoDismissTimer();

	// Clear buttons
	ClearButtons();

	Super::NativeDestruct();
}

void UPUPopupWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
}

void UPUPopupWidget::SetPopupData(const FPopupData& InPopupData)
{
	CurrentPopupData = InPopupData;

	// Update title
	if (TitleText)
	{
		if (InPopupData.Title.IsEmpty())
		{
			TitleText->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			TitleText->SetText(InPopupData.Title);
			TitleText->SetVisibility(ESlateVisibility::Visible);
		}
	}

	// Update message
	if (MessageText)
	{
		MessageText->SetText(InPopupData.Message);
	}

	// Update icon
	if (IconImage)
	{
		if (InPopupData.Icon)
		{
			IconImage->SetBrushFromTexture(InPopupData.Icon);
			IconImage->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			IconImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// Update close button visibility
	if (CloseButton)
	{
		CloseButton->SetVisibility(InPopupData.bShowCloseButton ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	// Create buttons
	CreateButtons();

	// Start auto-dismiss timer if needed
	if (InPopupData.bAutoDismiss)
	{
		StartAutoDismissTimer();
	}
	else
	{
		StopAutoDismissTimer();
	}

	// Update popup style based on type
	UpdatePopupStyle();

	UE_LOG(LogTemp, Log, TEXT("UPUPopupWidget::SetPopupData - Popup data set: %s"), *InPopupData.Title.ToString());
}

void UPUPopupWidget::Close(FName ButtonID)
{
	// Stop auto-dismiss timer
	StopAutoDismissTimer();

	// Notify GameInstance
	if (UWorld* World = GetWorld())
	{
		if (UPUProjectUmeowmiGameInstance* GameInstance = Cast<UPUProjectUmeowmiGameInstance>(World->GetGameInstance()))
		{
			GameInstance->NotifyPopupClosed(ButtonID);
		}
	}

	// Remove from viewport
	RemoveFromParent();

	UE_LOG(LogTemp, Log, TEXT("UPUPopupWidget::Close - Popup closed with button ID: %s"), *ButtonID.ToString());
}

void UPUPopupWidget::CreateButtons()
{
	ClearButtons();

	if (!ButtonsContainer)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPUPopupWidget::CreateButtons - ButtonsContainer is not set!"));
		return;
	}

	// If no buttons specified, create a default OK button
	if (CurrentPopupData.Buttons.Num() == 0)
	{
		FPopupButtonData DefaultButton;
		DefaultButton.ButtonID = FName(TEXT("OK"));
		DefaultButton.ButtonLabel = FText::FromString(TEXT("OK"));
		DefaultButton.bIsPrimary = true;
		CurrentPopupData.Buttons.Add(DefaultButton);
	}

	// Create buttons from data
	for (const FPopupButtonData& ButtonData : CurrentPopupData.Buttons)
	{
		if (!ButtonWidgetClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("UPUPopupWidget::CreateButtons - ButtonWidgetClass is not set!"));
			continue;
		}

		// Create the button widget
		UUserWidget* NewButtonWidget = CreateWidget<UUserWidget>(GetWorld(), ButtonWidgetClass);
		if (!NewButtonWidget)
		{
			UE_LOG(LogTemp, Warning, TEXT("UPUPopupWidget::CreateButtons - Failed to create button widget"));
			continue;
		}

		// Try to get UButton from the widget (if it's a UButton or contains one)
		UButton* ButtonComponent = nullptr;
		
		// Check if the widget itself is a UButton
		if (UButton* DirectButton = Cast<UButton>(NewButtonWidget))
		{
			ButtonComponent = DirectButton;
		}
		else
		{
			// Try to find a UButton component inside the widget
			// This assumes your custom button widget has a UButton child named "Button" or similar
			// You can customize this based on your widget structure
			if (WidgetTree)
			{
				// Search for a button in the widget tree
				TArray<UWidget*> AllWidgets;
				WidgetTree->GetAllWidgets(AllWidgets);
				for (UWidget* Widget : AllWidgets)
				{
					if (UButton* FoundButton = Cast<UButton>(Widget))
					{
						ButtonComponent = FoundButton;
						break;
					}
				}
			}
		}

		// Store button ID mapping (use the widget, not just the button component)
		// We'll need to update HandleButtonClickWithID to accept UUserWidget too
		if (ButtonComponent)
		{
			ButtonIDMap.Add(ButtonComponent, ButtonData.ButtonID);
			// Bind click event if we found a button component
			ButtonComponent->OnClicked.AddDynamic(this, &UPUPopupWidget::HandleButtonClick);
		}
		else
		{
			// If no button component found, store the widget itself
			// The custom widget should call HandleButtonClickWithWidget instead
			UE_LOG(LogTemp, Log, TEXT("UPUPopupWidget::CreateButtons - Custom button widget created, no UButton component found. Widget should call HandleButtonClickWithWidget when clicked."));
		}

		// Try to set button text/data via Blueprint function
		// Look for a function called "SetButtonData" or "SetButtonText" in the widget
		FName SetButtonDataFunctionName = TEXT("SetButtonData");
		UFunction* SetButtonDataFunction = NewButtonWidget->GetClass()->FindFunctionByName(SetButtonDataFunctionName);
		if (SetButtonDataFunction)
		{
			// Create a struct on scope to pass FPopupButtonData
			FStructOnScope StructOnScope(FPopupButtonData::StaticStruct());
			FPopupButtonData* ButtonDataCopy = (FPopupButtonData*)StructOnScope.GetStructMemory();
			*ButtonDataCopy = ButtonData;
			
			NewButtonWidget->ProcessEvent(SetButtonDataFunction, StructOnScope.GetStructMemory());
		}
		else
		{
			// Fallback: try SetButtonText function
			FName SetButtonTextFunctionName = TEXT("SetButtonText");
			UFunction* SetButtonTextFunction = NewButtonWidget->GetClass()->FindFunctionByName(SetButtonTextFunctionName);
			if (SetButtonTextFunction)
			{
				// Call with FText parameter
				FText ButtonLabel = ButtonData.ButtonLabel;
				NewButtonWidget->ProcessEvent(SetButtonTextFunction, &ButtonLabel);
			}
			else if (ButtonComponent)
			{
				// Last resort: set tooltip
				ButtonComponent->SetToolTipText(ButtonData.ButtonLabel);
			}
		}

		// Add to container
		ButtonsContainer->AddChild(NewButtonWidget);
		
		// Store widget reference for cleanup
		if (ButtonComponent)
		{
			SpawnedButtons.Add(ButtonComponent);
		}
		
		// Also store the widget itself for cleanup
		SpawnedButtonWidgets.Add(NewButtonWidget);

		UE_LOG(LogTemp, Log, TEXT("UPUPopupWidget::CreateButtons - Created button: %s"), *ButtonData.ButtonLabel.ToString());
	}
}

void UPUPopupWidget::ClearButtons()
{
	if (ButtonsContainer)
	{
		ButtonsContainer->ClearChildren();
	}

	// Clear references
	for (UButton* Button : SpawnedButtons)
	{
		if (IsValid(Button))
		{
			Button->OnClicked.RemoveAll(this);
		}
	}
	SpawnedButtons.Empty();
	ButtonIDMap.Empty();
}

void UPUPopupWidget::StartAutoDismissTimer()
{
	StopAutoDismissTimer();

	if (CurrentPopupData.bAutoDismiss && CurrentPopupData.AutoDismissTime > 0.0f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				AutoDismissTimer,
				this,
				&UPUPopupWidget::OnAutoDismissTimer,
				CurrentPopupData.AutoDismissTime,
				false
			);
		}
	}
}

void UPUPopupWidget::OnAutoDismissTimer()
{
	Close(NAME_None);
}

void UPUPopupWidget::StopAutoDismissTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoDismissTimer);
	}
}

void UPUPopupWidget::OnButtonClicked(FName ButtonID)
{
	Close(ButtonID);
}

void UPUPopupWidget::HandleButtonClick()
{
	// Fallback handler - if buttons don't call HandleButtonClickWithID, use this
	// This is called by the close button or if buttons aren't set up properly
	Close(NAME_None);
}

void UPUPopupWidget::HandleButtonClickWithID(UButton* ClickedButton)
{
	if (!ClickedButton)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPUPopupWidget::HandleButtonClickWithID - ClickedButton is null"));
		Close(NAME_None);
		return;
	}

	// Look up the button ID from the map
	FName ButtonID = GetButtonID(ClickedButton);
	if (ButtonID == NAME_None)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPUPopupWidget::HandleButtonClickWithID - Button ID not found for button: %s"), *ClickedButton->GetName());
		Close(NAME_None);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("UPUPopupWidget::HandleButtonClickWithID - Button clicked: %s"), *ButtonID.ToString());
	Close(ButtonID);
}

void UPUPopupWidget::HandleButtonClickWithIDDirect(FName ButtonID)
{
	if (ButtonID == NAME_None)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPUPopupWidget::HandleButtonClickWithIDDirect - ButtonID is None"));
		Close(NAME_None);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("UPUPopupWidget::HandleButtonClickWithIDDirect - Button clicked: %s"), *ButtonID.ToString());
	Close(ButtonID);
}

void UPUPopupWidget::UpdatePopupStyle()
{
	// Update visual style based on PopupType
	// This is a placeholder - you can customize colors, borders, etc. based on CurrentPopupData.PopupType
	if (PopupBorder)
	{
		// You can set different border colors/styles based on PopupType
		// Example: Red for Error, Yellow for Warning, Blue for Info, etc.
		// This would require setting up styles in Blueprint or using dynamic materials
	}
}

FName UPUPopupWidget::GetButtonID(UButton* Button) const
{
	if (Button && ButtonIDMap.Contains(Button))
	{
		return ButtonIDMap[Button];
	}
	return NAME_None;
}

