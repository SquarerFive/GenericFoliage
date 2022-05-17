// Copyright Aiden. S. All rights reserved

#pragma once

#include "AssetTypeActions_Base.h"

class FAssetTypeActions_GenericFoliageType : public FAssetTypeActions_Base
{
public:
	uint32 Category;
	
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;
};
