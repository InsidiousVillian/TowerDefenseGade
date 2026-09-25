#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SafeTowerBase.generated.h"

/**
 * Native parent for BP_TowerBase.
 * Runs a safe fire path (valid-index + valid-actor + no Get after damage)
 * and clears the Blueprint FireTimerHandle so the old AttemptFire graph cannot crash.
 * TowerHealth avoids clashing with the existing Blueprint Health variable.
 */
UCLASS()
class TOWERDEFENSE_GADE_API ASafeTowerBase : public AActor
{
	GENERATED_BODY()

public:
	ASafeTowerBase();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Destroyed() override;
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	/** Cached target used for this fire. Never re-Get index 0 after ApplyDamage. */
	UPROPERTY(BlueprintReadWrite, Category = "Combat")
	TObjectPtr<AActor> CurrentTarget;

	/** C++ hit points. Named TowerHealth so it does not clash with Blueprint Health. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float TowerHealth = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float MaxTowerHealth = 5.0f;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void RunSafeFire();

	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetTowerHealth() const { return TowerHealth; }

protected:
	void ClearBlueprintFireTimer();
	void EnsureSafeFireTimer();
	float ReadBlueprintFireRate() const;
	FArrayProperty* FindTargetArrayProperty() const;
	AActor* GetArrayActorAt(FArrayProperty* ArrayProp, FScriptArrayHelper& Helper, int32 Index) const;
	void RemoveActorFromTargetArray(AActor* Actor);
	void ReleaseParentBuildSocket();
	double GetTargetHealth(AActor* Actor) const;

	FTimerHandle SafeFireTimerHandle;

	/** Last FireRate used to arm the looping timer. Re-checked each shot in RunSafeFire. */
	float CachedFireRate = 1.0f;
};
