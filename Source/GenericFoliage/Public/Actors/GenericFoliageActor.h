// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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

public:
	// UPROPERTY(VisibleAnywhere, Transient, DuplicateTransient, Category = "ProceduralFoliage")
	// TArray<UFoliageCaptureComponent*> FoliageCaptureComponents;

private:
	UPROPERTY(EditAnywhere, Category = "ProceduralFoliage")
	AActor* ProjectionImplementation;
	
	UPROPERTY(VisibleAnywhere, Transient, Category = "ProceduralFoliage")
	FVector LastUpdatePosition;

	UPROPERTY(VisibleAnywhere, Transient, Category = "ProceduralFoliage")
	UTextureRenderTarget2D* SceneColourRT;

	UPROPERTY(VisibleAnywhere, Transient, Category = "ProceduralFoliage")
	UTextureRenderTarget2D* SceneDepthRT;

	float UpdateFrequency = 0.2f;
	float UpdateTime = 0.f;

private:
	void GetCameraInfo(FVector& Location, FRotator& Rotation, bool& bSuccess) const;

	bool IsReadyToUpdate() const;
	
	void SetupTextureTargets();

	TArray<UFoliageCaptureComponent*> GetFoliageCaptureComponents() const;

public:
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

private:
	bool bUsingSharedResources = true;
	TArray<TFunction<void()>> TickQueue;
};
