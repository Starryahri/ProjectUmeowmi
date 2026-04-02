// Copyright Epic Games, Inc. All Rights Reserved.

#include "PUJournalWidget.h"
#include "PUScorecardWidget.h"
#include "PUJournalTabListWidget.h"
#include "PUJournalSectionWidget.h"
#include "PURecipesSectionWidget.h"
#include "../ProjectUmeowmiCharacter.h"
#include "../DishCustomization/PUDishCustomizationComponent.h"
#include "../PUProjectUmeowmiGameInstance.h"
#include "UObject/UObjectIterator.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "CommonActivatableWidgetSwitcher.h"
#include "CommonAnimatedSwitcher.h"
#include "CommonButtonBase.h"
#include "Components/VerticalBox.h"
#include "Components/WidgetSwitcher.h"
#include "Components/TextBlock.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"

namespace
{
	/** WBP_HUD may be Collapsed during dish customization; restore visibility up the parent chain so the journal can draw. */
	void UnhideCollapsedAncestors(UWidget* Leaf)
	{
		for (UWidget* W = Leaf; W; W = Cast<UWidget>(W->GetParent()))
		{
			const ESlateVisibility Vis = W->GetVisibility();
			if (Vis == ESlateVisibility::Collapsed || Vis == ESlateVisibility::Hidden)
			{
				W->SetVisibility(ESlateVisibility::Visible);
			}
		}
	}

	/** True when this player's pawn is the character in an active dish customization session. */
	bool IsLocalPlayerInActiveDishCustomization(UWorld* World, APlayerController* PC)
	{
		if (!World || !PC)
		{
			return false;
		}
		APawn* Pawn = PC->GetPawn();
		if (!Pawn)
		{
			return false;
		}
		for (TObjectIterator<UPUDishCustomizationComponent> It; It; ++It)
		{
			UPUDishCustomizationComponent* Comp = *It;
			if (!IsValid(Comp) || Comp->GetWorld() != World)
			{
				continue;
			}
			if (!Comp->IsCustomizing())
			{
				continue;
			}
			if (Comp->GetCurrentCharacter() == Pawn)
			{
				return true;
			}
		}
		return false;
	}

	void ApplyJournalInputLayerForWidget(const UUserWidget* Widget, bool bPush)
	{
		if (!Widget)
		{
			return;
		}
		APlayerController* PC = Widget->GetOwningPlayer();
		if (!PC)
		{
			return;
		}
		if (AProjectUmeowmiCharacter* Char = Cast<AProjectUmeowmiCharacter>(PC->GetPawn()))
		{
			if (bPush)
			{
				Char->PushJournalInputMappingLayer();
			}
			else
			{
				Char->PopJournalInputMappingLayer();
			}
		}
	}

}

UPUJournalWidget::UPUJournalWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, LastSelectedTabID(NAME_None)
{
}

void UPUJournalWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RegisterJournalTabs();
}

void UPUJournalWidget::NativeDestruct()
{
	ApplyJournalInputLayerForWidget(this, false);
	if (TabList)
	{
		TabList->OnTabButtonCreation.RemoveDynamic(this, &UPUJournalWidget::OnTabButtonCreated);
	}
	SectionWidgets.Empty();
	SectionTabIds.Empty();
	ResolvedTabEntries.Empty();
	Super::NativeDestruct();
}

void UPUJournalWidget::OpenJournal()
{
	ApplyJournalInputLayerForWidget(this, true);
	SetVisibility(ESlateVisibility::Visible);
	UnhideCollapsedAncestors(this);
	if (UWorld* World = GetWorld())
	{
		if (UPUProjectUmeowmiGameInstance* GI = World->GetGameInstance<UPUProjectUmeowmiGameInstance>())
		{
			GI->NotifyJournalOpened();
		}
	}
	// During dish customization the HUD is collapsed and the dish widget is ~Z 250 — bring journal to the foreground only then.
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (IsLocalPlayerInActiveDishCustomization(GetWorld(), PC))
		{
			const int32 JournalZ = FMath::Max(JournalViewportZOrderWhenOpen, PUJournalViewportZOrderDuringDishCustomization);
			AddToViewport(JournalZ);
		}
	}

	if (TabList && bRestoreLastTabOnOpen && LastSelectedTabID != NAME_None && HasTabId(LastSelectedTabID))
	{
		TabList->SelectTabByID(LastSelectedTabID, true);
	}
	else if (TabList)
	{
		const FName DefaultId = GetEffectiveDefaultTabId();
		if (DefaultId != NAME_None)
		{
			TabList->SelectTabByID(DefaultId, true);
		}
	}
}

