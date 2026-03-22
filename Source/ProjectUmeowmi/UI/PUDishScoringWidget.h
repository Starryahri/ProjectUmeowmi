#pragma once

#include "CoreMinimal.h"
#include "PUCommonUserWidget.h"
#include "PUDishScoringWidget.generated.h"

class AProjectUmeowmiCharacter;

/**
 * Root widget for dish scoring mode (animations, nested scorecard sections, etc.).
 * The player character calls BeginDishScoringModeWithWidget with this widget to show it and track scoring state.
 */
UCLASS(Blueprintable, BlueprintType)
class PROJECTUMEOWMI_API UPUDishScoringWidget : public UPUCommonUserWidget
{
	GENERATED_BODY()

public:
	UPUDishScoringWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Dish Scoring")
	AProjectUmeowmiCharacter* GetDishScoringOwnerCharacter() const { return DishScoringOwnerCharacter.Get(); }

	/** Ends dish scoring mode on the owner character (removes this widget from the viewport when applicable). */
	UFUNCTION(BlueprintCallable, Category = "Dish Scoring")
	void RequestEndDishScoringMode();

	/** Called after this widget is on the viewport at the dish-scoring layer and the owner character is set. */
	UFUNCTION(BlueprintNativeEvent, Category = "Dish Scoring")
	void OnEnteredDishScoringMode();
	virtual void OnEnteredDishScoringMode_Implementation();

	/** Called when leaving dish scoring (before RemoveFromParent). Owner character is still valid until this returns. */
	UFUNCTION(BlueprintNativeEvent, Category = "Dish Scoring")
	void OnExitingDishScoringMode();
	virtual void OnExitingDishScoringMode_Implementation();

	void SetDishScoringOwnerCharacter(AProjectUmeowmiCharacter* InCharacter);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Dish Scoring")
	TObjectPtr<AProjectUmeowmiCharacter> DishScoringOwnerCharacter;
};
