#pragma once
#include "engine/common/common.hpp"
#include "engine/renderer/window.hpp"
#include "glad/glad.h"

namespace Ty
{

typedef GLuint APIHandle;
#define API_HANDLE_INVALID      MAX_U32
#define HANDLE_INVALID          MAX_U32

#define RESOURCE_PATH "../resources/"
#define SHADER_PATH RESOURCE_PATH"shaders/"
#define TEXTURE_PATH RESOURCE_PATH"textures/"
#define MODELS_PATH RESOURCE_PATH"models/"

template <typename T>
struct Handle
{
    u32 value = HANDLE_INVALID;

    inline bool IsValid() { return value != HANDLE_INVALID; }
};

template<typename T>
inline bool operator==(const Handle<T> a, const Handle<T> b) { return a.value == b.value; }
template<typename T>
inline bool operator!=(const Handle<T> a, const Handle<T> b) { return a.value != b.value; }
template<typename T>
inline bool operator<(const Handle<T> a, const Handle<T> b) { return a.value < b.value; }
template<typename T>
inline bool operator>(const Handle<T> a, const Handle<T> b) { return a.value > b.value; }

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

struct MeshVertex
{
    v3f position = {};
    v3f normal = {};
    v2f texcoord = {};
};

struct Mesh
{
    Handle<Buffer> h_VertexBuffer;
    Handle<Buffer> h_IndexBuffer;
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
    Handle<Shader> h_VS;
    Handle<Shader> h_PS;
    APIHandle apiHandle = API_HANDLE_INVALID;
};

#define MATERIAL_MAX_TEXTURES   16
struct Material
{
    // TODO(caio)#RENDER: Material properties whenever supporting PBR and such.
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

struct MeshRenderable
{
    Handle<Mesh> h_Mesh;
    Handle<ShaderPipeline> h_Shader;
    Handle<Material> h_Material;

    // Renderable Uniforms
    m4f u_Model = Identity();
    i32 u_UseAlphaMask = 0;
};

#define RENDER_TARGET_MAX_COLOR_ATTACHMENTS 8
struct RenderTarget
{
    APIHandle apiHandle             = API_HANDLE_INVALID;
    APIHandle depthStencilAPIHandle = API_HANDLE_INVALID;
    Handle<Texture> colorAttachments[RENDER_TARGET_MAX_COLOR_ATTACHMENTS];
    u8 colorAttachmentCount         = 0;
    u32 width   = 0;
    u32 height  = 0;
};

Handle<Buffer>          Renderer_CreateBuffer(u8* bufferData, u64 bufferCount, u64 bufferStride, BufferType bufferType);
Handle<Texture>         Renderer_CreateTexture(u8* textureData, u32 textureWidth, u32 textureHeight, TextureFormat textureFormat, TextureParams textureParams);
//Handle<Mesh>            Renderer_CreateMesh(Handle<Buffer> h_VertexBuffer, Handle<Buffer> h_IndexBuffer);
Handle<Mesh>            Renderer_CreateMesh(Handle<Buffer> h_VertexBuffer, Handle<Buffer> h_IndexBuffer, VertexLayout vertexLayout);
Handle<Shader>          Renderer_CreateShader(std::string_view shaderSrc, ShaderType shaderType);
Handle<ShaderPipeline>  Renderer_CreateShaderPipeline(Handle<Shader> h_VS, Handle<Shader> h_PS);
Handle<Material>        Renderer_CreateMaterial(Handle<Texture>* h_MaterialTextureArray, u8 materialTextureCount);
Handle<RenderTarget>    Renderer_CreateRenderTarget(u32 rtWidth, u32 rtHeight, Handle<Texture>* h_RenderTexturesArray, u8 renderTextureCount);
Handle<MeshRenderable>  Renderer_CreateMeshRenderable(Handle<Mesh> h_Mesh, Handle<ShaderPipeline> h_Shader, Handle<Material> h_Material);

Handle<Texture>    Renderer_LoadTextureAsset(std::string_view assetPath);
std::vector<Handle<MeshRenderable>> Renderer_CreateRenderablesFromModel(std::string_view assetPath, Handle<ShaderPipeline> h_Shader);

void    Renderer_SetCamera(Camera camera);
void    Renderer_SetViewport(RenderViewport viewport);
void    Renderer_BindRenderTarget(Handle<RenderTarget> h_RenderTarget);
void    Renderer_UnbindRenderTarget();
void    Renderer_BindMesh(Handle<Mesh> h_Mesh);
void    Renderer_BindShaderPipeline(Handle<ShaderPipeline> h_Shader);
void    Renderer_BindMaterial(Handle<Material> h_Material);
void    Renderer_UpdateTextureMips(Handle<Texture> h_Texture);
void    Renderer_BindUniforms(const MeshRenderable& renderable);
void    Renderer_Clear(v4f clearColor);
void    Renderer_DrawMeshRenderable(MeshRenderable& renderable);

Camera& Renderer_GetCamera();

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
    std::vector<RenderTarget> renderTargets;

    std::vector<MeshRenderable> meshRenderables;
};

struct AssetDatabase
{
    std::unordered_map<std::string, u32> loadedAssets;
};

inline RendererData rendererData    = {};
inline AssetDatabase assetDatabase  = {};

inline Camera&         Renderer_GetCamera() { return rendererData.camera; }
inline Buffer&         Renderer_GetBuffer(Handle<Buffer> h_Buffer) { return rendererData.buffers[h_Buffer.value]; }
inline Texture&        Renderer_GetTexture(Handle<Texture> h_Texture) { return rendererData.textures[h_Texture.value]; }
inline Mesh&           Renderer_GetMesh(Handle<Mesh> h_Mesh) { return rendererData.meshes[h_Mesh.value]; }
inline Shader&         Renderer_GetShader(Handle<Shader> h_Shader) { return rendererData.shaders[h_Shader.value]; }
inline ShaderPipeline& Renderer_GetShaderPipeline(Handle<ShaderPipeline> h_ShaderPipeline) { return rendererData.shaderPipelines[h_ShaderPipeline.value]; }
inline Material&       Renderer_GetMaterial(Handle<Material> h_Material) { return rendererData.materials[h_Material.value]; }
inline RenderTarget&   Renderer_GetRenderTarget(Handle<RenderTarget> h_RenderTarget) { return rendererData.renderTargets[h_RenderTarget.value]; }
inline MeshRenderable& Renderer_GetMeshRenderable(Handle<MeshRenderable> h_MeshRenderable) { return rendererData.meshRenderables[h_MeshRenderable.value]; }

template <typename T>
Handle<T> Renderer_GetAsset(std::string_view assetPath)
{
    std::string assetPath_str(assetPath);
    if(!assetDatabase.loadedAssets.count(assetPath_str))
    {
        return { HANDLE_INVALID };
    }
    Handle<T> result { assetDatabase.loadedAssets[assetPath_str] };
    return result;
}

} // namespace Ty
