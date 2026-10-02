#include "SafeGridGenerator.h"

#include "SafeTowerBase.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"

namespace
{
	void WriteActorHealth(AActor* Actor, double Health)
	{
		if (FDoubleProperty* HealthProp = FindFProperty<FDoubleProperty>(Actor->GetClass(), FName(TEXT("Health"))))
		{
			HealthProp->SetPropertyValue_InContainer(Actor, Health);
			return;
		}

		if (FFloatProperty* HealthProp = FindFProperty<FFloatProperty>(Actor->GetClass(), FName(TEXT("Health"))))
		{
			HealthProp->SetPropertyValue_InContainer(Actor, static_cast<float>(Health));
		}
	}

	bool ActorHasHealth(const AActor* Actor)
	{
		return FindFProperty<FDoubleProperty>(Actor->GetClass(), FName(TEXT("Health")))
			|| FindFProperty<FFloatProperty>(Actor->GetClass(), FName(TEXT("Health")));
	}

	bool StartingHealthFor(const AActor* Actor, double& OutHealth)
	{
		const FString Name = Actor->GetClass()->GetName();
		if (Name.Contains(TEXT("Ranged")))
		{
			OutHealth = 75.0;
		}
		else if (Name.Contains(TEXT("Heavy")))
		{
			OutHealth = 140.0;
		}
		else if (Name.Contains(TEXT("Light_Enemy")) || Name.Contains(TEXT("BP_Enemy")))
		{
			OutHealth = 40.0;
		}
		else if (Name.Contains(TEXT("Mortar")))
		{
			OutHealth = 70.0;
		}
		else if (Name.Contains(TEXT("Infantry")))
		{
			OutHealth = 45.0;
		}
		else if (Actor->IsA(ASafeTowerBase::StaticClass()))
		{
			OutHealth = 120.0;
		}
		else
		{
			return false;
		}

		return ActorHasHealth(Actor);
	}

	float ShotDamageForDefender(const AActor* Actor)
	{
		const FString Name = Actor->GetClass()->GetName();
		if (Name.Contains(TEXT("Mortar")))
		{
			return 35.0f;
		}
		if (Name.Contains(TEXT("Infantry")))
		{
			return 8.0f;
		}
		return 0.0f;
	}

	float ReadFireRate(const AActor* Actor)
	{
		if (const FDoubleProperty* RateProp = FindFProperty<FDoubleProperty>(Actor->GetClass(), FName(TEXT("FireRate"))))
		{
			return FMath::Max(0.05f, static_cast<float>(RateProp->GetPropertyValue_InContainer(Actor)));
		}
		if (const FFloatProperty* RateProp = FindFProperty<FFloatProperty>(Actor->GetClass(), FName(TEXT("FireRate"))))
		{
			return FMath::Max(0.05f, RateProp->GetPropertyValue_InContainer(Actor));
		}
		return 1.0f;
	}

	void ClearNamedTimer(AActor* Actor, const TCHAR* PropertyName)
	{
		FStructProperty* HandleProp = FindFProperty<FStructProperty>(Actor->GetClass(), FName(PropertyName));
		if (!HandleProp || HandleProp->Struct != TBaseStructure<FTimerHandle>::Get())
		{
			return;
		}

		FTimerHandle* Handle = HandleProp->ContainerPtrToValuePtr<FTimerHandle>(Actor);
		if (!Handle || !Handle->IsValid() || !Actor->GetWorld())
		{
			return;
		}

		Actor->GetWorld()->GetTimerManager().ClearTimer(*Handle);
	}

	double ReadActorHealth(const AActor* Actor)
	{
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

	bool FireAtFirstTarget(AActor* Defender, float Damage)
	{
		FArrayProperty* ArrayProp = FindFProperty<FArrayProperty>(Defender->GetClass(), FName(TEXT("TargetArray")));
		if (!ArrayProp)
		{
			return false;
		}

		FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Defender));
		const FObjectProperty* Inner = CastField<FObjectProperty>(ArrayProp->Inner);
		if (!Inner || Helper.Num() <= 0 || !Helper.IsValidIndex(0))
		{
			return false;
		}

