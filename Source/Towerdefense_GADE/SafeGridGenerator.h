#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SafeGridGenerator.generated.h"

class USplineComponent;

/**
 * Native parent for BP_GridGenerator.
 * Clears the Blueprint CS_SpawnEnemy timer and spawns with a Length-1
 * Enemyclasses pick so RandomIntegerInRange(0, 3) cannot read past the array.
 */
UCLASS()
class TOWERDEFENSE_GADE_API ASafeGridGenerator : public AActor
{
	GENERATED_BODY()

public:
	ASafeGridGenerator();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Safe random class from Enemyclasses. Returns null if the array is empty. */
	UFUNCTION(BlueprintPure, Category = "Combat")
	TSubclassOf<AActor> PickRandomEnemyClass() const;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void RunSafeSpawn();

protected:
	void ClearBlueprintSpawnTimer();
	void EnsureSafeSpawnTimer();
	void SeparateSpawnFromExit();
	float GetSpawnInterval() const;
	USplineComponent* GetPathSpline() const;
	int32 ReadIntProperty(FName PropertyName, int32 Fallback) const;
	double ReadDoubleProperty(FName PropertyName, double Fallback) const;
	void WriteVector2DArray(FName PropertyName, const TArray<FVector2D>& Values);
	FArrayProperty* FindEnemyClassesProperty() const;
	UClass* GetClassAt(FArrayProperty* ArrayProp, FScriptArrayHelper& Helper, int32 Index) const;

	FTimerHandle SafeSpawnTimerHandle;
	float LastSpawnInterval = -1.0f;
};
