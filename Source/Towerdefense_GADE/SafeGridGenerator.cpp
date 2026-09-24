#include "SafeGridGenerator.h"

#include "Components/SplineComponent.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"

ASafeGridGenerator::ASafeGridGenerator()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
}

void ASafeGridGenerator::BeginPlay()
{
	Super::BeginPlay();
	ClearBlueprintSpawnTimer();
}

void ASafeGridGenerator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ClearBlueprintSpawnTimer();
	EnsureSafeSpawnTimer();
}

void ASafeGridGenerator::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SafeSpawnTimerHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void ASafeGridGenerator::ClearBlueprintSpawnTimer()
{
	FStructProperty* HandleProp = FindFProperty<FStructProperty>(GetClass(), FName(TEXT("SpawnTimerHandle")));
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

float ASafeGridGenerator::GetSpawnInterval() const
{
	if (const FDoubleProperty* IntervalProp = FindFProperty<FDoubleProperty>(GetClass(), FName(TEXT("SpawnInterval"))))
	{
		return FMath::Max(0.05f, static_cast<float>(IntervalProp->GetPropertyValue_InContainer(this)));
	}

	if (const FFloatProperty* IntervalProp = FindFProperty<FFloatProperty>(GetClass(), FName(TEXT("SpawnInterval"))))
	{
		return FMath::Max(0.05f, IntervalProp->GetPropertyValue_InContainer(this));
	}

	return 1.0f;
}

USplineComponent* ASafeGridGenerator::GetPathSpline() const
{
	if (const FObjectProperty* SplineProp = FindFProperty<FObjectProperty>(GetClass(), FName(TEXT("PathSpline"))))
	{
		return Cast<USplineComponent>(SplineProp->GetObjectPropertyValue_InContainer(this));
	}

	return FindComponentByClass<USplineComponent>();
}

void ASafeGridGenerator::EnsureSafeSpawnTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	USplineComponent* Spline = GetPathSpline();
	if (!Spline || Spline->GetNumberOfSplinePoints() <= 0)
	{
		return;
	}

	const float Interval = GetSpawnInterval();
	if (World->GetTimerManager().IsTimerActive(SafeSpawnTimerHandle) && FMath::IsNearlyEqual(LastSpawnInterval, Interval))
	{
		return;
	}

	LastSpawnInterval = Interval;
	World->GetTimerManager().SetTimer(
		SafeSpawnTimerHandle,
		this,
		&ASafeGridGenerator::RunSafeSpawn,
		Interval,
		true);
}

FArrayProperty* ASafeGridGenerator::FindEnemyClassesProperty() const
{
	if (FArrayProperty* Prop = FindFProperty<FArrayProperty>(GetClass(), FName(TEXT("Enemyclasses"))))
	{
		return Prop;
	}

	return FindFProperty<FArrayProperty>(GetClass(), FName(TEXT("EnemyClasses")));
}

UClass* ASafeGridGenerator::GetClassAt(FArrayProperty* ArrayProp, FScriptArrayHelper& Helper, int32 Index) const
{
	if (!ArrayProp || !Helper.IsValidIndex(Index))
	{
		return nullptr;
	}

	if (const FClassProperty* Inner = CastField<FClassProperty>(ArrayProp->Inner))
	{
		return Cast<UClass>(Inner->GetObjectPropertyValue(Helper.GetRawPtr(Index)));
	}

	if (const FObjectProperty* Inner = CastField<FObjectProperty>(ArrayProp->Inner))
	{
		return Cast<UClass>(Inner->GetObjectPropertyValue(Helper.GetRawPtr(Index)));
	}

	if (const FSoftClassProperty* Inner = CastField<FSoftClassProperty>(ArrayProp->Inner))
	{
		return Cast<UClass>(Inner->GetPropertyValue(Helper.GetRawPtr(Index)).Get());
	}

	return nullptr;
}

TSubclassOf<AActor> ASafeGridGenerator::PickRandomEnemyClass() const
{
	FArrayProperty* ArrayProp = FindEnemyClassesProperty();
	if (!ArrayProp)
	{
		return nullptr;
	}

	FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(this));
	if (Helper.Num() <= 0)
	{
		return nullptr;
	}

	const int32 Index = FMath::RandRange(0, Helper.Num() - 1);
	if (!Helper.IsValidIndex(Index))
	{
		return nullptr;
	}

	return GetClassAt(ArrayProp, Helper, Index);
}

void ASafeGridGenerator::RunSafeSpawn()
{
	ClearBlueprintSpawnTimer();

	const TSubclassOf<AActor> EnemyClass = PickRandomEnemyClass();
	if (!*EnemyClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	USplineComponent* Spline = GetPathSpline();
	FVector Location = GetActorLocation();
	if (Spline && Spline->GetNumberOfSplinePoints() > 0)
	{
		Location = Spline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
	}

	const FTransform SpawnTransform(FRotator::ZeroRotator, Location);
	AActor* Spawned = World->SpawnActor<AActor>(EnemyClass, SpawnTransform);
	if (!IsValid(Spawned) || !Spline)
	{
		return;
	}

	if (FObjectProperty* SplineProp = FindFProperty<FObjectProperty>(Spawned->GetClass(), FName(TEXT("TargetSpline"))))
	{
		SplineProp->SetObjectPropertyValue_InContainer(Spawned, Spline);
	}
}