		AActor* Target = Cast<AActor>(Inner->GetObjectPropertyValue(Helper.GetRawPtr(0)));
		if (!IsValid(Target))
		{
			Helper.RemoveValues(0);
			return false;
		}

		UGameplayStatics::ApplyDamage(Target, Damage, nullptr, Defender, nullptr);
		if (!IsValid(Target) || ReadActorHealth(Target) <= 0.0)
		{
			for (int32 Index = Helper.Num() - 1; Index >= 0; --Index)
			{
				AActor* Existing = Cast<AActor>(Inner->GetObjectPropertyValue(Helper.GetRawPtr(Index)));
				if (Existing == Target || !IsValid(Existing))
				{
					Helper.RemoveValues(Index);
				}
			}
		}

		return true;
	}

	float EnemyStrikeDamage(const AActor* Actor)
	{
		const FString Name = Actor->GetClass()->GetName();
		if (Name.Contains(TEXT("Ranged")))
		{
			return 12.0f;
		}
		if (Name.Contains(TEXT("Heavy")))
		{
			return 18.0f;
		}
		if (Name.Contains(TEXT("Light_Enemy")) || Name.Contains(TEXT("BP_Enemy")))
		{
			return 8.0f;
		}
		return 0.0f;
	}

	bool EnemyIsAttacking(const AActor* Actor)
	{
		const FBoolProperty* AttackingProp = FindFProperty<FBoolProperty>(Actor->GetClass(), FName(TEXT("isAttacking?")));
		if (!AttackingProp)
		{
			AttackingProp = FindFProperty<FBoolProperty>(Actor->GetClass(), FName(TEXT("isAttacking")));
		}
		if (!AttackingProp)
		{
			return true;
		}
		return AttackingProp->GetPropertyValue_InContainer(Actor);
	}

	bool StrikeTargetTower(AActor* Enemy, float Damage)
	{
		if (!EnemyIsAttacking(Enemy))
		{
			return false;
		}

		const FObjectProperty* TargetProp = FindFProperty<FObjectProperty>(Enemy->GetClass(), FName(TEXT("TargetTower")));
		if (!TargetProp)
		{
			return false;
		}

		AActor* Target = Cast<AActor>(TargetProp->GetObjectPropertyValue_InContainer(Enemy));
		if (!IsValid(Target))
		{
			return false;
		}

		UGameplayStatics::ApplyDamage(Target, Damage, nullptr, Enemy, nullptr);
		return true;
	}
}

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
	SeparateSpawnFromExit();
}

void ASafeGridGenerator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ClearBlueprintSpawnTimer();
	ApplyDistinctCombatStats(DeltaSeconds);
}

void ASafeGridGenerator::ApplyDistinctCombatStats(float DeltaSeconds)
{
	for (auto It = StatsAssigned.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
	}
	for (auto It = DefenderShotCooldown.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}
	for (auto It = EnemyStrikeCooldown.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor) || Actor == this)
		{
			continue;
		}

		if (!StatsAssigned.Contains(Actor))
		{
			double Health = 0.0;
			if (StartingHealthFor(Actor, Health))
			{
				WriteActorHealth(Actor, Health);
				StatsAssigned.Add(Actor);
			}
		}

		const float DefenderDamage = ShotDamageForDefender(Actor);
		if (DefenderDamage > 0.0f)
		{
			if (Actor->GetClass()->GetName().Contains(TEXT("Infantry")))
			{
				ClearNamedTimer(Actor, TEXT("FireTimerHandle"));
			}

			float& Cooldown = DefenderShotCooldown.FindOrAdd(Actor, 0.0f);
			Cooldown -= DeltaSeconds;
			if (Cooldown <= 0.0f && FireAtFirstTarget(Actor, DefenderDamage))
			{
				Cooldown = ReadFireRate(Actor);
			}
		}

		const float EnemyDamage = EnemyStrikeDamage(Actor);
		if (EnemyDamage <= 0.0f)
		{
			continue;
		}

		ClearNamedTimer(Actor, TEXT("AttackTimerHandle"));
		float& StrikeCooldown = EnemyStrikeCooldown.FindOrAdd(Actor, 0.0f);
		StrikeCooldown -= DeltaSeconds;
		if (StrikeCooldown <= 0.0f && StrikeTargetTower(Actor, EnemyDamage))
		{
			StrikeCooldown = 1.0f;
		}
	}
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

