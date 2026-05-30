#include "PUPipelineStageMinigameModuleWidget.h"

#include "../DishCustomization/PUIngredientBase.h"
#include "../DishCustomization/PUDishCustomizationComponent.h"
#include "PUDishCustomizationWidget.h"
#include "PUChopStripMinigameBehavior.h"
#include "PUCookingStripMinigameBehavior.h"
#include "PUMarinateStripMinigameBehavior.h"
#include "PUPlatingStripMinigameBehavior.h"
#include "PUIngredientSlot.h"
#include "PUIngredientDragDropOperation.h"
#include "PUStripMinigameBehavior.h"
#include "PUStripMinigameProgressBarWidget.h"
#include "PUUObjectSafety.h"
#include "UObject/UObjectIterator.h"
#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/OverlaySlot.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CanvasPanel.h"
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

    UImage* FindImageByWidgetName(UUserWidget* Owner, FName WidgetName)
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

    void CollectOrderedPanelImages(UPanelWidget* Panel, TArray<UImage*>& OutImages, int32 MaxCount)
    {
        if (!Panel || MaxCount <= 0)
        {
            return;
        }

        const int32 ChildCount = Panel->GetChildrenCount();
        for (int32 ChildIndex = 0; ChildIndex < ChildCount && OutImages.Num() < MaxCount; ++ChildIndex)
        {
            UWidget* Child = Panel->GetChildAt(ChildIndex);
            if (UImage* Image = Cast<UImage>(Child))
            {
                OutImages.Add(Image);
            }
            else if (UPanelWidget* ChildPanel = Cast<UPanelWidget>(Child))
            {
                CollectOrderedPanelImages(ChildPanel, OutImages, MaxCount);
            }
        }
    }

    UWidget* FindWidgetByName(UUserWidget* Owner, FName WidgetName)
    {
        if (!Owner || WidgetName.IsNone())
        {
            return nullptr;
        }

        if (Owner->WidgetTree)
        {
            if (UWidget* Found = Owner->WidgetTree->FindWidget(WidgetName))
            {
                return Found;
            }
        }
        return Owner->GetWidgetFromName(WidgetName);
    }

    bool TryParseMarinationBowlSlotIndex(FName WidgetName, int32& OutIndex)
    {
        const FString Name = WidgetName.ToString();
        static const TCHAR* Prefixes[] = {
            TEXT("MarinationBowlSlot"),
            TEXT("MarinateBowlSlot")};

        for (const TCHAR* Prefix : Prefixes)
        {
            if (!Name.StartsWith(Prefix))
            {
                continue;
            }

            FString Suffix = Name.Mid(FCString::Strlen(Prefix));
            Suffix.TrimStartAndEndInline();
            if (Suffix.StartsWith(TEXT("_")))
            {
                Suffix = Suffix.Mid(1);
            }
            if (Suffix.IsEmpty() || !Suffix.IsNumeric())
            {
                continue;
            }

            OutIndex = FCString::Atoi(*Suffix);
            return true;
        }

        return false;
    }

    void CollectMarinationBowlNamedWidgets(UWidget* Widget, TArray<TPair<int32, UWidget*>>& OutWidgets)
    {
        if (!Widget)
        {
            return;
        }

        int32 SlotIndex = INDEX_NONE;
        if (TryParseMarinationBowlSlotIndex(Widget->GetFName(), SlotIndex))
        {
            OutWidgets.Add(TPair<int32, UWidget*>(SlotIndex, Widget));
        }

        if (UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
        {
            const int32 ChildCount = Panel->GetChildrenCount();
            for (int32 ChildIndex = 0; ChildIndex < ChildCount; ++ChildIndex)
            {
                CollectMarinationBowlNamedWidgets(Panel->GetChildAt(ChildIndex), OutWidgets);
            }
        }
    }

    void CopyWidgetSlotLayoutFromSource(UWidget* SourceWidget, UWidget* DestWidget)
    {
        if (!SourceWidget || !DestWidget || !SourceWidget->Slot || !DestWidget->Slot)
        {
            return;
        }

        if (UOverlaySlot* SourceOverlay = Cast<UOverlaySlot>(SourceWidget->Slot))
        {
            if (UOverlaySlot* DestOverlay = Cast<UOverlaySlot>(DestWidget->Slot))
            {
                DestOverlay->SetHorizontalAlignment(SourceOverlay->GetHorizontalAlignment());
                DestOverlay->SetVerticalAlignment(SourceOverlay->GetVerticalAlignment());
                DestOverlay->SetPadding(SourceOverlay->GetPadding());
                return;
            }
        }

        if (UCanvasPanelSlot* SourceCanvas = Cast<UCanvasPanelSlot>(SourceWidget->Slot))
        {
            if (UCanvasPanelSlot* DestCanvas = Cast<UCanvasPanelSlot>(DestWidget->Slot))
            {
                DestCanvas->SetAnchors(SourceCanvas->GetAnchors());
                DestCanvas->SetAlignment(SourceCanvas->GetAlignment());
                DestCanvas->SetOffsets(SourceCanvas->GetOffsets());
                DestCanvas->SetZOrder(SourceCanvas->GetZOrder() + 1);
                DestCanvas->SetAutoSize(SourceCanvas->GetAutoSize());
                return;
            }
        }
    }
}

