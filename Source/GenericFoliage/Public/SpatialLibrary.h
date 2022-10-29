// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UDynamicMesh.h"
#include "GeometryScript/GeometryScriptTypes.h"
#include "SpatialLibrary.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct GENERICFOLIAGE_API FSpatialFeature
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		UDynamicMesh* Geometry;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		FGeometryScriptDynamicMeshBVH BVH;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		TMap<FString, FString> Properties;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		int32 Type = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		int32 Id = 0;
};

UCLASS()
class GENERICFOLIAGE_API USpatialLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	static TArray<FSpatialFeature> ParseGeoJSON(const FString& InGeoJSON, UDynamicMeshPool* MeshPool);

	/** Calculates the great circle distance between two points on a planet, returns the result in metres.
	 *
	 * @param Radius : Radius of the planet in metres
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure)
		static double HaversineDistance(const FVector2D& PointA, const FVector2D& PointB, double Radius = 6371e3);

	/**
	 * Calculates the delta longitude and latitude needed to move a distance
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure)
		static FVector2D HaversineDeltaDegrees(const FVector2D& Origin, const double& Distance, double Radius = 6371e3);
	
};
