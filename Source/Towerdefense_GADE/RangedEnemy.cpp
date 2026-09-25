#include "RangedEnemy.h"

#include "EnemyData.h"
#include "SafeTowerBase.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ARangedEnemy::ARangedEnemy()
{
	EnemyType = EEnemyType::Ranged;
}

void ARangedEnemy::BeginPlay()
{
	Super::BeginPlay();
}

void ARangedEnemy::ApplyEnemyData()
{
	Super::ApplyEnemyData();

	if (!EnemyData)
	{
		return;
	}

	ShootRange = EnemyData->ShootRange;
	ShootInterval = EnemyData->ShootInterval;
	TowerDamage = EnemyData->TowerDamage;
}

void ARangedEnemy::Tick(float DeltaSeconds)
{
	UpdateTowerFocus();
	Super::Tick(DeltaSeconds);
}

void ARangedEnemy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopShootTimer();
	Super::EndPlay(EndPlayReason);
}

bool ARangedEnemy::CanMoveAlongPath() const
{
	return !IsValid(FocusedTower);
}

void ARangedEnemy::UpdateTowerFocus()
{
	if (bIsDying)
	{
		FocusedTower = nullptr;
		StopShootTimer();
		return;
	}

	AActor* BestTower = nullptr;
	float BestDistSq = ShootRange * ShootRange;

	TArray<AActor*> Towers;
	UGameplayStatics::GetAllActorsOfClass(this, ASafeTowerBase::StaticClass(), Towers);
	for (AActor* Tower : Towers)
	{
		if (!IsValid(Tower))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(GetActorLocation(), Tower->GetActorLocation());
		if (DistSq <= BestDistSq)
		{
			BestDistSq = DistSq;
			BestTower = Tower;
		}
	}

	const bool bHadTarget = IsValid(FocusedTower);
	FocusedTower = BestTower;

	if (IsValid(FocusedTower))
	{
		if (!bHadTarget)
		{
			StartShootTimer();
		}
	}
	else
	{
		StopShootTimer();
	}
}

void ARangedEnemy::StartShootTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (World->GetTimerManager().IsTimerActive(ShootTimerHandle))
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		ShootTimerHandle,
		this,
		&ARangedEnemy::FireAtFocusedTower,
		FMath::Max(0.05f, ShootInterval),
		true);
}

void ARangedEnemy::StopShootTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ShootTimerHandle);
	}
}

void ARangedEnemy::FireAtFocusedTower()
{
	if (!IsValid(FocusedTower) || bIsDying)
	{
		StopShootTimer();
		FocusedTower = nullptr;
		return;
	}

	UGameplayStatics::ApplyDamage(FocusedTower, TowerDamage, nullptr, this, nullptr);

	if (!IsValid(FocusedTower))
	{
		StopShootTimer();
		FocusedTower = nullptr;
	}
}
