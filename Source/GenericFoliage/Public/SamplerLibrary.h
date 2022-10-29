// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SamplerLibrary.generated.h"

/**
 * 
 */
UCLASS()
class GENERICFOLIAGE_API USamplerLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// https://www.cs.ubc.ca/~rbridson/docs/bridson-siggraph07-poissondisk.pdf
	// https://www.youtube.com/watch?v=7WcmyxyFO7o&ab_channel=SebastianLague
	static TArray<FVector2d> PoissonDiscSampling2d(
		const double Radius,
		const FVector2d RegionSize,
		const int32 RejectionThreshold = 30
	);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="PoissonDiscSampling2d"))
	static TArray<FVector2D> K2_PoissonDiscSampling2d(
		const double Radius,
		const FVector2D RegionSize,
		const int32 RejectionThreshold = 30
	);
};
