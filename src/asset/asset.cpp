#include "../core/debug.hpp"
#include "../core/ds.hpp"
#include "../core/file.hpp"
#include "../core/memory.hpp"
#include "../core/string.hpp"
#include "./asset.hpp"
#include "./model.cpp"

#define STB_IMAGE_IMPLEMENTATION    // Just this file (needs to be here for malloc redefines)
#define STBI_MALLOC(sz) ty::mem::Alloc(sz)
#define STBI_REALLOC(p, newsz) ty::mem::Realloc(p, newsz)
#define STBI_FREE(p) ty::mem::Free(p)
#define STBI_ASSERT(x) ASSERT(x)
#include "../third_party/stb/stb_image.h"
#include "shaderc/shaderc.h"


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

    loadedAssets = MakeMap<String, u64>(ASSET_MAX_ASSETS);
    shaders = MakeHList<Shader>(ASSET_MAX_SHADERS);
    images = MakeHList<Image>(ASSET_MAX_IMAGES);
    materials = MakeHList<Material>(ASSET_MAX_MATERIALS);
    models = MakeHList<Model>(ASSET_MAX_MODELS);


    gltfSamplers = MakeHList<GltfSampler>();
    gltfMaterials = MakeHList<GltfMaterial>();
    gltfMeshes = MakeHList<GltfMesh>();
    gltfNodes = MakeHList<GltfNode>();
    gltfModels = MakeHList<GltfModel>();
}

Handle<Shader> LoadShader(file::Path assetPath)
{
    if(IsLoaded(assetPath)) return Handle<Shader>(loadedAssets[assetPath.str]);
    mem::SetContext(&assetHeap);

    String shaderStr = file::ReadFileToString(assetPath);
    String shaderExt = assetPath.Extension();
    ShaderType type;
    shaderc_shader_kind shadercType;
    if(shaderExt == IStr(".vert"))
    {
        type = SHADER_TYPE_VERTEX;
        shadercType = shaderc_vertex_shader;
    }
    else if(shaderExt == IStr(".frag"))
    {
        type = SHADER_TYPE_PIXEL;
        shadercType = shaderc_fragment_shader;
    }
    else if(shaderExt == IStr(".comp"))
    {
        type = SHADER_TYPE_COMPUTE;
        shadercType = shaderc_compute_shader;
    }
    else ASSERT(0);

    shaderc_compiler_t compiler = shaderc_compiler_initialize();
    shaderc_compile_options_t options = shaderc_compile_options_initialize();
#if TY_DEBUG
    shaderc_compile_options_set_generate_debug_info(options);
#endif
    shaderc_compilation_result_t compiled = shaderc_compile_into_spv(
            compiler,
            (char*)shaderStr.data,
            shaderStr.len,
            shadercType,
            assetPath.CStr(),
            "main",
            options);
    u64 errorCount = shaderc_result_get_num_errors(compiled);
    ASSERT(errorCount == 0);
    //TODO(caio): Error and warning formatting

    u64 compiledLen = shaderc_result_get_length(compiled);
    u8* compiledData = (u8*)shaderc_result_get_bytes(compiled);
    u8* resultData = (u8*)mem::Alloc(compiledLen);
    memcpy(resultData, compiledData, compiledLen);

    shaderc_result_release(compiled);
    shaderc_compile_options_release(options);
    shaderc_compiler_release(compiler);

    Shader shader = {};
    shader.type = type;
    shader.size = compiledLen;
    shader.data = resultData;
    MStr(assetPathStr, MAX_PATH);
    str::Append(assetPathStr, assetPath.str);
    shader.path = file::MakePath(assetPathStr);
    Handle<Shader> result = shaders.Insert(shader);

    loadedAssets.Insert(assetPath.str, result.GetData());
    return result;
}

Handle<Image> LoadImageFile(file::Path assetPath, bool flipVertical)
{
    if(IsLoaded(assetPath)) return Handle<Image>(loadedAssets[assetPath.str]);
    mem::SetContext(&assetHeap);

    u64 assetFileSize = 0;
    u8* assetFileData = file::ReadFileToBuffer(assetPath, &assetFileSize);

    Image image = {};
    MStr(assetPathStr, MAX_PATH);
    str::Append(assetPathStr, assetPath.str);
    image.path = file::MakePath(assetPathStr);

    i32 width, height, channels;
    stbi_set_flip_vertically_on_load(flipVertical);
    u8* data = stbi_load_from_memory(assetFileData, assetFileSize, &width, &height, &channels, STBI_rgb_alpha);     // Hardcoded 4 channels for now
    
    image.width = width;
    image.height = height;
    image.channels = channels;
    image.data = data;

    Handle<Image> result = images.Insert(image);
    loadedAssets.Insert(assetPath.str, result.GetData());

    mem::Free(assetFileData);

    return result;
}

void UnloadImage(Handle<Image> hImage)
{
    ASSERT(hImage.IsValid());
    mem::SetContext(&assetHeap);

    Image& image = images[hImage];
    mem::Free(image.data);
    images.Remove(hImage);
}

};
};
