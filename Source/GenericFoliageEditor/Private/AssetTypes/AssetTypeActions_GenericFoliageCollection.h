// Copyright Aiden. Soedjarwo. All rights reserved

#pragma once
#include "AssetTypeActions_Base.h"

class GENERICFOLIAGEEDITOR_API FAssetTypeActions_GenericFoliageCollection : public FAssetTypeActions_Base
{
public:
	uint32 Category;
	
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;
};