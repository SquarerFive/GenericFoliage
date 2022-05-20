// Fill out your copyright notice in the Description page of Project Settings.


#include "Foliage/GenericFoliageType.h"

UGenericFoliageType::UGenericFoliageType()
{
	RandomStream = FRandomStream(RandomSeed);
}

FGuid UGenericFoliageType::GetGuid()
{
	if (!Guid.IsValid())
	{
		Guid = FGuid::NewGuid();
	}

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
