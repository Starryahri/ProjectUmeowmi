#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PUDialogueBox.generated.h"

class UTextBlock;
class UCommonRichTextBlock;
class UImage;
class UVerticalBox;
class UButton;
class UDlgContext;
class UPUDialogueOption;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UCameraComponent;

/**
 * A dialogue box widget that can be used to display conversations and text in the game.
 */
UCLASS()
class PROJECTUMEOWMI_API UPUDialogueBox : public UUserWidget
{
    GENERATED_BODY()

public:
    UPUDialogueBox(const FObjectInitializer& ObjectInitializer);

    /** The current active dialogue context */
    UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
    UDlgContext* CurrentContext;

    /** Widget to display the speaker's name */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* ParticipantNameText;

    /** Widget to display the dialogue text. Supports rich text markup (color, italic, underline) via tags like <StyleName>text</>. Use Common Rich Text Block in Blueprint; assign a Data Table with Rich Text Style Row to Text Styles Set. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UCommonRichTextBlock* DialogueText;

    /** Widget to display the participant's image */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UImage* ParticipantImage;

    /** Optional larger portrait slot. Shown when the active node's Node Data is UPUDialogueNodeData with bUseGiantPortraitSlot. Widget name in WBP must match exactly. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UImage* GiantParticipantImage;

    /** Widget to contain the dialogue options */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UVerticalBox* DialogueOptions;

    /** Called when the widget is constructed */
    virtual void NativeConstruct() override;

    /** Called when the widget is destroyed */
    virtual void NativeDestruct() override;

    /** Called every frame */
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    /** Click anywhere to advance dialogue (playtest feedback) */
    virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    /** Handle F (skip) and E/Space (advance) when dialogue has keyboard focus - fixes F key not reaching Enhanced Input */
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
    virtual FReply NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

    /** Event called when the dialogue box is opened */
    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    void Open(UDlgContext* ActiveContext);

    /** Event called when the dialogue box is closed */
    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    void Close();

    /** Event called when the dialogue box content is updated */
    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    void Update(UDlgContext* ActiveContext);

    /** Set the vignette material at runtime (if not set in Blueprint) */
    UFUNCTION(BlueprintCallable, Category = "Vignette")
    void SetVignetteMaterial(UMaterialInterface* NewVignetteMaterial);

    /** Debug function to check if vignette material is set */
    UFUNCTION(BlueprintCallable, Category = "Vignette|Debug")
    void DebugVignetteMaterial() const;

    /** Check if typewriter effect is currently animating */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dialogue")
    bool IsTypewriterActive() const { return bTypewriterActive; }

    /** Instantly complete the typewriter effect (show full text). Called when player skips. */
    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    void CompleteTypewriter();

    /** Advance dialogue (skip typewriter or go to next line). Call when player presses Interact during dialogue. */
    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    void AdvanceDialogue();

    /** Request focus (e.g. when popup closes and dialogue is still visible). Returns the widget to focus, or nullptr. */
    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    UWidget* GetFocusTarget() const;

    /** Skip mode: fast typewriter, no sound, auto-advance when single option. Toggle via SetSkipMode or Skip button. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dialogue|Skip")
    bool IsSkipMode() const { return bSkipMode; }

    UFUNCTION(BlueprintCallable, Category = "Dialogue|Skip")
    void SetSkipMode(bool bEnabled);

    /** Optional Skip button. If bound in Blueprint, clicking toggles skip mode. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UButton* SkipButton;

    // Implementation functions
    virtual void Open_Implementation(UDlgContext* ActiveContext);
    virtual void Close_Implementation();
    virtual void Update_Implementation(UDlgContext* ActiveContext);

protected:
    /** Vignette post process material to animate (can be a Material or Material Instance) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vignette|Settings")
    TSoftObjectPtr<UMaterialInterface> VignetteMaterial;

    /** Direct material reference (MORE RELIABLE - Use this one!) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vignette|Settings", meta = (ToolTip = "Set your vignette Material Instance here. This is more reliable than the soft pointer."))
    TObjectPtr<UMaterialInterface> VignetteMaterialDirect = nullptr;

    /** Parameter name in the vignette material to control intensity (default: "Intensity") */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vignette")
    FName VignetteIntensityParameterName = TEXT("Intensity");

    /** Target vignette intensity when dialogue is open (0.0 to 1.0) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vignette", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float VignetteIntensityTarget = 1.0f;

    /** Vignette fade in duration in seconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vignette|Settings", meta = (ClampMin = "0.0", ToolTip = "How long it takes for the vignette to fade in when dialogue opens"))
    float VignetteFadeInDuration = 1.0f;

    /** Vignette fade out duration in seconds */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vignette|Settings", meta = (ClampMin = "0.0", ToolTip = "How long it takes for the vignette to fade out when dialogue closes"))
    float VignetteFadeOutDuration = 1.0f;

    /** Typewriter delay (seconds per char) when skip mode is active. Lower = faster. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Skip")
    float SkipModeCharacterDelay = 0.005f;

private:
    /** Dynamic material instance for the vignette */
    UPROPERTY()
    UMaterialInstanceDynamic* VignetteDynamicMaterial = nullptr;

    /** Current vignette intensity value */
    float CurrentVignetteIntensity = 0.0f;

    /** Target vignette intensity value */
    float TargetVignetteIntensity = 0.0f;

    /** Starting vignette intensity when animation begins */
    float StartVignetteIntensity = 0.0f;

    /** Time elapsed during vignette animation */
    float VignetteAnimationTime = 0.0f;

    /** Current fade duration being used for this animation */
    float CurrentFadeDuration = 0.5f;

    /** Timer handle for vignette animation */
    FTimerHandle VignetteAnimationTimer;

    /** Whether vignette animation is currently active */
    bool bVignetteAnimating = false;

    /** Typewriter effect state */
    FString FullDialogueText;
    int32 TypewriterCurrentIndex = 0;       /**< Current visible character index (not raw string index) */
    int32 TypewriterTotalVisibleChars = 0; /**< Total visible characters; cached when typewriter starts */
    FTimerHandle TypewriterTimerHandle;
    FTimerHandle AutoAdvanceTimerHandle;
    bool bTypewriterActive = false;

    /** Skip mode: fast typewriter, no sound, auto-advance on single option */
    bool bSkipMode = false;

    /** Set when typewriter completes in skip mode; Tick performs the advance and clears it */
    bool bPendingSkipAdvance = false;

    /** Called when typewriter completes in skip mode to auto-advance (deferred to next tick) */
    void OnTypewriterCompleteAutoAdvance();

    UFUNCTION()
    void OnSkipButtonClicked();

    /** Advance typewriter by one character (called by timer) */
    void AdvanceTypewriter();

    /** Returns substring of InText up to TargetVisibleCount visible characters. Skips markup tags when counting so tags never appear as raw text. Supports <TagName>content</> format. */
    static FString GetSubstringUpToVisibleCharacter(const FString& InText, int32 TargetVisibleCount);

    /** Returns total number of visible (non-tag) characters in InText. */
    static int32 GetVisibleCharacterCount(const FString& InText);

    /** Initialize the vignette material */
    void InitializeVignetteMaterial();

    /** Update vignette intensity (called by timer) */
    void UpdateVignetteIntensity();

    /** Start animating vignette to target value */
    void AnimateVignetteToTarget(float TargetIntensity);

    /** Get the player's camera component */
    UCameraComponent* GetPlayerCamera() const;
};