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
#include "../core/async.hpp"
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
    file::Path path;
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
    u8* data = NULL;
};

struct Image : Asset
{
    u32 width = 0;
    u32 height = 0;
    u32 channels = 0;
    u8* data = NULL;
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
    Handle<Image> hImage = HANDLE_INVALID_VALUE;
    Handle<GltfSampler> hSampler = HANDLE_INVALID_VALUE;
    u32 texCoordIndex = 0;
};

struct GltfMaterial
{
    String name;

    GltfTexture texBaseColor;
    math::v4f fBaseColor = {1,1,1,1};

    GltfTexture texMetallicRoughness;
    f32 fMetallic = 1.f;
    f32 fRoughness = 0.f;

    GltfTexture texNormal;
    f32 fNormalScale = 1.f;

    GltfTexture texOcclusion;
    f32 fOcclusionStrength = 0.f;
    
    GltfTexture texEmissive;
    math::v3f fEmissive = {0,0,0};
};

struct GltfPrimitive
{
    Handle<GltfMaterial> hMaterial = HANDLE_INVALID_VALUE;
    
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
    List<GltfPrimitive> primitives;
};

struct GltfNode
{
    List<Handle<GltfNode>> hChildren;
    math::m4f transform = math::Identity();
    Handle<GltfMesh> hMesh;
};

struct GltfModel : Asset
{
    Handle<GltfNode> root = HANDLE_INVALID_VALUE;

    List<u32> indices = {};
    List<f32> vPositions = {};
    List<f32> vNormals = {};
    List<f32> vTexCoords0 = {};
    List<f32> vTexCoords1 = {};
    List<f32> vTexCoords2 = {};
    List<Handle<Image>> hImages = {};
    List<Handle<GltfSampler>> hSamplers = {};
    List<Handle<GltfMaterial>> hMaterials = {};
};


// ========================================================
// [ASSET LISTS]

inline mem::HeapAllocator assetHeap;

inline HList<Shader> shaders;
inline HList<Image> images;

inline Shader& GetShader(Handle<Shader> hAsset) { return shaders[hAsset]; }
inline Image& GetImage(Handle<Image> hAsset)    { return images[hAsset]; }

inline HList<GltfSampler>   gltfSamplers;
inline HList<GltfMaterial>  gltfMaterials;
inline HList<GltfMesh>      gltfMeshes;
inline HList<GltfNode>      gltfNodes;
inline HList<GltfModel>     gltfModels;

inline GltfSampler& GetGltfSampler(Handle<GltfSampler> hAsset)      { return gltfSamplers[hAsset]; }
inline GltfMaterial& GetGltfMaterial(Handle<GltfMaterial> hAsset)   { return gltfMaterials[hAsset]; }
inline GltfMesh& GetGltfMesh(Handle<GltfMesh> hAsset)               { return gltfMeshes[hAsset]; }
inline GltfNode& GetGltfNode(Handle<GltfNode> hAsset)               { return gltfNodes[hAsset]; }
inline GltfModel& GetGltfModel(Handle<GltfModel> hAsset)               { return gltfModels[hAsset]; }

// Index asset handles by path
inline HashMap<String, u64> loadedAssets;

// ========================================================
// [ASSET OPERATIONS]
void Init();

Handle<Shader>      LoadShader(file::Path assetPath);
Handle<Image>       LoadImageFile(file::Path assetPath, bool flipVertical = true);
Handle<GltfModel>   LoadGltfModel(file::Path assetPath);

void                UnloadImage(Handle<Image> hImage);
void                UnloadGltfModel(Handle<GltfModel> hModel);

inline bool         IsLoaded(file::Path assetPath) { return loadedAssets.HasKey(assetPath.str); }

// TODO(caio):
// - Freeing asset memory
//      - Shaders

};
};
