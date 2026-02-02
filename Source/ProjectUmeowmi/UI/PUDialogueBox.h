#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PUDialogueBox.generated.h"

class UTextBlock;
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

    /** Widget to display the dialogue text */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* DialogueText;

    /** Widget to display the participant's image */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UImage* ParticipantImage;

    /** Widget to contain the dialogue options */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UVerticalBox* DialogueOptions;

    /** Called when the widget is constructed */
    virtual void NativeConstruct() override;

    /** Called when the widget is destroyed */
    virtual void NativeDestruct() override;

    /** Called every frame */
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

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

    /** Initialize the vignette material */
    void InitializeVignetteMaterial();

    /** Update vignette intensity (called by timer) */
    void UpdateVignetteIntensity();

    /** Start animating vignette to target value */
    void AnimateVignetteToTarget(float TargetIntensity);

    /** Get the player's camera component */
    UCameraComponent* GetPlayerCamera() const;
};