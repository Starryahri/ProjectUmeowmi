// Copyright Epic Games, Inc. All Rights Reserved.

#include "PURecipeIngredientEntryWidget.h"
#include "Components/CheckBox.h"
#include "Components/TextBlock.h"

UPURecipeIngredientEntryWidget::UPURecipeIngredientEntryWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UPURecipeIngredientEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UpdateDisplay();
}

void UPURecipeIngredientEntryWidget::SetData(const FText& InIngredientName, bool bChecked)
{
	IngredientName = InIngredientName;
	UpdateDisplay();
	if (IngredientCheckBox)
	{
		IngredientCheckBox->SetIsChecked(bChecked);
	}
}

bool UPURecipeIngredientEntryWidget::IsChecked() const
{
	return IngredientCheckBox ? IngredientCheckBox->IsChecked() : false;
}

void UPURecipeIngredientEntryWidget::SetChecked(bool bChecked)
{
	if (IngredientCheckBox)
	{
		IngredientCheckBox->SetIsChecked(bChecked);
	}
}

void UPURecipeIngredientEntryWidget::UpdateDisplay()
{
	if (IngredientNameText)
	{
		IngredientNameText->SetText(IngredientName);
	}
}
