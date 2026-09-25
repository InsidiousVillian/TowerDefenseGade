#pragma once

#include "CoreMinimal.h"
#include "EnemyType.generated.h"

/** Used later by the wave system to weight which enemies to spawn. */
UENUM(BlueprintType)
enum class EEnemyType : uint8
{
	Light UMETA(DisplayName = "Light"),
	Heavy UMETA(DisplayName = "Heavy"),
	Ranged UMETA(DisplayName = "Ranged")
};
