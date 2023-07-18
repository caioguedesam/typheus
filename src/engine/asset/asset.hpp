// ========================================================
// ASSET
// Asset library to load and manage various types of assets.
// Uses some external libraries for loading purposes.
// @Caio Guedes, 2023
// ========================================================
#pragma once
#include "engine/core/base.hpp"
#include "engine/core/memory.hpp"
#include "engine/core/file.hpp"
#include "engine/core/async.hpp"
#include "engine/core/math.hpp"
#include "engine/core/ds.hpp"

namespace ty
{
namespace asset
{
// ========================================================
// [ASSET TYPES]
// TODO(caio): Asset types that I really want to support:
// - GLTF models
// - JSON
// - WAV audio

struct Asset
{
    file::Path path;
    //TODO(caio): Other relevant attributes (load datetime?)
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

struct Material : Asset
{
    // Properties
    math::v3f ambientColor = {1,1,1};
    math::v3f diffuseColor = {1,1,1};
    math::v3f specularColor = {0,0,0};
    f32 specularExponent = 1.f;

    // Maps
    file::Path ambientMap = {};
    file::Path diffuseMap = {};
    file::Path specularMap = {};
    file::Path alphaMap = {};
    file::Path bumpMap = {};

    // TODO(caio): PBR properties (emissive, roughness, metallic...)
    // TODO(caio): Other properties (transparency, refraction, ...)
};

struct ModelGroup
{
    List<u32> indices = {};
    Handle<Material> material = {};
};

struct Model : Asset
{
    List<f32> vertices = {};
    List<ModelGroup> groups = {};
};

// ========================================================
// [ASSET LISTS]

inline mem::HeapAllocator assetHeap;

inline List<Shader> shaders;
inline List<Image> images;
inline List<Material> materials;
inline List<Model> models;

// Index asset handles by path
inline HashMap<String, u32> loadedAssets;

// ========================================================
// [ASSET OPERATIONS]
void Init();

Handle<Shader>      LoadShader(file::Path assetPath);
Handle<Image>       LoadImageFile(file::Path assetPath, bool flipVertical = true);
Handle<Model>       LoadModelOBJ(file::Path assetPath, bool flipVerticalTexcoord = false);

inline bool         IsLoaded(file::Path assetPath) { return loadedAssets.HasKey(assetPath.str); }

};
};
