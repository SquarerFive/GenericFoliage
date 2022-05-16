// Fill out your copyright notice in the Description page of Project Settings.


#include "Compute/CopyRenderTargetToCpu.h"
#include "Engine/TextureRenderTarget2D.h"
#include "RenderGraph.h"

IMPLEMENT_GLOBAL_SHADER(FCopyRenderTargetToCpu, "/Shaders/CopyRenderTargetToCpu.usf", "main", SF_Compute);

void FCopyRenderTargetToCpu::Compute(FRHICommandListImmediate& RHICmdList, UTextureRenderTarget2D* InRenderTarget,
                                     TArray<float>& OutData)
{
	// Must be in the render thread to run this
	check(IsInRenderingThread());

	// Create the RDG builder and allocate the shader parameter struct
	FRDGBuilder GraphBuilder(RHICmdList);
	FParameters* Params = GraphBuilder.AllocParameters<FParameters>();

	// Set our texture
	Params->InRenderTarget = InRenderTarget->GetRenderTargetResource()->GetTexture2DRHI();

	// Create our uav
	OutData.SetNumUninitialized(InRenderTarget->SizeX * InRenderTarget->SizeY);
	FRDGBufferRef OutDataBuffer = CreateStructuredBuffer(GraphBuilder, TEXT("Out_Buffer"), sizeof(float), OutData.Num(), OutData.GetData(),
	                   sizeof(float) * OutData.Num());
	
	Params->OutRenderTargetData = GraphBuilder.CreateUAV(OutDataBuffer, EPixelFormat::PF_R32_FLOAT, ERDGUnorderedAccessViewFlags::None);
	
	// Set params
	Params->InRenderTargetDimensions = FIntPoint(InRenderTarget->SizeX, InRenderTarget->SizeY);

	// Run the graph
	TShaderMapRef<FCopyRenderTargetToCpu> Shader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("Copy RT (32bit) data"), Shader, Params, FIntVector(
		Params->InRenderTargetDimensions.X, Params->InRenderTargetDimensions.Y, 1
	));

	// Copy the buffer back onto the cpu
	auto BufferExternal = GraphBuilder.ConvertToExternalBuffer(OutDataBuffer);
	GraphBuilder.QueueBufferExtraction(Params->OutRenderTargetData->GetParent(), &BufferExternal, ERHIAccess::CPURead);

	GraphBuilder.Execute();

	check(BufferExternal.IsValid());
	void* Data = RHICmdList.LockBuffer(
		BufferExternal->GetRHI(),
	0, sizeof(float) * OutData.Num(), EResourceLockMode::RLM_ReadOnly);

	FMemory::Memcpy(OutData.GetData(), Data, sizeof(float) * OutData.Num());

	RHICmdList.UnlockBuffer(BufferExternal->GetRHI());
}
