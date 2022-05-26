// Copyright Aiden. S. All Rights Reserved


#include "Actors/GenericFoliageActor.h"

#include "GenericFoliage.h"
#include "Actors/Components/FoliageCaptureComponent.h"
#include "Actors/Components/FoliageInstancedMeshPool.h"
#include "Components/SceneCaptureComponent2D.h"
#include "GameFramework/PlayerController.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

#if WITH_EDITOR
#include "Editor.h"
#include "EditorViewportClient.h"
#endif

// Sets default values
AGenericFoliageActor::AGenericFoliageActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//
	SetRootComponent(CreateDefaultSubobject<USceneComponent>("Root"));

	bAllowTickBeforeBeginPlay = false;
}

void AGenericFoliageActor::Setup()
{
	SetupFoliageCaptureComponents();
	RebuildInstancedMeshPool(true);
	SetupTextureTargets();

	for (UFoliageCaptureComponent* CaptureComponent : GetFoliageCaptureComponents())
	{
		CaptureComponent->SetDiameter(Diameter);
	}
}

// Called when the game starts or when spawned
void AGenericFoliageActor::BeginPlay()
{
	Super::BeginPlay();

	Setup();
}

bool AGenericFoliageActor::ShouldTickIfViewportsOnly() const
{
	return true;
}

void AGenericFoliageActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (IsValid(GetWorld())) {
		UE_LOG(LogGenericFoliage, Verbose, TEXT("Called construction script"));
		if (bUsingSharedResources)
		{
			CaptureTasksPerTick = 1;
		}

		bool bCanSetup = HasAnyFoliageTypes();

		if (bCanSetup)
		{
			Setup();
		}
	}
}

#if WITH_EDITOR
void AGenericFoliageActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	bool bUpdateFoliage = false;

	for (UFoliageCaptureComponent* CaptureComponent : GetFoliageCaptureComponents())
	{
		if (IsValid(CaptureComponent))
		{
			if (CaptureComponent->Diameter != Diameter)
			{
				bUpdateFoliage = true;
			}
			CaptureComponent->SetDiameter(Diameter);
		}
	}

	if (TilePixelSize != SceneNormalRT->SizeX)
	{
		SetupTextureTargets();
		bUpdateFoliage = true;
	}

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(AGenericFoliageActor, TileCount) ||
		PropertyChangedEvent.GetPropertyName()
		== GET_MEMBER_NAME_CHECKED(AGenericFoliageActor, FoliageTypes))
	{
		CaptureTickQueue.Empty();
		FoliageTickQueue.Empty();
		if (HasAnyFoliageTypes())
		{
			Setup();
			bUpdateFoliage = true;
		}
	}

	if (bUpdateFoliage)
	{
		bForceUpdate = true;
	}
}
#endif

