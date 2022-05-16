// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Components/FoliageCaptureComponent.h"

#include "GenericFoliage.h"
#include "GenericFoliageCompute.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Actors/GenericFoliageActor.h"
#include "Async/Async.h"
#include "Kismet/KismetRenderingLibrary.h"

// Sets default values for this component's properties
UFoliageCaptureComponent::UFoliageCaptureComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	
	bWantsInitializeComponent = true;
	bAutoRegister = true;

	// ...

	
	PointCloudComponent = CreateDefaultSubobject<ULidarPointCloudComponent>("_LidarPointCloud");
	PointCloudComponent->SetupAttachment(this);
	PointCloudComponent->SetRelativeRotation(FRotator(0, 90, 90));
	PointCloudComponent->PointSize = 0;
	

	Diameter =  2.0 * 100000.0;
	
	//
	
	// SceneColourCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(CreateComponentName(TEXT("SceneColourCapture")));
	SceneColourCapture = CreateDefaultSubobject<USceneCaptureComponent2D>("_SceneColourCapture");
	SceneColourCapture->ProjectionType = ECameraProjectionMode::Orthographic;
	SceneColourCapture->CaptureSource = ESceneCaptureSource::SCS_Normal;
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
	
	if (!bIsUsingSharedResources)
	{
		SetupTextureTargets();

		SceneColourCapture->TextureTarget = SceneColourRT;
		SceneDepthCapture->TextureTarget = SceneDepthRT;
	}
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

	ENQUEUE_RENDER_COMMAND(FReadRenderTargets)(
		[this, SceneColourRT_Target2D, SceneDepthRT_Target2D](FRHICommandListImmediate& RHICmdList)
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
			FUnorderedAccessViewRHIRef SceneDepthUAVRef = SceneDepthRT_Target2D->GetRenderTargetResource()->GetTextureRenderTarget2DResource()->GetUnorderedAccessViewRHI();

			TArray<FLinearColor> SceneColourData;


			RHICmdList.ReadSurfaceData(
				SceneColourRHI,
				FIntRect(0, 0, SceneColourRT_Target2D->SizeX, SceneColourRT_Target2D->SizeY),
				SceneColourData,
				FReadSurfaceDataFlags{ERangeCompressionMode::RCM_MinMaxNorm}
			);

			
			TArray<FColor> SceneDepthData;

			// Get our shader
			
			FGenericFoliageComputeModule& ComputeModule = FModuleManager::GetModuleChecked<FGenericFoliageComputeModule>(TEXT("GenericFoliageCompute"));

			
			/*
			RHICmdList.ReadSurfaceData(
				SceneDepthRHI,
				FIntRect(0, 0, SceneColourRT_Target2D->SizeX, SceneColourRT_Target2D->SizeY),
				SceneDepthData,
				FReadSurfaceDataFlags{ERangeCompressionMode::RCM_MinMax}
			);
			*/
			
			TArray<float> SceneDepthValues;

			ComputeModule.CopyRenderTargetToCpu(RHICmdList, SceneDepthRT_Target2D, SceneDepthValues);
			
			int32 Width = SceneColourRT_Target2D->SizeX, Height = SceneDepthRT_Target2D->SizeY;
			
			AsyncTask(ENamedThreads::GameThread, [this, Width, Height, SceneColourData, SceneDepthValues]()
			{
				FVector Right = GetRightVector();
				FVector Forward = GetForwardVector();
				const double ZoneWidth = Diameter / 2.0;
				Compute_Internal(SceneColourData, SceneDepthValues, Width, Height, FBox(
					(GetComponentLocation() - (Right * ZoneWidth)) - (Forward * ZoneWidth),
					(GetComponentLocation() + (Right * ZoneWidth)) + (Forward * ZoneWidth)
				));
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
	UE_LOG(LogGenericFoliage, Display,  TEXT("Initializing component"));
}


// Called every frame
void UFoliageCaptureComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                             FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UFoliageCaptureComponent::PrepareForCapture(UTextureRenderTarget2D* InSceneColourRT,
                                                 UTextureRenderTarget2D* InSceneDepthRT)
{
	if (bIsUsingSharedResources)
	{
		SceneColourRT = InSceneColourRT;
		SceneDepthRT = InSceneDepthRT;

		SceneColourCapture->TextureTarget = InSceneColourRT;
		SceneDepthCapture->TextureTarget = InSceneDepthRT;
	}
}

void UFoliageCaptureComponent::Capture()
{
	SceneColourCapture->CaptureScene();
	SceneDepthCapture->CaptureScene();
}

void UFoliageCaptureComponent::Finish()
{
	if (bIsUsingSharedResources)
	{
		SceneColourRT = nullptr;
		SceneDepthRT = nullptr;
		SceneColourCapture->TextureTarget = nullptr;
		SceneDepthCapture->TextureTarget = nullptr;
	}
}

void UFoliageCaptureComponent::SetupTextureTargets()
{
	SceneColourRT = NewObject<UTextureRenderTarget2D>(this, CreateComponentName(TEXT("SceneColourRT")), RF_Transient);
	check(SceneColourRT);

	SceneColourRT->InitCustomFormat(512, 512, EPixelFormat::PF_B8G8R8A8, true);
	SceneColourRT->RenderTargetFormat = RTF_RGBA8;
	SceneColourRT->UpdateResourceImmediate(true);
	
	SceneDepthRT = NewObject<UTextureRenderTarget2D>(this, CreateComponentName(TEXT("SceneDepthRT")), RF_Transient);
	check(SceneDepthRT);

	// SceneDepthRT->InitCustomFormat(512, 512, EPixelFormat::PF_A32B32G32R32F, true);
	SceneDepthRT->RenderTargetFormat = RTF_R32f;
	SceneDepthRT->InitAutoFormat(512, 512);
	SceneDepthRT->UpdateResourceImmediate(true);
}

bool UFoliageCaptureComponent::IsReadyToUpdate() const
{
	return bReadyToUpdate;
}

ULidarPointCloudComponent* UFoliageCaptureComponent::ResolvePointCloudComponent()
{
	if (!IsValid(PointCloudComponent->GetPointCloud()))
	{
		PointCloud = NewObject<ULidarPointCloud>(this, CreateComponentName("PointCloudAsset"), RF_Transient | RF_DuplicateTransient);
		PointCloudComponent->SetPointCloud(PointCloud);
		PointCloudComponent->GetPointCloud()->SetOptimizedForDynamicData(true);
	}
	return PointCloudComponent;
}

void UFoliageCaptureComponent::Compute_Internal(const TArray<FLinearColor>& SceneColourData,
                                                const TArray<float>& SceneDepthData, int32 Width,
                                                int32 Height, FBox WorldBounds)
{
	if (!IsValid(this))
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid context"));
		return;
	}

	ResolvePointCloudComponent();
	PointCloud->Initialize(FBox(FVector(-Diameter/2.0, -Diameter/2.0, -100000.0), FVector(Diameter/2.0, Diameter/2.0, 100000.0)));
	
	Viz_Points.Reset(0);
	
	AGenericFoliageActor* Parent = Cast<AGenericFoliageActor>(GetOwner());
	check(IsValid(Parent));
	
	Viz_Points.SetNumUninitialized(Width * Height);
	
	for (int32 y = 0; y < Height; ++y)
	{
		for (int32 x = 0; x < Width; ++x)
		{
			const int32 i = y * Width + x;

			float Depth = 0.f;
			if (SceneDepthData.IsValidIndex(i))
			{
				Depth = SceneDepthData[i];
			//	UE_LOG(LogTemp, Display, TEXT("Depth: %f %f"), Depth, FMath::Lerp(SceneDepthCapture->GetRelativeLocation().Z, 0, Depth));
			}
			
			FVector RelativePosition = FVector(
				FMath::Lerp(-Diameter/2.0, Diameter/2.0, static_cast<double>(x) / static_cast<double>(Width)),
				FMath::Lerp(-Diameter/2.0, Diameter/2.0, static_cast<double>(y) / static_cast<double>(Height)),
				-Depth
			);
			
			FVector WorldPosition = GetComponentTransform().TransformPosition(RelativePosition);
			
			const FLinearColor& Data = SceneColourData[i] * 15.0;

			Viz_Points[i] = FLidarPointCloudPoint(
				FVector3f(GetComponentTransform().InverseTransformPosition(WorldPosition)),
				Data.R, Data.G, Data.B, 1.f, 0
			);
		}
	}
	
	// PointCloud->InsertPoints(Viz_Points, ELidarPointCloudDuplicateHandling::Ignore, false, FVector::ZeroVector);

	//PointCloud->RefreshBounds();
	PointCloud->InsertPoints_NoLock(Viz_Points.GetData(), Viz_Points.Num(), ELidarPointCloudDuplicateHandling::Ignore, false, FVector::ZeroVector);
	bReadyToUpdate = true;
}

FName UFoliageCaptureComponent::CreateComponentName(const FString& ComponentName) const
{
	return FName(FString::Printf(TEXT("%ix%i_%s"), TileID.X, TileID.Y, *ComponentName));
}
