#pragma once
#include "engine/common/common.hpp"
#include "engine/renderer/window.hpp"
#include "glad/glad.h"

namespace Ty
{

typedef GLuint APIHandle;
#define API_HANDLE_INVALID      MAX_U32

enum DepthCompare : u32
{
    DEPTH_COMPARE_NEVER = 0,
    DEPTH_COMPARE_ALWAYS,
    DEPTH_COMPARE_LT,
    DEPTH_COMPARE_LE,
    DEPTH_COMPARE_GT,
    DEPTH_COMPARE_GE,
    DEPTH_COMPARE_E,
    DEPTH_COMPARE_NE,
};

enum BufferType : u32
{
    BUFFER_TYPE_INVALID = 0,
    BUFFER_TYPE_VERTEX,
    BUFFER_TYPE_INDEX,
    // TODO(caio)#RENDER: Add more buffer types whenever needed.
};

struct Buffer
{
    APIHandle apiHandle = API_HANDLE_INVALID;
    BufferType type     = BUFFER_TYPE_INVALID;
    u64 count           = 0;
    u64 stride          = 0;
    u8* data            = nullptr;

    u64 GetSize();
};

enum TextureFormat : u32
{
    TEXTURE_FORMAT_INVALID = 0,
    TEXTURE_FORMAT_R8,
    TEXTURE_FORMAT_RG8,
    TEXTURE_FORMAT_RGB8,
    TEXTURE_FORMAT_RGBA8,
    TEXTURE_FORMAT_RGBA16F,
    TEXTURE_FORMAT_D32,
    // TODO(caio)#RENDER: Add more texture formats whenever needed.
};

enum TextureParam_Wrap : u32
{
    TEXTURE_WRAP_REPEAT,
    TEXTURE_WRAP_CLAMP,
};

enum TextureParam_Filter : u32
{
    TEXTURE_FILTER_NEAREST,
    TEXTURE_FILTER_LINEAR,
};

struct TextureParams
{
    TextureParam_Wrap   wrapMode            = TEXTURE_WRAP_REPEAT;
    TextureParam_Filter filterMode_Min      = TEXTURE_FILTER_LINEAR;
    TextureParam_Filter filterMode_Max      = TEXTURE_FILTER_LINEAR;
    bool useMips                            = true;
    v4f borderColor                         = {0.f, 0.f, 0.f, 0.f};
};

struct Texture
{
    APIHandle apiHandle     = API_HANDLE_INVALID;
    TextureFormat format    = TEXTURE_FORMAT_INVALID;
    TextureParams params    = {};
    u32 width               = 0;
    u32 height              = 0;
    u8* data                = nullptr;

    u64 GetChannelCount();
    u64 GetSize();
};

#define CUBEMAP_FACES 6
struct Cubemap
{
    APIHandle apiHandle     = API_HANDLE_INVALID;
    TextureFormat format    = TEXTURE_FORMAT_INVALID;
    TextureParams params    = {};
    u32 width               = 0;
    u32 height              = 0;
    u8* data[CUBEMAP_FACES] = {0,0,0,0,0,0};
};

enum VertexAttributeType : u32
{
    VERTEX_ATTRIBUTE_FLOAT,
    VERTEX_ATTRIBUTE_VEC2,
    VERTEX_ATTRIBUTE_VEC3,
};

#define MAX_VERTEX_ATTRIBUTES 8
struct VertexLayout
{
    u32 count = 0;
    VertexAttributeType attributes[MAX_VERTEX_ATTRIBUTES] = {};
};

struct Mesh
{
    APIHandle apiHandle = API_HANDLE_INVALID;
    Handle<Buffer> h_VertexBuffer;
    Handle<Buffer> h_IndexBuffer;
};

enum ShaderStageType : u32
{
    SHADERSTAGE_TYPE_INVALID = 0,
    SHADERSTAGE_TYPE_VERTEX,
    SHADERSTAGE_TYPE_PIXEL,
    // TODO(caio)#RENDER: Add more shader types whenever needed.
};

struct ShaderStage
{
    APIHandle apiHandle     = API_HANDLE_INVALID;
    ShaderStageType type         = SHADERSTAGE_TYPE_INVALID;
};

struct Shader
{
    APIHandle apiHandle = API_HANDLE_INVALID;
    Handle<ShaderStage> h_VS;
    Handle<ShaderStage> h_PS;
};

#define MATERIAL_MAX_TEXTURES   16
struct Material
{
    Handle<Texture> h_Textures[MATERIAL_MAX_TEXTURES];
    u8 count = 0;
};

struct Camera
{
    v3f position    = {0,0,0};
    v3f front       = {0,0,-1};
    v3f right       = {1,0,0};
    v3f up          = {0,1,0};
    f32 fov         = 45.f;
    f32 nearPlane   = 0.001f;
    f32 farPlane    = 1000.f;

