// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGenericFoliageCompute, Log, All);

class GENERICFOLIAGECOMPUTE_API FGenericFoliageComputeModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void CopyRenderTargetToCpu(FRHICommandListImmediate& RHICmdList, UTextureRenderTarget2D* InRenderTarget, TArray<float>& OutData);
	void CopyRenderTargetToCpu(FRHICommandListImmediate& RHICmdList, UTextureRenderTarget2D* InRenderTarget, TArray<FVector3f>& OutData);
	void CopyRenderTargetToCpu(FRHICommandListImmediate& RHICmdList, UTextureRenderTarget2D* InRenderTarget, TArray<FVector4f>& OutData);
	void CopyRenderTargetToCpu(FRHICommandListImmediate& RHICmdList, UTextureRenderTarget2D* InRenderTarget, TArray<FLinearColor>& OutData);
};