void UPUPipelineStageMinigameModuleWidget::NativeConstruct()
{
    Super::NativeConstruct();
    ResolveStripMinigameProgressBarWidget();
    ResolveStripMinigameFoodImageWidget();
    ResolveMarinationBowlSlotTargets();
    ResolveMarinationBowlFrontImage();
    EnsureMarinationBowlFrontOnTop();
    ResolvePlatingDishAreaWidgets();
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

void UPUPipelineStageMinigameModuleWidget::ResolveMarinationBowlSlotTargets()
{
    ResolvedMarinationBowlSlotImages.Empty();
    ResolvedMarinationBowlIngredientSlots.Empty();

    const int32 DesiredCount = FMath::Clamp(MarinationBowlVisualCount, 1, DefaultMarinationBowlVisualCount);
    TArray<TObjectPtr<UImage>> SlotImagesByIndex;
    SlotImagesByIndex.SetNum(DesiredCount);

    UImage* ExplicitImageSlots[] = {
        MarinationBowlSlot0.Get(),
        MarinationBowlSlot1.Get(),
        MarinationBowlSlot2.Get(),
        MarinationBowlSlot3.Get(),
        MarinationBowlSlot4.Get()};

    for (int32 SlotIndex = 0; SlotIndex < DesiredCount; ++SlotIndex)
    {
        if (IsValid(ExplicitImageSlots[SlotIndex]))
        {
            SlotImagesByIndex[SlotIndex] = ExplicitImageSlots[SlotIndex];
            continue;
        }

        const FName FallbackName(*FString::Printf(TEXT("MarinationBowlSlot%d"), SlotIndex));
        if (UImage* FallbackImage = Cast<UImage>(FindWidgetByName(this, FallbackName)))
        {
            SlotImagesByIndex[SlotIndex] = FallbackImage;
        }
    }

    TArray<TPair<int32, UWidget*>> NamedWidgets;
    if (WidgetTree && WidgetTree->RootWidget)
    {
        CollectMarinationBowlNamedWidgets(WidgetTree->RootWidget, NamedWidgets);
    }
    if (MarinationBowlSlotPanel)
    {
        CollectMarinationBowlNamedWidgets(MarinationBowlSlotPanel, NamedWidgets);
    }

    NamedWidgets.Sort([](const TPair<int32, UWidget*>& A, const TPair<int32, UWidget*>& B)
    {
        if (A.Key != B.Key)
        {
            return A.Key < B.Key;
        }
        return A.Value < B.Value;
    });

    TSet<UWidget*> SeenWidgets;
    for (const TPair<int32, UWidget*>& Entry : NamedWidgets)
    {
        UWidget* Widget = Entry.Value;
        if (!Widget || SeenWidgets.Contains(Widget))
        {
            continue;
        }
        SeenWidgets.Add(Widget);

        if (Entry.Key >= 0 && Entry.Key < DesiredCount)
        {
            if (UImage* Image = Cast<UImage>(Widget))
            {
                if (!IsValid(SlotImagesByIndex[Entry.Key]))
                {
                    SlotImagesByIndex[Entry.Key] = Image;
                }
                continue;
            }

            if (UPUIngredientSlot* IngredientSlot = Cast<UPUIngredientSlot>(Widget))
            {
                if (ResolvedMarinationBowlIngredientSlots.Num() < DesiredCount)
                {
                    ResolvedMarinationBowlIngredientSlots.Add(IngredientSlot);
                }
            }
        }
    }

    if (MarinationBowlSlotPanel)
    {
        int32 NextFallbackSlotIndex = 0;
        TArray<UImage*> PanelImages;
        CollectOrderedPanelImages(MarinationBowlSlotPanel, PanelImages, DesiredCount);
        for (UImage* PanelImage : PanelImages)
        {
            if (!IsValid(PanelImage))
            {
                continue;
            }

            while (NextFallbackSlotIndex < DesiredCount && IsValid(SlotImagesByIndex[NextFallbackSlotIndex]))
            {
                ++NextFallbackSlotIndex;
            }
            if (NextFallbackSlotIndex >= DesiredCount)
            {
                break;
            }

            SlotImagesByIndex[NextFallbackSlotIndex] = PanelImage;
            ++NextFallbackSlotIndex;
        }
    }

    for (int32 SlotIndex = 0; SlotIndex < DesiredCount; ++SlotIndex)
    {
        if (IsValid(SlotImagesByIndex[SlotIndex]))
        {
            ResolvedMarinationBowlSlotImages.Add(SlotImagesByIndex[SlotIndex]);
        }
    }
}

void UPUPipelineStageMinigameModuleWidget::ResolveMarinationBowlFrontImage()
{
    if (IsValid(BowlFront))
    {
        ResolvedMarinationBowlFrontImage = BowlFront;
        return;
    }

    ResolvedMarinationBowlFrontImage = Cast<UImage>(FindWidgetByName(this, TEXT("BowlFront")));
}

void UPUPipelineStageMinigameModuleWidget::EnsureMarinationBowlFrontOnTop()
{
    ResolveMarinationBowlFrontImage();

    UImage* FrontImage = ResolvedMarinationBowlFrontImage.Get();
    if (!IsValid(FrontImage))
    {
        return;
    }

    UPanelWidget* ParentPanel = Cast<UPanelWidget>(FrontImage->GetParent());
    if (!ParentPanel)
    {
        return;
    }

    if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(FrontImage->Slot))
    {
        int32 MaxOtherZOrder = MIN_int32;
        const int32 ChildCount = ParentPanel->GetChildrenCount();
        for (int32 ChildIndex = 0; ChildIndex < ChildCount; ++ChildIndex)
        {
            UWidget* Child = ParentPanel->GetChildAt(ChildIndex);
            if (!IsValid(Child) || Child == FrontImage)
            {
                continue;
            }

            if (UCanvasPanelSlot* ChildCanvasSlot = Cast<UCanvasPanelSlot>(Child->Slot))
            {
                MaxOtherZOrder = FMath::Max(MaxOtherZOrder, ChildCanvasSlot->GetZOrder());
            }
        }

        if (MaxOtherZOrder != MIN_int32 && CanvasSlot->GetZOrder() <= MaxOtherZOrder)
        {
            CanvasSlot->SetZOrder(MaxOtherZOrder + 1);
        }
        return;
    }

    const int32 CurrentIndex = ParentPanel->GetChildIndex(FrontImage);
    const int32 LastIndex = ParentPanel->GetChildrenCount() - 1;
    if (CurrentIndex < 0 || CurrentIndex >= LastIndex)
    {
        return;
    }

    if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(FrontImage->Slot))
    {
        const EHorizontalAlignment HAlign = OverlaySlot->GetHorizontalAlignment();
        const EVerticalAlignment VAlign = OverlaySlot->GetVerticalAlignment();
        const FMargin OverlayPadding = OverlaySlot->GetPadding();

        ParentPanel->RemoveChild(FrontImage);
        ParentPanel->AddChild(FrontImage);

        if (UOverlaySlot* NewOverlaySlot = Cast<UOverlaySlot>(FrontImage->Slot))
        {
            NewOverlaySlot->SetHorizontalAlignment(HAlign);
            NewOverlaySlot->SetVerticalAlignment(VAlign);
            NewOverlaySlot->SetPadding(OverlayPadding);
        }
        return;
    }

    ParentPanel->RemoveChild(FrontImage);
    ParentPanel->AddChild(FrontImage);
}

