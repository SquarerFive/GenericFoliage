// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Components/FoliageInstancedMeshPool.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"

// Sets default values for this component's properties
UFoliageInstancedMeshPool::UFoliageInstancedMeshPool()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UFoliageInstancedMeshPool::BeginPlay()
{
	Super::BeginPlay();

	// ...
}


// Called every frame
void UFoliageInstancedMeshPool::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UFoliageInstancedMeshPool::RebuildHISMPool(const TArray<UGenericFoliageType*>& FoliageTypes)
{
	for (auto& HISMPair: HISMPool)
	{
		HISMPair.Value->DestroyComponent();
	}
	
	HISMPool.Reset();

	for (UGenericFoliageType* FoliageType: FoliageTypes)
	{
		if (!IsValid(FoliageType))
		{
			continue;
		}
		if (!IsValid(FoliageType->FoliageMesh))
		{
			continue;
		}

		UHierarchicalInstancedStaticMeshComponent* HISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(GetOwner(), FName(*FString::Printf(TEXT("%s_HISM_Pooled"), *GetName())));
		HISM->bAffectDynamicIndirectLighting = false;
		HISM->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules {EAttachmentRule::KeepWorld, false});
		HISM->ClearInstances();
		HISM->SetStaticMesh(FoliageType->FoliageMesh);
		HISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		HISM->bDisableCollision = true;
		HISM->RegisterComponent();
		
		HISMPool.Add(FoliageType->GetGuid(), HISM);
	}
}

