// Copyright Aiden. S. All Rights Reserved


#include "Actors/Components/FoliageCaptureComponent.h"

#include "GenericFoliage.h"
#include "GenericFoliageCompute.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Actors/GenericFoliageActor.h"
#include "Async/Async.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/InstancedStaticMesh.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"

struct FTiledFoliageBuilder
{
	FTransform RelativeTransform;
	int32 InstanceCount;
	TArray<FTransform> Transforms;

	FTiledFoliageBuilder(
		const FTransform& InRelativeTransform,
		const FBox& InMeshBox
	) : RelativeTransform(InRelativeTransform),
	    InstanceCount(0)
	{
	}

	void Build(const TArray<FTransform>& InTransforms)
	{
		InstanceCount = InTransforms.Num();

		Transforms.SetNumUninitialized(InstanceCount);
		ParallelFor(InstanceCount, [&](int32 Index)
		{
			Transforms[Index] = InTransforms[Index].GetRelativeTransform(RelativeTransform);
		});
	}

	void ApplyToHISM(UHierarchicalInstancedStaticMeshComponent* HISM)
	{
		check(IsInGameThread());
		check(InstanceCount > 0);
		HISM->PreAllocateInstancesMemory(Transforms.Num());
		HISM->AddInstances(Transforms, false);
	}
};

// Sets default values for this component's properties
UFoliageCaptureComponent::UFoliageCaptureComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;


	bWantsInitializeComponent = true;
	bAutoRegister = true;

	// ...

	/*
	PointCloudComponent = CreateDefaultSubobject<ULidarPointCloudComponent>("_LidarPointCloud");
	PointCloudComponent->SetupAttachment(this);
	PointCloudComponent->SetRelativeRotation(FRotator(0, 90, 90));
	PointCloudComponent->PointSize = 0;
	*/

	Diameter = 2.0 * 100000.0;

	//

	// SceneColourCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(CreateComponentName(TEXT("SceneColourCapture")));
	SceneColourCapture = CreateDefaultSubobject<USceneCaptureComponent2D>("_SceneColourCapture");
	SceneColourCapture->ProjectionType = ECameraProjectionMode::Orthographic;
	SceneColourCapture->CaptureSource = ESceneCaptureSource::SCS_BaseColor;
	SceneColourCapture->OrthoWidth = Diameter; // km
	SceneColourCapture->bCaptureEveryFrame = false;
	SceneColourCapture->bCaptureOnMovement = false;

	SceneColourCapture->SetupAttachment(this);

	// SceneDepthCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(CreateComponentName("SceneDepthCapture"));
	SceneDepthCapture = CreateDefaultSubobject<USceneCaptureComponent2D>("_SceneDepthCapture");
	SceneDepthCapture->ProjectionType = ECameraProjectionMode::Orthographic;
	SceneDepthCapture->CaptureSource = ESceneCaptureSource::SCS_SceneDepth;
	SceneDepthCapture->OrthoWidth = Diameter; // km
	SceneDepthCapture->bCaptureEveryFrame = false;
	SceneDepthCapture->bCaptureOnMovement = false;


	SceneDepthCapture->SetupAttachment(this);

	SceneNormalCapture = CreateDefaultSubobject<USceneCaptureComponent2D>("_SceneNormalCapture");
	SceneNormalCapture->ProjectionType = ECameraProjectionMode::Orthographic;
	SceneNormalCapture->CaptureSource = ESceneCaptureSource::SCS_Normal;
	SceneNormalCapture->OrthoWidth = Diameter; // km
	SceneNormalCapture->bCaptureEveryFrame = false;
	SceneNormalCapture->bCaptureOnMovement = false;

	SceneNormalCapture->SetupAttachment(this);
}

UFoliageCaptureComponent::~UFoliageCaptureComponent()
{
}