bool UPUPipelineStageMinigameModuleWidget::TryGetMarinationBowlDisplayVisual(
    const FIngredientInstance& IngredientInstance,
    UTexture2D*& OutTexture,
    FLinearColor& OutTint)
{
    OutTexture = nullptr;
    OutTint = FLinearColor::White;

    const FGameplayTag EffectiveTag = IngredientInstance.IngredientTag.IsValid()
        ? IngredientInstance.IngredientTag
        : IngredientInstance.IngredientData.IngredientTag;
    if (!EffectiveTag.IsValid())
    {
        return false;
    }

    const FPUIngredientBase& IngredientData = IngredientInstance.IngredientData;
    OutTexture = IngredientData.PreviewTexture;
    if (!OutTexture)
    {
        OutTexture = IngredientData.PreppedTexture;
    }
    if (!OutTexture)
    {
        OutTexture = IngredientData.GetCutVisualTexture(EPUIngredientCutVisualTier::Whole);
    }

    return OutTexture != nullptr;
}

void UPUPipelineStageMinigameModuleWidget::ApplyMarinationBowlVisualsForIngredient(
    const FIngredientInstance& IngredientInstance,
    UPUIngredientSlot* SourceStripSlot,
    int32 BowlIngredientIndex)
{
    ResolveMarinationBowlSlotTargets();

    UTexture2D* Texture = nullptr;
    FLinearColor Tint = FLinearColor::White;
    if (!ResolveMarinationIngredientVisual(IngredientInstance, SourceStripSlot, Texture, Tint) || !Texture)
    {
        return;
    }

    const int32 ClampedIndex = FMath::Max(0, BowlIngredientIndex);
    if (ClampedIndex == 0)
    {
        ApplyMarinationBaseBowlVisuals(Texture, Tint, IngredientInstance);
    }
    else
    {
        const int32 SlotCount = FMath::Max(
            ResolvedMarinationBowlSlotImages.Num(),
            ResolvedMarinationBowlIngredientSlots.Num());
        if (SlotCount > 0)
        {
            const int32 LayerIndex = ClampedIndex - 1;
            if (SpawnedMarinationBowlLayerImages.Num() <= LayerIndex)
            {
                SpawnedMarinationBowlLayerImages.SetNum(LayerIndex + 1);
            }

            for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
            {
                UWidget* AnchorWidget = GetMarinationBowlAnchorWidget(SlotIndex);
                if (!AnchorWidget)
                {
                    continue;
                }

                SpawnMarinationBowlImageAtSlot(LayerIndex, SlotIndex, Texture, Tint, AnchorWidget);
            }
        }
    }

    EnsureMarinationBowlFrontOnTop();
}

