#include "CopyRenderTargetToCpu_Base.h"
#include "Engine/TextureRenderTarget2D.h"
#include "RenderGraph.h"
#include "GenericFoliage/Public/GenericFoliage.h"

DEFINE_COPY_RENDER_TARGET_TO_CPU_CS(FLinearColor)
DEFINE_COPY_RENDER_TARGET_TO_CPU_CS(FVector4f)
DEFINE_COPY_RENDER_TARGET_TO_CPU_CS(FVector3f)
DEFINE_COPY_RENDER_TARGET_TO_CPU_CS(FVector2f)
DEFINE_COPY_RENDER_TARGET_TO_CPU_CS(float)