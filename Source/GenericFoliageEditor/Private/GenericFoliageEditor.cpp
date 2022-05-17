// Copyright Epic Games, Inc. All Rights Reserved.

#include "GenericFoliageEditor.h"

#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "AssetTypes/AssetTypeActions_GenericFoliageType.h"

#define LOCTEXT_NAMESPACE "FGenericFoliageEditorModule"

DEFINE_LOG_CATEGORY(LogGenericFoliageEditor)

void FGenericFoliageEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	uint32 AssetCategory = AssetTools.RegisterAdvancedAssetCategory(
		FName(TEXT("GenericFoliage")),
		FText::FromString(TEXT("Generic Foliage")));

	AssetTypeActions_GenericFoliageType_Ptr = MakeShareable(
				new FAssetTypeActions_GenericFoliageType()
			);

	AssetTypeActions_GenericFoliageType_Ptr->Category = AssetCategory;
	
	AssetTools.RegisterAssetTypeActions(
			AssetTypeActions_GenericFoliageType_Ptr.ToSharedRef()
	);
}

void FGenericFoliageEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	if (FModuleManager::Get().IsModuleLoaded("AssetTools") && AssetTypeActions_GenericFoliageType_Ptr.IsValid())
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		AssetTools.UnregisterAssetTypeActions(AssetTypeActions_GenericFoliageType_Ptr.ToSharedRef());
	}
}
#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FGenericFoliageEditorModule, GenericFoliageEditor)