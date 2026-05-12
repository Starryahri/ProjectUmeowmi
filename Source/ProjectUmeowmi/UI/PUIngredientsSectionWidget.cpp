// Copyright Epic Games, Inc. All Rights Reserved.

#include "PUIngredientsSectionWidget.h"
#include "PUJournalAspectRowWidget.h"
#include "PUJournalSlotWidget.h"
#include "PUJournalWidget.h"
#include "../PUProjectUmeowmiGameInstance.h"
#include "../DishCustomization/PUDishBase.h"
#include "../DishCustomization/PUDishBlueprintLibrary.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Input/NavigationReply.h"
#include "TimerManager.h"
#include "Types/SlateEnums.h"

namespace
{
	void ApplyUniformGridSlotLayout(UUniformGridSlot* GridSlot, int32 Row, int32 Column)
	{
		if (!GridSlot)
		{
			return;
		}
		GridSlot->SetRow(Row);
		GridSlot->SetColumn(Column);
		GridSlot->SetHorizontalAlignment(HAlign_Fill);
		GridSlot->SetVerticalAlignment(VAlign_Fill);
	}

	bool FindIngredientRow(UDataTable* Table, const FGameplayTag& Tag, FPUIngredientBase& OutRow)
	{
		if (!Table || !Tag.IsValid())
		{
			return false;
		}
		for (const FName& RowName : Table->GetRowNames())
		{
			if (FPUIngredientBase* Row = Table->FindRow<FPUIngredientBase>(RowName, TEXT("FindIngredientRow")))
			{
				if (Row->IngredientTag == Tag)
				{
					OutRow = *Row;
					return true;
				}
			}
		}
		return false;
	}
}

UPUIngredientsSectionWidget::UPUIngredientsSectionWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UPUIngredientsSectionWidget::OnSectionActivated_Implementation()
{
	Super::OnSectionActivated_Implementation();
	RefreshIngredientsGrid();
}

