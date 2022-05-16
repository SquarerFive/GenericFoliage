// Copyright Epic Games, Inc. All Rights Reserved.

#include "GenericFoliageCompute.h"

#include "Compute/CopyRenderTargetToCpu.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FGenericFoliageComputeModule"

DEFINE_LOG_CATEGORY(LogGenericFoliageCompute)

void FGenericFoliageComputeModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	AddShaderSourceDirectoryMapping(
		TEXT("/Shaders"), FPaths::Combine(
			IPluginManager::Get().FindPlugin(TEXT("GenericFoliage"))->GetBaseDir(),
			TEXT("Shaders")
		)
	);
}

void FGenericFoliageComputeModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

void FGenericFoliageComputeModule::CopyRenderTargetToCpu(FRHICommandListImmediate& RHICmdList,
	UTextureRenderTarget2D* InRenderTarget, TArray<float>& OutData)
{
	TShaderMapRef<FCopyRenderTargetToCpu> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	check(Shader.IsValid());
	Shader->Compute(RHICmdList, InRenderTarget, OutData);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FGenericFoliageComputeModule, GenericFoliageCompute)