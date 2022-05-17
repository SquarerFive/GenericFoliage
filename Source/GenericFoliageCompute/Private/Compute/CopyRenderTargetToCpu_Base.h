// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"

/*
 * This is used to extract 32 bit data from a render target, as it seems RHIReadTexture2D is limited to 16 bits...
 */

template<typename DataType>
struct TShaderDataType
{
};

template<>
struct TShaderDataType<float>
{
	static constexpr auto TypeName = "float";
	static constexpr EPixelFormat PixelFormat = PF_R32_FLOAT;
};

template<>
struct TShaderDataType<FVector2f>
{
	static constexpr auto TypeName = "float2";
	static constexpr EPixelFormat PixelFormat = PF_G32R32F;
};

template<>
struct TShaderDataType<FVector3f>
{
	static constexpr auto TypeName = "float3";
	static constexpr EPixelFormat PixelFormat = PF_R32G32B32F;
};

template<>
struct TShaderDataType<FVector4f>
{
public:
	static constexpr auto TypeName = "float4";
	static constexpr EPixelFormat PixelFormat = PF_A32B32G32R32F;
};

template<>
struct TShaderDataType<FLinearColor>
{
public:
	static constexpr auto TypeName = "float4";
	static constexpr EPixelFormat PixelFormat = PF_A32B32G32R32F;
};


/**
 * macro wrapper of CopyRenderTargetToCpu to support multiple types... maybe we should use templates?
 */

#define DECLARE_COPY_RENDER_TARGET_TO_CPU_CS(PixelDataType) \
class FCopyRenderTargetToCpu_##PixelDataType : public FGlobalShader \
{ \
public: \
	typedef FCopyRenderTargetToCpu_##PixelDataType FType; \
	\
	DECLARE_GLOBAL_SHADER(FCopyRenderTargetToCpu_##PixelDataType)\
	SHADER_USE_PARAMETER_STRUCT(FCopyRenderTargetToCpu_##PixelDataType, FGlobalShader) \
		BEGIN_SHADER_PARAMETER_STRUCT(FParameters, ) \
		SHADER_PARAMETER_TEXTURE(Texture2D<PixelDataType>, InRenderTarget) \
		SHADER_PARAMETER_SAMPLER(SamplerState, InRenderTargetSampler)\
		SHADER_PARAMETER(FIntPoint, InRenderTargetDimensions) \
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<PixelDataType>, OutRenderTargetData) \
	END_SHADER_PARAMETER_STRUCT() \
	\
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) \
	{ \
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5); \
	} \
	void Compute(FRHICommandListImmediate& RHICmdList, UTextureRenderTarget2D* InRenderTarget, TArray<PixelDataType>& OutData); \
	static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& Environment) \
	{ \
		Environment.SetDefine(TEXT("PIXEL_DATA_TYPE"), FString(TShaderDataType<PixelDataType>::TypeName)); \
	} \
};

#define DEFINE_COPY_RENDER_TARGET_TO_CPU_CS_EX(PixelDataType, ShaderFilename, ShaderEntrypoint) \
	IMPLEMENT_GLOBAL_SHADER(FCopyRenderTargetToCpu_##PixelDataType, ShaderFilename, ShaderEntrypoint, SF_Compute) \
void FCopyRenderTargetToCpu_##PixelDataType::Compute( \
	FRHICommandListImmediate& RHICmdList, UTextureRenderTarget2D* InRenderTarget, TArray<PixelDataType>& OutData) \
{ \
	constexpr EPixelFormat PixelFormat = TShaderDataType<PixelDataType>::PixelFormat; \
	\
	check(IsInRenderingThread()); \
	\
	/* Create the graph builder*/	\
	FRDGBuilder GraphBuilder(RHICmdList); \
	\
	/* Allocate our params */ \
	FParameters* Params = GraphBuilder.AllocParameters<FParameters>(); \
	\
	/* Check that the render target is valid */ \
	check(IsValid(InRenderTarget)); \
	check(InRenderTarget->GetRenderTargetResource()); \
	\
	Params->InRenderTarget = InRenderTarget->GetRenderTargetResource()->GetTexture2DRHI(); \
	Params->InRenderTargetSampler = RHICreateSamplerState(FSamplerStateInitializerRHI( \
		ESamplerFilter::SF_Point \
		\
	)); \
	OutData.SetNumUninitialized(InRenderTarget->SizeX * InRenderTarget->SizeY); \
	FRDGBufferRef OutDataBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("Out_Buffer"), sizeof(PixelDataType), OutData.Num(), OutData.GetData(), \
					   sizeof(PixelDataType) * OutData.Num()); \
	Params->OutRenderTargetData = GraphBuilder.CreateUAV(OutDataBuffer, PixelFormat, ERDGUnorderedAccessViewFlags::None); \
	Params->InRenderTargetDimensions = FIntPoint(InRenderTarget->SizeX, InRenderTarget->SizeY); \
	TShaderMapRef<FType> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel)); \
	FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("Copy RT (32bit) data"), Shader, Params, FIntVector( \
		Params->InRenderTargetDimensions.X, Params->InRenderTargetDimensions.Y, 1 \
	)); \
	auto BufferExternal = GraphBuilder.ConvertToExternalBuffer(OutDataBuffer); \
	GraphBuilder.QueueBufferExtraction(Params->OutRenderTargetData->GetParent(), &BufferExternal, ERHIAccess::CPURead); \
	GraphBuilder.Execute(); \
	check(BufferExternal.IsValid()); \
	void* Data = RHICmdList.LockBuffer( \
		BufferExternal->GetRHI(), \
	0, sizeof(PixelDataType) * OutData.Num(), EResourceLockMode::RLM_ReadOnly); \
	FMemory::Memcpy(OutData.GetData(), Data, sizeof(PixelDataType) * OutData.Num()); \
	RHICmdList.UnlockBuffer(BufferExternal->GetRHI()); \
}

#define DEFINE_COPY_RENDER_TARGET_TO_CPU_CS(PixelDataType) DEFINE_COPY_RENDER_TARGET_TO_CPU_CS_EX(PixelDataType, "/Shaders/CopyRenderTargetToCpu_Type.usf", "main")

DECLARE_COPY_RENDER_TARGET_TO_CPU_CS(FLinearColor)
DECLARE_COPY_RENDER_TARGET_TO_CPU_CS(FVector4f)
DECLARE_COPY_RENDER_TARGET_TO_CPU_CS(FVector3f)
DECLARE_COPY_RENDER_TARGET_TO_CPU_CS(FVector2f)
DECLARE_COPY_RENDER_TARGET_TO_CPU_CS(float)