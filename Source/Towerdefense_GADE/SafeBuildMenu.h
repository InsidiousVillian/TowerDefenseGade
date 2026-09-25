#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SafeBuildMenu.generated.h"

class UButton;
class UTextBlock;

/**
 * Native parent for WBP_BuildMenu.
 * BindWidget names match the existing widget names in WBP_BuildMenu so the
 * buy buttons wire up in C++ with no Blueprint graph work.
 */
UCLASS()
class TOWERDEFENSE_GADE_API USafeBuildMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION()
	void BuyTower();

	UFUNCTION()
	void BuyMortar();

	UFUNCTION()
	void BuyInfantry();

protected:
	/** Exact names from WBP_BuildMenu.uasset. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Buydefender;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Buydefender_1;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Buydefender_2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_118;

private:
	void BindBuyButtons();
	void CacheButtonLabels();
	void ApplyAffordability();
	void TryBuy(FName SpawnFunctionName, double Cost);
	double ReadPlayerCurrency() const;
	void WritePlayerCurrency(double NewValue) const;
	AActor* GetParentSocket() const;
	UTextBlock* FindLabelOnButton(UButton* Button) const;

	TMap<TObjectPtr<UButton>, FText> OriginalButtonLabels;

	static constexpr double TowerCost = 50.0;
	static constexpr double MortarCost = 100.0;
	static constexpr double InfantryCost = 150.0;
};
