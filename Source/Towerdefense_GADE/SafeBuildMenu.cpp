#include "SafeBuildMenu.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"

void USafeBuildMenu::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bBuyButtonsBound)
	{
		return;
	}

	bBuyButtonsBound = true;
	BindBuyButtons();
	ApplyAffordability();
}

void USafeBuildMenu::BindBuyButtons()
{
	if (UButton* TowerButton = Cast<UButton>(GetWidgetFromName(TEXT("Buydefender"))))
	{
		TowerButton->OnClicked.Clear();
		TowerButton->OnClicked.AddDynamic(this, &USafeBuildMenu::BuyTower);
	}

	if (UButton* MortarButton = Cast<UButton>(GetWidgetFromName(TEXT("Buydefender_1"))))
	{
		MortarButton->OnClicked.Clear();
		MortarButton->OnClicked.AddDynamic(this, &USafeBuildMenu::BuyMortar);
	}

	if (UButton* InfantryButton = Cast<UButton>(GetWidgetFromName(TEXT("Buydefender_2"))))
	{
		InfantryButton->OnClicked.Clear();
		InfantryButton->OnClicked.AddDynamic(this, &USafeBuildMenu::BuyInfantry);
	}
}

void USafeBuildMenu::ApplyAffordability()
{
	const double Gold = ReadPlayerCurrency();
	const bool bCanAffordAny = Gold + KINDA_SMALL_NUMBER >= TowerCost;

	struct FOffer
	{
		const TCHAR* WidgetName;
		double Cost;
	};

	const FOffer Offers[] = {
		{TEXT("Buydefender"), TowerCost},
		{TEXT("Buydefender_1"), MortarCost},
		{TEXT("Buydefender_2"), InfantryCost},
	};

	for (const FOffer& Offer : Offers)
	{
		if (UButton* Button = Cast<UButton>(GetWidgetFromName(FName(Offer.WidgetName))))
		{
			const bool bShow = bCanAffordAny && Gold + KINDA_SMALL_NUMBER >= Offer.Cost;
			Button->SetIsEnabled(bShow);
			Button->SetVisibility(bCanAffordAny ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
	}

	if (!bCanAffordAny && WidgetTree)
	{
		WidgetTree->ForEachWidget([](UWidget* Widget)
		{
			if (UTextBlock* Text = Cast<UTextBlock>(Widget))
			{
				Text->SetText(FText::FromString(TEXT("Not enough gold")));
			}
		});
	}
}

void USafeBuildMenu::BuyTower()
{
	TryBuy(TEXT("SpawnTower"), TowerCost);
}

void USafeBuildMenu::BuyMortar()
{
	TryBuy(TEXT("SpawnMortar"), MortarCost);
}

void USafeBuildMenu::BuyInfantry()
{
	TryBuy(TEXT("SpawnInfantry"), InfantryCost);
}

void USafeBuildMenu::TryBuy(FName SpawnFunctionName, double Cost)
{
	const double Gold = ReadPlayerCurrency();
	if (Gold + KINDA_SMALL_NUMBER < Cost)
	{
		ApplyAffordability();
		return;
	}

	AActor* Socket = GetParentSocket();
	if (!IsValid(Socket))
	{
		return;
	}

	UFunction* SpawnFn = Socket->FindFunction(SpawnFunctionName);
	if (!SpawnFn)
	{
		return;
	}

	WritePlayerCurrency(Gold - Cost);
	Socket->ProcessEvent(SpawnFn, nullptr);
	RemoveFromParent();
}

double USafeBuildMenu::ReadPlayerCurrency() const
{
	const AGameModeBase* GameMode = UGameplayStatics::GetGameMode(this);
	if (!IsValid(GameMode))
	{
		return 0.0;
	}

	if (const FDoubleProperty* AsDouble = FindFProperty<FDoubleProperty>(GameMode->GetClass(), TEXT("PlayerCurrency")))
	{
		return AsDouble->GetPropertyValue_InContainer(GameMode);
	}

	if (const FFloatProperty* AsFloat = FindFProperty<FFloatProperty>(GameMode->GetClass(), TEXT("PlayerCurrency")))
	{
		return AsFloat->GetPropertyValue_InContainer(GameMode);
	}

	if (const FIntProperty* AsInt = FindFProperty<FIntProperty>(GameMode->GetClass(), TEXT("PlayerCurrency")))
	{
		return static_cast<double>(AsInt->GetPropertyValue_InContainer(GameMode));
	}

	return 0.0;
}

void USafeBuildMenu::WritePlayerCurrency(double NewValue) const
{
	AGameModeBase* GameMode = UGameplayStatics::GetGameMode(this);
	if (!IsValid(GameMode))
	{
		return;
	}

	if (FDoubleProperty* AsDouble = FindFProperty<FDoubleProperty>(GameMode->GetClass(), TEXT("PlayerCurrency")))
	{
		AsDouble->SetPropertyValue_InContainer(GameMode, NewValue);
		return;
	}

	if (FFloatProperty* AsFloat = FindFProperty<FFloatProperty>(GameMode->GetClass(), TEXT("PlayerCurrency")))
	{
		AsFloat->SetPropertyValue_InContainer(GameMode, static_cast<float>(NewValue));
		return;
	}

	if (FIntProperty* AsInt = FindFProperty<FIntProperty>(GameMode->GetClass(), TEXT("PlayerCurrency")))
	{
		AsInt->SetPropertyValue_InContainer(GameMode, FMath::RoundToInt(NewValue));
	}
}

AActor* USafeBuildMenu::GetParentSocket() const
{
	const FObjectProperty* SocketProp = FindFProperty<FObjectProperty>(GetClass(), TEXT("ParentSocket"));
	if (!SocketProp)
	{
		return nullptr;
	}

	return Cast<AActor>(SocketProp->GetObjectPropertyValue_InContainer(this));
}
