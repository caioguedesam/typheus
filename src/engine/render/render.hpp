// ========================================================
// RENDER
// Real-time rendering API implemented with Vulkan.
// Will support more features in the future, such as compute and raytracing?
// @Caio Guedes, 2023
// ========================================================

#pragma once
#include "engine/core/base.hpp"
#include "engine/core/debug.hpp"
#include "engine/core/math.hpp"

#include "vulkan/vulkan.h"

namespace ty
{
namespace render
{
};
};

// Renderer API draft v3 (for Vulkan backend)
//
//  [1. RENDER N TEXTURED TRIANGLES ON SCREEN WITH DIFFERENT WORLD MATRICES]
//
//  [1.1. INIT]
//  concurrentFrames = 3;
//  Init(concurrentFrames, windowRect)                          // Initializes render context, swap chain, command queues, buffers and sync primitives for each concurrent frame
//
//  // Assets
//  ShaderAsset assetTriangleVS = LoadShaderAsset("triangleVS.vert");
//  ShaderAsset assetTrianglePS = LoadShaderAsset("trianglePS.frag");
//  TextureAsset assetCheckerTexture = LoadTextureAsset("checker.png");
//  f32 triangleVertices[] = {...};
//  u32 triangleIndices[] = {...};
//
//  struct SceneData
//  {
//      m4f view;
//      m4f proj;
//  } sceneData;
//
//  int triangleCount = 2;
//  struct ObjectData
//  {
//      m4f model;
//  } objectData[triangleCount];
//
//  // Render resources
//  Shader triangleVS = CreateShaderResource(SHADER_TYPE_VERTEX, assetTriangleVS);
//  Shader trianglePS = CreateShaderResource(SHADER_TYPE_PIXEL, assetTrianglePS);
//  
//  TextureDesc checkerTextureDesc = {};
//  checkerTextureDesc.type = TEXTURE_TYPE_2D;
//  checkerTextureDesc.format = FORMAT_RGBA8_SRGB;
//  checkerTextureDesc.width = assetCheckerTexture.width;
//  checkerTextureDesc.height = assetCheckerTexture.height;
//  checkerTextureDesc.mipCount = 1;
//  Texture checkerTexture = CreateTextureResource(checkerTextureDesc, assetCheckerTexture.data);
//
//  SamplerDesc defaultSamplerDesc = {};
//  defaultSamplerDesc.minFilter = SAMPLER_FILTER_LINEAR;
//  defaultSamplerDesc.magFilter = SAMPLER_FILTER_LINEAR;
//  defaultSamplerDesc.wrapU = SAMPLER_WRAP_REPEAT;
//  defaultSamplerDesc.wrapV = SAMPLER_WRAP_REPEAT;
//  defaultSamplerDesc.enableAnisotropy = true;
//  Sampler defaultSampler = CreateSamplerResource(defaultSamplerDesc);
//
//  VertexAttribute triangleVBAttributes[] =
//  {
//      VERTEX_ATTRIB_V3F, VERTEX_ATTRIB_V3F, VERTEX_ATTRIB_V2F,
//  };
//  VertexLayout triangleVBLayout = CreateVertexLayout(triangleVBAttributes);
//  Buffer triangleVB = CreateVertexBufferResource(triangleVBLayout, triangleVertices);
//  Buffer triangleIB = CreateIndexBufferResource(triangleIndices);
//
//  Buffer sceneDataUB = CreateUniformBuffer(sceneData);
//  Buffer objectDataUB = CreateDynamicUniformBuffer(objectData);
//
//  // Render pass
//  RenderPassDesc mainPassDesc = {};
//  mainPassDesc.outputCount = 1;
//  mainPassDesc.outputFormats = { FORMAT_RGBA8_SRGB };
//  mainPassDesc.hasDepthOutput = true;
//  mainPassDesc.depthFormats = { FORMAT_D32_FLOAT };
//  mainPassDesc.outputWidth = windowRect.width;
//  mainPassDesc.outputHeight = windowRect.height;
//  RenderPass mainPass = CreateRenderPass(mainPassDesc, concurrentFrames);
//
//  // Resource binding
//  // ResourceBindLayout --> VkDescriptorSetLayout
//  // ResourceBindSet --> multiple VkDescriptorSets (one per frame, matching layout)
//  //                 --> STATIC: VkDescriptorSet contains fixed offsets. DYNAMIC: Offset is set on bind time (such as for dynamic UBOs)
//  ResourceBindLayout globalResourceBindLayout[] =
//  {
//      { RESOURCE_TYPE_UNIFORM_BUFFER, SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL },           // (set = 0, binding = 0)
//      { RESOURCE_TYPE_SAMPLED_TEXTURE, SHADER_TYPE_PIXEL },                               // (set = 0, binding = 1)
//  };
//  ResourceBindLayout objectResourceBindLayout[] =
//  {
//      { RESOURCE_TYPE_DYNAMIC_UNIFORM_BUFFER, SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL },   // (set = 1, binding = 0)
//  };
//
//  ResourceBindSet globalResourceBindSet = CreateResourceBindSet(RESOURCE_BIND_STATIC, globalResourceBindLayout, { sceneDataUB, checkerTexture });
//  ResourceBindSet objectResourceBindSet = CreateResourceBindSet(RESOURCE_BIND_DYNAMIC, objectResourceBindLayout, { objectDataUB });
//
//  // Graphics pipeline
//  GraphicsPipelineDesc pipelineDesc = {};
//  pipelineDesc.renderPass = mainPass;
//  pipelineDesc.vertexLayout = triangleVBLayout;
//  pipelineDesc.vertexShader = triangleVS;
//  pipelineDesc.pixelShader = trianglePS;
//  pipelineDesc.resourceBindSetCount = 2;
//  pipelineDesc.resourceBindLayouts = { globalResourceBindLayout, objectResourceBindLayout };
//  pipelineDesc.primitive = PRIMITIVE_TRIANGLE_LIST;
//  pipelineDesc.fillMode = FILL_MODE_SOLID;
//  pipelineDesc.cullMode = CULL_MODE_BACK;
//  pipelineDesc.frontFace = FRONT_FACE_CCW;
//  GraphicsPipeline mainPassPipeline = CreateGraphicsPipeline(pipelineDesc);
//
//
//  [1.2. RENDER LOOP]
//  
//  BeginFrame(frameIndex);                                     // Updates sync primitives and swap chain image to match the current frame
//  CommandBuffer cmd = GetCommandBuffer(frameIndex);           // Gets the command buffer allocated for working in the given frame (commands can also be immediate)
//  BeginRenderPass(cmd, mainPass);
//  
//  // Update uniform buffer resources (CPU -> GPU)
//  UploadUniformBufferData(sceneDataUB, ...);      // View data
//  UploadUniformBufferData(objectDataUB, ...);     // World matrices for each scene object
//
//  BindGraphicsPipeline(cmd, mainPassPipeline);
//  SetViewport(cmd, ...);
//  SetScissorRect(cmd, ...);
//
//  BindResourceSet(cmd, globalResourceBindSet);            // Binds with no offset/static offset
//  BindVertexBuffer(cmd, triangleVB);
//  BindIndexBuffer(cmd, triangleIB);
//
//  for(int i = 0; i < triangleCount; i++)
//  {
//      BindResourceSet(cmd, objectResourceBindSet, i);     // Binds with dynamic offset
//      DrawIndexed(cmd, triangleIB.count);
//  }
//
//  EndRenderPass(cmd, mainPass);
//  
//  SubmitCommandBuffer(cmd);
//  frameIndex = EndFrame(frameIndex);                          // When frame ends, frame index is incremented to the next concurrent frame id
