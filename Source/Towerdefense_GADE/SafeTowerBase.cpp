#include "SafeTowerBase.h"

#include "EnemyBase.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"

ASafeTowerBase::ASafeTowerBase()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
}

void ASafeTowerBase::BeginPlay()
{
	Super::BeginPlay();

	TowerHealth = MaxTowerHealth;
	CachedFireRate = ReadBlueprintFireRate();

	ClearBlueprintFireTimer();
	EnsureSafeFireTimer();
}

void ASafeTowerBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ClearBlueprintFireTimer();
}

void ASafeTowerBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SafeFireTimerHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void ASafeTowerBase::Destroyed()
{
	ReleaseParentBuildSocket();
	Super::Destroyed();
}

float ASafeTowerBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	const float Actual = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (Actual <= 0.0f)
	{
		return 0.0f;
	}

	TowerHealth -= Actual;
	if (TowerHealth <= 0.0f)
	{
		TowerHealth = 0.0f;
		Destroy();
	}

	return Actual;
}

void ASafeTowerBase::ClearBlueprintFireTimer()
{
	FStructProperty* HandleProp = FindFProperty<FStructProperty>(GetClass(), FName(TEXT("FireTimerHandle")));
	if (!HandleProp || HandleProp->Struct != TBaseStructure<FTimerHandle>::Get())
	{
		return;
	}

	FTimerHandle* Handle = HandleProp->ContainerPtrToValuePtr<FTimerHandle>(this);
	if (!Handle || !Handle->IsValid())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(*Handle);
	}
}

float ASafeTowerBase::ReadBlueprintFireRate() const
{
	if (const FDoubleProperty* RateProp = FindFProperty<FDoubleProperty>(GetClass(), FName(TEXT("FireRate"))))
	{
		return FMath::Max(0.05f, static_cast<float>(RateProp->GetPropertyValue_InContainer(this)));
	}

	if (const FFloatProperty* RateProp = FindFProperty<FFloatProperty>(GetClass(), FName(TEXT("FireRate"))))
	{
		return FMath::Max(0.05f, RateProp->GetPropertyValue_InContainer(this));
	}

	return 1.0f;
}

void ASafeTowerBase::EnsureSafeFireTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (World->GetTimerManager().IsTimerActive(SafeFireTimerHandle))
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		SafeFireTimerHandle,
		this,
		&ASafeTowerBase::RunSafeFire,
		CachedFireRate,
		true);
}

FArrayProperty* ASafeTowerBase::FindTargetArrayProperty() const
{
	return FindFProperty<FArrayProperty>(GetClass(), FName(TEXT("TargetArray")));
}

AActor* ASafeTowerBase::GetArrayActorAt(FArrayProperty* ArrayProp, FScriptArrayHelper& Helper, int32 Index) const
{
	if (!ArrayProp || !Helper.IsValidIndex(Index))
	{
		return nullptr;
	}

	const FObjectProperty* Inner = CastField<FObjectProperty>(ArrayProp->Inner);
	if (!Inner)
	{
		return nullptr;
	}

	return Cast<AActor>(Inner->GetObjectPropertyValue(Helper.GetRawPtr(Index)));
}

void ASafeTowerBase::RemoveActorFromTargetArray(AActor* Actor)
{
	FArrayProperty* ArrayProp = FindTargetArrayProperty();
	if (!ArrayProp)
	{
		return;
	}

	FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(this));
	const FObjectProperty* Inner = CastField<FObjectProperty>(ArrayProp->Inner);
	if (!Inner)
	{
		return;
	}

	for (int32 Index = Helper.Num() - 1; Index >= 0; --Index)
	{
		AActor* Existing = Cast<AActor>(Inner->GetObjectPropertyValue(Helper.GetRawPtr(Index)));
		if (Existing == Actor || !IsValid(Existing))
		{
			Helper.RemoveValues(Index);
		}
	}
}

double ASafeTowerBase::GetTargetHealth(AActor* Actor) const
{
	if (!IsValid(Actor))
	{
		return 0.0;
	}

	if (const AEnemyBase* Enemy = Cast<AEnemyBase>(Actor))
	{
		return Enemy->GetCurrentHealth();
	}

	// Fallback while Blueprints are still mid-transition.
	if (const FDoubleProperty* HealthProp = FindFProperty<FDoubleProperty>(Actor->GetClass(), FName(TEXT("Health"))))
	{
		return HealthProp->GetPropertyValue_InContainer(Actor);
	}

	if (const FFloatProperty* HealthProp = FindFProperty<FFloatProperty>(Actor->GetClass(), FName(TEXT("Health"))))
	{
		return HealthProp->GetPropertyValue_InContainer(Actor);
	}

	return 1.0;
}

void ASafeTowerBase::RunSafeFire()
{
	ClearBlueprintFireTimer();

	FArrayProperty* ArrayProp = FindTargetArrayProperty();
	if (!ArrayProp)
	{
		return;
	}

	FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(this));

	if (Helper.Num() <= 0 || !Helper.IsValidIndex(0))
	{
		CurrentTarget = nullptr;
		return;
	}

	CurrentTarget = GetArrayActorAt(ArrayProp, Helper, 0);
	if (!IsValid(CurrentTarget))
	{
		Helper.RemoveValues(0);
		CurrentTarget = nullptr;
		return;
	}

	AActor* TargetToDamage = CurrentTarget;
	UGameplayStatics::ApplyDamage(TargetToDamage, 1.0f, nullptr, this, nullptr);

	if (!IsValid(TargetToDamage) || GetTargetHealth(TargetToDamage) <= 0.0)
	{
		RemoveActorFromTargetArray(TargetToDamage);
		CurrentTarget = nullptr;
	}
}

void ASafeTowerBase::ReleaseParentBuildSocket()
{
	const FObjectProperty* SocketProp = FindFProperty<FObjectProperty>(GetClass(), FName(TEXT("ParentBuild SOcket")));
	if (!SocketProp)
	{
		return;
	}

	UObject* SocketObject = SocketProp->GetObjectPropertyValue_InContainer(this);
	if (!IsValid(SocketObject))
	{
		return;
	}

	if (FBoolProperty* OccupiedProp = FindFProperty<FBoolProperty>(SocketObject->GetClass(), FName(TEXT("isOccupied?"))))
	{
		OccupiedProp->SetPropertyValue_InContainer(SocketObject, false);
	}
}