void UPUIngredientsSectionWidget::RefreshIngredientsGrid(bool bApplyDefaultDetailAndFocus)
{
	if (!IngredientsGrid)
	{
		return;
	}

	IngredientsGrid->ClearChildren();

	if (GridCellMinWidth > 0.f)
	{
		IngredientsGrid->SetMinDesiredSlotWidth(GridCellMinWidth);
	}
	if (GridCellMinHeight > 0.f)
	{
		IngredientsGrid->SetMinDesiredSlotHeight(GridCellMinHeight);
	}

	if (!JournalSlotClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UPUProjectUmeowmiGameInstance* GI = World->GetGameInstance<UPUProjectUmeowmiGameInstance>();
	if (!GI)
	{
		return;
	}

	UDataTable* Table = GI->GetIngredientDataTable();
	if (!Table)
	{
		return;
	}

	const int32 NumCols = FMath::Max(1, GridNumColumns);

	auto SortByDisplayName = [](TArray<FPUIngredientBase>& Arr) {
		Arr.Sort([](const FPUIngredientBase& A, const FPUIngredientBase& B) {
			return A.DisplayName.CompareTo(B.DisplayName) < 0;
		});
	};

	// --- Fixed slot count: pack unlocked first, then locked (no gaps), then empty padding ---
	if (TotalGridSlots > 0)
	{
		TArray<FPUIngredientBase> AllRows;
		for (const FName& RowName : Table->GetRowNames())
		{
			if (FPUIngredientBase* Row = Table->FindRow<FPUIngredientBase>(RowName, TEXT("RefreshIngredientsGrid")))
			{
				if (!Row->IngredientTag.IsValid())
				{
					continue;
				}
				AllRows.Add(*Row);
			}
		}
		SortByDisplayName(AllRows);

		TArray<FPUIngredientBase> UnlockedRows;
		TArray<FPUIngredientBase> LockedRows;
		UnlockedRows.Reserve(AllRows.Num());
		LockedRows.Reserve(AllRows.Num());
		for (const FPUIngredientBase& Row : AllRows)
		{
			if (GI->IsIngredientUnlocked(Row.IngredientTag))
			{
				UnlockedRows.Add(Row);
			}
			else
			{
				LockedRows.Add(Row);
			}
		}

		TArray<FPUIngredientBase> PackedOrder;
		PackedOrder.Reserve(UnlockedRows.Num() + LockedRows.Num());
		PackedOrder.Append(UnlockedRows);
		PackedOrder.Append(LockedRows);

		const int32 SlotCount = TotalGridSlots;
		for (int32 Index = 0; Index < SlotCount; ++Index)
		{
			UPUJournalSlotWidget* JournalSlot = CreateWidget<UPUJournalSlotWidget>(this, JournalSlotClass);
			if (!JournalSlot)
			{
				continue;
			}

			if (Index >= PackedOrder.Num())
			{
				JournalSlot->ConfigureIngredientSlot(FGameplayTag(), nullptr, true, false);
			}
			else
			{
				const FPUIngredientBase& IngredientRow = PackedOrder[Index];
				const bool bUnlocked = GI->IsIngredientUnlocked(IngredientRow.IngredientTag);
				UTexture2D* Icon = IngredientRow.PantryTexture ? IngredientRow.PantryTexture : IngredientRow.PreviewTexture;
				if (bUnlocked)
				{
					JournalSlot->ConfigureIngredientSlot(IngredientRow.IngredientTag, Icon, false, false);
					JournalSlot->OnEntryHovered.AddDynamic(this, &UPUIngredientsSectionWidget::OnJournalSlotHovered);
				}
				else
				{
					JournalSlot->ConfigureIngredientSlot(IngredientRow.IngredientTag, nullptr, false, true);
				}
			}

			if (UUniformGridSlot* GridSlot = IngredientsGrid->AddChildToUniformGrid(JournalSlot))
			{
				ApplyUniformGridSlotLayout(GridSlot, Index / NumCols, Index % NumCols);
			}
		}

		SetupIngredientsGridNavigation();
		if (bApplyDefaultDetailAndFocus)
		{
			if (bSelectFirstIngredientOnRefresh)
			{
				if (UnlockedRows.Num() > 0)
				{
					ShowIngredientDetail(UnlockedRows[0].IngredientTag);
				}
				else
				{
					ShowIngredientDetail(FGameplayTag());
				}
			}
			if (bFocusFirstGridSlotOnRefresh)
			{
				ScheduleFocusFirstIngredientsGridSlot();
			}
		}
		return;
	}

	// --- Legacy: one slot per ingredient row that passes the unlock filter ---
	TArray<FPUIngredientBase> Rows;
	for (const FName& RowName : Table->GetRowNames())
	{
		if (FPUIngredientBase* Row = Table->FindRow<FPUIngredientBase>(RowName, TEXT("RefreshIngredientsGrid")))
		{
			if (!Row->IngredientTag.IsValid())
			{
				continue;
			}
			if (bOnlyShowUnlockedIngredients && !GI->IsIngredientUnlocked(Row->IngredientTag))
			{
				continue;
			}
			Rows.Add(*Row);
		}
	}

	SortByDisplayName(Rows);

	int32 Index = 0;
	for (const FPUIngredientBase& IngredientRow : Rows)
	{
		UPUJournalSlotWidget* JournalSlot = CreateWidget<UPUJournalSlotWidget>(this, JournalSlotClass);
		if (!JournalSlot)
		{
			continue;
		}

		UTexture2D* Icon = IngredientRow.PantryTexture ? IngredientRow.PantryTexture : IngredientRow.PreviewTexture;
		JournalSlot->ConfigureIngredientSlot(IngredientRow.IngredientTag, Icon, false, false);
		JournalSlot->OnEntryHovered.AddDynamic(this, &UPUIngredientsSectionWidget::OnJournalSlotHovered);

		if (UUniformGridSlot* GridSlot = IngredientsGrid->AddChildToUniformGrid(JournalSlot))
		{
			ApplyUniformGridSlotLayout(GridSlot, Index / NumCols, Index % NumCols);
		}
		++Index;
	}

	SetupIngredientsGridNavigation();

	if (bApplyDefaultDetailAndFocus)
	{
		if (bSelectFirstIngredientOnRefresh)
		{
			if (Rows.Num() > 0)
			{
				ShowIngredientDetail(Rows[0].IngredientTag);
			}
			else
			{
				ShowIngredientDetail(FGameplayTag());
			}
		}
		if (bFocusFirstGridSlotOnRefresh)
		{
			ScheduleFocusFirstIngredientsGridSlot();
		}
	}
}

void UPUIngredientsSectionWidget::SetupIngredientsGridNavigation()
{
	if (!IngredientsGrid)
	{
		return;
	}
	const int32 NumCols = FMath::Max(1, GridNumColumns);
	const int32 NumChildren = IngredientsGrid->GetChildrenCount();
	if (NumChildren == 0)
	{
		return;
	}

	for (int32 i = 0; i < NumChildren; ++i)
	{
		UPUJournalSlotWidget* Cell = Cast<UPUJournalSlotWidget>(IngredientsGrid->GetChildAt(i));
		if (!Cell || !Cell->GetIsEnabled())
		{
			continue;
		}

		const int32 Col = i % NumCols;

		auto WireDirection = [this, NumChildren](UPUJournalSlotWidget* From, EUINavigation Direction, int32 NeighborIndex)
		{
			if (NeighborIndex < 0 || NeighborIndex >= NumChildren)
			{
				From->SetNavigationRuleBase(Direction, EUINavigationRule::Stop);
				return;
			}
			if (UPUJournalSlotWidget* Neighbor = Cast<UPUJournalSlotWidget>(IngredientsGrid->GetChildAt(NeighborIndex)))
			{
				if (Neighbor->GetIsEnabled())
				{
					From->SetNavigationRuleExplicit(Direction, Neighbor);
				}
				else
				{
					From->SetNavigationRuleBase(Direction, EUINavigationRule::Stop);
				}
			}
			else
			{
				From->SetNavigationRuleBase(Direction, EUINavigationRule::Stop);
			}
		};

		if (i >= NumCols)
		{
			WireDirection(Cell, EUINavigation::Up, i - NumCols);
		}
		else
		{
			Cell->SetNavigationRuleBase(EUINavigation::Up, EUINavigationRule::Stop);
		}

		if (i + NumCols < NumChildren)
		{
			WireDirection(Cell, EUINavigation::Down, i + NumCols);
		}
		else
		{
			Cell->SetNavigationRuleBase(EUINavigation::Down, EUINavigationRule::Stop);
		}

		if (Col > 0)
		{
			WireDirection(Cell, EUINavigation::Left, i - 1);
		}
		else
		{
			Cell->SetNavigationRuleBase(EUINavigation::Left, EUINavigationRule::Stop);
		}

		if (Col < NumCols - 1 && i + 1 < NumChildren)
		{
			WireDirection(Cell, EUINavigation::Right, i + 1);
		}
		else
		{
			Cell->SetNavigationRuleBase(EUINavigation::Right, EUINavigationRule::Stop);
		}
	}
}

void UPUIngredientsSectionWidget::ScheduleFocusFirstIngredientsGridSlot()
{
	if (!IngredientsGrid)
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UPUIngredientsSectionWidget::TryFocusFirstInteractableIngredientsSlot));
}