void UFoliageCaptureComponent::Compute()
{
	ensure(IsInGameThread());
	ensure(bReadyToUpdate);

	bReadyToUpdate = false;

	UTextureRenderTarget2D* SceneColourRT_Target2D = SceneColourRT;
	UTextureRenderTarget2D* SceneDepthRT_Target2D = SceneDepthRT;
	UTextureRenderTarget2D* SceneNormalRT_Target2D = SceneNormalRT;

	this->Builders = CreateFoliageBuilders();

	ENQUEUE_RENDER_COMMAND(FReadRenderTargets)(
		[this, SceneColourRT_Target2D, SceneDepthRT_Target2D, SceneNormalRT_Target2D](
		FRHICommandListImmediate& RHICmdList)
		{
			if (!IsValid(SceneColourRT_Target2D) || !IsValid(SceneDepthRT_Target2D))
			{
				UE_LOG(LogTemp, Error, TEXT("Render targets are not valid!"));
				return;
			}

			if (!SceneColourRT_Target2D->GetRenderTargetResource() || !SceneDepthRT_Target2D->GetRenderTargetResource())
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to get render target resources!"));
				return;
			}

			FRHITexture2D* SceneColourRHI = SceneColourRT_Target2D->GetRenderTargetResource()->
			                                                        GetTexture2DRHI();
			FRHITexture2D* SceneNormalRHI = SceneNormalRT_Target2D->GetRenderTargetResource()->GetTexture2DRHI();

			TArray<FLinearColor> SceneColourData;
			TArray<FLinearColor> SceneNormalData;

			// Get our shader
			FGenericFoliageComputeModule& ComputeModule = FModuleManager::GetModuleChecked<
				FGenericFoliageComputeModule>(TEXT("GenericFoliageCompute"));

			// If we support using ReadSurfaceData, then call it as it is slightly faster. otherwise call the CS
			if (SceneColourRHI->GetFormat() == EPixelFormat::PF_B8G8R8A8)
			{
				RHICmdList.ReadSurfaceData(
					SceneColourRHI,
					FIntRect(0, 0, SceneColourRT_Target2D->SizeX, SceneColourRT_Target2D->SizeY),
					SceneColourData,
					FReadSurfaceDataFlags{ERangeCompressionMode::RCM_MinMaxNorm}
				);
			}
			else
			{
				ComputeModule.CopyRenderTargetToCpu(RHICmdList, SceneColourRT_Target2D, SceneColourData);
			}

			RHICmdList.ReadSurfaceData(
				SceneNormalRHI,
				FIntRect(0, 0, SceneNormalRHI->GetSizeX(), SceneNormalRHI->GetSizeY()),
				SceneNormalData,
				FReadSurfaceDataFlags{ERangeCompressionMode::RCM_MinMaxNorm}
			);

			TArray<float> SceneDepthValues;

			ComputeModule.CopyRenderTargetToCpu(RHICmdList, SceneDepthRT_Target2D, SceneDepthValues);

			int32 Width = SceneColourRT_Target2D->SizeX, Height = SceneDepthRT_Target2D->SizeY;

			Async(EAsyncExecution::Thread, [
				      this, Width, Height,
				      SceneColourData = MoveTemp(SceneColourData),
				      SceneNormalData = MoveTemp(SceneNormalData),
				      SceneDepthValues = MoveTemp(SceneDepthValues)]() mutable
			      {
				      FVector Right = GetRightVector();
				      FVector Forward = GetForwardVector();
				      const double ZoneWidth = Diameter / 2.0;
				      Compute_Internal(SceneColourData, SceneNormalData, SceneDepthValues, Width, Height);
			      });
		}

	);
}


// Called when the game starts
void UFoliageCaptureComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
}

void UFoliageCaptureComponent::InitializeComponent()
{
	Super::InitializeComponent();
	UE_LOG(LogGenericFoliage, Display, TEXT("Initializing component"));
}


// Called every frame
void UFoliageCaptureComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                             FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UFoliageCaptureComponent::PrepareForCapture(UTextureRenderTarget2D* InSceneColourRT,
                                                 UTextureRenderTarget2D* InSceneNormalRT,
                                                 UTextureRenderTarget2D* InSceneDepthRT)
{
	BoundingBox = FBox(
		FVector(-Diameter / 2.0, -Diameter / 2.0, -300000),
		FVector(Diameter / 2.0, Diameter / 2.0, 300000)
	);

	if (bIsUsingSharedResources)
	{
		SceneColourRT = InSceneColourRT;
		SceneNormalRT = InSceneNormalRT;
		SceneDepthRT = InSceneDepthRT;

		SceneColourCapture->TextureTarget = InSceneColourRT;
		SceneNormalCapture->TextureTarget = InSceneNormalRT;
		SceneDepthCapture->TextureTarget = InSceneDepthRT;
	}
}

void UFoliageCaptureComponent::Capture()
{
	SceneColourCapture->CaptureScene();
	SceneNormalCapture->CaptureScene();
	SceneDepthCapture->CaptureScene();
}

