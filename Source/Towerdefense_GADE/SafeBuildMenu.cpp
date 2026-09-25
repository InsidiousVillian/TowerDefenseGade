#include "SafeBuildMenu.h"

#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"

void USafeBuildMenu::NativeConstruct()
{
	Super::NativeConstruct();
	BindBuyButtons();
	CacheButtonLabels();
	ApplyAffordability();
}

void USafeBuildMenu::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	// Refresh while open so buttons update when gold changes.
	ApplyAffordability();
}

void USafeBuildMenu::BindBuyButtons()
{
	if (Buydefender)
	{
		Buydefender->OnClicked.Clear();
		Buydefender->OnClicked.AddDynamic(this, &USafeBuildMenu::BuyTower);
	}

	if (Buydefender_1)
	{
		Buydefender_1->OnClicked.Clear();
		Buydefender_1->OnClicked.AddDynamic(this, &USafeBuildMenu::BuyMortar);
	}

	if (Buydefender_2)
	{
		Buydefender_2->OnClicked.Clear();
		Buydefender_2->OnClicked.AddDynamic(this, &USafeBuildMenu::BuyInfantry);
	}
}

void USafeBuildMenu::CacheButtonLabels()
{
	OriginalButtonLabels.Reset();

	const TArray<UButton*> Buttons = { Buydefender.Get(), Buydefender_1.Get(), Buydefender_2.Get() };
	for (UButton* Button : Buttons)
	{
		if (UTextBlock* Label = FindLabelOnButton(Button))
		{
			OriginalButtonLabels.Add(Button, Label->GetText());
		}
	}
}

UTextBlock* USafeBuildMenu::FindLabelOnButton(UButton* Button) const
{
	if (!Button)
	{
		return nullptr;
	}

	if (UTextBlock* Direct = Cast<UTextBlock>(Button->GetContent()))
	{
		return Direct;
	}

	if (UPanelWidget* Panel = Cast<UPanelWidget>(Button->GetContent()))
	{
		for (int32 Index = 0; Index < Panel->GetChildrenCount(); ++Index)
		{
			if (UTextBlock* Child = Cast<UTextBlock>(Panel->GetChildAt(Index)))
			{
				return Child;
			}
		}
	}

	return nullptr;
}

void USafeBuildMenu::ApplyAffordability()
{
	const double Gold = ReadPlayerCurrency();
	const bool bCanAffordAny = Gold + KINDA_SMALL_NUMBER >= TowerCost;

	struct FOffer
	{
		UButton* Button;
		double Cost;
	};

	const FOffer Offers[] = {
		{Buydefender.Get(), TowerCost},
		{Buydefender_1.Get(), MortarCost},
		{Buydefender_2.Get(), InfantryCost},
	};

	for (const FOffer& Offer : Offers)
	{
		if (!Offer.Button)
		{
			continue;
		}

		const bool bCanAfford = bCanAffordAny && Gold + KINDA_SMALL_NUMBER >= Offer.Cost;
		Offer.Button->SetIsEnabled(bCanAfford);
		Offer.Button->SetVisibility(bCanAffordAny ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

		if (UTextBlock* Label = FindLabelOnButton(Offer.Button))
		{
			if (!bCanAfford && bCanAffordAny)
			{
				// Only the unaffordable buy buttons — not every text block in the widget.
				Label->SetText(FText::FromString(TEXT("Not enough gold")));
			}
			else if (const FText* Original = OriginalButtonLabels.Find(Offer.Button))
			{
				Label->SetText(*Original);
			}
		}
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
