// Fill out your copyright notice in the Description page of Project Settings.


#include "Foliage/GenericFoliageType.h"

FGuid UGenericFoliageType::GetGuid()
{
	if (!Guid.IsValid())
	{
		Guid = FGuid::NewGuid();
	}
	
	return Guid;
}