bool UPUPipelineStageMinigameModuleWidget::ResolveMarinationIngredientVisual(
    const FIngredientInstance& IngredientInstance,
    UPUIngredientSlot* SourceStripSlot,
    UTexture2D*& OutTexture,
    FLinearColor& OutTint) const
{
    (void)SourceStripSlot;
    return TryGetMarinationBowlDisplayVisual(IngredientInstance, OutTexture, OutTint);
}

UWidget* UPUPipelineStageMinigameModuleWidget::GetMarinationBowlAnchorWidget(int32 SlotIndex) const
{
    if (ResolvedMarinationBowlSlotImages.IsValidIndex(SlotIndex))
    {
        return ResolvedMarinationBowlSlotImages[SlotIndex];
    }

    if (ResolvedMarinationBowlIngredientSlots.IsValidIndex(SlotIndex))
    {
        return ResolvedMarinationBowlIngredientSlots[SlotIndex];
    }

    return nullptr;
}

void UPUPipelineStageMinigameModuleWidget::ApplyMarinationBowlImageVisual(
    UImage* BowlImage,
    UTexture2D* Texture,
    const FLinearColor& Tint)
{
    if (!IsValid(BowlImage) || !Texture)
    {
        return;
    }

    BowlImage->SetBrushFromTexture(Texture, true);
    FSlateBrush Brush = BowlImage->GetBrush();
    Brush.ImageSize = FVector2D(MarinationBowlImageDrawSize, MarinationBowlImageDrawSize);
    BowlImage->SetBrush(Brush);
    BowlImage->SetColorAndOpacity(Tint);
    BowlImage->SetDesiredSizeOverride(FVector2D(MarinationBowlImageDrawSize, MarinationBowlImageDrawSize));
    BowlImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

    if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(BowlImage->Slot))
    {
        CanvasSlot->SetSize(FVector2D(MarinationBowlImageDrawSize, MarinationBowlImageDrawSize));
    }
}

void UPUPipelineStageMinigameModuleWidget::ApplyMarinationBaseBowlVisuals(
    UTexture2D* Texture,
    const FLinearColor& Tint,
    const FIngredientInstance& IngredientInstance)
{
    if (!Texture)
    {
        return;
    }

    TSet<UImage*> AppliedImages;
    auto ApplyToImage = [&](UImage* BowlImage)
    {
        if (!IsValid(BowlImage) || AppliedImages.Contains(BowlImage))
        {
            return;
        }

        AppliedImages.Add(BowlImage);
        ApplyMarinationBowlImageVisual(BowlImage, Texture, Tint);
    };

    ApplyToImage(MarinationBowlSlot0.Get());
    ApplyToImage(MarinationBowlSlot1.Get());
    ApplyToImage(MarinationBowlSlot2.Get());
    ApplyToImage(MarinationBowlSlot3.Get());
    ApplyToImage(MarinationBowlSlot4.Get());

    for (UImage* BowlImage : ResolvedMarinationBowlSlotImages)
    {
        ApplyToImage(BowlImage);
    }

    for (int32 SlotIndex = 0; SlotIndex < ResolvedMarinationBowlIngredientSlots.Num(); ++SlotIndex)
    {
        UPUIngredientSlot* BowlSlot = ResolvedMarinationBowlIngredientSlots[SlotIndex];
        if (!IsValid(BowlSlot))
        {
            continue;
        }

        BowlSlot->SetIngredientInstance(IngredientInstance);
        BowlSlot->UpdateDisplay();
        BowlSlot->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }
}

void UPUPipelineStageMinigameModuleWidget::SpawnMarinationBowlImageAtSlot(
    int32 LayerIndex,
    int32 SlotIndex,
    UTexture2D* Texture,
    const FLinearColor& Tint,
    UWidget* AnchorWidget)
{
    if (LayerIndex < 0 || !IsValid(AnchorWidget) || !Texture || !WidgetTree)
    {
        return;
    }

    UPanelWidget* ParentPanel = Cast<UPanelWidget>(AnchorWidget->GetParent());
    if (!ParentPanel)
    {
        return;
    }

    if (SpawnedMarinationBowlLayerImages.IsValidIndex(LayerIndex)
        && SpawnedMarinationBowlLayerImages[LayerIndex].SlotImages.IsValidIndex(SlotIndex)
        && IsValid(SpawnedMarinationBowlLayerImages[LayerIndex].SlotImages[SlotIndex]))
    {
        UImage* ExistingImage = SpawnedMarinationBowlLayerImages[LayerIndex].SlotImages[SlotIndex];
        ApplyMarinationBowlImageVisual(ExistingImage, Texture, Tint);
        return;
    }

    UImage* SpawnedImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
    if (!IsValid(SpawnedImage))
    {
        return;
    }

    ParentPanel->AddChild(SpawnedImage);
    CopyWidgetSlotLayoutFromSource(AnchorWidget, SpawnedImage);

    if (UImage* AnchorImage = Cast<UImage>(AnchorWidget))
    {
        SpawnedImage->SetBrush(AnchorImage->GetBrush());
    }

    ApplyMarinationBowlImageVisual(SpawnedImage, Texture, Tint);

    if (SpawnedMarinationBowlLayerImages.Num() <= LayerIndex)
    {
        SpawnedMarinationBowlLayerImages.SetNum(LayerIndex + 1);
    }
    if (SpawnedMarinationBowlLayerImages[LayerIndex].SlotImages.Num() <= SlotIndex)
    {
        SpawnedMarinationBowlLayerImages[LayerIndex].SlotImages.SetNum(SlotIndex + 1);
    }
    SpawnedMarinationBowlLayerImages[LayerIndex].SlotImages[SlotIndex] = SpawnedImage;
}

