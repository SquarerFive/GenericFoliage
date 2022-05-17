// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "LidarPointCloudComponent.h"
#include "FoliageCaptureComponent.generated.h"

class IProjectionInterface;
class UDynamicMeshComponent;
class ULidarPointCloudComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
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
	
	void Capture();

	void Compute();

	void Finish();

	/** Creates our render targets. Only used here if bIsUsingSharedResources is not true. */ 
	void SetupTextureTargets();

	bool IsReadyToUpdate() const;

	ULidarPointCloudComponent* ResolvePointCloudComponent();
	
public:
	UPROPERTY(VisibleAnywhere, Category = "ProceduralFoliage")
	FIntPoint TileID = FIntPoint::ZeroValue;
	
	UPROPERTY(VisibleAnywhere, Category = "ProceduralFoliage")
	AActor* Projection;

	UPROPERTY(Transient)
	USceneCaptureComponent2D* SceneColourCapture;
	
	UPROPERTY(Transient)
	USceneCaptureComponent2D* SceneDepthCapture;
	
	UPROPERTY(Transient)
	USceneCaptureComponent2D* SceneNormalCapture;
	
	UPROPERTY(Transient)
	UTextureRenderTarget2D* SceneColourRT;
	
	UPROPERTY(Transient)
	UTextureRenderTarget2D* SceneDepthRT;
	
	UPROPERTY(Transient)
	UTextureRenderTarget2D* SceneNormalRT;

	UPROPERTY(Transient)
	ULidarPointCloudComponent* PointCloudComponent;

	UPROPERTY()
	double Diameter =  200000.;

	UPROPERTY(VisibleAnywhere, Category = "ProceduralFoliage")
	double DistanceAboveSurface = 2000.0;

private:
	void Compute_Internal(
		const TArray<FLinearColor>& SceneColourData,
		const TArray<float>& SceneDepthData,
		int32 Width,
		int32 Height,
		FBox WorldBounds
	);

	FName CreateComponentName(const FString& ComponentName) const;

private:
	UPROPERTY(Transient)
	TArray<FLidarPointCloudPoint> Viz_Points;
	UPROPERTY(Transient)
	bool bIsUsingSharedResources = true;
	UPROPERTY(Transient)
	bool bReadyToUpdate = true;
	UPROPERTY(Transient)
	ULidarPointCloud* PointCloud;
};
