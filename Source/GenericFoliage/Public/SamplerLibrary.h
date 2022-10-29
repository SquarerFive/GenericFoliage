// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SamplerLibrary.generated.h"

/**
 * 
 */

struct FPoissonDiscSamplingSettings
{
	bool bUseGeographicCoordinates = false;
	FVector2D Origin = FVector2D::ZeroVector;
	double Radius = 100.0;
};

UCLASS()
class GENERICFOLIAGE_API USamplerLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// https://www.cs.ubc.ca/~rbridson/docs/bridson-siggraph07-poissondisk.pdf
	// https://www.youtube.com/watch?v=7WcmyxyFO7o&ab_channel=SebastianLague
	static TArray<FVector2D> PoissonDiscSampling(
		const double Radius,
		const FVector2D RegionSize,
		const int32 RejectionThreshold = 30,
		const FPoissonDiscSamplingSettings Settings = FPoissonDiscSamplingSettings()
	);

	UFUNCTION(BlueprintCallable, meta=(DisplayName="PoissonDiscSampling2d"))
	static TArray<FVector2D> K2_PoissonDiscSampling2d(
		const double Radius,
		const FVector2D RegionSize,
		const int32 RejectionThreshold = 30
	);
};
