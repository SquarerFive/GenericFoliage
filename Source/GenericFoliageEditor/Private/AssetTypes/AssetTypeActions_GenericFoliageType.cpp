// Copyright Aiden. Soedjarwo. All rights reserved

#include "AssetTypeActions_GenericFoliageType.h"

#include "GenericFoliageEditor.h"
#include "Foliage/GenericFoliageType.h"

FText FAssetTypeActions_GenericFoliageType::GetName() const
{
	return FText::FromString(TEXT("Generic Foliage Type"));
}

FColor FAssetTypeActions_GenericFoliageType::GetTypeColor() const
{
	return FColor(32, 124, 32, 255);
}

UClass* FAssetTypeActions_GenericFoliageType::GetSupportedClass() const
{
	return UGenericFoliageType::StaticClass();
}

uint32 FAssetTypeActions_GenericFoliageType::GetCategories()
{
	return Category;
}
