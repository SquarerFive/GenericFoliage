// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/ClusterFoliageActor.h"

#include "GenericFoliage.h"
#include "SamplerLibrary.h"
#include "Actors/Components/FoliageInstancedMeshPool.h"
#include "Async/Async.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "GeometryScript/MeshSpatialFunctions.h"
#include "Kismet/KismetMathLibrary.h"
#include "Spatial/MeshAABBTree3.h"


class FClusterFoliageSpawnerThread : public FRunnable
{
public:
	FClusterFoliageSpawnerThread(AClusterFoliageActor* InClusterFoliageActor, int32 InFeatureId,
	                             TFunction<void(int32, TMap<FGuid, TArray<FTransform>>)> InCallback
	)
		: ClusterFoliageActor(InClusterFoliageActor), FeatureId(InFeatureId),
		  Callback(InCallback)
	{
		bIsRunning = true;
		Thread = MakeShareable(FRunnableThread::Create(
			this, TEXT("ClusterFoliageSpawnerThread")
		));
	}

	virtual bool Init() override
	{
		return true;
	}

	virtual uint32 Run() override
	{
		if (!IsValid(ClusterFoliageActor))
		{
			bIsRunning = false;
			return 1;
		}

		FSpatialFeature Feature = ClusterFoliageActor->GetFeatureById(FeatureId);
		TMap<FGuid, TArray<FTransform>> FoliageTransforms;

		Feature.Geometry->ProcessMesh([&](const FDynamicMesh3& Mesh)
		{
			const auto Bounds = Mesh.GetBounds();
			const auto Bounds2d = FBox2d(FVector2D(Bounds.Min.X, Bounds.Min.Y), FVector2D(Bounds.Max.X, Bounds.Max.Y));
			const double Width = Bounds.Width();
			const double Height = Bounds.Height();

			for (const auto Type : ClusterFoliageActor->Collection->Collection[Feature.Type].FoliageTypes)
			{
				auto Points = USamplerLibrary::PoissonDiscSampling2d(Type->Density, FVector2d(Width, Height));

				for (FVector2d& Point : Points)
				{
					Point += Bounds2d.Min;
				}

				if (!IsValid(ClusterFoliageActor) || !IsValid(Feature.Geometry)) { return; }

				TArray<FVector2d> FilteredPoints;
				TArray<FTransform> PointsWorld;

				for (const FVector2d& Point : Points)
				{
					FVector Normal = FVector::Zero();
					const double Altitude = ClusterFoliageActor->GetTerrainBaseHeight(
						FVector(Point.X, Point.Y, 0), Normal);

					bool bInside = false;

					TEnumAsByte<EGeometryScriptContainmentOutcomePins> Outcome;
					UGeometryScriptLibrary_MeshSpatial::IsPointInsideMesh(
						Feature.Geometry, Feature.BVH, FVector(Point.X, Point.Y, 100), {}, bInside,
						Outcome);

					if (!bInside)
					{
						continue;
					}

					FVector EngineLocation = ClusterFoliageActor->GeographicToEngineLocation(
							FVector(Point.X, Point.Y, Altitude)) +
						Type->GetRandomLocalOffset();

					FVector UpVector = ClusterFoliageActor->GetUpVectorFromEngineLocation(EngineLocation);
					FRotator RotationAtPoint = Type->bAlignToSurfaceNormal
						                           ? Normal.Rotation()
						                           : UKismetMathLibrary::MakeRotFromZ(UpVector);

					const double Angle = FMath::RadiansToDegrees(FMath::Acos(
						Normal | UpVector
					));

					if (Angle > Type->SlopeAngleThreshold)
					{
						continue;
					}

					if (Type->bEnableRandomRotation)
					{
						RotationAtPoint = (Type->GetRandomRotator().Quaternion() * RotationAtPoint.Quaternion()).
							Rotator();
					}

					FTransform Transform;
					Transform.SetLocation(
						EngineLocation
					);
					Transform.SetScale3D(Type->GetRandomScale());
					Transform.SetRotation(RotationAtPoint.Quaternion());

					PointsWorld.Emplace(MoveTemp(Transform));
				}

				FoliageTransforms.Emplace(Type->GetGuid(), MoveTemp(PointsWorld));
			}
		});

		AsyncTask(ENamedThreads::GameThread,
		          [Callback = this->Callback, FeatureId = this->FeatureId, FoliageTransforms]()
		          {
			          Callback(FeatureId, FoliageTransforms);
		          });
		return 0;
	}

	void Cancel()
	{
		if (Thread.IsValid())
		{
			this->Thread->Kill();
			bIsCancelled = true;
		}
	}

	virtual void Exit() override
	{
	}

	bool IsRunning() const
	{
		return bIsRunning;
	}

private:
	std::atomic<bool> bIsCancelled = false;
	std::atomic<bool> bIsRunning = false;
	TSharedPtr<FRunnableThread> Thread;
	AClusterFoliageActor* ClusterFoliageActor;
	int32 FeatureId;
	TFunction<void(int32, TMap<FGuid, TArray<FTransform>>)> Callback;
};

// Sets default values
AClusterFoliageActor::AClusterFoliageActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MeshPool = CreateDefaultSubobject<UDynamicMeshPool>("MeshPool", true);
	InstancedMeshPool = CreateDefaultSubobject<UFoliageInstancedMeshPool>("InstancedMeshPool", true);
}

// Called when the game starts or when spawned
void AClusterFoliageActor::BeginPlay()
{
	Super::BeginPlay();
}

