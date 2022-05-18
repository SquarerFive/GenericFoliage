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

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bRandomX = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bRandomY = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bRandomZ = true;

	FVector GetRandom(const FRandomStream& RandStream) const
	{
		if (Min == Max)
		{
			return Min;
		}

		return FVector(
			bRandomX ? RandStream.FRandRange(Min.X, Max.X) : 0.0,
			bRandomY ? RandStream.FRandRange(Min.Y, Max.Y) : 0.0,
			bRandomZ ? RandStream.FRandRange(Min.Z, Max.Z) : 0.0
		);
	}
};

USTRUCT(BlueprintType)
struct FRotatorInterval
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FRotator Min = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FRotator Max = FRotator(360.0, 360.0, 360.0);

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bRandomYaw = true;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bRandomPitch = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bRandomRoll = false;

	FRotator GetRandom(const FRandomStream& RandStream) const
	{
		if (Min == Max)
		{
			return Min;
		}

		return FRotator(
			bRandomPitch ? RandStream.FRandRange(Min.Pitch, Max.Pitch) : 0.0,
			bRandomYaw ? RandStream.FRandRange(Min.Yaw, Max.Yaw) : 0.0,
			bRandomRoll ? RandStream.FRandRange(Min.Roll, Max.Roll) : 0.0
		);
	}
};

UCLASS()
class GENERICFOLIAGE_API UGenericFoliageType : public UObject
{
	GENERATED_BODY()

	UGenericFoliageType();
	
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

	/** Culling distances. Defines the distance where culling begins and the distance where it's fully culled */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = General)
	FInt32Interval CullingDistanceRange = FInt32Interval(32768, 65536);

	/** Whether each instance should align to the surface normal, otherwise it'll just face upwards from the planet. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = General)
	bool bAlignToSurfaceNormal = false;

	/** Initial random seed to apply to this asset */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = General)
	int32 RandomSeed = 1337;
	
	/** If enabled, we will only spawn this foliage type in the nearest tile */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = General)
	bool bOnlySpawnInNearestTile = false;

	/** Should we enable collision? */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = General)
	TEnumAsByte<ECollisionEnabled::Type> IsCollisionEnabled;

	/** If enabled, a random rotator will initially be applied to each instance */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = General)
	bool bEnableRandomRotation = false;

	/** Range of angles to randomly apply */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = General, meta = (EditCondition="bEnableRandomRotation"))
	FRotatorInterval RotatorRange;
	
public:
	UFUNCTION()
	FGuid GetGuid();

	UFUNCTION()
	FVector GetRandomScale() const;
	
	UFUNCTION()
	FRotator GetRandomRotator() const;
	
private:
	UPROPERTY()
	FGuid Guid;

	UPROPERTY()
	FRandomStream RandomStream;
};