// Called every frame
void AGenericFoliageActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Debug

	/*
	TArray<FString> TilesCurrentlyBuilding;
	for (const FIntPoint& Point : GetBuildingTiles())
	{
		TilesCurrentlyBuilding.Add(Point.ToString());
	}

	int32 InstanceCount = GetTotalInstanceCount();

	GEngine->AddOnScreenDebugMessage(
		-1, DeltaTime, FColor::Red, FString::Printf(
			TEXT("%s - Tiles building: [%s] - Instance count: %i - CanUpdate: %s"),
			*GetName(), *FString::Join(TilesCurrentlyBuilding, TEXT(", ")),
			InstanceCount, IsReadyToUpdate() ? TEXT("true") : TEXT("false")
		)
	);
	*/

	// __

	if (UpdateTime > UpdateFrequency && !bDisableUpdates && IsReadyToUpdate())
	{
		UpdateTime = 0.f;

		bool bHasCamera = false;
		FVector CameraLocation;
		FRotator CameraRotation;

		GetCameraInfo(CameraLocation, CameraRotation, bHasCamera);
		if (bHasCamera)
		{
			double Velocity = (CameraLocation - LastCameraPosition).Length();
			LastCameraPosition = CameraLocation;

			CameraLocation = AdjustWorldPositionHeightToPlanet(CameraLocation, 2000);
			UpdateNearestTileID(CameraLocation);

			if ((FVector::Distance(CameraLocation, LastUpdatePosition) > Diameter * (float)TileCount.Size() * 0.95 &&
				Velocity < VelocityUpdateThreshold) || bForceUpdate)
			{

				UE_LOG(LogGenericFoliage, Verbose, TEXT("Updated foliage with camera position from to: %s %s"),
					*LastUpdatePosition.ToString(),
					*CameraLocation.ToString()
				);

				LastUpdatePosition = CameraLocation;
				CaptureTickQueue.Empty();
				FoliageTickQueue.Empty();

				for (UFoliageCaptureComponent* CaptureComponent : GetFoliageCaptureComponents())
				{
					CaptureTickQueue.Emplace([this, CaptureComponent, CameraLocation]()
					{
						if (IsValid(CaptureComponent))
						{
							const FRotator NewCameraRotation = UKismetMathLibrary::ComposeRotators(
								FRotator(-90.f, 90, 0), CalculateEastNorthUp(CameraLocation));

							CaptureComponent->DistanceAboveSurface = 2000.0;
							CaptureComponent->SetWorldLocation(CameraLocation);
							CaptureComponent->SetRelativeRotation(NewCameraRotation);

							FVector TilePosition = CaptureComponent->GetComponentLocation() +
								CaptureComponent->GetRightVector() * (CaptureComponent->Diameter * CaptureComponent->
									TileID.
									X) +
								CaptureComponent->GetUpVector() * (CaptureComponent->Diameter * CaptureComponent->TileID
									.Y);

							CaptureComponent->SetWorldLocation(TilePosition);

							CaptureComponent->PrepareForCapture(SceneColourRT, SceneNormalRT, SceneDepthRT);
							CaptureComponent->Capture();
							CaptureComponent->Compute();
							CaptureComponent->Finish();
						}
					});
				}
				bForceUpdate = false;
			}
		}
	}
	else
	{
		UpdateTime += DeltaTime;
	}

	if (CaptureTickQueue.Num() > 0)
	{
		if (IsReadyToUpdate())
		{
			for (int32 i = 0; i < CaptureTasksPerTick; ++i)
			{
				if (CaptureTickQueue.Num() == 0)
				{
					break;
				}
				CaptureTickQueue[0]();
				CaptureTickQueue.RemoveAt(0, 1, true);
			}
		}
	}

	if (FoliageTickQueue.Num() > 0)
	{
		for (int32 i = 0; i < FoliageTasksPerTick; ++i)
		{
			if (FoliageTickQueue.Num() == 0)
			{
				break;
			}
			FoliageTickQueue[0]();
			FoliageTickQueue.RemoveAt(0, 1, true);
		}
	}
}

void AGenericFoliageActor::GetCameraInfo(FVector& Location, FRotator& Rotation, bool& bSuccess) const
{
	bSuccess = false;

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC)
	{
		FMinimalViewInfo ViewInfo;
		PC->CalcCamera(GetWorld()->GetDeltaSeconds(), ViewInfo);
		Location = ViewInfo.Location;
		Rotation = ViewInfo.Rotation;
		bSuccess = true;
	}
	else
	{
#if WITH_EDITOR
		const FEditorViewportClient* Client = static_cast<FEditorViewportClient*>(GEditor->GetActiveViewport()->
			GetClient());
		if (Client)
		{
			Location = Client->GetViewLocation();
			Rotation = Client->GetViewRotation();
			bSuccess = true;
		}
#endif
	}
}

bool AGenericFoliageActor::IsReadyToUpdate() const
{
	bool bIsReadyToUpdate = true;
	for (const UFoliageCaptureComponent* FoliageCaptureComponent : GetFoliageCaptureComponents())
	{
		if (IsValid(FoliageCaptureComponent))
		{
			bIsReadyToUpdate = bIsReadyToUpdate && (FoliageCaptureComponent->IsReadyToUpdate());
		}
	}
	if (FoliageTypes.Num() == 0)
	{
		bIsReadyToUpdate = false;
	}
	else
	{
		for (UGenericFoliageType* FoliageType : FoliageTypes)
		{
			for (const auto& TilePair : TileInstancedMeshPools)
			{
				bIsReadyToUpdate = bIsReadyToUpdate && TilePair.Value->HISMPool.Contains(FoliageType->GetGuid());
			}
		}
	}

	return bIsReadyToUpdate;
}

