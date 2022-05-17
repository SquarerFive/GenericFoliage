// Copyright Aiden. S. All Rights Reserved

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

	bool IntersectsRGB(const FLinearColor& Other) const
	{
		return Other.R >= Min.R && Other.G >= Min.G && Other.B >= Min.B &&
			Other.R <= Max.R && Other.G <= Max.G && Other.B <= Max.B;
	}
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

	FVector GetRandom() const
	{
		if (Min == Max)
		{
			return Min;
		}

		return FMath::Lerp(Min, Max, FRandomStream().FRand());
	}
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

public:
	FGuid GetGuid();

private:
	UPROPERTY()
	FGuid Guid;
};