void UPUIngredientsSectionWidget::ScheduleFocusIngredientGridSlot(const FGameplayTag& IngredientTag)
{
	if (!IngredientTag.IsValid() || !IngredientsGrid)
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const FGameplayTag TagCopy = IngredientTag;
	World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([WeakThis = TWeakObjectPtr<UPUIngredientsSectionWidget>(this), TagCopy]()
	{
		if (UPUIngredientsSectionWidget* Self = WeakThis.Get())
		{
			Self->TryFocusIngredientGridSlot(TagCopy);
		}
	}));
}

void UPUIngredientsSectionWidget::TryFocusIngredientGridSlot(const FGameplayTag& IngredientTag)
{
	if (!IngredientTag.IsValid() || !IngredientsGrid)
	{
		return;
	}
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}
	const int32 Num = IngredientsGrid->GetChildrenCount();
	for (int32 i = 0; i < Num; ++i)
	{
		UPUJournalSlotWidget* Cell = Cast<UPUJournalSlotWidget>(IngredientsGrid->GetChildAt(i));
		if (!Cell || Cell->GetEntryTag() != IngredientTag)
		{
			continue;
		}
		if (!Cell->GetIsEnabled() || !Cell->IsVisible() || Cell->IsSlotEmpty())
		{
			continue;
		}
		Cell->SetUserFocus(PC);
		Cell->ApplySlateHoverForGamepadFocus();
		break;
	}
}

void UPUIngredientsSectionWidget::TryFocusFirstInteractableIngredientsSlot()
{
	if (!IngredientsGrid)
	{
		return;
	}
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}
	const int32 Num = IngredientsGrid->GetChildrenCount();
	for (int32 i = 0; i < Num; ++i)
	{
		if (UPUJournalSlotWidget* Cell = Cast<UPUJournalSlotWidget>(IngredientsGrid->GetChildAt(i)))
		{
			if (Cell->GetIsEnabled() && Cell->IsVisible())
			{
				Cell->SetUserFocus(PC);
				// Slate hover attribute is not set by NativeOnFocusReceived alone when focus lands on first tick.
				Cell->ApplySlateHoverForGamepadFocus();
				return;
			}
		}
	}
}

