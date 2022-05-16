// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ProjectionInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(Blueprintable)
class UProjectionInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class GENERICFOLIAGE_API IProjectionInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	/** This is used to retrieve the rotation to face the camera towards the planet surface */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ProceduralFoliage")
		FRotator GetCaptureRotation(const FVector& InLocation);

	/** Projects in-game coordinates to geographic coords */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ProceduralFoliage")
		FVector ProjectEngineToGeographic(const FVector& InLocation);

	/** Projects geographic coordinates to geographic coords */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ProceduralFoliage")
		FVector ProjectGeographicToEngine(const FVector& InCartographic);
};
