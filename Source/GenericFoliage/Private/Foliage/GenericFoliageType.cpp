// Fill out your copyright notice in the Description page of Project Settings.


#include "Foliage/GenericFoliageType.h"

UGenericFoliageType::UGenericFoliageType()
{
	RandomStream = FRandomStream(RandomSeed);

	ResetGUID();
}

FGuid UGenericFoliageType::GetGuid()
{
	return Guid;
}

FVector UGenericFoliageType::GetRandomScale() const
{
	return ScaleRange.GetRandom(RandomStream);
}

FRotator UGenericFoliageType::GetRandomRotator() const
{
	return RotatorRange.GetRandom(RandomStream);
}

FVector UGenericFoliageType::GetRandomLocalOffset() const
{
	return RandomLocalOffsetRange.GetRandom(RandomStream);
}

void UGenericFoliageType::ResetGUID()
{
	FGuid RandomGuid(
		FMath::RandRange(0, RAND_MAX),
		FMath::RandRange(0, RAND_MAX),
		FMath::RandRange(0, RAND_MAX),
		FMath::RandRange(0, RAND_MAX)
	);

	FPlatformMisc::CreateGuid(RandomGuid);

	Guid = RandomGuid;
}