void UPUPipelineStageMinigameModuleWidget::ClearSpawnedMarinationBowlImages()
{
    for (FPUMarinationBowlSpawnedLayer& Layer : SpawnedMarinationBowlLayerImages)
    {
        for (UImage* SpawnedImage : Layer.SlotImages)
        {
            if (IsValid(SpawnedImage))
            {
                SpawnedImage->RemoveFromParent();
            }
        }
    }
    SpawnedMarinationBowlLayerImages.Empty();
}

void UPUPipelineStageMinigameModuleWidget::ClearMarinationBowlLayerVisuals(int32 BowlIngredientIndex)
{
    if (BowlIngredientIndex <= 0)
    {
        for (UImage* BowlImage : ResolvedMarinationBowlSlotImages)
        {
            if (IsValid(BowlImage))
            {
                BowlImage->SetVisibility(ESlateVisibility::Collapsed);
            }
        }

        UImage* BoundImages[] = {
            MarinationBowlSlot0.Get(),
            MarinationBowlSlot1.Get(),
            MarinationBowlSlot2.Get(),
            MarinationBowlSlot3.Get(),
            MarinationBowlSlot4.Get()};
        for (UImage* BoundImage : BoundImages)
        {
            if (IsValid(BoundImage))
            {
                BoundImage->SetVisibility(ESlateVisibility::Collapsed);
            }
        }

        for (UPUIngredientSlot* BowlSlot : ResolvedMarinationBowlIngredientSlots)
        {
            if (IsValid(BowlSlot))
            {
                const FIngredientInstance EmptyInstance;
                BowlSlot->SetIngredientInstance(EmptyInstance);
                BowlSlot->UpdateDisplay();
            }
        }
        return;
    }

    const int32 LayerIndex = BowlIngredientIndex - 1;
    if (!SpawnedMarinationBowlLayerImages.IsValidIndex(LayerIndex))
    {
        return;
    }

    for (UImage* SpawnedImage : SpawnedMarinationBowlLayerImages[LayerIndex].SlotImages)
    {
        if (IsValid(SpawnedImage))
        {
            SpawnedImage->RemoveFromParent();
        }
    }
    SpawnedMarinationBowlLayerImages[LayerIndex].SlotImages.Empty();
}

