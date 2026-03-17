#include "PUPopupWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Components/Border.h"
#include "Components/PanelWidget.h"
#include "../PUProjectUmeowmiGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Blueprint/WidgetTree.h"
#include "UObject/StructOnScope.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Events.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/GameViewportSubsystem.h"

UPUPopupWidget::UPUPopupWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Default to UButton, but can be overridden in Blueprint
	ButtonWidgetClass = nullptr; // Will be set in Blueprint
	// Ensure popup can receive focus for controller navigation
	SetIsFocusable(true);
}

void UPUPopupWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Bind close button if it exists (UButton is focusable by default)
	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(this, &UPUPopupWidget::HandleButtonClick);
	}
}

void UPUPopupWidget::NativeDestruct()
{
	// Stop timers
	StopAutoDismissTimer();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeferredFocusTimerHandle);
	}

	// Clear buttons
	ClearButtons();

	Super::NativeDestruct();
}

void UPUPopupWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Deferred focus fallback: apply on first tick if timer hasn't run yet
	if (!bHasAppliedDeferredFocus)
	{
		ApplyDeferredFocus();
	}
}

FReply UPUPopupWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	// F or A (gamepad) or Enter/Space - close/confirm popup (same as clicking primary button)
	if (Key == EKeys::F || Key == EKeys::Gamepad_FaceButton_Bottom || Key == EKeys::Enter || Key == EKeys::SpaceBar)
	{
		// Trigger primary action: first button, or close button, or Close(NAME_None)
		if (SpawnedButtons.Num() > 0 && IsValid(SpawnedButtons[0]))
		{
			FName ButtonID = GetButtonID(SpawnedButtons[0]);
			if (ButtonID != NAME_None)
			{
				HandleButtonClickWithIDDirect(ButtonID);
			}
			else
			{
				HandleButtonClick();
			}
			return FReply::Handled();
		}
		if (SpawnedButtonWidgets.Num() > 0 && CurrentPopupData.Buttons.Num() > 0)
		{
			HandleButtonClickWithIDDirect(CurrentPopupData.Buttons[0].ButtonID);
			return FReply::Handled();
		}
		if (CloseButton && CloseButton->GetVisibility() == ESlateVisibility::Visible)
		{
			HandleButtonClick();
			return FReply::Handled();
		}
		Close(NAME_None);
		return FReply::Handled();
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

void UPUPopupWidget::ApplyDeferredFocus()
{
	if (bHasAppliedDeferredFocus) return;

	UWidget* FocusTarget = GetPreferredFocusTarget();
	if (!FocusTarget) return;

	TSharedPtr<SWidget> SlateWidget = FocusTarget->GetCachedWidget();
	if (!SlateWidget.IsValid()) return;

	// Ensure UserWidget targets are focusable (UButton is focusable by default)
	if (UUserWidget* UserWidgetTarget = Cast<UUserWidget>(FocusTarget))
	{
		UserWidgetTarget->SetIsFocusable(true);
	}

	// Set keyboard focus (works for both keyboard and gamepad)
	FSlateApplication::Get().SetKeyboardFocus(SlateWidget);

	// SetUserFocus routes gamepad input to this widget - required for controller
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
		{
			FSlateApplication::Get().SetUserFocus(LocalPlayer->GetControllerId(), SlateWidget.ToSharedRef(), EFocusCause::SetDirectly);
		}
	}

	bHasAppliedDeferredFocus = true;
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

	// Schedule deferred focus for next frame - required for controller/gamepad (focus fails if set immediately)
	bHasAppliedDeferredFocus = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeferredFocusTimerHandle);
		World->GetTimerManager().SetTimer(DeferredFocusTimerHandle, this, &UPUPopupWidget::ApplyDeferredFocus, 0.05f, false);
	}

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

	// Apply viewport layout (alignment, position offset, size)
	ApplyViewportLayout();

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

		// Try to get UButton from the widget (if it contains one)
		UButton* ButtonComponent = nullptr;
		
		// Try to find a UButton component inside the widget
		// This assumes your custom button widget has a UButton child named "Button" or similar
		// You can customize this based on your widget structure
		if (NewButtonWidget->WidgetTree)
		{
			// Search for a button in the widget tree
			TArray<UWidget*> AllWidgets;
			NewButtonWidget->WidgetTree->GetAllWidgets(AllWidgets);
			for (UWidget* Widget : AllWidgets)
			{
				if (UButton* FoundButton = Cast<UButton>(Widget))
				{
					ButtonComponent = FoundButton;
					break;
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
		bool bLabelSet = false;
		FName SetButtonDataFunctionName = TEXT("SetButtonData");
		UFunction* SetButtonDataFunction = NewButtonWidget->GetClass()->FindFunctionByName(SetButtonDataFunctionName);
		if (SetButtonDataFunction)
		{
			FStructOnScope StructOnScope(FPopupButtonData::StaticStruct());
			FPopupButtonData* ButtonDataCopy = (FPopupButtonData*)StructOnScope.GetStructMemory();
			*ButtonDataCopy = ButtonData;
			NewButtonWidget->ProcessEvent(SetButtonDataFunction, StructOnScope.GetStructMemory());
			bLabelSet = true;
		}
		else
		{
			FName SetButtonTextFunctionName = TEXT("SetButtonText");
			UFunction* SetButtonTextFunction = NewButtonWidget->GetClass()->FindFunctionByName(SetButtonTextFunctionName);
			if (SetButtonTextFunction)
			{
				FText ButtonLabel = ButtonData.ButtonLabel;
				NewButtonWidget->ProcessEvent(SetButtonTextFunction, &ButtonLabel);
				bLabelSet = true;
			}
		}

		// Fallback: find TextBlock and set text directly
		auto TrySetTextBlock = [&](UTextBlock* TextBlock) -> bool
		{
			if (TextBlock)
			{
				TextBlock->SetText(ButtonData.ButtonLabel);
				return true;
			}
			return false;
		};

		if (!bLabelSet && NewButtonWidget->WidgetTree)
		{
			// 1. If ButtonLabelWidgetName is set, find that specific widget
			if (ButtonLabelWidgetName != NAME_None)
			{
				if (UWidget* NamedWidget = NewButtonWidget->WidgetTree->FindWidget(ButtonLabelWidgetName))
				{
					bLabelSet = TrySetTextBlock(Cast<UTextBlock>(NamedWidget));
				}
			}

			// 2. Try Button's direct content (common: Button contains TextBlock as child)
			if (!bLabelSet && ButtonComponent)
			{
				if (UPanelWidget* ButtonPanel = Cast<UPanelWidget>(ButtonComponent))
				{
					if (ButtonPanel->GetChildrenCount() > 0)
					{
						UWidget* ButtonContent = ButtonPanel->GetChildAt(0);
						bLabelSet = TrySetTextBlock(Cast<UTextBlock>(ButtonContent));
					}
				}
			}

			// 3. Try common TextBlock names
			if (!bLabelSet)
			{
				static const FName CommonNames[] = { TEXT("ButtonLabel"), TEXT("ButtonText"), TEXT("LabelText"), TEXT("TextBlock"), TEXT("Label") };
				for (const FName& Name : CommonNames)
				{
					if (UWidget* NamedWidget = NewButtonWidget->WidgetTree->FindWidget(Name))
					{
						if (UTextBlock* TextBlock = Cast<UTextBlock>(NamedWidget))
						{
							bLabelSet = TrySetTextBlock(TextBlock);
							break;
						}
					}
				}
			}

			// 4. Fall back to first TextBlock in widget tree
			if (!bLabelSet)
			{
				TArray<UWidget*> AllWidgets;
				NewButtonWidget->WidgetTree->GetAllWidgets(AllWidgets);
				for (UWidget* Widget : AllWidgets)
				{
					if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
					{
						bLabelSet = TrySetTextBlock(TextBlock);
						break;
					}
				}
			}
		}

		if (!bLabelSet && ButtonComponent)
		{
			ButtonComponent->SetToolTipText(ButtonData.ButtonLabel);
			UE_LOG(LogTemp, Warning, TEXT("UPUPopupWidget::CreateButtons - Could not find TextBlock for button label '%s'. Set ButtonLabelWidgetName on WBP_Popup to your TextBlock's name."), *ButtonData.ButtonLabel.ToString());
		}

		// Ensure custom button widgets are focusable (UButton is focusable by default)
		NewButtonWidget->SetIsFocusable(true);

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

void UPUPopupWidget::ApplyViewportLayout()
{
	if (!IsInViewport())
	{
		return;
	}

	UGameViewportSubsystem* Subsystem = UGameViewportSubsystem::Get(GetWorld());
	if (!Subsystem)
	{
		return;
	}

	FGameViewportWidgetSlot ViewportSlot = Subsystem->GetWidgetSlot(this);

	// Anchors and alignment: (0,0)=top-left, (0.5,0.5)=center, (1,1)=bottom-right
	float H = CurrentPopupData.HorizontalAlignment;
	float V = CurrentPopupData.VerticalAlignment;
	ViewportSlot.Anchors = FAnchors(H, V, H, V);
	ViewportSlot.Alignment = FVector2D(H, V);

	// Position offset (pixels) - Left/Top in FMargin
	if (CurrentPopupData.PositionOffset != FVector2D::ZeroVector)
	{
		ViewportSlot.Offsets = FMargin(CurrentPopupData.PositionOffset.X, CurrentPopupData.PositionOffset.Y, 0.0f, 0.0f);
	}

	Subsystem->SetWidgetSlot(this, ViewportSlot);

	// Size override
	if (CurrentPopupData.SizeOverride.X > 0 && CurrentPopupData.SizeOverride.Y > 0)
	{
		SetDesiredSizeInViewport(CurrentPopupData.SizeOverride);
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

UWidget* UPUPopupWidget::GetPreferredFocusTarget() const
{
	// Prefer first button so user can immediately press A to confirm
	if (SpawnedButtons.Num() > 0 && IsValid(SpawnedButtons[0]))
	{
		return SpawnedButtons[0];
	}
	// Custom button widgets (no UButton child) - use first widget
	if (SpawnedButtonWidgets.Num() > 0 && IsValid(SpawnedButtonWidgets[0]))
	{
		return SpawnedButtonWidgets[0];
	}
	// Fall back to close button if visible
	if (CloseButton && CloseButton->GetVisibility() == ESlateVisibility::Visible)
	{
		return CloseButton;
	}
	// Fall back to popup root
	return const_cast<UPUPopupWidget*>(this);
}
