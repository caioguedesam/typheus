#include "engine/core/debug.hpp"
#include "engine/asset/asset.hpp"
#include "engine/core/ds.hpp"
#include "engine/core/file.hpp"
#include "engine/core/memory.hpp"

#define STB_IMAGE_IMPLEMENTATION    // Just this file (needs to be here for malloc redefines)
#define STBI_MALLOC(sz) ty::mem::Alloc(sz)
#define STBI_REALLOC(p, newsz) ty::mem::Realloc(p, newsz)
#define STBI_FREE(p) ty::mem::Free(p)
#define STBI_ASSERT(x) ASSERT(x)
#include "stb_image.h"

#include "engine/asset/model.cpp"

namespace ty
{
namespace asset
{

#define ASSET_MAX_BINARY_DATA 256
#define ASSET_MAX_IMAGES 1024
#define ASSET_MAX_MATERIALS 1024
#define ASSET_MAX_MODELS 256
#define ASSET_MAX_ASSETS 2048

#define ASSET_MEMORY GB(1)

void Init()
{
    assetHeap = mem::MakeHeapAllocator(ASSET_MEMORY);
    mem::SetContext(&assetHeap);

    assetDatabase.loadedAssets = MakeMap<String, u32>(ASSET_MAX_ASSETS);
    assetDatabase.binaryDataAssets = MakeList<BinaryData>(ASSET_MAX_BINARY_DATA);
    assetDatabase.imageAssets = MakeList<Image>(ASSET_MAX_IMAGES);
    assetDatabase.materialAssets = MakeList<Material>(ASSET_MAX_MATERIALS);
    assetDatabase.modelAssets = MakeList<Model>(ASSET_MAX_MODELS);
}

Handle<BinaryData> LoadBinaryFile(file::Path assetPath)
{
    if(IsLoaded(assetPath)) return { assetDatabase.loadedAssets[assetPath.str] };
    mem::SetContext(&assetHeap);

    BinaryData blob = {};

    u64 assetFileSize;
    u8* assetFileData = file::ReadFileToBuffer(assetPath, &assetFileSize);

    blob.data = assetFileData;
    blob.size = assetFileSize;

    assetDatabase.binaryDataAssets.Push(blob);
    Handle<BinaryData> result = { (u32)assetDatabase.binaryDataAssets.count - 1 };
    assetDatabase.loadedAssets.Insert(assetPath.str, result.value);

    return result;
}

Handle<Image> LoadImageFile(file::Path assetPath, bool flipVertical)
{
    if(IsLoaded(assetPath)) return { assetDatabase.loadedAssets[assetPath.str] };
    mem::SetContext(&assetHeap);

    u64 assetFileSize = 0;
    u8* assetFileData = file::ReadFileToBuffer(assetPath, &assetFileSize);

    Image image = {};

    i32 width, height, channels;
    stbi_set_flip_vertically_on_load(flipVertical);
    PathToCStr(assetPath, assetPathCstr);
    u8* data = stbi_load_from_memory(assetFileData, assetFileSize, &width, &height, &channels, STBI_rgb_alpha);     // Hardcoded 4 channels for now
    
    image.width = width;
    image.height = height;
    image.channels = channels;
    image.data = data;

    assetDatabase.imageAssets.Push(image);
    Handle<Image> result = { (u32)assetDatabase.imageAssets.count - 1 };
    assetDatabase.loadedAssets.Insert(assetPath.str, result.value);

    mem::Free(assetFileData);

    return result;
}

};
};