int32 ASafeGridGenerator::ReadIntProperty(const FName PropertyName, const int32 Fallback) const
{
	if (const FIntProperty* Prop = FindFProperty<FIntProperty>(GetClass(), PropertyName))
	{
		return Prop->GetPropertyValue_InContainer(this);
	}
	return Fallback;
}

double ASafeGridGenerator::ReadDoubleProperty(const FName PropertyName, const double Fallback) const
{
	if (const FDoubleProperty* Prop = FindFProperty<FDoubleProperty>(GetClass(), PropertyName))
	{
		return Prop->GetPropertyValue_InContainer(this);
	}
	if (const FFloatProperty* Prop = FindFProperty<FFloatProperty>(GetClass(), PropertyName))
	{
		return Prop->GetPropertyValue_InContainer(this);
	}
	return Fallback;
}

void ASafeGridGenerator::WriteVector2DArray(const FName PropertyName, const TArray<FVector2D>& Values)
{
	FArrayProperty* ArrayProp = FindFProperty<FArrayProperty>(GetClass(), PropertyName);
	if (!ArrayProp || !CastField<FStructProperty>(ArrayProp->Inner))
	{
		return;
	}

	FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(this));
	Helper.Resize(Values.Num());
	for (int32 Index = 0; Index < Values.Num(); ++Index)
	{
		*reinterpret_cast<FVector2D*>(Helper.GetRawPtr(Index)) = Values[Index];
	}
}

