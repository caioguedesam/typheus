#include "../core/file.hpp"
#include "../core/memory.hpp"
#include "../core/string.hpp"
#include "./asset.hpp"
#include "src/core/base.hpp"

#define STB_IMAGE_IMPLEMENTATION    // Just this file (needs to be here for malloc redefines)

namespace ty
{
namespace asset
{
    // Need this for usage with the STBI memory macros.
    // TODO(caio): #THREADSAFE This is certainly not thread-safe.
    mem::Arena* stbiArena = NULL;
}
}

#define STBI_MALLOC(sz) ty::mem::ArenaPush(ty::asset::stbiArena, sz)
// TODO(caio): Verify if this is correct (pseudo realloc implementation, just allocate again and copy the new size over
#define STBI_REALLOC(p, newsz) ty::mem::ArenaPushCopy(ty::asset::stbiArena, newsz, p, newsz)
#define STBI_FREE(p)
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

Context MakeAssetContext(u64 arenaSize, u64 tempArenaSize)
{
    Context ctx = {};
    ctx.arena = mem::MakeArena(arenaSize);
    ctx.tempArena = mem::MakeArena(tempArenaSize);

    ctx.loadedAssets = MakeMap<String, handle>(ctx.arena, ASSET_MAX_ASSETS);
    ctx.shaders = MakeSArray<Shader>(ctx.arena, ASSET_MAX_SHADERS);
    ctx.images = MakeSArray<Image>(ctx.arena, ASSET_MAX_IMAGES);
    ctx.modelsGLTF = MakeSArray<GltfModel>(ctx.arena, ASSET_MAX_MODELS);

    return ctx;
}

bool IsLoaded(Context* ctx, String assetPath)
{
    ASSERT(ctx);
    return ctx->loadedAssets.HasKey(assetPath);
}

handle LoadShader(Context* ctx, String assetPath)
{
    if(IsLoaded(ctx, assetPath))
    {
        return ctx->loadedAssets[assetPath];
    }

    //mem::ArenaClear(ctx->tempArena);
    String shaderStr = file::ReadFileToString(ctx->tempArena, assetPath);
    String shaderExt = file::PathExt(assetPath);
    ShaderType type;
    shaderc_shader_kind shadercType;
    if(shaderExt == ".vert")
    {
        type = SHADER_TYPE_VERTEX;
        shadercType = shaderc_vertex_shader;
    }
    else if(shaderExt == ".frag")
    {
        type = SHADER_TYPE_PIXEL;
        shadercType = shaderc_fragment_shader;
    }
    else if(shaderExt == ".comp")
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
    if(errorCount != 0)
    {
        LOGLF("SHADER COMPILE", "%s", shaderc_result_get_error_message(compiled));
        ASSERT(0);
    }

    u64 compiledLen = shaderc_result_get_length(compiled);
    byte* compiledData = (byte*)shaderc_result_get_bytes(compiled);
    byte* resultData = (byte*)mem::ArenaPush(ctx->arena, compiledLen);
    memcpy(resultData, compiledData, compiledLen);

    shaderc_result_release(compiled);
    shaderc_compile_options_release(options);
    shaderc_compiler_release(compiler);

    Shader shader = {};
    shader.type = type;
    shader.size = compiledLen;
    shader.data = resultData;
    shader.path = Str(ctx->arena, assetPath);
    
    handle result = ctx->shaders.Push(shader);
    ctx->loadedAssets.Insert(assetPath, result);
    return result;
}

handle LoadImageFile(Context* ctx, String assetPath, bool flipVertical)
{
    if(IsLoaded(ctx, assetPath))
    {
        return ctx->loadedAssets[assetPath];
    }

    //TODO(caio): This assetFileData memory is not used after parsed into an image asset.
    // Maybe deal with that waste later.
    u64 assetFileSize = 0;
    //mem::ArenaClear(ctx->tempArena);
    byte* assetFileData = file::ReadFileToBuffer(ctx->tempArena, assetPath, &assetFileSize);

    Image image = {};
    image.path = Str(ctx->arena, assetPath);

    i32 width, height, channels;
    stbiArena = ctx->arena;
    stbi_set_flip_vertically_on_load(flipVertical);
    byte* data = stbi_load_from_memory(assetFileData, assetFileSize, &width, &height, &channels, STBI_rgb_alpha);     // Hardcoded 4 channels for now
    
    image.width = width;
    image.height = height;
    //image.channels = channels;
    image.channels = 4;
    image.data = data;

    handle result = ctx->images.Push(image);
    ctx->loadedAssets.Insert(assetPath, result);
    return result;
}

};
};
