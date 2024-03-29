// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Components/FoliageInstancedMeshPool.h"

#include "GenericFoliage.h"
#include "Actors/GenericFoliageActor.h"
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

void UFoliageInstancedMeshPool::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	Super::OnComponentDestroyed(bDestroyingHierarchy);

	for (auto& HISMPair: HISMPool)
	{
		HISMPair.Value->ClearInstances();
		HISMPair.Value->DestroyComponent();
	}
	HISMPool.Empty();
}


// Called every frame
void UFoliageInstancedMeshPool::TickComponent(float DeltaTime, ELevelTick TickType,
                                              FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UFoliageInstancedMeshPool::RebuildHISMPool(const TArray<UGenericFoliageType*>& InFoliageTypes)
{
	check(IsInGameThread());
	if (HISMPool.Num() > 0)
	{
		for (auto& HISMPair : HISMPool)
		{
			if (IsValid(HISMPair.Value))
			{
				HISMPair.Value->DestroyComponent();
			}
		}
	
		HISMPool.Reset();
	}
	
	FoliageTypes = InFoliageTypes;

	bool bHasAnyInvalidMesh = false;

	for (UGenericFoliageType* FoliageType : InFoliageTypes)
	{
		if (!IsValid(FoliageType))
		{
			bHasAnyInvalidMesh = true;
			continue;
		}
		if (!IsValid(FoliageType->FoliageMesh))
		{
			bHasAnyInvalidMesh = true;
			continue;
		}

		UHierarchicalInstancedStaticMeshComponent* HISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(
			GetOwner());
		HISM->bAffectDynamicIndirectLighting = false;
		HISM->bSelectable = false;
		HISM->AttachToComponent(GetOwner()->GetRootComponent(),
		                        FAttachmentTransformRules{EAttachmentRule::KeepWorld, false});
		HISM->ClearInstances();
		HISM->SetStaticMesh(FoliageType->FoliageMesh);
		HISM->SetCullDistances(FoliageType->CullingDistanceRange.Min, FoliageType->CullingDistanceRange.Max);
		HISM->bCastStaticShadow = false;
		
		if (FoliageType->IsCollisionEnabled.GetValue() == ECollisionEnabled::NoCollision)
		{
			HISM->SetCollisionProfileName("NoCollision");
			HISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			HISM->SetCanEverAffectNavigation(false);
			HISM->bDisableCollision = true;
			HISM->bHasPerInstanceHitProxies = false;
			
		}
		else
		{
			HISM->SetCollisionEnabled(FoliageType->IsCollisionEnabled);
		}
		HISM->RegisterComponent();

		HISMPool.Add(FoliageType->GetGuid(), HISM);
	}

	AGenericFoliageActor* Parent = Cast<AGenericFoliageActor>(GetOwner());
	if (IsValid(Parent))
	{
		Parent->SetIsReadyToUpdate(!bHasAnyInvalidMesh);
	}
}

void UFoliageInstancedMeshPool::ToggleCollision(bool bNewEnableCollision)
{
	if (bNewEnableCollision != bEnableCollision)
	{
		bEnableCollision = bNewEnableCollision;

		for (UGenericFoliageType* FoliageType : FoliageTypes)
		{
			if (!IsValid(FoliageType)) { continue; }
			
			if (FoliageType->IsCollisionEnabled.GetValue() != ECollisionEnabled::NoCollision)
			{
				UHierarchicalInstancedStaticMeshComponent* HISM = HISMPool[FoliageType->GetGuid()];
				if (HISM->GetCollisionEnabled() != FoliageType->IsCollisionEnabled.GetValue() && bEnableCollision)
				{
					HISM->bDisableCollision = false;
					HISM->SetCollisionEnabled(FoliageType->IsCollisionEnabled.GetValue());
					// HISM->CreatePhysicsState();
					HISM->SetCanEverAffectNavigation(true);
				}
				else if (!bEnableCollision)
				{
					HISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
					HISM->bDisableCollision = true;
					HISM->SetCanEverAffectNavigation(false);
				}
			}
		}
	}
}

int32 UFoliageInstancedMeshPool::GetTotalInstanceCount() const
{
	int32 Count = 0;

	for (const auto& HISMPair: HISMPool)
	{
		if (IsValid(HISMPair.Value))
		{
			Count += HISMPair.Value->GetInstanceCount();
		}
	}
	
	return Count;
}
