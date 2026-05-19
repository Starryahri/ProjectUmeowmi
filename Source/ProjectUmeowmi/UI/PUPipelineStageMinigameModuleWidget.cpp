#include "PUPipelineStageMinigameModuleWidget.h"

#include "../DishCustomization/PUDishCustomizationComponent.h"
#include "PUDishCustomizationWidget.h"
#include "PUChopStripMinigameBehavior.h"
#include "PUIngredientSlot.h"
#include "PUStripMinigameBehavior.h"
#include "PUStripMinigameProgressBarWidget.h"
#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"

namespace
{
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
    SyncStripMinigameProgressBarBinding();
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
    (void)StripSlot;
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
    ReceiveStripMinigamePresentationChanged(bActive, ContextStripSlot);
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
    TeardownStripMinigameBehavior();

    UPUStripMinigameBehavior* Behavior = nullptr;
    if (IsValid(StripMinigameBehavior))
    {
        Behavior = DuplicateObject<UPUStripMinigameBehavior>(StripMinigameBehavior, this);
    }
    else
    {
        TSubclassOf<UPUStripMinigameBehavior> BehaviorClass = StripMinigameBehaviorClass;
        if (!BehaviorClass)
        {
            BehaviorClass = ResolveStripMinigameBehaviorClass(StageDescriptor);
        }
        if (BehaviorClass)
        {
            Behavior = NewObject<UPUStripMinigameBehavior>(this, BehaviorClass);
        }
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
    if (StripMinigameProgressBar)
    {
        StripMinigameProgressBar->UnbindFromStripMinigameBehavior();
    }
    ActiveStripMinigameBehavior = nullptr;
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
