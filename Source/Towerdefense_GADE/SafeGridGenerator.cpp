#include "SafeGridGenerator.h"

#include "EnemyBase.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
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
	SeparateSpawnFromExit();
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

	if (AEnemyBase* Enemy = Cast<AEnemyBase>(Spawned))
	{
		Enemy->SetFollowSpline(Spline);
	}
	else if (FObjectProperty* SplineProp = FindFProperty<FObjectProperty>(Spawned->GetClass(), FName(TEXT("TargetSpline"))))
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

	// Convert grid coords through the actor transform so meshes, sockets and the
	// spline stay aligned when the generator is not at the world origin.
	const FTransform ActorTM = GetActorTransform();
	auto GridToWorld = [TileSize, &ActorTM](const FVector2D& Coord)
	{
		const FVector Local(Coord.X * TileSize, Coord.Y * TileSize, 0.0);
		return ActorTM.TransformPosition(Local);
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
			// true = world space (matches spline points and socket spawns below).
			Target->AddInstance(InstanceTransform, true);
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
					Obstacle->AddInstance(FTransform(Location), true);
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
