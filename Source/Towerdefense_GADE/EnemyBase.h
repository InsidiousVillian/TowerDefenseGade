#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyType.h"
#include "EnemyBase.generated.h"

class UEnemyData;
class USplineComponent;

/**
 * Native parent for enemy Blueprints (BP_Enemy / BP_Enemy2).
 * Owns stats from UEnemyData, moves along the grid path spline,
 * takes damage via ApplyDamage, and awards gold / damages the base on death or exit.
 *
 * Property names intentionally avoid clashing with existing Blueprint variables
 * (Health, TargetSpline, Distance, MovementSpeed, BaseDamage, Damage, ...).
 */
UCLASS()
class TOWERDEFENSE_GADE_API AEnemyBase : public AActor
{
	GENERATED_BODY()

public:
	AEnemyBase();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	/** Called by the grid generator after spawn. */
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void SetFollowSpline(USplineComponent* Spline);

	UFUNCTION(BlueprintPure, Category = "Enemy")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	float GetMaxHealthValue() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	float GetPathSpeed() const { return PathSpeed; }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	float GetExitDamage() const { return ExitDamage; }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	float GetGoldReward() const { return GoldReward; }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	float GetSpawnCost() const { return SpawnCost; }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	EEnemyType GetEnemyType() const { return EnemyType; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	TObjectPtr<UEnemyData> EnemyData;

protected:
	virtual void ApplyEnemyData();
	virtual void AdvanceAlongPath(float DeltaSeconds);
	virtual bool CanMoveAlongPath() const { return true; }

	void HandleReachedExit();
	void HandleDeath();
	void AwardGoldToPlayer() const;
	void DamagePlayerBase() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
	float CurrentHealth = 3.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
	float MaxHealth = 3.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
	float PathSpeed = 300.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
	float ExitDamage = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
	float GoldReward = 10.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
	float SpawnCost = 10.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy")
	EEnemyType EnemyType = EEnemyType::Light;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy")
	TObjectPtr<USplineComponent> FollowSpline;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy")
	float DistanceAlongSpline = 0.0f;

	bool bIsDying = false;
};
