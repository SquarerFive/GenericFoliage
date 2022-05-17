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
		const TArray<UGenericFoliageType*>& FoliageTypes
	);

public:
	// Map that stores our ISMs. these are mapped against a GUID which comes from a foliage type 
	UPROPERTY(Transient)
	TMap<FGuid, UHierarchicalInstancedStaticMeshComponent*> HISMPool;
		
};
