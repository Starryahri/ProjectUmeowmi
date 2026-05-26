#include "PUPipelineStageMinigameModuleWidget.h"

#include "../DishCustomization/PUIngredientBase.h"
#include "../DishCustomization/PUDishCustomizationComponent.h"
#include "PUDishCustomizationWidget.h"
#include "PUChopStripMinigameBehavior.h"
#include "PUIngredientSlot.h"
#include "PUStripMinigameBehavior.h"
#include "PUStripMinigameProgressBarWidget.h"
#include "PUUObjectSafety.h"
#include "UObject/UObjectIterator.h"
#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Engine/Texture2D.h"

namespace
{
    UImage* FindFoodImageDescendant(UWidget* Widget, const UPUStripMinigameProgressBarWidget* ProgressBarToSkip)
    {
        if (!Widget)
        {
            return nullptr;
        }

        if (Widget == ProgressBarToSkip)
        {
            return nullptr;
        }

        if (UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
        {
            const int32 ChildCount = Panel->GetChildrenCount();
            for (int32 ChildIndex = 0; ChildIndex < ChildCount; ++ChildIndex)
            {
                if (UImage* Found = FindFoodImageDescendant(Panel->GetChildAt(ChildIndex), ProgressBarToSkip))
                {
                    return Found;
                }
            }
            return nullptr;
        }

        return Cast<UImage>(Widget);
    }

    UImage* FindFoodImageByName(UPUPipelineStageMinigameModuleWidget* Owner, FName WidgetName)
    {
        if (!Owner || WidgetName.IsNone())
        {
            return nullptr;
        }

        UWidget* NamedWidget = nullptr;
        if (Owner->WidgetTree)
        {
            NamedWidget = Owner->WidgetTree->FindWidget(WidgetName);
        }
        if (!NamedWidget)
        {
            NamedWidget = Owner->GetWidgetFromName(WidgetName);
        }
        return Cast<UImage>(NamedWidget);
    }

    UPUStripMinigameProgressBarWidget* FindStripMinigameProgressBarDescendant(UWidget* Widget)
    {
        if (!Widget)
        {
            return nullptr;
        }

        if (UPUStripMinigameProgressBarWidget* ProgressBar = Cast<UPUStripMinigameProgressBarWidget>(Widget))
        {
            return ProgressBar;
        }

        if (UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
        {
            const int32 ChildCount = Panel->GetChildrenCount();
            for (int32 ChildIndex = 0; ChildIndex < ChildCount; ++ChildIndex)
            {
                if (UPUStripMinigameProgressBarWidget* Found =
                        FindStripMinigameProgressBarDescendant(Panel->GetChildAt(ChildIndex)))
                {
                    return Found;
                }
            }
        }

        return nullptr;
    }
}

void UPUPipelineStageMinigameModuleWidget::NativeConstruct()
{
    Super::NativeConstruct();
    ResolveStripMinigameProgressBarWidget();
    ResolveStripMinigameFoodImageWidget();
    SyncStripMinigameProgressBarBinding();
}

void UPUPipelineStageMinigameModuleWidget::NativeDestruct()
{
    TeardownStripMinigameBehavior();
    Super::NativeDestruct();
}

void UPUPipelineStageMinigameModuleWidget::BeginDestroy()
{
    TeardownStripMinigameBehavior();
    Super::BeginDestroy();
}

void UPUPipelineStageMinigameModuleWidget::ResolveStripMinigameProgressBarWidget()
{
    if (IsValid(StripMinigameProgressBar))
    {
        return;
    }

    const FName PreferredName = StripMinigameProgressBarWidgetName.IsNone()
        ? FName(TEXT("StripMinigameProgressBar"))
        : StripMinigameProgressBarWidgetName;

    UWidget* NamedWidget = nullptr;
    if (WidgetTree)
    {
        NamedWidget = WidgetTree->FindWidget(PreferredName);
    }
    if (!NamedWidget)
    {
        NamedWidget = GetWidgetFromName(PreferredName);
    }
    if (NamedWidget)
    {
        StripMinigameProgressBar = Cast<UPUStripMinigameProgressBarWidget>(NamedWidget);
    }

    if (!StripMinigameProgressBar && StageMinigameUIPanel)
    {
        StripMinigameProgressBar = FindStripMinigameProgressBarDescendant(StageMinigameUIPanel);
    }

    if (!StripMinigameProgressBar && WidgetTree && WidgetTree->RootWidget)
    {
        StripMinigameProgressBar = FindStripMinigameProgressBarDescendant(WidgetTree->RootWidget);
    }
}

void UPUPipelineStageMinigameModuleWidget::ResolveStripMinigameFoodImageWidget()
{
    if (IsValid(FoodToBeChopped))
    {
        ResolvedStripMinigameFoodImage = FoodToBeChopped;
        return;
    }

    if (IsValid(StripMinigameFoodImage))
    {
        ResolvedStripMinigameFoodImage = StripMinigameFoodImage;
        return;
    }

    TArray<FName> CandidateNames;
    CandidateNames.Add(FName(TEXT("FoodToBeChopped")));
    if (!StripMinigameFoodImageWidgetName.IsNone())
    {
        CandidateNames.Add(StripMinigameFoodImageWidgetName);
    }
    CandidateNames.Add(FName(TEXT("StripMinigameFoodImage")));
    CandidateNames.Add(FName(TEXT("FoodImage")));
    CandidateNames.Add(FName(TEXT("IngredientFoodImage")));
    CandidateNames.Add(FName(TEXT("ChoppingFoodImage")));

    for (const FName& CandidateName : CandidateNames)
    {
        if (UImage* NamedImage = FindFoodImageByName(this, CandidateName))
        {
            ResolvedStripMinigameFoodImage = NamedImage;
            return;
        }
    }
}

void UPUPipelineStageMinigameModuleWidget::ApplyStripMinigameFoodVisual()
{
    ResolveStripMinigameFoodImageWidget();
    UImage* FoodImage = ResolvedStripMinigameFoodImage.Get();
    if (!FoodImage)
    {
        return;
    }

    UTexture2D* Texture = GetStripMinigameFoodTexture();
    const FLinearColor Tint = GetStripMinigameFoodTint();

    if (Texture)
    {
        FoodImage->SetBrushFromTexture(Texture, true);
    }
    FoodImage->SetColorAndOpacity(Tint);
    FoodImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UPUPipelineStageMinigameModuleWidget::ApplyStripSlotFocusPreviewVisual(UPUIngredientSlot* StripSlot)
{
    ResolveStripMinigameFoodImageWidget();
    UImage* FoodImage = ResolvedStripMinigameFoodImage.Get();
    if (!FoodImage)
    {
        return;
    }

    // Always clear a prior cut-tier multiply tint before applying the next preview frame.
    FoodImage->SetColorAndOpacity(FLinearColor::White);

    if (!IsValid(StripSlot) || StripSlot->IsEmpty())
    {
        return;
    }

    const FPUIngredientBase& IngredientData = StripSlot->GetIngredientInstance().IngredientData;
    if (UTexture2D* WholeTexture = IngredientData.GetCutVisualTexture(EPUIngredientCutVisualTier::Whole))
    {
        FoodImage->SetBrushFromTexture(WholeTexture, true);
    }

    FoodImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UPUPipelineStageMinigameModuleWidget::InitializeStageModule_Implementation(
    UPUDishCustomizationWidget* InOwnerShell,
    UPUDishCustomizationComponent* InCustomizationComponent,
    const FPUDishCustomizationStageDescriptor& StageDescriptor)
{
    OwnerShell = InOwnerShell;
    CustomizationComponent = InCustomizationComponent;
    bStripMinigameActive = false;
    CachedStageDescriptor = StageDescriptor;
    bHasCachedStageDescriptor = true;
    SetupStripMinigameBehavior(StageDescriptor);
    ApplyStageMinigameUIPanelVisibility();
}

int32 UPUPipelineStageMinigameModuleWidget::GetCurrentStageIndex() const
{
    if (IsValid(CustomizationComponent))
    {
        return CustomizationComponent->GetActiveCustomizationPipelineIndex();
    }
    if (IsValid(OwnerShell))
    {
        return OwnerShell->GetActiveCustomizationPipelineIndex();
    }
    return INDEX_NONE;
}

void UPUPipelineStageMinigameModuleWidget::ShutdownStageModule_Implementation()
{
    if (bStripMinigameActive)
    {
        SetStripMinigameActive(false, nullptr);
    }
    TeardownStripMinigameBehavior();
    bStripMinigameActive = false;
    ApplyStageMinigameUIPanelVisibility();
    OwnerShell = nullptr;
    CustomizationComponent = nullptr;
    bHasCachedStageDescriptor = false;
}

void UPUPipelineStageMinigameModuleWidget::OnIngredientStripSlotFocusChanged_Implementation(UPUIngredientSlot* StripSlot)
{
    HandleIngredientStripSlotFocusChanged(StripSlot);
}

void UPUPipelineStageMinigameModuleWidget::HandleIngredientStripSlotFocusChanged_Implementation(UPUIngredientSlot* StripSlot)
{
    if (bStripMinigameActive)
    {
        return;
    }

    ApplyStripSlotFocusPreviewVisual(StripSlot);
}

bool UPUPipelineStageMinigameModuleWidget::ToggleStageMinigameFromIngredientStripSlot_Implementation(UPUIngredientSlot* StripSlot)
{
    if (!StripSlot)
    {
        return false;
    }
    if (!bStripMinigameActive)
    {
        if (!StripSlot->CanUseStageMinigameHotkey())
        {
            return false;
        }
        if (IsValid(ActiveStripMinigameBehavior)
            && !ActiveStripMinigameBehavior->CanStartStripMinigameForSlot(StripSlot))
        {
            return false;
        }
    }
    SetStripMinigameActive(!bStripMinigameActive, StripSlot);
    return true;
}

bool UPUPipelineStageMinigameModuleWidget::TryConsumeStripMinigameKey(const FKey& Key)
{
    if (!bStripMinigameActive || !IsValid(ActiveStripMinigameBehavior))
    {
        return false;
    }
    return ActiveStripMinigameBehavior->TryConsumeMinigameKey(Key);
}

bool UPUPipelineStageMinigameModuleWidget::TryReleaseStripMinigameKey(const FKey& Key)
{
    if (!bStripMinigameActive || !IsValid(ActiveStripMinigameBehavior))
    {
        return false;
    }
    return ActiveStripMinigameBehavior->TryReleaseMinigameKey(Key);
}

void UPUPipelineStageMinigameModuleWidget::SetStripMinigameActive(bool bActive, UPUIngredientSlot* ContextStripSlot)
{
    if (bStripMinigameActive == bActive)
    {
        return;
    }

    bStripMinigameActive = bActive;
    StripMinigameContextStripSlot = bActive ? ContextStripSlot : nullptr;
    if (bActive)
    {
        bNextChopStrikeUsesAnimationA = true;
    }
    ApplyStageMinigameUIPanelVisibility();

    if (IsValid(OwnerShell))
    {
        OwnerShell->SetIngredientRailStripInteractionLocked(bActive, ContextStripSlot);
        if (bActive && IsValid(ContextStripSlot))
        {
            OwnerShell->NotifyMountedStageModuleOfStripSlotFocus(ContextStripSlot);
        }
    }

    if (IsValid(ActiveStripMinigameBehavior))
    {
        ActiveStripMinigameBehavior->HandleStripMinigameSessionChanged(bActive, ContextStripSlot);
    }

    SyncStripMinigameProgressBarBinding();

    if (bActive)
    {
        ApplyStripMinigameFoodVisual();
    }
    else
    {
        UPUIngredientSlot* BrowsingPreviewSlot = ContextStripSlot;
        if (IsValid(OwnerShell))
        {
            if (UPUIngredientSlot* FocusedStrip = OwnerShell->FindFocusedIngredientRailStripSlot())
            {
                BrowsingPreviewSlot = FocusedStrip;
            }
        }
        ApplyStripSlotFocusPreviewVisual(BrowsingPreviewSlot);
    }

    ReceiveStripMinigamePresentationChanged(bActive, ContextStripSlot);
}

UTexture2D* UPUPipelineStageMinigameModuleWidget::GetStripMinigameFoodTexture() const
{
    const UPUChopStripMinigameBehavior* ChopBehavior = Cast<UPUChopStripMinigameBehavior>(ActiveStripMinigameBehavior);
    return ChopBehavior ? ChopBehavior->GetMinigameIngredientDisplayTexture() : nullptr;
}

FLinearColor UPUPipelineStageMinigameModuleWidget::GetStripMinigameFoodTint() const
{
    const UPUChopStripMinigameBehavior* ChopBehavior = Cast<UPUChopStripMinigameBehavior>(ActiveStripMinigameBehavior);
    return ChopBehavior ? ChopBehavior->GetMinigameIngredientDisplayTint() : FLinearColor::White;
}

void UPUPipelineStageMinigameModuleWidget::SyncStripMinigameProgressBarBinding()
{
    ResolveStripMinigameProgressBarWidget();

    if (!StripMinigameProgressBar)
    {
        return;
    }

    if (bStripMinigameActive && IsValid(ActiveStripMinigameBehavior))
    {
        StripMinigameProgressBar->BindToStripMinigameBehavior(ActiveStripMinigameBehavior);
    }
    else
    {
        StripMinigameProgressBar->UnbindFromStripMinigameBehavior();
    }
}

TSubclassOf<UPUStripMinigameBehavior> UPUPipelineStageMinigameModuleWidget::ResolveStripMinigameBehaviorClass_Implementation(
    const FPUDishCustomizationStageDescriptor& StageDescriptor) const
{
    if (StageDescriptor.StageId.IsValid())
    {
        if (const TSubclassOf<UPUStripMinigameBehavior>* MappedClass =
                StripMinigameBehaviorByStageId.Find(StageDescriptor.StageId))
        {
            if (MappedClass->Get())
            {
                return *MappedClass;
            }
        }

        static const FGameplayTag StageChoppingTag = FGameplayTag::RequestGameplayTag(FName("Stage.Chopping"), false);
        if (StageChoppingTag.IsValid() && StageDescriptor.StageId.MatchesTag(StageChoppingTag))
        {
            return UPUChopStripMinigameBehavior::StaticClass();
        }
    }

    return nullptr;
}

void UPUPipelineStageMinigameModuleWidget::SetupStripMinigameBehavior(
    const FPUDishCustomizationStageDescriptor& StageDescriptor)
{
    if (StripMinigameProgressBar)
    {
        StripMinigameProgressBar->UnbindFromStripMinigameBehavior();
    }

    UPUStripMinigameBehavior* PreviousBehavior = ActiveStripMinigameBehavior.Get();
    ActiveStripMinigameBehavior = nullptr;

    if (IsValid(PreviousBehavior) && PreviousBehavior != StripMinigameBehavior)
    {
        PreviousBehavior->DisconnectFromOwner(this, StripMinigameProgressBar);
    }

    TSubclassOf<UPUStripMinigameBehavior> BehaviorClass = StripMinigameBehaviorClass;
    if (!BehaviorClass)
    {
        BehaviorClass = ResolveStripMinigameBehaviorClass(StageDescriptor);
    }

    UPUStripMinigameBehavior* Behavior = nullptr;
    if (IsValid(StripMinigameBehavior))
    {
        Behavior = StripMinigameBehavior;
    }
    else if (IsValid(PreviousBehavior) && PreviousBehavior->GetClass() == BehaviorClass)
    {
        Behavior = PreviousBehavior;
    }
    else if (BehaviorClass)
    {
        Behavior = NewObject<UPUStripMinigameBehavior>(this, BehaviorClass);
    }

    if (!Behavior)
    {
        return;
    }

    ActiveStripMinigameBehavior = Behavior;
    ActiveStripMinigameBehavior->InitializeBehavior(this);
    ActiveStripMinigameBehavior->HandleStageModuleInitialized(StageDescriptor);
    SyncStripMinigameProgressBarBinding();
}

void UPUPipelineStageMinigameModuleWidget::TeardownStripMinigameBehavior()
{
    if (bStripMinigameActive)
    {
        SetStripMinigameActive(false, nullptr);
    }

    if (StripMinigameProgressBar)
    {
        StripMinigameProgressBar->UnbindFromStripMinigameBehavior();
    }

    UPUStripMinigameBehavior* Behavior = ActiveStripMinigameBehavior.Get();
    ActiveStripMinigameBehavior = nullptr;
    StripMinigameContextStripSlot = nullptr;
    ResolvedStripMinigameFoodImage = nullptr;

    if (!IsValid(Behavior))
    {
        return;
    }

    Behavior->DisconnectFromOwner(this, StripMinigameProgressBar);
}

void UPUPipelineStageMinigameModuleWidget::SanitizeStaleObjectReferences()
{
    if (OwnerShell != nullptr && !PUObjectReferenceSafety::IsLiveObject(OwnerShell))
    {
        OwnerShell = nullptr;
    }
    if (CustomizationComponent != nullptr && !PUObjectReferenceSafety::IsLiveObject(CustomizationComponent))
    {
        CustomizationComponent = nullptr;
    }
    if (StripMinigameProgressBar != nullptr && !PUObjectReferenceSafety::IsLiveObject(StripMinigameProgressBar))
    {
        StripMinigameProgressBar = nullptr;
    }
    else if (StripMinigameProgressBar)
    {
        StripMinigameProgressBar->SanitizeBoundBehaviorReference();
    }
    if (StripMinigameContextStripSlot != nullptr && !PUObjectReferenceSafety::IsLiveObject(StripMinigameContextStripSlot))
    {
        StripMinigameContextStripSlot = nullptr;
    }
    if (ActiveStripMinigameBehavior != nullptr && !PUObjectReferenceSafety::IsLiveObject(ActiveStripMinigameBehavior))
    {
        ActiveStripMinigameBehavior = nullptr;
    }
    else if (ActiveStripMinigameBehavior)
    {
        ActiveStripMinigameBehavior->SanitizeStaleObjectReferences();
    }
    if (ResolvedStripMinigameFoodImage != nullptr && !PUObjectReferenceSafety::IsLiveObject(ResolvedStripMinigameFoodImage))
    {
        ResolvedStripMinigameFoodImage = nullptr;
    }
    if (FoodToBeChopped != nullptr && !PUObjectReferenceSafety::IsLiveObject(FoodToBeChopped))
    {
        FoodToBeChopped = nullptr;
    }
    if (StripMinigameFoodImage != nullptr && !PUObjectReferenceSafety::IsLiveObject(StripMinigameFoodImage))
    {
        StripMinigameFoodImage = nullptr;
    }
}

void UPUPipelineStageMinigameModuleWidget::SanitizeAllLiveStageMinigameModules()
{
    for (TObjectIterator<UPUPipelineStageMinigameModuleWidget> It; It; ++It)
    {
        UPUPipelineStageMinigameModuleWidget* Module = *It;
        if (PUObjectReferenceSafety::CanQueryUObject(Module) && !Module->HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed))
        {
            Module->SanitizeStaleObjectReferences();
        }
    }
}

void UPUPipelineStageMinigameModuleWidget::ApplyStageMinigameUIPanelVisibility()
{
    if (!StageMinigameUIPanel)
    {
        return;
    }
    StageMinigameUIPanel->SetVisibility(
        bStripMinigameActive ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}

void UPUPipelineStageMinigameModuleWidget::NotifyStripMinigameChopPressed()
{
    PlayStripMinigameChopAnimation();
    ReceiveStripMinigameChopPressed();
    ReceiveStripMinigameChopPlayed();
}

void UPUPipelineStageMinigameModuleWidget::NotifyStripMinigameChopReleased()
{
    ReceiveStripMinigameChopReleased();
}

void UPUPipelineStageMinigameModuleWidget::PlayStripMinigameChopAnimation_Implementation()
{
    UWidgetAnimation* AnimationToPlay = nullptr;

    if (ChopStrikeAnimationA && ChopStrikeAnimationB)
    {
        AnimationToPlay = bNextChopStrikeUsesAnimationA ? ChopStrikeAnimationA.Get() : ChopStrikeAnimationB.Get();
        bNextChopStrikeUsesAnimationA = !bNextChopStrikeUsesAnimationA;
    }
    else if (ChopStrikeAnimation)
    {
        AnimationToPlay = ChopStrikeAnimation;
    }
    else if (ChopStrikeAnimationA)
    {
        AnimationToPlay = ChopStrikeAnimationA;
    }
    else if (ChopStrikeAnimationB)
    {
        AnimationToPlay = ChopStrikeAnimationB;
    }

    if (AnimationToPlay)
    {
        PlayAnimation(AnimationToPlay, 0.f, 1, EUMGSequencePlayMode::Forward, 1.f, false);
    }
}
