#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EnemyType.h"
#include "EnemyData.generated.h"

/**
 * Data-driven stats for one enemy type.
 * Create assets like DA_Enemy_Light in the editor (or run setup_phase2.py).
 * Names avoid clashing with existing Blueprint variables on BP_Enemy.
 */
UCLASS(BlueprintType)
class TOWERDEFENSE_GADE_API UEnemyData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	EEnemyType EnemyType = EEnemyType::Light;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "1.0"))
	float MaxHealth = 3.0f;

	/** Units per second along the path spline. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "1.0"))
	float PathSpeed = 300.0f;

	/** Damage applied to the player's base when this enemy reaches the exit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "1.0"))
	float ExitDamage = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "0.0"))
	float GoldReward = 10.0f;

	/** Cost used later by the adaptive wave system when picking this enemy. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (ClampMin = "0.0"))
	float SpawnCost = 10.0f;

	/** Ranged only: how close a tower must be before this enemy stops to shoot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ranged", meta = (ClampMin = "0.0"))
	float ShootRange = 0.0f;

	/** Ranged only: seconds between shots at a tower. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ranged", meta = (ClampMin = "0.05"))
	float ShootInterval = 1.0f;

	/** Ranged only: damage dealt to a tower per shot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ranged", meta = (ClampMin = "0.0"))
	float TowerDamage = 1.0f;
};
