// Copyright Aiden. S. All Rights Reserved


#include "Actors/GenericFoliageActor.h"
#include "Actors/Components/FoliageCaptureComponent.h"
#include "Actors/Components/FoliageInstancedMeshPool.h"
#include "Components/SceneCaptureComponent2D.h"
#include "GameFramework/PlayerController.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetMathLibrary.h"

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

			// only enable collision on the nearest tile to the camera
			if (x == 0 && y == 0)
			{
				InstancedMeshPool->bEnableCollision = true;
			} else
			{
				InstancedMeshPool->bEnableCollision = false;
			}
			
			TileInstancedMeshPools.Add(FIntPoint(x, y), InstancedMeshPool);
		}
	}


	SetupTextureTargets();
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
}

#if WITH_EDITOR
void AGenericFoliageActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (bDisableUpdates)
	{
		for (UFoliageCaptureComponent* CaptureComponent : GetFoliageCaptureComponents())
		{
			if (IsValid(CaptureComponent))
			{
				if (IsValid(CaptureComponent->PointCloudComponent->GetPointCloud()))
				{
					CaptureComponent->PointCloudComponent->GetPointCloud()->Initialize(FBox(FVector(0.0), FVector(100.0)));
				}
			}
		}
	}
}
#endif

// Called every frame
void AGenericFoliageActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (UpdateTime > UpdateFrequency && !bDisableUpdates)
	{
		UpdateTime = 0.f;

		bool bHasCamera = false;
		FVector CameraLocation;
		FRotator CameraRotation;

		GetCameraInfo(CameraLocation, CameraRotation, bHasCamera);
		if (bHasCamera)
		{
			CameraLocation = AdjustWorldPositionHeightToPlanet(CameraLocation, 2000);

			if (FVector::Distance(CameraLocation, LastUpdatePosition) > 2 * 100000)
			{
				LastUpdatePosition = CameraLocation;
				TickQueue.Empty();
				for (int32 Index = 0; Index < GetRootComponent()->GetNumChildrenComponents(); ++Index)
				{
					TickQueue.Emplace([this, Index, CameraLocation]()
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
			}
		}
	}
	else
	{
		UpdateTime += DeltaTime;
	}

	if (TickQueue.Num() > 0)
	{
		if (IsReadyToUpdate())
		{
			TickQueue.Pop()();
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
			bIsReadyToUpdate = bIsReadyToUpdate && FoliageCaptureComponent->IsReadyToUpdate();
		}
	}
	return bIsReadyToUpdate;
}

void AGenericFoliageActor::SetupTextureTargets()
{
	if (bUsingSharedResources)
	{
		SceneColourRT = NewObject<UTextureRenderTarget2D>(this, "SceneColourRT", RF_Transient);
		check(SceneColourRT);

		SceneColourRT->InitCustomFormat(512, 512, EPixelFormat::PF_B8G8R8A8, true);
		SceneColourRT->RenderTargetFormat = RTF_RGBA8;
		SceneColourRT->UpdateResourceImmediate(true);

		SceneDepthRT = NewObject<UTextureRenderTarget2D>(this, "SceneDepthRT", RF_Transient);
		check(SceneDepthRT);

		SceneDepthRT->RenderTargetFormat = RTF_R32f;
		SceneDepthRT->InitAutoFormat(512, 512);
		SceneDepthRT->UpdateResourceImmediate(true);

		SceneNormalRT = NewObject<UTextureRenderTarget2D>(this, "SceneNormalRT", RF_Transient);
		check(SceneNormalRT);
		
		SceneNormalRT->RenderTargetFormat = RTF_RGBA8;
		SceneNormalRT->SRGB = true;
		SceneNormalRT->LODGroup = TextureGroup::TEXTUREGROUP_WorldNormalMap;
		SceneNormalRT->InitAutoFormat(512, 512);
		SceneNormalRT->UpdateResourceImmediate(true);
	}
}

void AGenericFoliageActor::RebuildInstancedMeshPool()
{
	TickQueue.Emplace([this]()
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
		Result.Add(Cast<UFoliageCaptureComponent>(Comp));
	}
	return Result;
}

void AGenericFoliageActor::EnqueueTickTask(TFunction<void()>&& InFunc)
{
	TickQueue.Emplace(InFunc);
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
