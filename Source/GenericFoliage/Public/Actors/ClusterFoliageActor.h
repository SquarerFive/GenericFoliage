// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SpatialLibrary.h"
#include "Components/DynamicMeshComponent.h"
#include "Foliage/GenericFoliageCollection.h"
#include "GameFramework/Actor.h"
#include "ClusterFoliageActor.generated.h"

class UDynamicMeshPool;
class UFoliageInstancedMeshPool;

UCLASS()
class GENERICFOLIAGE_API AClusterFoliageActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AClusterFoliageActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	// virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void BeginDestroy() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual FVector GeographicToEngineLocation(const FVector& GeographicLocation);
	virtual FVector EngineToGeographicLocation(const FVector& EngineLocation);
	virtual FVector GetUpVectorFromGeographicLocation(const FVector& GeographicLocation);
	virtual FVector GetUpVectorFromEngineLocation(const FVector& EngineLocation);
	virtual double GetTerrainBaseHeight(const FVector& GeographicLocation, FVector& OutNormal);

	void SetupInstancedMeshPool();
	FSpatialFeature GetFeatureById(int32 Id);
	

	UFUNCTION(BlueprintCallable)
	void LoadGeoJSON(const FString& Data);
	
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		FString JsonData;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		bool bShowBoundary = false;

	/** If enabled, then estimate the radius in metres for  the sampler. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		bool bEstimationTransform = true;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		UMaterialInterface* DebugMaterial = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		UGenericFoliageCollection* Collection = nullptr;

private:
	UPROPERTY(Transient)
	UDynamicMeshPool* MeshPool = nullptr;

	UPROPERTY(Transient)
	TArray<FSpatialFeature> Features;

	UPROPERTY(Transient)
	TArray<UDynamicMeshComponent*> MeshComponents;

	UPROPERTY(Transient)
	UFoliageInstancedMeshPool* InstancedMeshPool = nullptr;

	TMap<int32, TSharedPtr<class FClusterFoliageSpawnerThread>> Threads;

	bool AnyThreadsRunning() const;
	void KillAllThreads();
};