void AGenericFoliageActor::SetupTextureTargets()
{
	if (bUsingSharedResources)
	{
		auto MakeName = [&](FString InName)
		{
			return FName(*FString::Printf(TEXT("%s_%s_%i"), *InName, *GetName(), TilePixelSize));
		};

		SceneColourRT = NewObject<UTextureRenderTarget2D>(this, MakeName("SceneColourRT"), RF_Transient);
		check(SceneColourRT);

		SceneColourRT->InitCustomFormat(TilePixelSize, TilePixelSize, EPixelFormat::PF_B8G8R8A8, true);
		SceneColourRT->RenderTargetFormat = RTF_RGBA8;
		SceneColourRT->UpdateResourceImmediate(true);

		SceneDepthRT = NewObject<UTextureRenderTarget2D>(this, MakeName("SceneDepthRT"), RF_Transient);
		check(SceneDepthRT);

		SceneDepthRT->RenderTargetFormat = RTF_R32f;
		SceneDepthRT->InitAutoFormat(TilePixelSize, TilePixelSize);
		SceneDepthRT->UpdateResourceImmediate(true);

		SceneNormalRT = NewObject<UTextureRenderTarget2D>(this, MakeName("SceneNormalRT"), RF_Transient);
		check(SceneNormalRT);

		SceneNormalRT->RenderTargetFormat = RTF_RGBA8;
		SceneNormalRT->SRGB = true;
		SceneNormalRT->LODGroup = TextureGroup::TEXTUREGROUP_WorldNormalMap;
		SceneNormalRT->InitAutoFormat(TilePixelSize, TilePixelSize);
		SceneNormalRT->UpdateResourceImmediate(true);
	}
}

void AGenericFoliageActor::SetupFoliageCaptureComponents()
{
	for (UFoliageCaptureComponent* CaptureComponent : GetFoliageCaptureComponents())
	{
		if (IsValid(CaptureComponent))
		{
			CaptureComponent->DestroyComponent();
		}
	}

	for (auto& KV : TileInstancedMeshPools)
	{
		if (IsValid(KV.Value))
		{
			KV.Value->DestroyComponent();
		}
	}

	TileInstancedMeshPools.Reset();

	for (int x = -TileCount.X; x <= TileCount.X; ++x)
	{
		for (int y = -TileCount.Y; y <= TileCount.Y; ++y)
		{
			UFoliageCaptureComponent* FoliageCaptureComponent = Cast<UFoliageCaptureComponent>(
				AddComponentByClass(UFoliageCaptureComponent::StaticClass(), false, FTransform::Identity, false)
			);

			check(FoliageCaptureComponent);
			FoliageCaptureComponent->TileID = {x, y};
			FoliageCaptureComponent->AttachToComponent(GetRootComponent(),
			                                           FAttachmentTransformRules{EAttachmentRule::KeepRelative, false});

			if (!FoliageCaptureComponent->IsUsingSharedResources())
			{
				FoliageCaptureComponent->SetupTextureTargets(TilePixelSize);
			}

			UFoliageInstancedMeshPool* InstancedMeshPool = Cast<UFoliageInstancedMeshPool>(
				AddComponentByClass(UFoliageInstancedMeshPool::StaticClass(), true, FTransform::Identity, false)
			);

			check(InstancedMeshPool);

			if (x == 0 && y == 0)
			{
				InstancedMeshPool->bEnableCollision = true;
			}
			else
			{
				InstancedMeshPool->bEnableCollision = false;
			}

			TileInstancedMeshPools.Add(FIntPoint(x, y), InstancedMeshPool);
		}
	}
}

void AGenericFoliageActor::RebuildInstancedMeshPool(bool bImmediate /* false */)
{
	if (TileInstancedMeshPools.Num() == 0)
	{
		return;
	}

	auto Task = [this]()
	{
		for (const TPair<FIntPoint, UFoliageInstancedMeshPool*>& MeshPoolPair : TileInstancedMeshPools)
		{
			if (FoliageTypes.Num() > 0 && IsValid(MeshPoolPair.Value))
			{
				MeshPoolPair.Value->RebuildHISMPool(FoliageTypes);
			}
		}
	};

	// bReadyToUpdate is set from the mesh pool, if not valid we will refresh.
	if (bImmediate)
	{
		Task();
	}
	else
	{
		CaptureTickQueue.Emplace(MoveTemp(Task));
	}
}

TArray<UFoliageCaptureComponent*> AGenericFoliageActor::GetFoliageCaptureComponents() const
{
	TArray<USceneComponent*> ChildComponents;
	GetRootComponent()->GetChildrenComponents(false, ChildComponents);

	TArray<UFoliageCaptureComponent*> Result;

	for (USceneComponent* Comp : ChildComponents)
	{
		UFoliageCaptureComponent* CastedComp = Cast<UFoliageCaptureComponent>(Comp);
		if (IsValid(CastedComp))
		{
			Result.Add(CastedComp);
		}
	}

	if (Result.Num() > 0)
	{
		Algo::Sort(Result, [&](const UFoliageCaptureComponent* A, const UFoliageCaptureComponent* B)
		{
			return A->TileID.SizeSquared() < B->TileID.SizeSquared();
			// return FVector2D::DistSquared(FVector2D(A->TileID), FVector2D(NearestTileID)) < FVector2D::DistSquared(FVector2D(B->TileID), FVector2D(NearestTileID));
		});
	}

	return Result;
}

