#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SafeTowerBase.generated.h"

/**
 * Native parent for BP_TowerBase.
 * Runs a safe fire path (valid-index + valid-actor + no Get after damage)
 * and clears the Blueprint FireTimerHandle so the old AttemptFire graph cannot crash.
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

	/** Cached target used for this fire. Never re-Get index 0 after ApplyDamage. */
	UPROPERTY(BlueprintReadWrite, Category = "Combat")
	TObjectPtr<AActor> CurrentTarget;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void RunSafeFire();

protected:
	void ClearBlueprintFireTimer();
	void EnsureSafeFireTimer();
	float GetBlueprintFireRate() const;
	FArrayProperty* FindTargetArrayProperty() const;
	AActor* GetArrayActorAt(FArrayProperty* ArrayProp, FScriptArrayHelper& Helper, int32 Index) const;
	void RemoveActorFromTargetArray(AActor* Actor);
	void ReleaseParentBuildSocket();
	double GetActorHealth(AActor* Actor) const;

	FTimerHandle SafeFireTimerHandle;
};
