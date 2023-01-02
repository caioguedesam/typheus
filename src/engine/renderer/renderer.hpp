#pragma once
#include "engine/common/common.hpp"
#include "engine/renderer/window.hpp"
#include "glad/glad.h"

namespace Ty
{

typedef u32 ResourceHandle;
typedef GLuint APIHandle;
#define API_HANDLE_INVALID      MAX_U32
#define RESOURCE_INVALID        MAX_U32

enum BufferType : u32
{
    BUFFER_TYPE_INVALID = 0,
    BUFFER_TYPE_VERTEX,
    BUFFER_TYPE_INDEX,
    // TODO(caio)#RENDER: Add more buffer types whenever needed.
};

struct Buffer
{
    BufferType type     = BUFFER_TYPE_INVALID;
    APIHandle apiHandle = API_HANDLE_INVALID;
    u64 count           = 0;
    u64 stride          = 0;
    u8* data            = nullptr;

    u64 GetSize();
};

enum TextureFormat : u32
{
    TEXTURE_FORMAT_INVALID = 0,
    TEXTURE_FORMAT_R8,
    TEXTURE_FORMAT_R8G8B8,
    TEXTURE_FORMAT_R8G8B8A8,
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
    TextureParam_Wrap wrapMode              = TEXTURE_WRAP_REPEAT;
    TextureParam_Filter filterMode_Min      = TEXTURE_FILTER_LINEAR;
    TextureParam_Filter filterMode_Max      = TEXTURE_FILTER_LINEAR;
    bool useMips                            = true;
};

struct Texture
{
    TextureFormat format    = TEXTURE_FORMAT_INVALID;
    TextureParams params    = {};
    u32 width               = 0;
    u32 height              = 0;
    APIHandle apiHandle     = API_HANDLE_INVALID;
    u8* data                = nullptr;

    u64 GetChannelCount();
    u64 GetSize();
};

struct MeshVertex
{
    v3f position = {};
    v3f normal = {};
    v2f texcoord = {};
};

struct Mesh
{
    ResourceHandle h_VertexBuffer   = RESOURCE_INVALID;
    ResourceHandle h_IndexBuffer    = RESOURCE_INVALID;
    APIHandle apiHandle             = API_HANDLE_INVALID;
};

enum ShaderType : u32
{
    SHADER_TYPE_INVALID = 0,
    SHADER_TYPE_VERTEX,
    SHADER_TYPE_PIXEL,
    // TODO(caio)#RENDER: Add more shader types whenever needed.
};

struct Shader
{
    ShaderType type         = SHADER_TYPE_INVALID;
    APIHandle apiHandle     = API_HANDLE_INVALID;
    std::string_view src    = {};
};

struct ShaderPipeline
{
    ResourceHandle h_VS     = RESOURCE_INVALID;
    ResourceHandle h_PS     = RESOURCE_INVALID;
    APIHandle apiHandle     = API_HANDLE_INVALID;
};

#define MATERIAL_MAX_TEXTURES   16
struct Material
{
    // TODO(caio)#RENDER: Material properties whenever supporting PBR and such.
    ResourceHandle h_Textures[MATERIAL_MAX_TEXTURES];
    u8 count = 0;
};

struct Camera
{
    v3f position    = {0,0,0};
    v3f target      = {0,0,-1};
    v3f up          = {0,1,0};
    f32 fov         = 45.f;
    f32 nearPlane   = 0.001f;
    f32 farPlane    = 1000.f;

    m4f GetView();
    m4f GetProjection(const Window& window);

    void Move(v3f newPosition);
    void Rotate(f32 rotationAngle, v3f rotationAxis);
};

struct RenderViewport
{
    v2i bottomLeft      = {};
    u32 width           = 0;
    u32 height          = 0;
};

struct Renderable
{
    ResourceHandle h_Mesh       = RESOURCE_INVALID;
    ResourceHandle h_Shader     = RESOURCE_INVALID;
    ResourceHandle h_Material   = RESOURCE_INVALID;

    // Renderable Uniforms
    m4f u_Model = Identity();
    i32 u_UseAlphaMask = 0;
};

void    Renderer_SetCamera(Camera camera);
void    Renderer_SetViewport(RenderViewport viewport);

ResourceHandle    Renderer_CreateBuffer(u8* bufferData, u64 bufferCount, u64 bufferStride, BufferType bufferType);
ResourceHandle    Renderer_CreateTexture(u8* textureData, u32 textureWidth, u32 textureHeight, TextureFormat textureFormat, TextureParams textureParams);
ResourceHandle    Renderer_CreateMesh(ResourceHandle h_VertexBuffer, ResourceHandle h_IndexBuffer);
ResourceHandle    Renderer_CreateShader(std::string_view shaderSrc, ShaderType shaderType);
ResourceHandle    Renderer_CreateShaderPipeline(ResourceHandle h_VS, ResourceHandle h_PS);
ResourceHandle    Renderer_CreateMaterial(ResourceHandle* h_MaterialTextureArray, u8 materialTextureCount);

ResourceHandle    Renderer_CreateRenderable(ResourceHandle h_Mesh, ResourceHandle h_Shader, ResourceHandle h_Material);
Renderable&       Renderer_GetRenderable(ResourceHandle h_Renderable);

ResourceHandle    Renderer_LoadTextureAsset(std::string_view assetPath);
std::vector<ResourceHandle> Renderer_CreateRenderablesFromModel(std::string_view assetPath, ResourceHandle h_Shader);

void    Renderer_BindMesh(ResourceHandle mesh);
void    Renderer_BindShaderPipeline(ResourceHandle shaderPipeline);
void    Renderer_BindMaterial(ResourceHandle material);
void    Renderer_BindUniforms(const Renderable& renderable);

void    Renderer_Init(u32 windowWidth, u32 windowHeight, const char* windowName, Window* outWindow);
void    Renderer_RenderFrame();

struct RendererData
{
    Window* window              = nullptr;
    Camera camera               = {};
    RenderViewport viewport     = {};

    std::vector<Buffer> buffers;
    std::vector<Texture> textures;
    std::vector<Mesh> meshes;
    std::vector<Shader> shaders;
    std::vector<ShaderPipeline> shaderPipelines;
    std::vector<Material> materials;

    std::vector<Renderable> renderables;
};

struct AssetDatabase
{
    std::unordered_map<std::string, ResourceHandle> loadedAssets;
};

ResourceHandle Renderer_GetAsset(std::string_view assetPath);

// TODO(caio)#RENDER: Remove this loading obj function from here later.
void LoadOBJModel(std::string_view assetPath, std::vector<MeshVertex>* outVertices, std::vector<u32>* outIndices);

} // namespace Ty