void UFoliageCaptureComponent::Finish()
{
	if (bIsUsingSharedResources)
	{
		SceneColourRT = nullptr;
		SceneNormalRT = nullptr;
		SceneDepthRT = nullptr;
		SceneColourCapture->TextureTarget = nullptr;
		SceneNormalCapture->TextureTarget = nullptr;
		SceneDepthCapture->TextureTarget = nullptr;
	}
}

void UFoliageCaptureComponent::SetupTextureTargets(int32 TextureSize)
{
	SceneColourRT = NewObject<UTextureRenderTarget2D>(this, CreateComponentName(TEXT("SceneColourRT")), RF_Transient);
	check(SceneColourRT);

	SceneColourRT->InitCustomFormat(TextureSize, TextureSize, EPixelFormat::PF_B8G8R8A8, true);
	SceneColourRT->RenderTargetFormat = RTF_RGBA8;
	SceneColourRT->UpdateResourceImmediate(true);

	SceneDepthRT = NewObject<UTextureRenderTarget2D>(this, CreateComponentName(TEXT("SceneDepthRT")), RF_Transient);
	check(SceneDepthRT);

	SceneDepthRT->RenderTargetFormat = RTF_R32f;
	SceneDepthRT->InitAutoFormat(TextureSize, TextureSize);
	SceneDepthRT->UpdateResourceImmediate(true);

	SceneNormalRT = NewObject<UTextureRenderTarget2D>(this, "SceneNormalRT", RF_Transient);
	check(SceneNormalRT);

	SceneNormalRT->RenderTargetFormat = RTF_RGBA16f;
	SceneNormalRT->InitAutoFormat(TextureSize, TextureSize);
	SceneNormalRT->UpdateResourceImmediate(true);

	SceneColourCapture->TextureTarget = SceneColourRT;
	SceneDepthCapture->TextureTarget = SceneDepthRT;
	SceneNormalCapture->TextureTarget = SceneNormalRT;
}

bool UFoliageCaptureComponent::IsReadyToUpdate() const
{
	return bReadyToUpdate;
}

bool UFoliageCaptureComponent::IsUsingSharedResources() const
{
	return bIsUsingSharedResources;
}

void UFoliageCaptureComponent::SetDiameter(float InNewDiameter)
{
	Diameter = InNewDiameter;
	SceneColourCapture->OrthoWidth = Diameter;
	SceneDepthCapture->OrthoWidth = Diameter;
	SceneNormalCapture->OrthoWidth = Diameter;
}

ULidarPointCloudComponent* UFoliageCaptureComponent::ResolvePointCloudComponent()
{
	if (!IsValid(PointCloudComponent->GetPointCloud()))
	{
		PointCloud = NewObject<ULidarPointCloud>(this, CreateComponentName("PointCloudAsset"),
		                                         RF_Transient | RF_DuplicateTransient);
		PointCloudComponent->SetPointCloud(PointCloud);
		PointCloudComponent->GetPointCloud()->SetOptimizedForDynamicData(true);
	}
	return PointCloudComponent;
}

