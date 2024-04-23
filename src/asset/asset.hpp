// ========================================================
// ASSET
// Asset library to load and manage various types of assets.
// Uses some external libraries for loading purposes.
// @Caio Guedes, 2023
// ========================================================
#pragma once
#include "../core/base.hpp"
#include "../core/memory.hpp"
#include "../core/file.hpp"
#include "../core/math.hpp"
#include "../core/ds.hpp"

namespace ty
{
namespace asset
{
// ========================================================
// [ASSET TYPES]
// TODO(caio): Asset types that I really want to support:
// - WAV audio

struct Asset
{
    String path;
    //TODO(caio): Other relevant attributes (load datetime?)
    //TODO(caio): Maybe store these paths as hashes?
};

enum ShaderType
{
    SHADER_TYPE_VERTEX,
    SHADER_TYPE_PIXEL,
    SHADER_TYPE_COMPUTE,
};

struct Shader : Asset
{
    ShaderType type;
    u64 size = 0;
    byte* data = NULL;
};

struct Image : Asset
{
    u32 width = 0;
    u32 height = 0;
    u32 channels = 0;
    byte* data = NULL;
};

// ========================================================
// [GLTF]
#define TY_GLTF_BYTE 5120
#define TY_GLTF_UNSIGNED_BYTE 5121
#define TY_GLTF_SHORT 5122
#define TY_GLTF_UNSIGNED_SHORT 5123
#define TY_GLTF_UNSIGNED_INT 5125
#define TY_GLTF_FLOAT 5126
#define TY_GLTF_SAMPLER_MAG_FILTER_NEAREST 9728
#define TY_GLTF_SAMPLER_MAG_FILTER_LINEAR 9729
#define TY_GLTF_SAMPLER_MIN_FILTER_NEAREST 9728
#define TY_GLTF_SAMPLER_MIN_FILTER_LINEAR 9729
#define TY_GLTF_SAMPLER_MIN_FILTER_NEAREST_MIPMAP_NEAREST 9984
#define TY_GLTF_SAMPLER_MIN_FILTER_LINEAR_MIPMAP_NEAREST 9985
#define TY_GLTF_SAMPLER_MIN_FILTER_NEAREST_MIPMAP_LINEAR 9986
#define TY_GLTF_SAMPLER_MIN_FILTER_LINEAR_MIPMAP_LINEAR 9987
#define TY_GLTF_SAMPLER_WRAP_CLAMP_TO_EDGE 33071
#define TY_GLTF_SAMPLER_WRAP_MIRRORED_REPEAT 33648
#define TY_GLTF_SAMPLER_WRAP_REPEAT 10497
#define TY_GLTF_MAX_BUFFERS 8

struct GltfSampler
{
    u32 magFilter = 0;
    u32 minFilter = 0;
    u32 wrapS = 0;
    u32 wrapT = 0;
};

struct GltfTexture
{
    handle hImage = HANDLE_INVALID;
    GltfSampler sampler;
    u32 texCoordIndex = 0;
};

struct GltfMaterial
{
    String name;

    GltfTexture texBaseColor;
    Color4f fBaseColor = {1,1,1,1};

    GltfTexture texMetallicRoughness;
    f32 fMetallic = 1.f;
    f32 fRoughness = 0.f;

    GltfTexture texNormal;
    f32 fNormalScale = 1.f;

    GltfTexture texOcclusion;
    f32 fOcclusionStrength = 0.f;
    
    GltfTexture texEmissive;
    Color3f fEmissive = {0,0,0};
};

struct GltfPrimitive
{
    handle hMaterial = HANDLE_INVALID;
    
    Range rangeIndices;
    Range rangePositions;
    Range rangeNormals;
    Range rangeTexCoords0;
    Range rangeTexCoords1;
    Range rangeTexCoords2;

    //TODO(caio): Tangents, colors, etc.
    //TODO(caio): Primitive types other than triangle list
};

struct GltfMesh
{
    String name;
    SArray<GltfPrimitive> primitives;
};

struct GltfNode
{
    SArray<handle> hChildren;
    m4f transform = math::Identity();
    handle hMesh = HANDLE_INVALID;
};

struct GltfModel : Asset
{
    handle hRootNode = HANDLE_INVALID;

    // Internal subassets for GLTF assets are kept within, instead of on separate arrays on asset context.
    SArray<GltfTexture> textures = {};
    SArray<GltfMaterial> materials = {};
    SArray<GltfNode> nodes = {};
    SArray<GltfMesh> meshes = {};

    SArray<u32> indices = {};
    SArray<f32> vPositions = {};
    SArray<f32> vNormals = {};
    SArray<f32> vTexCoords0 = {};
    SArray<f32> vTexCoords1 = {};
    SArray<f32> vTexCoords2 = {};
};

// ========================================================
// [ASSET LISTS]

#define TY_ASSET_MAX_SHADERS 256
#define TY_ASSET_MAX_IMAGES 4096
#define TY_ASSET_MAX_GLTF_MODELS 256
#define TY_ASSET_MAX_ASSETS TY_ASSET_MAX_SHADERS + TY_ASSET_MAX_IMAGES + TY_ASSET_MAX_GLTF_MODELS

struct Context
{
    mem::Arena* arena;
    mem::Arena* tempArena;  // Used for temp allocations within functions. TODO(caio): Make it thread-safe someday.
    HashMap<String, handle> loadedAssets;

    SArray<Shader> shaders;
    SArray<Image> images;
    SArray<GltfModel> modelsGLTF;
};

Context MakeAssetContext(u64 arenaSize, u64 tempArenaSize);
bool    IsLoaded(Context* ctx, String assetPath);
handle  LoadShader(Context* ctx, String assetPath);
handle  LoadImageFile(Context* ctx, String assetPath, bool flipVertical = true);
handle  LoadModelGLTF(Context* ctx, String assetPath);

};
};
