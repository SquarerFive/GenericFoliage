// Copyright Aiden. S. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/FoliageInstancedMeshPool.h"
#include "Foliage/GenericFoliageType.h"
#include "GameFramework/Actor.h"
#include "Interface/ProjectionInterface.h"
#include "GenericFoliageActor.generated.h"

class UFoliageCaptureComponent;

UCLASS()
class GENERICFOLIAGE_API AGenericFoliageActor : public AActor 
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AGenericFoliageActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual bool ShouldTickIfViewportsOnly() const override;
	virtual void OnConstruction(const FTransform& Transform) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	void GetCameraInfo(FVector& Location, FRotator& Rotation, bool& bSuccess) const;

	bool IsReadyToUpdate() const;
	
	void SetupTextureTargets();

	void SetupFoliageCaptureComponents();

	void RebuildInstancedMeshPool();

	TArray<UFoliageCaptureComponent*> GetFoliageCaptureComponents() const;

	void UpdateNearestTileID(const FVector& InWorldLocation);
	
	TArray<FIntPoint> GetBuildingTiles() const;

	int32 GetTotalInstanceCount() const;

public:

	void EnqueueCaptureTickTask(TFunction<void()>&& InFunc);
	void EnqueueFoliageTickTask(TFunction<void()>&& InFunc);
	
	/** Transforms */

	/** Transforms in-game world coordinates to a local frame of reference */
	virtual FVector WorldToLocalPosition(const FVector& InWorldLocation) const;

	/** Transforms local to in-game world coordinates. */
	virtual FVector LocalToWorldPosition(const FVector& InLocalLocation) const;
	
	virtual FRotator CalculateEastNorthUp(const FVector& InWorldLocation) const;

	/** Snaps the world coordinate to be above the surface at a specific height */
	virtual FVector AdjustWorldPositionHeightToPlanet(const FVector& InWorldLocation, const double& Height) const;

	/** Sets the capture components to only capture these specific actors */
	virtual void SetShowOnlyActors(TArray<AActor*> InShowOnlyActors);

public:
#pragma region Blueprint Exposed Functions
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "ProceduralFoliage")
	void RefreshFoliage();

#pragma endregion 

#pragma region Properties
public:
	UPROPERTY(EditAnywhere, Category = "ProceduralFoliage")
	TArray<UGenericFoliageType*> FoliageTypes;

	/** World size of each tile  */
	UPROPERTY(EditAnywhere, Category = "ProceduralFoliage")
	float Diameter = 200000.0;

	UPROPERTY(EditAnywhere, Category = "ProceduralFoliage", meta = (ClampMin=64, ClampMax=1024, UIMin=64, UIMax=1024))
	int32 TilePixelSize = 512;

	// Number of tiles in the -x, +x and -y, +y dir;
	UPROPERTY(EditAnywhere, Category = "ProceduralFoliage")
	FIntPoint TileCount = FIntPoint(1, 1);

	/** Foliage will only regenerate if the camera velocity is below this threshold  */
	UPROPERTY(EditAnywhere, Category = "ProceduralFoliage")
	double VelocityUpdateThreshold = 12500;

	/** Maximum foliage tasks that can be run per tick */
	UPROPERTY(EditAnywhere, Category = "Async", meta = (UIMin=1))
	int32 FoliageTasksPerTick = 1;

	/** Maximum capture tasks that can be run per tick */
	UPROPERTY(EditAnywhere, Category = "Async", meta = (UIMin=1))
	int32 CaptureTasksPerTick = 1;

	UPROPERTY(Transient)
	TMap<FIntPoint, UFoliageInstancedMeshPool*> TileInstancedMeshPools;

	/** ID of the tile nearest to the camera */
	UPROPERTY(Transient)
	FIntPoint NearestTileID;

	/** ID of the tile nearest to the camera */
	UPROPERTY(Transient)
	bool bHasNearestTile = false;
	
private:
	UPROPERTY(EditAnywhere, Category = "ProceduralFoliage")
	bool bDisableUpdates = false;
	
	UPROPERTY(VisibleAnywhere, Transient, Category = "ProceduralFoliage")
	FVector LastUpdatePosition;

	UPROPERTY(Transient)
	UTextureRenderTarget2D* SceneColourRT;

	UPROPERTY(Transient)
	UTextureRenderTarget2D* SceneDepthRT;

	UPROPERTY(Transient)
	UTextureRenderTarget2D* SceneNormalRT;

	float UpdateFrequency = 0.05f;
	float UpdateTime = 0.f;
	FIntPoint LastNearestTileID;
	bool bForceUpdate = false;
	FVector LastCameraPosition = FVector::ZeroVector;

private:
	bool bUsingSharedResources = true;
	TArray<TFunction<void()>> CaptureTickQueue;
	TArray<TFunction<void()>> FoliageTickQueue;
#pragma endregion 
};
