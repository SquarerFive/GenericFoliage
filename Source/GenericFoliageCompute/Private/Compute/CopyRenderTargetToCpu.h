// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"

/**
 * 
 */
class FCopyRenderTargetToCpu : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FCopyRenderTargetToCpu)
	SHADER_USE_PARAMETER_STRUCT(FCopyRenderTargetToCpu, FGlobalShader)
		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_TEXTURE(Texture2D<float>, InRenderTarget)
		SHADER_PARAMETER(FIntPoint, InRenderTargetDimensions)
		// Output
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float>, OutRenderTargetData)
		
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	void Compute(FRHICommandListImmediate& RHICmdList, UTextureRenderTarget2D* InRenderTarget, TArray<float>& OutData);
};