void AGenericFoliageActor::UpdateNearestTileID(const FVector& InWorldLocation)
{
	bool bHasAnyTile = false;
	for (const UFoliageCaptureComponent* CaptureComponent : GetFoliageCaptureComponents())
	{
		const FVector TransformedPosition = CaptureComponent->GetComponentTransform().
		                                                      InverseTransformPosition(InWorldLocation);
		if (CaptureComponent->BoundingBox.IsInsideXY(TransformedPosition))
		{
			NearestTileID = CaptureComponent->TileID;
			if (LastNearestTileID != NearestTileID)
			{
				TileInstancedMeshPools[NearestTileID]->ToggleCollision(true);
			}
			LastNearestTileID = NearestTileID;
			bHasNearestTile = true;
			bHasAnyTile = true;
		}
		else
		{
			TileInstancedMeshPools[CaptureComponent->TileID]->ToggleCollision(false);
		}
	}

	if (!bHasAnyTile)
	{
		bHasNearestTile = false;
	}
}

TArray<FIntPoint> AGenericFoliageActor::GetBuildingTiles() const
{
	TArray<FIntPoint> Result;
	for (UFoliageCaptureComponent* CaptureComponent : GetFoliageCaptureComponents())
	{
		if (!CaptureComponent->IsReadyToUpdate())
		{
			Result.Add(CaptureComponent->TileID);
		}
	}
	return Result;
}

int32 AGenericFoliageActor::GetTotalInstanceCount() const
{
	int32 Count = 0;

	for (const auto& MeshPoolPair : TileInstancedMeshPools)
	{
		if (IsValid(MeshPoolPair.Value))
		{
			Count += MeshPoolPair.Value->GetTotalInstanceCount();
		}
	}

	return Count;
}

void AGenericFoliageActor::EnqueueCaptureTickTask(TFunction<void()>&& InFunc)
{
	CaptureTickQueue.Emplace(InFunc);
}

void AGenericFoliageActor::EnqueueFoliageTickTask(TFunction<void()>&& InFunc)
{
	FoliageTickQueue.Emplace(InFunc);
}

FVector AGenericFoliageActor::WorldToLocalPosition(const FVector& InWorldLocation) const
{
	return GetTransform().Inverse().TransformPosition(InWorldLocation);
}

FVector AGenericFoliageActor::LocalToWorldPosition(const FVector& InLocalLocation) const
{
	return GetTransform().TransformPosition(InLocalLocation);
}

FRotator AGenericFoliageActor::CalculateEastNorthUp(const FVector& InWorldLocation) const
{
	return FRotator(0.0);
}

FVector AGenericFoliageActor::AdjustWorldPositionHeightToPlanet(const FVector& InWorldLocation,
                                                                const double& Height) const
{
	FVector LocalPosition = WorldToLocalPosition(InWorldLocation);
	LocalPosition.Z = Height;

	return LocalToWorldPosition(
		LocalPosition
	);
}

void AGenericFoliageActor::SetShowOnlyActors(TArray<AActor*> InShowOnlyActors)
{
	for (UFoliageCaptureComponent* CaptureComponent : GetFoliageCaptureComponents())
	{
		if (IsValid(CaptureComponent) && IsValid(CaptureComponent->SceneColourCapture) && IsValid(
			CaptureComponent->SceneDepthCapture))
		{
			CaptureComponent->SceneColourCapture->ShowOnlyActors = InShowOnlyActors;
			CaptureComponent->SceneColourCapture->PrimitiveRenderMode =
				ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;

			CaptureComponent->SceneDepthCapture->ShowOnlyActors = InShowOnlyActors;
			CaptureComponent->SceneDepthCapture->PrimitiveRenderMode =
				ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
		}
	}
}

void AGenericFoliageActor::SetIsReadyToUpdate(bool bNewState)
{
	bReadyToUpdate = bNewState;
}

bool AGenericFoliageActor::HasAnyFoliageTypes()
{
	bool bCanSetup = true;
	for (UGenericFoliageType* FoliageType : FoliageTypes)
	{
		if (!IsValid(FoliageType))
		{
			bCanSetup = false;
			break;
		}
		else
		{
			if (!IsValid(FoliageType->FoliageMesh))
			{
				bCanSetup = false;
				break;
			}
		}
	}
	return bCanSetup;
}

void AGenericFoliageActor::RefreshFoliage()
{
	bForceUpdate = true;
}
