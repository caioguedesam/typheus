// ========================================================
// ASSET
// Asset library to load and manage assets for my C++ projects.
// Depends on common codebase and some libraries to load each asset type (such as stb_image).
// @Caio Guedes, 2023
// ========================================================
#pragma once
#include "engine/common/common.hpp"

namespace Ty
{
// ========================================================
// [ASSET TYPES]

struct AssetShader
{
    std::string src;
};

struct AssetTexture
{
    u32 width       = 0;
    u32 height      = 0;
    u32 channels    = 0;
    u8* data        = 0;
};

struct AssetModelObject
{
    std::vector<u32> indices;
    Handle<AssetTexture> textureAmbient;
    Handle<AssetTexture> textureDiffuse;
    Handle<AssetTexture> textureSpecular;
    Handle<AssetTexture> textureAlphaMask;
    Handle<AssetTexture> textureBump;
};

struct AssetModel
{
    std::vector<f32> vertices;
    std::vector<AssetModelObject> objects;
};

// ========================================================
// [ASSET LOADING]

Handle<AssetShader>     Asset_LoadShader(const std::string& assetPath);
Handle<AssetTexture>    Asset_LoadTexture(const std::string& assetPath);
Handle<AssetModel>      Asset_LoadModel_OBJ(const std::string& assetPath);

// ========================================================
// [ASSET DATA]
struct AssetTable
{
    std::vector<AssetShader*>        shaderAssets;
    std::vector<AssetTexture*>       textureAssets;
    std::vector<AssetModel*>         modelAssets;

    std::unordered_map<std::string, u32>    loadedAssets;
};
inline AssetTable assetTable;

inline bool Asset_IsLoaded(const std::string& assetPath) { return assetTable.loadedAssets.count(assetPath); }
inline AssetShader* Asset_GetShader(Handle<AssetShader> h_Asset) { return assetTable.shaderAssets[h_Asset.value]; }
inline AssetTexture* Asset_GetTexture(Handle<AssetTexture> h_Asset) { return assetTable.textureAssets[h_Asset.value]; }
inline AssetModel* Asset_GetModel(Handle<AssetModel> h_Asset) { return assetTable.modelAssets[h_Asset.value]; }

}   // namespace Ty
