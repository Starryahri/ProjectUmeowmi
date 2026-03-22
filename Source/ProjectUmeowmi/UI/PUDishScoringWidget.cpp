#include "PUDishScoringWidget.h"
#include "../ProjectUmeowmiCharacter.h"

UPUDishScoringWidget::UPUDishScoringWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UPUDishScoringWidget::SetDishScoringOwnerCharacter(AProjectUmeowmiCharacter* InCharacter)
{
	DishScoringOwnerCharacter = InCharacter;
}

void UPUDishScoringWidget::RequestEndDishScoringMode()
{
	if (AProjectUmeowmiCharacter* C = DishScoringOwnerCharacter.Get())
	{
		C->EndDishScoringMode();
	}
}

void UPUDishScoringWidget::OnEnteredDishScoringMode_Implementation()
{
}

void UPUDishScoringWidget::OnExitingDishScoringMode_Implementation()
{
}