void UPUJournalWidget::CloseJournal()
{
	if (TabList)
	{
		LastSelectedTabID = TabList->GetActiveTab();
	}
	SetVisibility(ESlateVisibility::Collapsed);
	if (UWorld* World = GetWorld())
	{
		if (UPUProjectUmeowmiGameInstance* GI = World->GetGameInstance<UPUProjectUmeowmiGameInstance>())
		{
			GI->NotifyJournalClosed();
		}
	}
	ApplyJournalInputLayerForWidget(this, false);
}

void UPUJournalWidget::SwitchToTabById(FName TabId)
{
	if (TabList && HasTabId(TabId))
	{
		TabList->SelectTabByID(TabId, true);
	}
}

FName UPUJournalWidget::GetActiveTabId() const
{
	if (TabList)
	{
		return TabList->GetActiveTab();
	}
	return NAME_None;
}

bool UPUJournalWidget::CycleRecipesDish(int32 Direction)
{
	const FName ResolvedRecipes = ResolveRecipesTabId();
	if (ResolvedRecipes == NAME_None || GetActiveTabId() != ResolvedRecipes) return false;

	UWorld* World = GetWorld();
	if (!World) return false;

	UPUProjectUmeowmiGameInstance* GI = World->GetGameInstance<UPUProjectUmeowmiGameInstance>();
	if (!GI) return false;

	const FGameplayTag NewTag = GI->CycleJournalDish(Direction);
	if (!NewTag.IsValid()) return false;

	UPURecipesSectionWidget* RecipesSection = GetRecipesSection();
	if (RecipesSection)
	{
		RecipesSection->DisplayDishByTag(NewTag);
	}
	return true;
}

void UPUJournalWidget::ShowDishInRecipesTab(const FGameplayTag& DishTag)
{
	if (!DishTag.IsValid())
	{
		return;
	}
	if (UWorld* World = GetWorld())
	{
		if (UPUProjectUmeowmiGameInstance* GI = World->GetGameInstance<UPUProjectUmeowmiGameInstance>())
		{
			GI->SetCurrentDishTag(DishTag);
		}
	}
	if (const FName RecipesTab = ResolveRecipesTabId(); RecipesTab != NAME_None)
	{
		SwitchToTabById(RecipesTab);
	}
	if (UPURecipesSectionWidget* RecipesSection = GetRecipesSection())
	{
		RecipesSection->DisplayDishByTag(DishTag);
	}
}

UPURecipesSectionWidget* UPUJournalWidget::GetRecipesSection() const
{
	if (RecipesTabId != NAME_None)
	{
		for (int32 i = 0; i < SectionWidgets.Num(); ++i)
		{
			if (SectionTabIds.IsValidIndex(i) && SectionTabIds[i] == RecipesTabId)
			{
				if (UPURecipesSectionWidget* W = Cast<UPURecipesSectionWidget>(SectionWidgets[i]))
				{
					return W;
				}
			}
		}
	}
	for (int32 i = 0; i < SectionWidgets.Num(); ++i)
	{
		if (UPURecipesSectionWidget* W = Cast<UPURecipesSectionWidget>(SectionWidgets[i]))
		{
			return W;
		}
	}
	return nullptr;
}