void ASafeGridGenerator::SeparateSpawnFromExit()
{
	const int32 Width = FMath::Max(1, ReadIntProperty(TEXT("GridWidth"), 1));
	const int32 Height = FMath::Max(1, ReadIntProperty(TEXT("GridHeight"), 1));
	const double TileSize = FMath::Max(1.0, ReadDoubleProperty(TEXT("TileSize"), 100.0));

	TArray<FVector2D> Path;
	auto TryWalk = [&]() -> bool
	{
		Path.Reset();
		TSet<FIntPoint> Occupied;
		FIntPoint Current(0, FMath::RandRange(0, Height - 1));
		Path.Add(FVector2D(Current.X, Current.Y));
		Occupied.Add(Current);

		const int32 MaxSteps = Width * Height;
		for (int32 Step = 0; Current.X < Width - 1 && Step < MaxSteps; ++Step)
		{
			TArray<FIntPoint> Options;
			const FIntPoint Deltas[] = { FIntPoint(1, 0), FIntPoint(0, 1), FIntPoint(0, -1) };
			for (const FIntPoint& Delta : Deltas)
			{
				const FIntPoint Next = Current + Delta;
				if (Next.X < 0 || Next.X >= Width || Next.Y < 0 || Next.Y >= Height)
				{
					continue;
				}
				if (Occupied.Contains(Next))
				{
					continue;
				}
				Options.Add(Next);
			}

			if (Options.Num() == 0)
			{
				return false;
			}

			FIntPoint Chosen = Options[FMath::RandRange(0, Options.Num() - 1)];
			if (FMath::FRand() < 0.65f)
			{
				for (const FIntPoint& Option : Options)
				{
					if (Option.X > Current.X)
					{
						Chosen = Option;
						break;
					}
				}
			}

			Current = Chosen;
			Path.Add(FVector2D(Current.X, Current.Y));
			Occupied.Add(Current);
		}

		return Path.Num() > 0 && FMath::IsNearlyEqual(Path.Last().X, static_cast<double>(Width - 1));
	};

	bool bReachedFarEdge = false;
	for (int32 Attempt = 0; Attempt < 32 && !bReachedFarEdge; ++Attempt)
	{
		bReachedFarEdge = TryWalk();
	}

	if (!bReachedFarEdge)
	{
		Path.Reset();
		const int32 Row = FMath::RandRange(0, Height - 1);
		for (int32 X = 0; X < Width; ++X)
		{
			Path.Add(FVector2D(X, Row));
		}
	}

	WriteVector2DArray(TEXT("PathGridCoords"), Path);
	WriteVector2DArray(TEXT("OccupiedCoords"), Path);

	auto FindISM = [this](const TCHAR* NameFragment) -> UInstancedStaticMeshComponent*
	{
		TArray<UInstancedStaticMeshComponent*> Components;
		GetComponents(Components);
		for (UInstancedStaticMeshComponent* Component : Components)
		{
			if (Component && Component->GetName().Contains(NameFragment))
			{
				return Component;
			}
		}
		return nullptr;
	};

	UInstancedStaticMeshComponent* Entrance = FindISM(TEXT("Entrance"));
	UInstancedStaticMeshComponent* Exit = FindISM(TEXT("Exit"));
	UInstancedStaticMeshComponent* PathMesh = FindISM(TEXT("ISM_Path"));
	UInstancedStaticMeshComponent* Obstacle = FindISM(TEXT("Obstacle"));
	for (UInstancedStaticMeshComponent* Mesh : { Entrance, Exit, PathMesh, Obstacle })
	{
		if (Mesh)
		{
			Mesh->ClearInstances();
		}
	}

	UClass* SocketClass = LoadClass<AActor>(nullptr, TEXT("/Game/Blueprints/BP_BuildSocket.BP_BuildSocket_C"));
	if (SocketClass)
	{
		TArray<AActor*> ExistingSockets;
		UGameplayStatics::GetAllActorsOfClass(this, SocketClass, ExistingSockets);
		for (AActor* Socket : ExistingSockets)
		{
			if (IsValid(Socket))
			{
				Socket->Destroy();
			}
		}
	}

	auto GridToWorld = [TileSize](const FVector2D& Coord)
	{
		return FVector(Coord.X * TileSize, Coord.Y * TileSize, 0.0);
	};

	for (int32 Index = 0; Index < Path.Num(); ++Index)
	{
		const FTransform InstanceTransform(GridToWorld(Path[Index]));
		UInstancedStaticMeshComponent* Target = PathMesh;
		if (Index == 0)
		{
			Target = Entrance;
		}
		else if (Index == Path.Num() - 1)
		{
			Target = Exit;
		}
		if (Target)
		{
			Target->AddInstance(InstanceTransform, false);
		}
	}

	TSet<FIntPoint> Occupied;
	for (const FVector2D& Coord : Path)
	{
		Occupied.Add(FIntPoint(FMath::RoundToInt(Coord.X), FMath::RoundToInt(Coord.Y)));
	}

	UWorld* World = GetWorld();
	for (int32 X = 0; X < Width; ++X)
	{
		for (int32 Y = 0; Y < Height; ++Y)
		{
			if (Occupied.Contains(FIntPoint(X, Y)))
			{
				continue;
			}

			const FVector Location = GridToWorld(FVector2D(X, Y));
			if (FMath::FRand() <= 0.3f)
			{
				if (Obstacle)
				{
					Obstacle->AddInstance(FTransform(Location), false);
				}
			}
			else if (World && SocketClass)
			{
				World->SpawnActor<AActor>(SocketClass, FTransform(Location));
			}
		}
	}

	if (USplineComponent* Spline = GetPathSpline())
	{
		Spline->ClearSplinePoints(false);
		for (const FVector2D& Coord : Path)
		{
			Spline->AddSplinePoint(GridToWorld(Coord), ESplineCoordinateSpace::World, false);
		}
		Spline->UpdateSpline();
	}

	if (Path.Num() > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("Path crosses the map from (%.0f, %.0f) to (%.0f, %.0f), %d tiles."),
			Path[0].X, Path[0].Y, Path.Last().X, Path.Last().Y, Path.Num());
	}
}
