// Copyright Aiden. S. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "LidarPointCloudComponent.h"
#include "FoliageCaptureComponent.generated.h"

class IProjectionInterface;
class UDynamicMeshComponent;
class ULidarPointCloudComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Transient)
class GENERICFOLIAGE_API UFoliageCaptureComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UFoliageCaptureComponent();
	virtual ~UFoliageCaptureComponent() override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void InitializeComponent() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void PrepareForCapture(
		UTextureRenderTarget2D* InSceneColourRT,
		UTextureRenderTarget2D* InSceneNormalRT,
		UTextureRenderTarget2D* InSceneDepthRT
	);

	/** Captures the scene */
	void Capture();

	/** Entry point to our foliage spawner */
	void Compute();

	/** Called at the end of the capture, sets any shared variables here to null */
	void Finish();

	/** Creates our render targets. Only used here if bIsUsingSharedResources is not true. */ 
	void SetupTextureTargets(int32 TextureSize);

	/** Is this component ready to be updated [foliage compute/capture]. False if an update is in progress */
	bool IsReadyToUpdate() const;

	bool IsUsingSharedResources() const;

	/** Sets the tile size in cm */
	void SetDiameter(float InNewDiameter);

	ULidarPointCloudComponent* ResolvePointCloudComponent();
	
public:
	UPROPERTY(VisibleAnywhere, Category = "ProceduralFoliage")
	FIntPoint TileID = FIntPoint::ZeroValue;
	
	UPROPERTY(VisibleAnywhere, Category = "ProceduralFoliage")
	AActor* Projection;

	UPROPERTY()
	USceneCaptureComponent2D* SceneColourCapture;
	
	UPROPERTY()
	USceneCaptureComponent2D* SceneDepthCapture;
	
	UPROPERTY()
	USceneCaptureComponent2D* SceneNormalCapture;
	
	UPROPERTY()
	UTextureRenderTarget2D* SceneColourRT;
	
	UPROPERTY()
	UTextureRenderTarget2D* SceneDepthRT;
	
	UPROPERTY()
	UTextureRenderTarget2D* SceneNormalRT;

	UPROPERTY()
	ULidarPointCloudComponent* PointCloudComponent;

	UPROPERTY()
	double Diameter =  200000.;

	UPROPERTY()
	FBox BoundingBox;

	UPROPERTY(VisibleAnywhere, Category = "ProceduralFoliage")
	double DistanceAboveSurface = 2000.0;

private:
	void Compute_Internal(
		const TArray<FLinearColor>& SceneColourData,
		const TArray<FLinearColor>& SceneNormalData,
		const TArray<float>& SceneDepthData,
		int32 Width,
		int32 Height
	);

	TMap<FGuid, TSharedPtr<struct FTiledFoliageBuilder>> CreateFoliageBuilders() const;

	FName CreateComponentName(const FString& ComponentName) const;

private:
	UPROPERTY()
	TArray<FLidarPointCloudPoint> Viz_Points;
	UPROPERTY()
	bool bIsUsingSharedResources = true;
	UPROPERTY()
	bool bReadyToUpdate = true;
	UPROPERTY()
	ULidarPointCloud* PointCloud;
	
	TMap<FGuid, TSharedPtr<struct FTiledFoliageBuilder>> Builders;
};