    m4f GetView();
    m4f GetProjection(const Window& window);

    void Move(v3f newPosition);
    void Rotate(f32 rotationAngle, v3f rotationAxis);
    void RotateYaw(f32 angle);
    void RotatePitch(f32 angle);
};

struct RenderViewport
{
    v2i bottomLeft      = {};
    u32 width           = 0;
    u32 height          = 0;
};

struct RenderTargetOutputDesc
{
    TextureFormat format;
    TextureParams params;
};

#define RENDER_TARGET_MAX_OUTPUTS 8
struct RenderTarget
{
    APIHandle apiHandle             = API_HANDLE_INVALID;
    Handle<Texture> outputs[RENDER_TARGET_MAX_OUTPUTS];
    Handle<Texture> depthOutput;
    u8 outputsCount                 = 0;
    u32 width                       = 0;
    u32 height                      = 0;
};

Handle<Buffer>              Renderer_CreateBuffer(BufferType type, u64 count, u64 stride, u8* data);
Handle<Texture>             Renderer_CreateTexture(TextureFormat format, TextureParams parameters, u32 width, u32 height, u8* data);
Handle<Cubemap>             Renderer_CreateCubemap(TextureFormat format, TextureParams parameters, u32 width, u32 height, u8* data[CUBEMAP_FACES]);
Handle<Mesh>                Renderer_CreateMesh(Handle<Buffer> h_vertexBuffer, Handle<Buffer> h_indexBuffer, VertexLayout vertexLayout);
void                        Renderer_CompileShaderStage(Handle<ShaderStage> h_shaderStage, std::string_view src);
Handle<ShaderStage>         Renderer_CreateShaderStage(ShaderStageType type, std::string_view src);
void                        Renderer_LinkShader(Handle<Shader> h_shader);
Handle<Shader>              Renderer_CreateShader(Handle<ShaderStage> h_vertexShader, Handle<ShaderStage> h_pixelShader);
Handle<Material>            Renderer_CreateMaterial(u8 texturesCount, Handle<Texture>* h_textures);
Handle<RenderTarget>        Renderer_CreateRenderTarget(u32 width, u32 height, u8 outputsCount, RenderTargetOutputDesc* outputsDesc);
Handle<RenderTarget>        Renderer_CreateDepthOnlyRenderTarget(u32 width, u32 height);
Handle<RenderTarget>        Renderer_GetDefaultRenderTarget();

void    Renderer_BindUniform_i32(std::string_view name, i32 value);
void    Renderer_BindUniform_u32(std::string_view name, u32 value);
void    Renderer_BindUniform_f32(std::string_view name, f32 value);
void    Renderer_BindUniform_v2f(std::string_view name, v2f value);
void    Renderer_BindUniform_v3f(std::string_view name, v3f value);
void    Renderer_BindUniform_m4f(std::string_view name, m4f value);

void    Renderer_BindRenderTarget(Handle<RenderTarget> h_renderTarget);
void    Renderer_BindMesh(Handle<Mesh> h_mesh);
void    Renderer_BindShader(Handle<Shader> h_shader);
void    Renderer_BindTexture(Handle<Texture> h_texture, u32 slot);
void    Renderer_BindCubemap(Handle<Cubemap> h_cubemap, u32 slot);
void    Renderer_BindMaterial(Handle<Material> h_material);
void    Renderer_UnbindRenderTarget();
// TODO(caio)#RENDER: Add more unbinds if needed

void    Renderer_SetDepthCompare(DepthCompare func);
void    Renderer_SetCamera(Camera camera);
void    Renderer_SetViewport(RenderViewport viewport);
void    Renderer_Clear(v4f color);
void    Renderer_GenerateMips(Handle<Texture> h_texture);
void    Renderer_GenerateMipsForRenderTarget(Handle<RenderTarget> h_renderTarget);
void    Renderer_DrawMesh();
void    Renderer_CopyTextureToBackbuffer(Handle<Texture> h_texture);
void    Renderer_CopyRenderTargetOutputToBackbuffer(Handle<RenderTarget> h_renderTarget, u8 outputIndex);
void    Renderer_CopyRenderTargetDepthOutputToBackbuffer(Handle<RenderTarget> h_renderTarget);
void    Renderer_CopyRenderTargetOutput(Handle<RenderTarget> h_src, Handle<RenderTarget> h_dest);
void    Renderer_CopyRenderTargetDepth(Handle<RenderTarget> h_src, Handle<RenderTarget> h_dest);

void    Renderer_Init(u32 windowWidth, u32 windowHeight, const char* windowName, Window* outWindow);
void    Renderer_BeginFrame();
void    Renderer_EndFrame();

struct RenderResourceTable
{
    std::vector<Buffer*>        bufferResources;
    std::vector<Texture*>       textureResources;
    std::vector<Cubemap*>       cubemapResources;
    std::vector<Mesh*>          meshResources;
    std::vector<ShaderStage*>   shaderStageResources;
    std::vector<Shader*>        shaderResources;
    std::vector<Material*>      materialResources;
    std::vector<RenderTarget*>  renderTargetResources;
};

struct RenderState
{
    Window* window          = nullptr;
    Camera camera           = {};
    RenderViewport viewport = {};
    DepthCompare depthCompare = DEPTH_COMPARE_LT;

