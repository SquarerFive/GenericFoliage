// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GenericFoliageCollection.generated.h"

/**
 * 
 */

class UGenericFoliageType;

USTRUCT(BlueprintType)
struct FGenericFoliageCollectionValue
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		TArray<UGenericFoliageType*> FoliageTypes;
};

UCLASS()
class GENERICFOLIAGE_API UGenericFoliageCollection : public UObject
{
	GENERATED_BODY()

public:
	/** A collection of foliage types, mapped by id */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = General)
	TMap<int32, FGenericFoliageCollectionValue> Collection;
};
