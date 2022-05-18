// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Foliage/GenericFoliageType.h"
#include "FoliageInstancedMeshPool.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class GENERICFOLIAGE_API UFoliageInstancedMeshPool : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UFoliageInstancedMeshPool();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void RebuildHISMPool(
		const TArray<UGenericFoliageType*>& InFoliageTypes
	);
	
	/** Toggles collision on this tile */
	virtual void ToggleCollision(bool bNewEnableCollision);

	/** Returns the total instance count of this tile */
	int32 GetTotalInstanceCount() const;
	
public:
	// Map that stores our ISMs. these are mapped against a GUID which comes from a foliage type 
	UPROPERTY(Transient)
	TMap<FGuid, UHierarchicalInstancedStaticMeshComponent*> HISMPool;

	/** Whether collision is enabled on our instances. typically this is set in its owning actor */
	UPROPERTY(Transient)
	bool bEnableCollision = true;

	/** Foliage types in this tile */
	UPROPERTY(Transient)
	TArray<UGenericFoliageType*> FoliageTypes;
};
