// Copyright Aiden. Soedjarwo. All rights reserved

#include "AssetTypeActions_GenericFoliageCollection.h"
#include "Foliage/GenericFoliageCollection.h"

FText FAssetTypeActions_GenericFoliageCollection::GetName() const
{
	return FText::FromString(TEXT("Generic Foliage Collection"));
}

FColor FAssetTypeActions_GenericFoliageCollection::GetTypeColor() const
{
	return FColor(32, 124, 32, 255);
}

UClass* FAssetTypeActions_GenericFoliageCollection::GetSupportedClass() const
{
	return UGenericFoliageCollection::StaticClass();
}

uint32 FAssetTypeActions_GenericFoliageCollection::GetCategories()
{
	return Category;
}