FName UPUJournalWidget::ResolveRecipesTabId() const
{
	if (RecipesTabId != NAME_None)
	{
		return RecipesTabId;
	}
	for (int32 i = 0; i < SectionWidgets.Num(); ++i)
	{
		if (Cast<UPURecipesSectionWidget>(SectionWidgets[i]))
		{
			return SectionTabIds.IsValidIndex(i) ? SectionTabIds[i] : NAME_None;
		}
	}
	return NAME_None;
}

bool UPUJournalWidget::HasTabId(FName TabId) const
{
	return SectionTabIds.Contains(TabId);
}

FName UPUJournalWidget::GetEffectiveDefaultTabId() const
{
	if (DefaultTabId != NAME_None && SectionTabIds.Contains(DefaultTabId))
	{
		return DefaultTabId;
	}
	if (SectionTabIds.Num() > 0)
	{
		return SectionTabIds[0];
	}
	return NAME_None;
}

void UPUJournalWidget::RegisterJournalTabs()
{
	// Super::NativeConstruct already ran the Journal Blueprint Construct — that graph may have called RegisterTab
	// on the tab list. Clear Common UI state and switcher slots before we apply Journal Tabs, and do this even
	// when TabButtonClass is unset so we never leave stale tabs or designer placeholders visible.
	if (TabList)
	{
		TabList->RemoveAllTabs();
	}
	if (ContentSwitcher)
	{
		ContentSwitcher->ClearChildren();
	}
	if (TabButtonsContainer)
	{
		TabButtonsContainer->ClearChildren();
	}

	SectionWidgets.Empty();
	SectionTabIds.Empty();
	ResolvedTabEntries.Empty();

	if (!TabList || !ContentSwitcher || !TabButtonClass)
	{
		return;
	}

	// Pass container and direct reference to this Journal - TabList reads padding from us at add-time
	if (TabButtonsContainer)
	{
		TabList->SetTabButtonsContainer(TabButtonsContainer);
	}
	TabList->SetOwningJournal(this);

	// Link tab list to switcher
	TabList->SetLinkedSwitcher(ContentSwitcher);

	// Disable transition animation so tab switch is instant (no fade-out-then-fade-in gap)
	ContentSwitcher->SetDisableTransitionAnimation(true);

	// Bind to set tab labels when buttons are created
	TabList->OnTabButtonCreation.AddDynamic(this, &UPUJournalWidget::OnTabButtonCreated);

	if (JournalTabs.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("PUJournalWidget: Journal Tabs is empty — add tab rows on the journal widget class defaults."));
		return;
	}

	const TArray<FPUJournalTabEntry>& TabsToRegister = JournalTabs;
	TSet<FName> UsedTabIds;
	int32 TabIndex = 0;

	for (int32 RowIndex = 0; RowIndex < TabsToRegister.Num(); ++RowIndex)
	{
		const FPUJournalTabEntry& Entry = TabsToRegister[RowIndex];
		if (!Entry.SectionWidgetClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("PUJournalWidget: Journal Tabs row %d skipped — Section Widget Class is not set (Tab Id would be '%s')."),
				RowIndex, Entry.TabId.IsNone() ? TEXT("(none)") : *Entry.TabId.ToString());
			continue;
		}
		if (Entry.TabId.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("PUJournalWidget: Journal Tabs row %d skipped — Tab Id is empty (set a non-None name in the row)."), RowIndex);
			continue;
		}
		if (UsedTabIds.Contains(Entry.TabId))
		{
			UE_LOG(LogTemp, Warning, TEXT("PUJournalWidget: Journal Tabs row %d duplicate Tab Id '%s' ignored."), RowIndex, *Entry.TabId.ToString());
			continue;
		}
		UsedTabIds.Add(Entry.TabId);

		UUserWidget* SectionWidget = CreateAndAddSectionWidget(Entry.SectionWidgetClass);
		if (!SectionWidget)
		{
			UE_LOG(LogTemp, Warning, TEXT("PUJournalWidget: failed to create section widget for tab '%s' (class %s)."),
				*Entry.TabId.ToString(), *GetNameSafe(Entry.SectionWidgetClass.Get()));
			continue;
		}

		if (!TabList->RegisterTab(Entry.TabId, TabButtonClass, SectionWidget, TabIndex))
		{
			UE_LOG(LogTemp, Error, TEXT("PUJournalWidget: RegisterTab failed for '%s' (duplicate id or Common UI error). Removing section widget."), *Entry.TabId.ToString());
			SectionWidget->RemoveFromParent();
			continue;
		}

		SectionWidgets.Add(SectionWidget);
		SectionTabIds.Add(Entry.TabId);
		ResolvedTabEntries.Add(Entry);
		++TabIndex;
	}

	if (TabIndex == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("PUJournalWidget: no journal tabs registered — %d row(s) in Journal Tabs but none succeeded. Each row needs a non-empty Tab Id and a valid Section Widget Class; see warnings above."),
			TabsToRegister.Num());
		return;
	}

	const FName DefaultId = GetEffectiveDefaultTabId();
	if (DefaultId != NAME_None)
	{
		TabList->SelectTabByID(DefaultId, true);
	}
}

