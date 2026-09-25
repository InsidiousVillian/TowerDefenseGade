#pragma once

#include "CoreMinimal.h"
#include "EnemyBase.h"
#include "RangedEnemy.generated.h"

/**
 * Ranged enemy: walks the path until a tower is within ShootRange,
 * then stops and damages that tower on a timer.
 * Parent for BP_Enemy3.
 */
UCLASS()
class TOWERDEFENSE_GADE_API ARangedEnemy : public AEnemyBase
{
	GENERATED_BODY()

public:
	ARangedEnemy();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void ApplyEnemyData() override;
	virtual bool CanMoveAlongPath() const override;

	void UpdateTowerFocus();
	void StartShootTimer();
	void StopShootTimer();

	UFUNCTION()
	void FireAtFocusedTower();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ranged")
	float ShootRange = 600.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ranged")
	float ShootInterval = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ranged")
	float TowerDamage = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Ranged")
	TObjectPtr<AActor> FocusedTower;

	FTimerHandle ShootTimerHandle;
};
