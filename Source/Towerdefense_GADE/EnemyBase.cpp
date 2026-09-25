#include "EnemyBase.h"

#include "EnemyData.h"
#include "Components/SplineComponent.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UnrealType.h"

AEnemyBase::AEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();
	ApplyEnemyData();
}

void AEnemyBase::ApplyEnemyData()
{
	if (!EnemyData)
	{
		return;
	}

	EnemyType = EnemyData->EnemyType;
	MaxHealth = EnemyData->MaxHealth;
	CurrentHealth = MaxHealth;
	PathSpeed = EnemyData->PathSpeed;
	ExitDamage = EnemyData->ExitDamage;
	GoldReward = EnemyData->GoldReward;
	SpawnCost = EnemyData->SpawnCost;
}

void AEnemyBase::SetFollowSpline(USplineComponent* Spline)
{
	FollowSpline = Spline;
	DistanceAlongSpline = 0.0f;
}

void AEnemyBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsDying)
	{
		return;
	}

	AdvanceAlongPath(DeltaSeconds);
}

void AEnemyBase::AdvanceAlongPath(float DeltaSeconds)
{
	if (!FollowSpline || FollowSpline->GetNumberOfSplinePoints() <= 0)
	{
		return;
	}

	if (!CanMoveAlongPath())
	{
		return;
	}

	DistanceAlongSpline += PathSpeed * DeltaSeconds;
	const float SplineLength = FollowSpline->GetSplineLength();

	if (DistanceAlongSpline >= SplineLength)
	{
		DistanceAlongSpline = SplineLength;
		const FVector EndLocation = FollowSpline->GetLocationAtDistanceAlongSpline(
			SplineLength, ESplineCoordinateSpace::World);
		const FRotator EndRotation = FollowSpline->GetRotationAtDistanceAlongSpline(
			SplineLength, ESplineCoordinateSpace::World);
		SetActorLocationAndRotation(EndLocation, EndRotation);
		HandleReachedExit();
		return;
	}

	const FVector Location = FollowSpline->GetLocationAtDistanceAlongSpline(
		DistanceAlongSpline, ESplineCoordinateSpace::World);
	const FRotator Rotation = FollowSpline->GetRotationAtDistanceAlongSpline(
		DistanceAlongSpline, ESplineCoordinateSpace::World);
	SetActorLocationAndRotation(Location, Rotation);
}

float AEnemyBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	const float Actual = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (bIsDying || Actual <= 0.0f)
	{
		return 0.0f;
	}

	CurrentHealth -= Actual;
	if (CurrentHealth <= 0.0f)
	{
		CurrentHealth = 0.0f;
		HandleDeath();
	}

	return Actual;
}

void AEnemyBase::HandleReachedExit()
{
	if (bIsDying)
	{
		return;
	}

	bIsDying = true;
	DamagePlayerBase();
	Destroy();
}

void AEnemyBase::HandleDeath()
{
	if (bIsDying)
	{
		return;
	}

	bIsDying = true;
	AwardGoldToPlayer();
	Destroy();
}

void AEnemyBase::AwardGoldToPlayer() const
{
	AGameModeBase* GameMode = UGameplayStatics::GetGameMode(this);
	if (!IsValid(GameMode) || GoldReward <= 0.0f)
	{
		return;
	}

	if (FDoubleProperty* AsDouble = FindFProperty<FDoubleProperty>(GameMode->GetClass(), TEXT("PlayerCurrency")))
	{
		const double CurrentGold = AsDouble->GetPropertyValue_InContainer(GameMode);
		AsDouble->SetPropertyValue_InContainer(GameMode, CurrentGold + GoldReward);
	}
	else if (FFloatProperty* AsFloat = FindFProperty<FFloatProperty>(GameMode->GetClass(), TEXT("PlayerCurrency")))
	{
		const float CurrentGold = AsFloat->GetPropertyValue_InContainer(GameMode);
		AsFloat->SetPropertyValue_InContainer(GameMode, CurrentGold + static_cast<float>(GoldReward));
	}
	else if (FIntProperty* AsInt = FindFProperty<FIntProperty>(GameMode->GetClass(), TEXT("PlayerCurrency")))
	{
		const int32 CurrentGold = AsInt->GetPropertyValue_InContainer(GameMode);
		AsInt->SetPropertyValue_InContainer(GameMode, CurrentGold + FMath::RoundToInt(GoldReward));
	}
}

void AEnemyBase::DamagePlayerBase() const
{
	AGameModeBase* GameMode = UGameplayStatics::GetGameMode(this);
	if (!IsValid(GameMode))
	{
		return;
	}

	const int32 Hits = FMath::Max(1, FMath::RoundToInt(ExitDamage));

	// Prefer the existing Blueprint function so game-over logic still runs.
	if (UFunction* SubtractFn = GameMode->FindFunction(FName(TEXT("SubtractPlayerHealth"))))
	{
		for (int32 Index = 0; Index < Hits; ++Index)
		{
			if (SubtractFn->ParmsSize == 0)
			{
				GameMode->ProcessEvent(SubtractFn, nullptr);
			}
			else
			{
				TArray<uint8> Parms;
				Parms.SetNumZeroed(SubtractFn->ParmsSize);
				for (FProperty* Parm = SubtractFn->PropertyLink; Parm; Parm = Parm->PropertyLinkNext)
				{
					if (!Parm->HasAnyPropertyFlags(CPF_Parm) || Parm->HasAnyPropertyFlags(CPF_ReturnParm))
					{
						continue;
					}

					if (FIntProperty* AsInt = CastField<FIntProperty>(Parm))
					{
						AsInt->SetPropertyValue_InContainer(Parms.GetData(), 1);
					}
					else if (FDoubleProperty* AsDouble = CastField<FDoubleProperty>(Parm))
					{
						AsDouble->SetPropertyValue_InContainer(Parms.GetData(), 1.0);
					}
					else if (FFloatProperty* AsFloat = CastField<FFloatProperty>(Parm))
					{
						AsFloat->SetPropertyValue_InContainer(Parms.GetData(), 1.0f);
					}
					break;
				}
				GameMode->ProcessEvent(SubtractFn, Parms.GetData());
			}
		}
		return;
	}

	if (FIntProperty* HealthProp = FindFProperty<FIntProperty>(GameMode->GetClass(), TEXT("PlayerHealth")))
	{
		const int32 NewHealth = FMath::Max(0, HealthProp->GetPropertyValue_InContainer(GameMode) - Hits);
		HealthProp->SetPropertyValue_InContainer(GameMode, NewHealth);
	}
	else if (FDoubleProperty* HealthProp = FindFProperty<FDoubleProperty>(GameMode->GetClass(), TEXT("PlayerHealth")))
	{
		const double NewHealth = FMath::Max(0.0, HealthProp->GetPropertyValue_InContainer(GameMode) - Hits);
		HealthProp->SetPropertyValue_InContainer(GameMode, NewHealth);
	}
}
