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

	void RebuildInstancedMeshPool();

	TArray<UFoliageCaptureComponent*> GetFoliageCaptureComponents() const;

public:

	void EnqueueTickTask(TFunction<void()>&& InFunc);
	
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

#pragma region Properties
public:
	UPROPERTY(EditAnywhere, Category = "ProceduralFoliage")
	TArray<UGenericFoliageType*> FoliageTypes;

	UPROPERTY(Transient)
	TMap<FIntPoint, UFoliageInstancedMeshPool*> TileInstancedMeshPools;
	
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

	float UpdateFrequency = 0.2f;
	float UpdateTime = 0.f;

private:
	bool bUsingSharedResources = true;
	TArray<TFunction<void()>> TickQueue;
#pragma endregion 
};