    Handle<RenderTarget>    h_activeRenderTarget;
    Handle<Shader>          h_activeShader;
    Handle<Material>        h_activeMaterial;
    Handle<Mesh>            h_activeMesh;
};

inline RenderResourceTable  renderResourceTable;
inline RenderState          renderState;

inline Camera&          Renderer_GetCamera() { return renderState.camera; }
inline RenderViewport&  Renderer_GetViewport() { return renderState.viewport; }
inline Window*          Renderer_GetWindow() { return renderState.window; }
inline DepthCompare     Renderer_GetDepthCompare() { return renderState.depthCompare; }

inline Buffer*          Renderer_GetBuffer(Handle<Buffer> h_resource) { return renderResourceTable.bufferResources[h_resource.value]; }
inline Texture*         Renderer_GetTexture(Handle<Texture> h_resource) { return renderResourceTable.textureResources[h_resource.value]; }
inline Cubemap*         Renderer_GetCubemap(Handle<Cubemap> h_resource) { return renderResourceTable.cubemapResources[h_resource.value]; }
inline Mesh*            Renderer_GetMesh(Handle<Mesh> h_resource) { return renderResourceTable.meshResources[h_resource.value]; }
inline ShaderStage*     Renderer_GetShaderStage(Handle<ShaderStage> h_resource) { return renderResourceTable.shaderStageResources[h_resource.value]; }
inline Shader*          Renderer_GetShader(Handle<Shader> h_resource) { return renderResourceTable.shaderResources[h_resource.value]; }
inline Material*        Renderer_GetMaterial(Handle<Material> h_resource) { return renderResourceTable.materialResources[h_resource.value]; }
inline RenderTarget*    Renderer_GetRenderTarget(Handle<RenderTarget> h_resource) { return renderResourceTable.renderTargetResources[h_resource.value]; }

// Default resources
inline Handle<Texture>          h_defaultWhiteTexture;
inline Handle<ShaderStage>      h_screenQuadVS;
inline Handle<ShaderStage>      h_screenQuadPS;
inline Handle<Shader>           h_screenQuadShader;
inline Handle<Material>         h_screenQuadMaterial;
inline Handle<Mesh>             h_screenQuadMesh;
inline Handle<Mesh>             h_defaultCubeMesh;
inline Handle<RenderTarget>     h_defaultRenderTarget;

} // namespace Ty
