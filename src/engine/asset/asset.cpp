#include "engine/core/debug.hpp"
#include "engine/asset/asset.hpp"
#include "engine/core/ds.hpp"
#include "engine/core/file.hpp"
#include "engine/core/memory.hpp"
#include "engine/core/string.hpp"

#define STB_IMAGE_IMPLEMENTATION    // Just this file (needs to be here for malloc redefines)
#define STBI_MALLOC(sz) ty::mem::Alloc(sz)
#define STBI_REALLOC(p, newsz) ty::mem::Realloc(p, newsz)
#define STBI_FREE(p) ty::mem::Free(p)
#define STBI_ASSERT(x) ASSERT(x)
#include "stb_image.h"
#include "shaderc/shaderc.h"

#include "engine/asset/model.cpp"

namespace ty
{
namespace asset
{

#define ASSET_MAX_SHADERS 256
#define ASSET_MAX_IMAGES 1024
#define ASSET_MAX_MATERIALS 1024
#define ASSET_MAX_MODELS 256
#define ASSET_MAX_ASSETS 2048

#define ASSET_MEMORY GB(1)

void Init()
{
    assetHeap = mem::MakeHeapAllocator(ASSET_MEMORY);
    mem::SetContext(&assetHeap);

    loadedAssets = MakeMap<String, u32>(ASSET_MAX_ASSETS);
    shaders = MakeList<Shader>(ASSET_MAX_SHADERS);
    images = MakeList<Image>(ASSET_MAX_IMAGES);
    materials = MakeList<Material>(ASSET_MAX_MATERIALS);
    models = MakeList<Model>(ASSET_MAX_MODELS);
}

Handle<Shader> LoadShader(file::Path assetPath)
{
    if(IsLoaded(assetPath)) return { loadedAssets[assetPath.str] };
    mem::SetContext(&assetHeap);

    String shaderStr = file::ReadFileToString(assetPath);
    String shaderExt = assetPath.GetExtension().str;
    ShaderType type;
    shaderc_shader_kind shadercType;
    if(StrEquals(shaderExt, IStr(".vert")))
    {
        type = SHADER_TYPE_VERTEX;
        shadercType = shaderc_vertex_shader;
    }
    else if(StrEquals(shaderExt, IStr(".frag")))
    {
        type = SHADER_TYPE_PIXEL;
        shadercType = shaderc_fragment_shader;
    }
    else if(StrEquals(shaderExt, IStr(".comp")))
    {
        type = SHADER_TYPE_COMPUTE;
        shadercType = shaderc_compute_shader;
    }
    else ASSERT(0);

    shaderc_compiler_t compiler = shaderc_compiler_initialize();
    ToCStr(assetPath.str, assetPathCstr);
    shaderc_compilation_result_t compiled = shaderc_compile_into_spv(
            compiler,
            (char*)shaderStr.data,
            shaderStr.len,
            shadercType,
            assetPathCstr,
            "main",
            NULL);
    u64 errorCount = shaderc_result_get_num_errors(compiled);
    ASSERT(errorCount == 0);
    //TODO(caio): Error and warning formatting

    u64 compiledLen = shaderc_result_get_length(compiled);
    u8* compiledData = (u8*)shaderc_result_get_bytes(compiled);
    u8* resultData = (u8*)mem::Alloc(compiledLen);
    memcpy(resultData, compiledData, compiledLen);

    shaderc_result_release(compiled);
    shaderc_compiler_release(compiler);

    Shader shader = {};
    shader.type = type;
    shader.size = compiledLen;
    shader.data = resultData;
    shader.path = file::MakePathAlloc(assetPath.str);
    shaders.Push(shader);

    Handle<Shader> result = { (u32)shaders.count - 1 };
    loadedAssets.Insert(assetPath.str, result.value);
    return result;
}

Handle<Image> LoadImageFile(file::Path assetPath, bool flipVertical)
{
    if(IsLoaded(assetPath)) return { loadedAssets[assetPath.str] };
    mem::SetContext(&assetHeap);

    u64 assetFileSize = 0;
    u8* assetFileData = file::ReadFileToBuffer(assetPath, &assetFileSize);

    Image image = {};
    image.path = file::MakePathAlloc(assetPath.str);

    i32 width, height, channels;
    stbi_set_flip_vertically_on_load(flipVertical);
    PathToCStr(assetPath, assetPathCstr);
    u8* data = stbi_load_from_memory(assetFileData, assetFileSize, &width, &height, &channels, STBI_rgb_alpha);     // Hardcoded 4 channels for now
    
    image.width = width;
    image.height = height;
    image.channels = channels;
    image.data = data;

    images.Push(image);
    Handle<Image> result = { (u32)images.count - 1 };
    loadedAssets.Insert(assetPath.str, result.value);

    mem::Free(assetFileData);

    return result;
}

};
};
