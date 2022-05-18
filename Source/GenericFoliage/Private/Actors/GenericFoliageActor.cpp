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

	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			UFoliageCaptureComponent* FoliageCaptureComponent = CreateDefaultSubobject<UFoliageCaptureComponent>(
				FName(FString::Printf(TEXT("%i%i_FoliageCapture"), x, y)), true);
			FoliageCaptureComponent->TileID = {x, y};
			FoliageCaptureComponent->SetupAttachment(GetRootComponent());

			UFoliageInstancedMeshPool* InstancedMeshPool = CreateDefaultSubobject<UFoliageInstancedMeshPool>(
				FName(FString::Printf(TEXT("%i%i_FoliageISMPool"), x, y)), true);

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

	bAllowTickBeforeBeginPlay = false;
}

// Called when the game starts or when spawned
void AGenericFoliageActor::BeginPlay()
{
	Super::BeginPlay();

	RebuildInstancedMeshPool();
}

bool AGenericFoliageActor::ShouldTickIfViewportsOnly() const
{
	return true;
}

void AGenericFoliageActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	RebuildInstancedMeshPool();
	SetupTextureTargets();
	UE_LOG(LogGenericFoliage, Display, TEXT("Called construction script"));

	for (UFoliageCaptureComponent* CaptureComponent: GetFoliageCaptureComponents())
	{
		CaptureComponent->SetDiameter(Diameter);
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
	TArray<FString> TilesCurrentlyBuilding;
	for (const FIntPoint& Point: GetBuildingTiles())
	{
		TilesCurrentlyBuilding.Add(Point.ToString());
	}

	int32 InstanceCount = GetTotalInstanceCount();

	GEngine->AddOnScreenDebugMessage(
		-1, DeltaTime, FColor::Red, FString::Printf(
			TEXT("%s - Tiles building: [%s] - Instance count: %i"),
			*GetName(), *FString::Join(TilesCurrentlyBuilding, TEXT(", ")),
			InstanceCount
		)
	);
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
			CameraLocation = AdjustWorldPositionHeightToPlanet(CameraLocation, 2000);
			UpdateNearestTileID(CameraLocation);

			if (FVector::Distance(CameraLocation, LastUpdatePosition) > Diameter || bForceUpdate)
			{
				LastUpdatePosition = CameraLocation;
				CaptureTickQueue.Empty();
				FoliageTickQueue.Empty();

				for (int32 Index = 0; Index < GetRootComponent()->GetNumChildrenComponents(); ++Index)
				{
					CaptureTickQueue.Emplace([this, Index, CameraLocation]()
					{
						UFoliageCaptureComponent* CaptureComponent = Cast<UFoliageCaptureComponent>(
							GetRootComponent()->GetChildComponent(Index));
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
			CaptureTickQueue.Pop()();
		}
	}

	if (FoliageTickQueue.Num() > 0)
	{
		FoliageTickQueue.Pop()();
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
			bIsReadyToUpdate = bIsReadyToUpdate && FoliageCaptureComponent->IsReadyToUpdate();
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

void AGenericFoliageActor::RebuildInstancedMeshPool()
{
	CaptureTickQueue.Emplace([this]()
	{
		for (auto& MeshPoolPair : TileInstancedMeshPools)
		{
			if (FoliageTypes.Num() > 0)
			{
				MeshPoolPair.Value->RebuildHISMPool(FoliageTypes);
			}
		}
	});
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
	for (UFoliageCaptureComponent* CaptureComponent: GetFoliageCaptureComponents())
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

	for (const auto& MeshPoolPair: TileInstancedMeshPools)
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

void AGenericFoliageActor::RefreshFoliage()
{
	bForceUpdate = true;
}