void UPUPipelineStageMinigameModuleWidget::ClearMarinationBowlVisuals()
{
    ClearSpawnedMarinationBowlImages();

    for (UImage* BowlImage : ResolvedMarinationBowlSlotImages)
    {
        if (IsValid(BowlImage))
        {
            BowlImage->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    UImage* BoundImages[] = {
        MarinationBowlSlot0.Get(),
        MarinationBowlSlot1.Get(),
        MarinationBowlSlot2.Get(),
        MarinationBowlSlot3.Get(),
        MarinationBowlSlot4.Get()};
    for (UImage* BoundImage : BoundImages)
    {
        if (IsValid(BoundImage))
        {
            BoundImage->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    for (UPUIngredientSlot* BowlSlot : ResolvedMarinationBowlIngredientSlots)
    {
        if (IsValid(BowlSlot))
        {
            const FIngredientInstance EmptyInstance;
            BowlSlot->SetIngredientInstance(EmptyInstance);
            BowlSlot->UpdateDisplay();
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
    ResolvePlatingDishAreaWidgets();
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
    ClearPlatingDishAreaVisuals();
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

void UPUPipelineStageMinigameModuleWidget::OnIngredientAddedToStripSlot_Implementation(
    UPUIngredientSlot* StripSlot,
    const FIngredientInstance& IngredientInstance)
{
    if (UPUMarinateStripMinigameBehavior* MarinateBehavior = Cast<UPUMarinateStripMinigameBehavior>(ActiveStripMinigameBehavior))
    {
        MarinateBehavior->HandleIngredientAddedToStripSlot(StripSlot, IngredientInstance);
        return;
    }

    if (IsValid(ActiveStripMinigameBehavior))
    {
        ActiveStripMinigameBehavior->HandleIngredientAddedToStripSlot(StripSlot, IngredientInstance);
        return;
    }

    ApplyMarinationBowlVisualsForIngredient(IngredientInstance, StripSlot, 0);
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
        if (Cast<UPUChopStripMinigameBehavior>(ActiveStripMinigameBehavior))
        {
            bNextChopStrikeUsesAnimationA = true;
        }
        else if (Cast<UPUMarinateStripMinigameBehavior>(ActiveStripMinigameBehavior)
                 || Cast<UPUCookingStripMinigameBehavior>(ActiveStripMinigameBehavior))
        {
            bNextMixStrikeUsesAnimationA = true;
        }
    }
    ApplyStageMinigameUIPanelVisibility();

    if (IsValid(OwnerShell))
    {
        OwnerShell->ApplyStripMinigameFooterPresentation(bActive);
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
        if (Cast<UPUChopStripMinigameBehavior>(ActiveStripMinigameBehavior))
        {
            ApplyStripMinigameFoodVisual();
        }
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

        static const FGameplayTag StageMarinateTag = FGameplayTag::RequestGameplayTag(FName("Stage.Marinate"), false);
        if (StageMarinateTag.IsValid() && StageDescriptor.StageId.MatchesTag(StageMarinateTag))
        {
            return UPUMarinateStripMinigameBehavior::StaticClass();
        }

        static const FGameplayTag StageCookingTag = FGameplayTag::RequestGameplayTag(FName("Stage.Cooking"), false);
        if (StageCookingTag.IsValid() && StageDescriptor.StageId.MatchesTag(StageCookingTag))
        {
            return UPUCookingStripMinigameBehavior::StaticClass();
        }

        static const FGameplayTag StagePlatingTag = FGameplayTag::RequestGameplayTag(FName("Stage.Plating"), false);
        if (StagePlatingTag.IsValid() && StageDescriptor.StageId.MatchesTag(StagePlatingTag))
        {
            return UPUPlatingStripMinigameBehavior::StaticClass();
        }

        static const FGameplayTag StageGarnishTag = FGameplayTag::RequestGameplayTag(FName("Stage.Garnish"), false);
        if (StageGarnishTag.IsValid() && StageDescriptor.StageId.MatchesTag(StageGarnishTag))
        {
            return UPUPlatingStripMinigameBehavior::StaticClass();
        }
    }

    return nullptr;
}

UPUCookingStripMinigameBehavior* UPUPipelineStageMinigameModuleWidget::GetActiveCookingStripMinigameBehavior() const
{
    return Cast<UPUCookingStripMinigameBehavior>(ActiveStripMinigameBehavior);
}

UPUPlatingStripMinigameBehavior* UPUPipelineStageMinigameModuleWidget::GetActivePlatingStripMinigameBehavior() const
{
    return Cast<UPUPlatingStripMinigameBehavior>(ActiveStripMinigameBehavior);
}

void UPUPipelineStageMinigameModuleWidget::AdvanceCookingStep()
{
    if (UPUCookingStripMinigameBehavior* CookingBehavior = GetActiveCookingStripMinigameBehavior())
    {
        CookingBehavior->AdvanceCookingStep();
    }
}

void UPUPipelineStageMinigameModuleWidget::ConfirmCookingStep()
{
    if (UPUCookingStripMinigameBehavior* CookingBehavior = GetActiveCookingStripMinigameBehavior())
    {
        CookingBehavior->ConfirmCurrentStep();
    }
}

void UPUPipelineStageMinigameModuleWidget::BindCookingStripMinigamePresentation(
    UPUCookingStripMinigameBehavior* CookingBehavior)
{
    if (!IsValid(CookingBehavior))
    {
        return;
    }

    CookingBehavior->OnCookingStepChanged.AddUniqueDynamic(
        this,
        &UPUPipelineStageMinigameModuleWidget::HandleCookingStepChangedForwarded);
    CookingBehavior->OnCookingProgressUpdated.AddUniqueDynamic(
        this,
        &UPUPipelineStageMinigameModuleWidget::HandleCookingProgressUpdatedForwarded);
    CookingBehavior->OnCookingCommitFinished.AddUniqueDynamic(
        this,
        &UPUPipelineStageMinigameModuleWidget::HandleCookingCommitFinishedForwarded);
}

void UPUPipelineStageMinigameModuleWidget::UnbindCookingStripMinigamePresentation(
    UPUCookingStripMinigameBehavior* CookingBehavior)
{
    if (!IsValid(CookingBehavior))
    {
        return;
    }

    CookingBehavior->OnCookingStepChanged.RemoveDynamic(
        this,
        &UPUPipelineStageMinigameModuleWidget::HandleCookingStepChangedForwarded);
    CookingBehavior->OnCookingProgressUpdated.RemoveDynamic(
        this,
        &UPUPipelineStageMinigameModuleWidget::HandleCookingProgressUpdatedForwarded);
    CookingBehavior->OnCookingCommitFinished.RemoveDynamic(
        this,
        &UPUPipelineStageMinigameModuleWidget::HandleCookingCommitFinishedForwarded);
}

void UPUPipelineStageMinigameModuleWidget::HandleCookingStepChangedForwarded(
    int32 StepIndex,
    FPUCookingMinigameStepDescriptor StepDescriptor)
{
    ReceiveCookingStepChanged(StepIndex, StepDescriptor);
}

void UPUPipelineStageMinigameModuleWidget::HandleCookingProgressUpdatedForwarded(
    int32 StepIndex,
    int32 StepProgressCompleted,
    int32 StepProgressRequired,
    float OverallProgressNormalized,
    int32 TotalStepCount)
{
    ReceiveCookingProgressUpdated(
        StepIndex,
        StepProgressCompleted,
        StepProgressRequired,
        OverallProgressNormalized,
        TotalStepCount);
}

void UPUPipelineStageMinigameModuleWidget::HandleCookingCommitFinishedForwarded(bool bAppliedPreparation)
{
    ReceiveCookingCommitFinished(bAppliedPreparation);
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

    if (UPUCookingStripMinigameBehavior* PreviousCooking = Cast<UPUCookingStripMinigameBehavior>(PreviousBehavior))
    {
        UnbindCookingStripMinigamePresentation(PreviousCooking);
    }

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

    if (UPUCookingStripMinigameBehavior* CookingBehavior = Cast<UPUCookingStripMinigameBehavior>(ActiveStripMinigameBehavior))
    {
        BindCookingStripMinigamePresentation(CookingBehavior);
    }

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

    if (UPUCookingStripMinigameBehavior* CookingBehavior = Cast<UPUCookingStripMinigameBehavior>(Behavior))
    {
        UnbindCookingStripMinigamePresentation(CookingBehavior);
    }

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

void UPUPipelineStageMinigameModuleWidget::NotifyStripMinigameMixPressed()
{
    PlayStripMinigameMixAnimation();
    ReceiveStripMinigameMixPressed();
}

void UPUPipelineStageMinigameModuleWidget::NotifyStripMinigameMixReleased()
{
    ReceiveStripMinigameMixReleased();
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

void UPUPipelineStageMinigameModuleWidget::PlayStripMinigameMixAnimation_Implementation()
{
    UWidgetAnimation* AnimationToPlay = nullptr;

    if (MixStrikeAnimationA && MixStrikeAnimationB)
    {
        AnimationToPlay = bNextMixStrikeUsesAnimationA ? MixStrikeAnimationA.Get() : MixStrikeAnimationB.Get();
        bNextMixStrikeUsesAnimationA = !bNextMixStrikeUsesAnimationA;
    }
    else if (MixStrikeAnimation)
    {
        AnimationToPlay = MixStrikeAnimation;
    }
    else if (MixStrikeAnimationA)
    {
        AnimationToPlay = MixStrikeAnimationA;
    }
    else     if (MixStrikeAnimationB)
    {
        AnimationToPlay = MixStrikeAnimationB;
    }

    if (AnimationToPlay)
    {
        PlayAnimation(AnimationToPlay, 0.f, 1, EUMGSequencePlayMode::Forward, 1.f, false);
    }
}

void UPUPipelineStageMinigameModuleWidget::ResolvePlatingDishAreaWidgets()
{
    if (!PlatingDishArea && WidgetTree)
    {
        static const FName DishAreaName(TEXT("PlatingDishArea"));
        if (UWidget* Found = WidgetTree->FindWidget(DishAreaName))
        {
            PlatingDishArea = Cast<UCanvasPanel>(Found);
        }
    }

    if (!PlatingDishDropTarget && WidgetTree)
    {
        static const FName DropTargetName(TEXT("PlatingDishDropTarget"));
        if (UWidget* Found = WidgetTree->FindWidget(DropTargetName))
        {
            PlatingDishDropTarget = Cast<UPUIngredientSlot>(Found);
        }
    }

    if (IsValid(PlatingDishDropTarget))
    {
        PlatingDishDropTarget->SetPlatingDishDropTarget(true);
        PlatingDishDropTarget->SetDragEnabled(false);
        if (IsValid(OwnerShell))
        {
            PlatingDishDropTarget->SetDishCustomizationWidget(OwnerShell);
        }
    }
}

FVector2D UPUPipelineStageMinigameModuleWidget::ComputePlatingLocalPositionInCanvas(
    UCanvasPanel* Canvas,
    UWidget* DropTargetWidget,
    const FVector2D& LocalPositionInDropTarget)
{
    if (!IsValid(Canvas))
    {
        return LocalPositionInDropTarget;
    }

    if (!IsValid(DropTargetWidget))
    {
        return LocalPositionInDropTarget;
    }

    const FGeometry& CanvasGeometry = Canvas->GetCachedGeometry();
    const FGeometry& DropGeometry = DropTargetWidget->GetCachedGeometry();
    const FVector2D DropAbsolute = DropGeometry.LocalToAbsolute(LocalPositionInDropTarget);
    return CanvasGeometry.AbsoluteToLocal(DropAbsolute);
}

UPUIngredientSlot* UPUPipelineStageMinigameModuleWidget::SpawnPlatedIngredientOnDishArea(
    const FIngredientInstance& IngredientInstance,
    const FVector2D& LocalPositionInCanvas)
{
    if (!IsValid(PlatingDishArea) || !IsValid(OwnerShell))
    {
        return nullptr;
    }

    const TSubclassOf<UPUIngredientSlot> SlotClass = OwnerShell->GetResolvedIngredientSlotClass();
    UPUIngredientSlot* NewSlot = CreateWidget<UPUIngredientSlot>(OwnerShell, SlotClass);
    if (!IsValid(NewSlot))
    {
        return nullptr;
    }

    NewSlot->SetDishCustomizationWidget(OwnerShell);
    NewSlot->SetLocation(EPUIngredientSlotLocation::Plating);
    NewSlot->SetPlatingDishArrangementSlot(true);
    NewSlot->SetDragEnabled(true);
    if (IsValid(CustomizationComponent) && CustomizationComponent->PreparationDataTable)
    {
        NewSlot->SetPreparationDataTable(CustomizationComponent->PreparationDataTable);
    }

    NewSlot->SetIngredientInstance(IngredientInstance);
    NewSlot->UpdateDisplay();

    if (UCanvasPanelSlot* CanvasSlot = PlatingDishArea->AddChildToCanvas(NewSlot))
    {
        const float DrawSize = FMath::Max(16.f, PlatingDishSlotDrawSize);
        CanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 0.f, 0.f));
        CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        CanvasSlot->SetAutoSize(false);
        CanvasSlot->SetSize(FVector2D(DrawSize, DrawSize));
        CanvasSlot->SetPosition(LocalPositionInCanvas);
    }

    SpawnedPlatingDishSlots.Add(NewSlot);
    return NewSlot;
}

bool UPUPipelineStageMinigameModuleWidget::TryMovePlatedIngredientOnDishArea(
    UPUIngredientSlot* ArrangementSlot,
    const FVector2D& LocalPositionInCanvas)
{
    if (!IsValid(ArrangementSlot) || !ArrangementSlot->IsPlatingDishArrangementSlot())
    {
        return false;
    }

    if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ArrangementSlot->Slot))
    {
        CanvasSlot->SetPosition(LocalPositionInCanvas);
        return true;
    }

    return false;
}

bool UPUPipelineStageMinigameModuleWidget::TryHandlePlatingDropOnDishArea(
    UPUIngredientSlot* DropTargetSlot,
    UPUIngredientDragDropOperation* DragOperation,
    const FVector2D& LocalPositionInDropTarget)
{
    if (!bStripMinigameActive || !IsValid(DragOperation) || !GetActivePlatingStripMinigameBehavior())
    {
        return false;
    }

    if (!IsValid(DropTargetSlot) || !DropTargetSlot->IsPlatingDishDropTarget())
    {
        return false;
    }

    if (!IsValid(PlatingDishArea))
    {
        ResolvePlatingDishAreaWidgets();
    }

    if (!IsValid(PlatingDishArea) || !IsValid(OwnerShell))
    {
        return false;
    }

    const FVector2D CanvasPosition = ComputePlatingLocalPositionInCanvas(
        PlatingDishArea.Get(),
        DropTargetSlot,
        LocalPositionInDropTarget);

    const int32 DraggedInstanceId = DragOperation->IngredientInstance.InstanceID;
    for (UPUIngredientSlot* ArrangementSlot : SpawnedPlatingDishSlots)
    {
        if (IsValid(ArrangementSlot)
            && ArrangementSlot->GetIngredientInstance().InstanceID == DraggedInstanceId)
        {
            return TryMovePlatedIngredientOnDishArea(ArrangementSlot, CanvasPosition);
        }
    }

    FIngredientInstance IngredientToPlace = DragOperation->IngredientInstance;
    if (IngredientToPlace.InstanceID == 0 || IngredientToPlace.Quantity <= 0)
    {
        return false;
    }

    UPUIngredientSlot* SourceRailSlot = OwnerShell->FindIngredientRailStripSlotByInstanceId(IngredientToPlace.InstanceID);
    if (!IsValid(SourceRailSlot))
    {
        return false;
    }

    if (UPUIngredientSlot* SpawnedSlot = SpawnPlatedIngredientOnDishArea(IngredientToPlace, CanvasPosition))
    {
        OwnerShell->NotifyPlatingIngredientMovedFromRailToDish(SourceRailSlot, SpawnedSlot);
        return true;
    }

    return false;
}

void UPUPipelineStageMinigameModuleWidget::SyncPlatingDishAreaToDishData()
{
    if (!IsValid(OwnerShell) || !IsValid(PlatingDishArea))
    {
        return;
    }

    const FGeometry& CanvasGeometry = PlatingDishArea->GetCachedGeometry();
    const FVector2D CanvasSize = CanvasGeometry.GetLocalSize();
    const float SafeWidth = FMath::Max(1.f, CanvasSize.X);
    const float SafeHeight = FMath::Max(1.f, CanvasSize.Y);

    FPUDishBase DishData = OwnerShell->GetCurrentDishData();
    for (UPUIngredientSlot* PlatedSlot : SpawnedPlatingDishSlots)
    {
        if (!IsValid(PlatedSlot) || PlatedSlot->IsEmpty())
        {
            continue;
        }

        const FIngredientInstance& Instance = PlatedSlot->GetIngredientInstance();
        if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(PlatedSlot->Slot))
        {
            const FVector2D LocalPos = CanvasSlot->GetPosition();
            const FVector NormalizedPos(
                LocalPos.X / SafeWidth,
                LocalPos.Y / SafeHeight,
                0.f);
            DishData.SetIngredientPlating(
                Instance.InstanceID,
                NormalizedPos,
                FRotator::ZeroRotator,
                FVector::OneVector);
        }
    }

    OwnerShell->UpdateDishData(DishData);
}

void UPUPipelineStageMinigameModuleWidget::ClearPlatingDishAreaVisuals()
{
    for (UPUIngredientSlot* PlatedSlot : SpawnedPlatingDishSlots)
    {
        if (IsValid(PlatedSlot))
        {
            PlatedSlot->RemoveFromParent();
        }
    }
    SpawnedPlatingDishSlots.Reset();
}