void UPUIngredientsSectionWidget::ShowIngredientDetail(const FGameplayTag& IngredientTag)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UPUProjectUmeowmiGameInstance* GI = World->GetGameInstance<UPUProjectUmeowmiGameInstance>();
	if (!GI)
	{
		return;
	}

	FPUIngredientBase Row;
	const bool bFound = FindIngredientRow(GI->GetIngredientDataTable(), IngredientTag, Row);
	ApplyIngredientDetail(IngredientTag, Row, bFound);
	if (bFound)
	{
		PopulateIngredientRecipesForIngredient(IngredientTag);
		RefreshIngredientJournalAspectRows(Row);
	}
	else
	{
		ClearIngredientRecipesList();
		ClearIngredientJournalAspectRows();
	}
}

void UPUIngredientsSectionWidget::ApplyIngredientDetail_Implementation(
	const FGameplayTag& IngredientTag,
	const FPUIngredientBase& IngredientData,
	bool bFoundInTable)
{
	if (!bFoundInTable)
	{
		if (IngredientTitleText)
		{
			IngredientTitleText->SetText(FText::GetEmpty());
		}
		if (IngredientDescriptionText)
		{
			IngredientDescriptionText->SetText(FText::GetEmpty());
		}
		if (IngredientDetailImage)
		{
			IngredientDetailImage->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}

	if (IngredientTitleText)
	{
		FText Title = IngredientData.DisplayName;
		if (Title.IsEmpty())
		{
			Title = FText::FromName(IngredientTag.GetTagName());
		}
		IngredientTitleText->SetText(Title);
	}

	if (IngredientDescriptionText)
	{
		IngredientDescriptionText->SetText(IngredientData.IngredientFlavorText);
	}

	if (IngredientDetailImage)
	{
		UTexture2D* Tex = IngredientData.PreviewTexture ? IngredientData.PreviewTexture : IngredientData.PantryTexture;
		if (Tex)
		{
			IngredientDetailImage->SetBrushFromTexture(Tex);
			IngredientDetailImage->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			IngredientDetailImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UPUIngredientsSectionWidget::OnJournalSlotHovered(FGameplayTag EntryTag)
{
	ShowIngredientDetail(EntryTag);
}

void UPUIngredientsSectionWidget::ClearIngredientRecipesList()
{
	if (IngredientRecipesScrollBox)
	{
		IngredientRecipesScrollBox->ClearChildren();
	}
}

void UPUIngredientsSectionWidget::ClearIngredientJournalAspectRows()
{
	if (IngredientFlavorAspectsContainer)
	{
		IngredientFlavorAspectsContainer->ClearChildren();
	}
	if (IngredientTextureAspectsContainer)
	{
		IngredientTextureAspectsContainer->ClearChildren();
	}
}

void UPUIngredientsSectionWidget::RefreshIngredientJournalAspectRows(const FPUIngredientBase& IngredientData)
{
	ClearIngredientJournalAspectRows();

	if (!IngredientFlavorAspectsContainer && !IngredientTextureAspectsContainer)
	{
		return;
	}

	TSubclassOf<UPUJournalAspectRowWidget> RowClass = JournalAspectRowClass;
	if (!RowClass)
	{
		RowClass = UPUJournalAspectRowWidget::StaticClass();
	}

	static const FName FlavorAspectNames[] = {
		FName(TEXT("Umami")),
		FName(TEXT("Salt")),
		FName(TEXT("Sweet")),
		FName(TEXT("Sour")),
		FName(TEXT("Bitter")),
		FName(TEXT("Spicy")),
	};

	static const FName TextureAspectNames[] = {
		FName(TEXT("Rich")),
		FName(TEXT("Juicy")),
		FName(TEXT("Tender")),
		FName(TEXT("Chewy")),
		FName(TEXT("Crispy")),
		FName(TEXT("Crumbly")),
	};

	auto AddRow = [&](UPanelWidget* Container, FName AspectName, float RawValue)
	{
		if (!Container)
		{
			return;
		}
		if (!bShowIngredientAspectRowsWithZeroValue && FMath::IsNearlyZero(RawValue))
		{
			return;
		}
		const int32 Stars = FMath::Clamp(FMath::RoundToInt(RawValue), 0, 5);
		UPUJournalAspectRowWidget* Row = CreateWidget<UPUJournalAspectRowWidget>(this, RowClass);
		if (!Row)
		{
			return;
		}
		Row->SetAspectNameAndStarRating(AspectName, Stars);
		Container->AddChild(Row);
	};

	if (IngredientFlavorAspectsContainer)
	{
		for (const FName& AspectName : FlavorAspectNames)
		{
			AddRow(IngredientFlavorAspectsContainer, AspectName, IngredientData.GetFlavorAspect(AspectName));
		}
	}

	if (IngredientTextureAspectsContainer)
	{
		for (const FName& AspectName : TextureAspectNames)
		{
			AddRow(IngredientTextureAspectsContainer, AspectName, IngredientData.GetTextureAspect(AspectName));
		}
	}
}

void UPUIngredientsSectionWidget::PopulateIngredientRecipesForIngredient(const FGameplayTag& IngredientTag)
{
	if (!IngredientRecipesScrollBox || !JournalSlotClass || !IngredientTag.IsValid())
	{
		ClearIngredientRecipesList();
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		ClearIngredientRecipesList();
		return;
	}

	UPUProjectUmeowmiGameInstance* GI = World->GetGameInstance<UPUProjectUmeowmiGameInstance>();
	if (!GI)
	{
		ClearIngredientRecipesList();
		return;
	}

	UDataTable* DishTable = GI->GetDishDataTable();
	if (!DishTable)
	{
		ClearIngredientRecipesList();
		return;
	}

	struct FDishSortEntry
	{
		FGameplayTag DishTag;
		FText DisplayName;
	};

	TArray<FDishSortEntry> Entries;

	for (const FName& RowName : DishTable->GetRowNames())
	{
		FPUDishBase* Row = DishTable->FindRow<FPUDishBase>(RowName, TEXT("PopulateIngredientRecipes"));
		if (!Row || !Row->DishTag.IsValid())
		{
			continue;
		}
		if (!Row->HasIngredient(IngredientTag))
		{
			continue;
		}

		const bool bUnlocked = GI->IsDishUnlocked(Row->DishTag);
		if (bOnlyShowUnlockedDishesInIngredientRecipeList && !bUnlocked)
		{
			continue;
		}

		FDishSortEntry E;
		E.DishTag = Row->DishTag;
		E.DisplayName = Row->GetCurrentDisplayName();
		if (E.DisplayName.IsEmpty())
		{
			E.DisplayName = FText::FromName(E.DishTag.GetTagName());
		}
		Entries.Add(E);
	}

	Entries.Sort([](const FDishSortEntry& A, const FDishSortEntry& B) {
		return A.DisplayName.CompareTo(B.DisplayName) < 0;
	});

	IngredientRecipesScrollBox->ClearChildren();

	for (const FDishSortEntry& E : Entries)
	{
		FPUDishBase DishData;
		if (!GI->GetDishDataForTag(E.DishTag, DishData))
		{
			continue;
		}

		const bool bUnlocked = GI->IsDishUnlocked(E.DishTag);

		UPUJournalSlotWidget* RecipeSlot = CreateWidget<UPUJournalSlotWidget>(this, JournalSlotClass);
		if (!RecipeSlot)
		{
			continue;
		}

		UTexture2D* Icon = UPUDishBlueprintLibrary::GetLoadedJournalTexture(DishData);
		if (!Icon)
		{
			Icon = UPUDishBlueprintLibrary::GetLoadedPreviewTexture(DishData);
		}

		RecipeSlot->ConfigureIngredientSlot(E.DishTag, Icon, false, !bUnlocked);
		RecipeSlot->OnEntryHovered.AddDynamic(this, &UPUIngredientsSectionWidget::OnIngredientRecipeDishSlotHovered);
		IngredientRecipesScrollBox->AddChild(RecipeSlot);
	}
}

void UPUIngredientsSectionWidget::OnIngredientRecipeDishSlotHovered(FGameplayTag DishTag)
{
	if (!DishTag.IsValid())
	{
		return;
	}
	if (UPUJournalWidget* Journal = GetTypedOuter<UPUJournalWidget>())
	{
		Journal->ShowDishInRecipesTab(DishTag);
	}
}