void UFoliageCaptureComponent::Compute_Internal(const TArray<FLinearColor>& SceneColourData,
                                                const TArray<FLinearColor>& SceneNormalData,
                                                const TArray<float>& SceneDepthData, int32 Width,
                                                int32 Height)
{
	if (!IsValid(this))
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid context"));
		return;
	}


	AGenericFoliageActor* Parent = Cast<AGenericFoliageActor>(GetOwner());
	check(IsValid(Parent));

	bool bCanUseParallel = true;

	for (UGenericFoliageType* FoliageType : Parent->FoliageTypes)
	{
		if (!IsValid(FoliageType)) { continue; }
		bCanUseParallel = FoliageType->Density == 1.f && bCanUseParallel;
	}

	const bool bEnableParallel = Width <= 512 && bCanUseParallel;

	auto StartPrepareSpawn = FDateTime::Now();

	TMap<FGuid, TArray<FTransform>> FoliageTransforms;
	for (UGenericFoliageType* FoliageType : Parent->FoliageTypes)
	{
		if (!IsValid(FoliageType))
		{
			continue;
		}

		FoliageTransforms.Add(FoliageType->GetGuid(), {});

		if (bEnableParallel)
		{
			FoliageTransforms[FoliageType->GetGuid()].SetNumUninitialized(Width * Height);
		}
	}

	FTransform AbsoluteTransform = FTransform(
		GetComponentTransform().TransformRotation(FRotator(0.0, 90.0, 90.0).Quaternion()),
		GetComponentLocation()
	);

	auto SampleGridColour = [](const TArray<FLinearColor>& InData, const float& U, const float& V, const int32& Width,
	                           const int32& Height)
	{
		const int32 OX = FMath::Floor(U);
		const int32 OY = FMath::Floor(V);
		const int32 NX = FMath::CeilToInt32(U);
		const int32 NY = FMath::CeilToInt32(V);

		const FLinearColor& V1 = FMath::BiLerp(
			InData[
				OY * Width + OX
			],
			InData[
				OY * Width + NX
			],
			InData[
				NY * Width + OX
			],
			InData[
				NY * Width + NX
			],
			U / static_cast<float>(Width), V / static_cast<float>(Height)
		);

		return V1;
	};

	auto SampleGridFloat = [](const TArray<float>& InData, const float& U, const float& V, const int32& Width,
	                          const int32& Height)
	{
		const int32 OX = FMath::Floor(U);
		const int32 OY = FMath::Floor(V);
		const int32 NX = FMath::CeilToInt32(U);
		const int32 NY = FMath::CeilToInt32(V);

		const float& V1 = FMath::BiLerp(
			InData[
				OY * Width + OX
			],
			InData[
				OY * Width + NX
			],
			InData[
				NY * Width + OX
			],
			InData[
				NY * Width + NX
			],
			U / static_cast<float>(Width), V / static_cast<float>(Height)
		);

		return V1;
	};

	if (bEnableParallel)
	{
		for (UGenericFoliageType* FoliageType : Parent->FoliageTypes)
		{
			if (!IsValid(FoliageType))
			{
				continue;
			}
			FGuid Guid = FoliageType->GetGuid();
			TArray<FTransform>& TransformsArray = FoliageTransforms[Guid];
			ParallelFor(Width * Height, [&](int32 Index)
			{
				int32 y = Index / Width;
				int32 x = Index % Width;
				const float& Depth = SceneDepthData[Index];

				FVector RelativePosition = FVector(
					FMath::Lerp(-Diameter / 2.0, Diameter / 2.0,
					            static_cast<double>(x) / static_cast<double>(Width)),
					FMath::Lerp(-Diameter / 2.0, Diameter / 2.0,
					            static_cast<double>(y) / static_cast<double>(Height)),
					-Depth
				);;

				const FLinearColor& ColourAtPoint = SceneColourData[Index] * 15;
				const FRotator NormalAtPoint = FVector(SceneNormalData[Index].Desaturate(-0.5f)).Rotation();

				FRotator AbsoluteRotator = FoliageType->bAlignToSurfaceNormal
					                           ? NormalAtPoint
					                           : AbsoluteTransform.Rotator();

				if (FoliageType->bEnableRandomRotation)
				{
					AbsoluteRotator = (FoliageType->GetRandomRotator().Quaternion() * AbsoluteRotator.Quaternion()).
						Rotator();
				}

				if (FoliageType->SpawnConstraint.IntersectsRGB(ColourAtPoint))
				{
					TransformsArray[Index] = (
						FTransform(
							AbsoluteRotator,
							AbsoluteTransform.TransformPosition(RelativePosition + FoliageType->LocalOffset),
							FoliageType->GetRandomScale()
						)
					);
				}
				else
				{
					TransformsArray[Index] = FTransform(FVector(NAN));
				}
			});

			TransformsArray = TransformsArray.FilterByPredicate(
				[&](const FTransform& Other) { return Other.IsValid(); });
		}
	}
	else
	{
		for (int32 y = 0; y < Height; ++y)
		{
			for (int32 x = 0; x < Width; ++x)
			{
				const int32 i = y * Width + x;

				for (UGenericFoliageType* FoliageType : Parent->FoliageTypes)
				{
					if (!IsValid(FoliageType))
					{
						continue;
					}

					// TODO: Find nearest tile to camera
					if (FoliageType->bOnlySpawnInNearestTile && TileID.X != 0 && TileID.Y != 0)
					{
						continue;
					}

					if (FoliageType->Density > 1.f)
					{
						int32 NumBetweenPixels = FMath::RoundToInt32(FoliageType->Density);
						for (int32 OffsetX = 0; OffsetX < NumBetweenPixels; ++OffsetX)
						{
							for (int32 OffsetY = 0; OffsetY < NumBetweenPixels; ++OffsetY)
							{
								float NewX = FMath::Lerp(static_cast<float>(x), static_cast<float>(x) + 1,
								                         static_cast<float>(OffsetX) / static_cast<float>(
									                         NumBetweenPixels));
								float NewY = FMath::Lerp(static_cast<float>(y), static_cast<float>(y) + 1,
								                         static_cast<float>(OffsetY) / static_cast<float>(
									                         NumBetweenPixels));

								if (FMath::CeilToInt(NewX) < Width && FMath::CeilToInt(NewY) < Height)
								{
									const FLinearColor ColourAtPoint = SampleGridColour(
										SceneColourData, NewX, NewY, Width, Height) * 15.f;
									const FRotator NormalAtPoint = FVector(SampleGridColour(
										SceneNormalData, NewX, NewY, Width, Height).Desaturate(-0.5f)).Rotation();
									const float DepthAtPoint = SampleGridFloat(
										SceneDepthData, NewX, NewY, Width, Height);

									FVector RelativePosition = FVector(
										FMath::Lerp(-Diameter / 2.0, Diameter / 2.0,
										            static_cast<double>(x) / static_cast<double>(Width)),
										FMath::Lerp(-Diameter / 2.0, Diameter / 2.0,
										            static_cast<double>(y) / static_cast<double>(Height)),
										-DepthAtPoint
									);

									FRotator AbsoluteRotator = FoliageType->bAlignToSurfaceNormal
										                           ? NormalAtPoint
										                           : AbsoluteTransform.Rotator();
									if (FoliageType->bEnableRandomRotation)
									{
										AbsoluteRotator = (FoliageType->GetRandomRotator().Quaternion() *
											AbsoluteRotator.Quaternion()).Rotator();
									}

									if (FoliageType->SpawnConstraint.IntersectsRGB(ColourAtPoint))
									{
										FoliageTransforms[FoliageType->GetGuid()].Emplace(
											FTransform(
												AbsoluteRotator,
												AbsoluteTransform.TransformPosition(
													RelativePosition + FoliageType->LocalOffset),
												FoliageType->GetRandomScale()
											)
										);
									}
								}
							}
						}
					}
					else
					{
						int32 NumBetweenPixels = FMath::RoundToInt32(1.f / FoliageType->Density);
						if ((x % NumBetweenPixels) == 0 && (y % NumBetweenPixels) == 0)
						{
							float Depth = 0.f;
							if (SceneDepthData.IsValidIndex(i))
							{
								Depth = SceneDepthData[i];
							}

							FVector RelativePosition = FVector(
								FMath::Lerp(-Diameter / 2.0, Diameter / 2.0,
								            static_cast<double>(x) / static_cast<double>(Width)),
								FMath::Lerp(-Diameter / 2.0, Diameter / 2.0,
								            static_cast<double>(y) / static_cast<double>(Height)),
								-Depth
							);

							const FLinearColor& ColourAtPoint = SceneColourData[i] * 15;
							const FRotator NormalAtPoint = FVector(SceneNormalData[i].Desaturate(-0.5f)).Rotation();

							FRotator AbsoluteRotator = FoliageType->bAlignToSurfaceNormal
								                           ? NormalAtPoint
								                           : AbsoluteTransform.Rotator();

							if (FoliageType->bEnableRandomRotation)
							{
								AbsoluteRotator = (FoliageType->GetRandomRotator().Quaternion() * AbsoluteRotator.
									Quaternion()).Rotator();
							}

							if (FoliageType->SpawnConstraint.IntersectsRGB(ColourAtPoint))
							{
								FoliageTransforms[FoliageType->GetGuid()].Emplace(
									FTransform(
										AbsoluteRotator,
										AbsoluteTransform.
										TransformPosition(RelativePosition + FoliageType->LocalOffset),
										FoliageType->GetRandomScale()
									)
								);
							}
						}
					}
				}
			}
		}
	}

	auto EndPrepareSpawn = FDateTime::Now();

	for (UGenericFoliageType* FoliageType : Parent->FoliageTypes)
	{
		FGuid Guid = FoliageType->GetGuid();

		const TArray<FTransform>& Transforms = FoliageTransforms[Guid];
		if (Builders[Guid].IsValid())
		{
			if (Transforms.Num() > 0)
			{
				Builders[Guid]->Build(Transforms);
			}
			else
			{
				// UE_LOG(LogGenericFoliage, Error, TEXT("No transforms (thread)"));
			}
		}
		else
		{
			UE_LOG(LogGenericFoliage, Error, TEXT("Builder is not valid (thread)"));
		}
	}

	/*
	if (bEnableParallel)
	{
		UE_LOG(LogGenericFoliage, Display, TEXT("Time taken to compute foliage transforms (parallel): %f seconds"),
		       (EndPrepareSpawn - StartPrepareSpawn).GetTotalSeconds());
	}
	else
	{
		UE_LOG(LogGenericFoliage, Display, TEXT("Time taken to compute foliage transforms (standard): %f seconds"),
		       (EndPrepareSpawn - StartPrepareSpawn).GetTotalSeconds());
	}
	*/
	


	
	AsyncTask(ENamedThreads::GameThread, [this, FoliageTransforms = MoveTemp(FoliageTransforms), Parent]()
	{
		if (!IsValid(this))
		{
			return;
		}

		// Execute last
		Parent->EnqueueFoliageTickTask([this]()
		{
			bReadyToUpdate = true;
		});

		for (UGenericFoliageType* FoliageType : Parent->FoliageTypes)
		{
			if (!IsValid(FoliageType))
			{
				continue;
			}

			const TArray<FTransform>& Transforms = FoliageTransforms[FoliageType->GetGuid()];

			// UE_LOG(LogGenericFoliage, Display, TEXT("Adding Foliage: %i"), Transforms.Num());

			if (Transforms.Num() > 0)
			{
				auto HISM = Parent->TileInstancedMeshPools[TileID]->HISMPool[FoliageType->GetGuid()];

				const int32 NumInstancesPerChunk = 25000;
				const int32 ExpectedChunks = Transforms.Num() / NumInstancesPerChunk;
				
				HISM->bAutoRebuildTreeOnInstanceChanges = false;

				// Executes last
				Parent->EnqueueFoliageTickTask([this, HISM]()
				{
					HISM->BuildTreeIfOutdated(true, true);
				});

				if (ExpectedChunks > 1)
				{
					for (int32 i = 0; i <= ExpectedChunks; ++i)
					{
						const int32 StartIndex = i * NumInstancesPerChunk;
						const int32 EndIndex =
							FMath::Clamp((i + 1) * NumInstancesPerChunk, 0, Transforms.Num() - 1);

						TArray<FTransform> ChunkedTransforms(
							TArrayView<const FTransform>(Transforms).Slice(StartIndex, (EndIndex - StartIndex)));
						Parent->EnqueueFoliageTickTask(
							[ChunkedTransforms = MoveTemp(ChunkedTransforms), HISM]()
							{
								HISM->AddInstances(
									ChunkedTransforms, false, true
								);
							}
						);
					}
				}
				else
				{
					Parent->EnqueueFoliageTickTask([HISM, Transforms]()
					{
						HISM->AddInstances(
							Transforms, false, true
						);
					});
				}

				// Executes first
				Parent->EnqueueFoliageTickTask([HISM]()
				{
					HISM->ClearInstances();
				});
				
			}
		}
	});
}

TMap<FGuid, TSharedPtr<FTiledFoliageBuilder>> UFoliageCaptureComponent::CreateFoliageBuilders() const
{
	AGenericFoliageActor* Parent = Cast<AGenericFoliageActor>(GetOwner());
	check(IsValid(Parent));

	TMap<FGuid, TSharedPtr<FTiledFoliageBuilder>> Result;

	for (UGenericFoliageType* FoliageType : Parent->FoliageTypes)
	{
		// TODO: Is Grass
		FGuid Guid = FoliageType->GetGuid();
		Result.Emplace(
			Guid,
			MakeShareable(new
				FTiledFoliageBuilder(
					Parent->TileInstancedMeshPools[TileID]->HISMPool[Guid]->GetComponentTransform(),
					FoliageType->FoliageMesh->GetBoundingBox()
				))
		);
	}

	return Result;
}

FName UFoliageCaptureComponent::CreateComponentName(const FString& ComponentName) const
{
	return FName(FString::Printf(TEXT("%ix%i_%s"), TileID.X, TileID.Y, *ComponentName));
}
