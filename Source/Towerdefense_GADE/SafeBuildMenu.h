#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SafeBuildMenu.generated.h"

class UButton;

/**
 * Native parent for WBP_BuildMenu.
 * Hides the shop when gold is below the cheapest defender, disables buttons
 * the player cannot afford, and charges the same amount each button checks.
 */
UCLASS()
class TOWERDEFENSE_GADE_API USafeBuildMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION()
	void BuyTower();

	UFUNCTION()
	void BuyMortar();

	UFUNCTION()
	void BuyInfantry();

private:
	void BindBuyButtons();
	void ApplyAffordability();
	void TryBuy(FName SpawnFunctionName, double Cost);
	double ReadPlayerCurrency() const;
	void WritePlayerCurrency(double NewValue) const;
	AActor* GetParentSocket() const;

	bool bBuyButtonsBound = false;

	static constexpr double TowerCost = 50.0;
	static constexpr double MortarCost = 100.0;
	static constexpr double InfantryCost = 150.0;
};
