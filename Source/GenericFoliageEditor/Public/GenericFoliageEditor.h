// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGenericFoliageEditor, Log, All);

class FGenericFoliageEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
private:
	TSharedPtr<class FAssetTypeActions_GenericFoliageType> AssetTypeActions_GenericFoliageType_Ptr;
	TSharedPtr<class FAssetTypeActions_GenericFoliageCollection> AssetTypeActions_GenericFoliageCollection_Ptr;
};
