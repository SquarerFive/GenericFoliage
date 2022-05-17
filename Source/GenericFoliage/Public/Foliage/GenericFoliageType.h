// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GenericFoliageType.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FLinearColorInterval
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FLinearColor Min;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FLinearColor Max;
};

USTRUCT(BlueprintType)
struct FVectorInterval
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector Min = FVector(1.0);
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector Max = FVector(1.0);
};

UCLASS()
class GENERICFOLIAGE_API UGenericFoliageType : public UObject
{
	GENERATED_BODY()
public:
	/** A static mesh representing this foliage type */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = General)
	UStaticMesh* FoliageMesh;

	/** Colour range where this foliage type will be spawned */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = General)
	FLinearColorInterval SpawnConstraint;

	/** Amount of instances to spawn between units (pixels) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = General)
	float Density = 1.f;

	/** Local offset applied to each instance */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = General)
	FVector LocalOffset = FVector::ZeroVector;

	/** Minimum and maximum scale applied to each instance */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = General)
	FVectorInterval ScaleRange;
};