void UPUJournalWidget::OnTabButtonCreated(FName TabId, UCommonButtonBase* TabButton)
{
	if (!TabButton) return;

	const FText DisplayText = GetTabDisplayText(TabId);
	UUserWidget* ButtonWidget = Cast<UUserWidget>(TabButton);
	if (!ButtonWidget || !ButtonWidget->WidgetTree) return;

	auto TrySetTextBlock = [&](UTextBlock* TextBlock) -> bool
	{
		if (TextBlock)
		{
			TextBlock->SetText(DisplayText);
			return true;
		}
		return false;
	};

	bool bLabelSet = false;

	// 1. If TabButtonLabelWidgetName is set, find that specific widget
	if (TabButtonLabelWidgetName != NAME_None)
	{
		if (UWidget* NamedWidget = ButtonWidget->WidgetTree->FindWidget(TabButtonLabelWidgetName))
		{
			bLabelSet = TrySetTextBlock(Cast<UTextBlock>(NamedWidget));
		}
	}

	// 2. Try common TextBlock names
	if (!bLabelSet)
	{
		static const FName CommonNames[] = { TEXT("ButtonLabel"), TEXT("ButtonText"), TEXT("TabLabel"), TEXT("LabelText"), TEXT("TextBlock"), TEXT("Label") };
		for (const FName& Name : CommonNames)
		{
			if (UWidget* NamedWidget = ButtonWidget->WidgetTree->FindWidget(Name))
			{
				if (UTextBlock* TextBlock = Cast<UTextBlock>(NamedWidget))
				{
					bLabelSet = TrySetTextBlock(TextBlock);
					break;
				}
			}
		}
	}

	// 3. Fall back to first TextBlock in widget tree
	if (!bLabelSet)
	{
		TArray<UWidget*> AllWidgets;
		ButtonWidget->WidgetTree->GetAllWidgets(AllWidgets);
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

FText UPUJournalWidget::GetTabDisplayText(FName TabId) const
{
	for (const FPUJournalTabEntry& Entry : ResolvedTabEntries)
	{
		if (Entry.TabId == TabId && !Entry.DisplayNameOverride.IsEmpty())
		{
			return Entry.DisplayNameOverride;
		}
	}

	if (TabId == FName(TEXT("People")))
	{
		return FText::FromString(TEXT("Villagers"));
	}

	// Default: use tab ID with first letter capitalized
	FString Str = TabId.ToString();
	if (Str.Len() > 0)
	{
		Str[0] = FChar::ToUpper(Str[0]);
	}
	return FText::FromString(Str);
}

UUserWidget* UPUJournalWidget::CreateAndAddSectionWidget(TSubclassOf<UUserWidget> WidgetClass)
{
	if (!WidgetClass || !ContentSwitcher) return nullptr;

	// Use this journal as the owning widget so CreateWidget works during NativeConstruct when GetOwningPlayer() is often still null.
	UUserWidget* Widget = CreateWidget<UUserWidget>(this, WidgetClass);
	if (!Widget) return nullptr;

	ContentSwitcher->AddChild(Widget);
	return Widget;
}