void AClusterFoliageActor::BeginDestroy()
{
	Super::BeginDestroy();
	KillAllThreads();
}

// Called every frame
void AClusterFoliageActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


FVector AClusterFoliageActor::GeographicToEngineLocation(const FVector& GeographicLocation)
{
	return GeographicLocation;
}

FVector AClusterFoliageActor::EngineToGeographicLocation(const FVector& EngineLocation)
{
	return EngineLocation;
}

FVector AClusterFoliageActor::GetUpVectorFromGeographicLocation(const FVector& GeographicLocation)
{
	return FVector(0, 0, 1);
}

FVector AClusterFoliageActor::GetUpVectorFromEngineLocation(const FVector& EngineLocation)
{
	return FVector(0, 0, 1);
}

double AClusterFoliageActor::GetTerrainBaseHeight(const FVector& GeographicLocation, FVector& OutNormal)
{
	FHitResult Result;
	const FVector StartLocation = GeographicToEngineLocation(
		FVector(GeographicLocation.X, GeographicLocation.Y, 9000.0));
	const FVector EndLocation =
		GeographicToEngineLocation(FVector(GeographicLocation.X, GeographicLocation.Y, -1000.0));
	if (GetWorld()->LineTraceSingleByChannel(Result, StartLocation, EndLocation, ECollisionChannel::ECC_Visibility))
	{
		OutNormal = Result.ImpactNormal;
		return EngineToGeographicLocation(Result.Location).Z;
	}

	return 0.0;
}

void AClusterFoliageActor::SetupInstancedMeshPool()
{
	if (!IsValid(Collection))
	{
		return;
	}

	InstancedMeshPool->bEnableCollision = true;
	TArray<UGenericFoliageType*> FoliageTypes;

	for (auto CollectionPair : Collection->Collection)
	{
		for (auto FoliageType : CollectionPair.Value.FoliageTypes)
		{
			FoliageTypes.AddUnique(FoliageType);
		}
	}

	InstancedMeshPool->RebuildHISMPool(FoliageTypes);
}

FSpatialFeature AClusterFoliageActor::GetFeatureById(int32 Id)
{
	FSpatialFeature* FoundFeature = Features.FindByPredicate([Id](const FSpatialFeature& Feature)
	{
		return Feature.Id == Id;
	});

	return *FoundFeature;
}

void AClusterFoliageActor::LoadGeoJSON(const FString& Data)
{
	if (AnyThreadsRunning())
	{
		UE_LOG(LogGenericFoliage, Warning, TEXT("Foliage still building, not loading GeoJSON!"));
		return;
	}

	for (auto DMC : MeshComponents)
	{
		DMC->DestroyComponent();
	}
	MeshComponents.Empty();

	MeshPool->ReturnAllMeshes();

	Features = USpatialLibrary::ParseGeoJSON(Data, MeshPool);

	SetupInstancedMeshPool();

	for (FSpatialFeature& Feature : Features)
	{
		if (!IsValid(Feature.Geometry))
		{
			continue;
		}

		int32 FeatureType = Feature.Type;

		if (!Collection->Collection.Contains(FeatureType))
		{
			continue;
		}

		Threads.Add(
			Feature.Id,
			MakeShareable(new FClusterFoliageSpawnerThread(
				this, Feature.Id, [&](int32 FeatureId, TMap<FGuid, TArray<FTransform>> InstancesMap)
				{
					Threads.Remove(FeatureId);
					for (auto& Pair : InstancesMap)
					{
						InstancedMeshPool->HISMPool[Pair.Key]->AddInstances(
							Pair.Value,
							false,
							true
						);
					}
				}
			))
		);

		if (bShowBoundary)
		{
			UDynamicMesh* VisualMesh = MeshPool->RequestMesh();
			VisualMesh->Reset();

			Feature.Geometry->ProcessMesh([&VisualMesh](const FDynamicMesh3& OtherMesh)
			{
				VisualMesh->EditMesh([&OtherMesh](FDynamicMesh3& Mesh)
				{
					Mesh.Copy(OtherMesh);
				});
			});

			VisualMesh->EditMesh(
				[this](UE::Geometry::FDynamicMesh3& Mesh)
				{
					for (int i = 0; i < Mesh.VertexCount(); ++i)
					{
						auto Vertex = Mesh.GetVertex(i);
						FVector Normal;
						Vertex.Z += GetTerrainBaseHeight(Vertex, Normal);
						Vertex = GeographicToEngineLocation(Vertex);
						Mesh.SetVertex(i, Vertex);
					}
				},
				EDynamicMeshChangeType::MeshVertexChange,
				EDynamicMeshAttributeChangeFlags::VertexPositions
			);

			UDynamicMeshComponent* DMC = Cast<UDynamicMeshComponent>(
				AddComponentByClass(UDynamicMeshComponent::StaticClass(), true, FTransform::Identity, false));
			ensure(DMC);

			DMC->SetDynamicMesh(VisualMesh);

			if (IsValid(DebugMaterial))
			{
				DMC->SetMaterial(0, DebugMaterial);
			}

			MeshComponents.Add(DMC);
		}
	}
}

bool AClusterFoliageActor::AnyThreadsRunning() const
{
	for (auto Pair : Threads)
	{
		if (Pair.Value.IsValid() && Pair.Value->IsRunning())
		{
			return true;
		}
	}

	return false;
}

void AClusterFoliageActor::KillAllThreads()
{
	for (auto Pair : Threads)
	{
		Pair.Value->Cancel();
	}
	Threads.Empty();
}
