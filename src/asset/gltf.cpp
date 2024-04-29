#include "../core/memory.hpp"
#include "../core/debug.hpp"
#include "../core/file.hpp"
#include "./json.hpp"
#include "./asset.hpp"
#include "src/core/base.hpp"
#include "src/core/ds.hpp"
#include "src/core/math.hpp"
#include "src/core/string.hpp"

namespace ty
{
namespace asset
{

void LoadModelGLTF_LoadBuffers(Context* ctx, JsonObject* gltfJson, String assetPath, byte** out)
{
    JsonArray& buffersJson = *gltfJson->GetArrayValue("buffers");
    ASSERT(buffersJson.count <= TY_GLTF_MAX_BUFFERS);
    for(i32 i = 0; i < buffersJson.count; i++)
    {
        JsonObject* bufferJson = buffersJson[i].AsObject();
        String bufferPath = StrConcat(ctx->tempArena, 
                file::PathFileDir(assetPath), 
                bufferJson->GetStringValue("uri"));
        out[i] = file::ReadFileToBuffer(ctx->tempArena, bufferPath);
    }
}

SArray<GltfTexture> LoadModelGLTF_LoadTextures(Context* ctx, JsonObject* gltfJson, String assetPath)
{
    JsonArray& texturesJson = *gltfJson->GetArrayValue("textures");
    JsonArray& imagesJson = *gltfJson->GetArrayValue("images");
    JsonArray& samplersJson = *gltfJson->GetArrayValue("samplers");

    SArray<GltfTexture> result = MakeSArray<GltfTexture>(ctx->arena, texturesJson.count);
    for(i32 i = 0; i < texturesJson.count; i++)
    {
        JsonObject* textureJson = texturesJson[i].AsObject();
        i32 imageJsonIndex = textureJson->GetNumberValue("source");
        i32 samplerJsonIndex = textureJson->GetNumberValue("sampler");

        JsonObject* imageJson = imagesJson[imageJsonIndex].AsObject();
        JsonObject* samplerJson = samplersJson[samplerJsonIndex].AsObject();

        String imagePath = StrConcat(ctx->tempArena, 
                file::PathFileDir(assetPath), 
                imageJson->GetStringValue("uri"));

        handle hImage = LoadImageFile(ctx, imagePath, false);
        GltfSampler sampler = {};
        sampler.minFilter = (u32)samplerJson->GetNumberValue("minFilter");
        sampler.magFilter = (u32)samplerJson->GetNumberValue("magFilter");
        sampler.wrapS = (u32)samplerJson->GetNumberValue("wrapS");
        sampler.wrapT = (u32)samplerJson->GetNumberValue("wrapT");

        GltfTexture texture = {};
        texture.hImage = hImage;
        texture.sampler = sampler;

        result.Push(texture);
    }
    return result;
}

SArray<GltfMaterial> LoadModelGLTF_LoadMaterials(Context* ctx, JsonObject* gltfJson, String assetPath, SArray<GltfTexture> gltfTextures)
{
    JsonArray& materialsJson = *gltfJson->GetArrayValue("materials");

    SArray<GltfMaterial> result = MakeSArray<GltfMaterial>(ctx->arena, materialsJson.count);
    for(i32 i = 0; i < materialsJson.count; i++)
    {
        GltfMaterial material = {};
        JsonObject* materialJson = materialsJson[i].AsObject();

        // Name
        String name;
        if(materialJson->GetStringValue("name", &name))
        {
            material.name = Str(ctx->arena, name);
        }

        JsonObject propertyJson;

        // PBR Metal/roughness
        if(materialJson->GetObjectValue("pbrMetallicRoughness", &propertyJson))
        {
            propertyJson.GetNumberValue("metallicFactor", &material.fMetallic);
            propertyJson.GetNumberValue("roughnessFactor", &material.fRoughness);
            JsonArray baseColorFactor;
            if(propertyJson.GetArrayValue("baseColorFactor", &baseColorFactor))
            {
                material.fBaseColor.r = baseColorFactor[0].AsNumber();
                material.fBaseColor.g = baseColorFactor[1].AsNumber();
                material.fBaseColor.b = baseColorFactor[2].AsNumber();
                material.fBaseColor.a = baseColorFactor[3].AsNumber();
            }
            JsonObject propertyTexture;
            if(propertyJson.GetObjectValue("baseColorTexture", &propertyTexture))
            {
                material.texBaseColor = gltfTextures[propertyTexture.GetNumberValue("index")];
                propertyTexture.GetNumberValue("texCoord", &material.texBaseColor.texCoordIndex);
            }
            if(propertyJson.GetObjectValue("metallicRoughnessTexture", &propertyTexture))
            {
                material.texMetallicRoughness = gltfTextures[propertyTexture.GetNumberValue("index")];
                propertyTexture.GetNumberValue("texCoord", &material.texMetallicRoughness.texCoordIndex);
            }
        }

        // Normal
        if(materialJson->GetObjectValue("normalTexture", &propertyJson))
        {
            propertyJson.GetNumberValue("scale", &material.fNormalScale);
            material.texNormal = gltfTextures[propertyJson.GetNumberValue("index")];
            propertyJson.GetNumberValue("texCoord", &material.texNormal.texCoordIndex);
        }

        // Occlusion
        if(materialJson->GetObjectValue("occlusionTexture", &propertyJson))
        {
            propertyJson.GetNumberValue("strength", &material.fOcclusionStrength);
            material.texOcclusion = gltfTextures[propertyJson.GetNumberValue("index")];
            propertyJson.GetNumberValue("texCoord", &material.texOcclusion.texCoordIndex);
        }

        // Emissive
        if(materialJson->GetObjectValue("emissiveTexture", &propertyJson))
        {
            material.texEmissive = gltfTextures[propertyJson.GetNumberValue("index")];
            propertyJson.GetNumberValue("texCoord", &material.texEmissive.texCoordIndex);
        }
        JsonArray emissiveFactorJson;
        if(materialJson->GetArrayValue("emissiveFactor", &emissiveFactorJson))
        {
            material.fEmissive.r = emissiveFactorJson[0].AsNumber();
            material.fEmissive.g = emissiveFactorJson[1].AsNumber();
            material.fEmissive.b = emissiveFactorJson[2].AsNumber();
        }

        // TODO(caio): I believe there are more possible properties here, like alpha and stuff.
        // Support them later if needed.

        result.Push(material);
    }

    return result;
}

#define TY_GLTF_MAX_ACCESSOR_RANGES 1024

//TODO(caio): There's a couple of duplicate functions here for attributes and indices, but that have different return types
// which could possibly be refactored further using macros.
Range LoadModelGLTF_LoadAccessorAttributeToTempArray(JsonObject* gltfJson, u32 accessorIndex, byte** buffers, DArray<f32>& tempArray)
{
    ASSERT(accessorIndex < TY_GLTF_MAX_ACCESSOR_RANGES);
    Range result = {};

    JsonArray& accessorsJson = *gltfJson->GetArrayValue("accessors");
    JsonArray& bufferViewsJson = *gltfJson->GetArrayValue("bufferViews");

    ASSERT(accessorIndex < accessorsJson.count);
    JsonObject* accessorJson = accessorsJson[accessorIndex].AsObject();
    JsonObject* bufferViewJson = bufferViewsJson[accessorJson->GetNumberValue("bufferView")].AsObject();

    byte* bufferCursor = buffers[(u32)bufferViewJson->GetNumberValue("buffer")];
    u32 offset = 0;
    if(bufferViewJson->GetNumberValue("byteOffset", &offset))
    {
        bufferCursor += offset;
    }
    if(accessorJson->GetNumberValue("byteOffset", &offset))
    {
        bufferCursor += offset;
    }

    u64 bufferViewStride = 0;
    bufferViewJson->GetNumberValue("byteStride", &bufferViewStride);

    u32 accessorElementComponents = 0;
    String accessorType = accessorJson->GetStringValue("type");
    if(accessorType == "SCALAR")    accessorElementComponents = 1;
    else if(accessorType == "VEC2") accessorElementComponents = 2;
    else if(accessorType == "VEC3") accessorElementComponents = 3;
    else if(accessorType == "VEC4") accessorElementComponents = 4;
    ASSERT(accessorElementComponents != 0);

    u32 accessorElementCount = 0;
    accessorJson->GetNumberValue("count", &accessorElementCount);
    ASSERT(accessorElementCount != 0);

    result.start = tempArray.count;
    for(i32 i = 0; i < accessorElementCount; i++)
    {
        byte* bufferCursorElementStart = bufferCursor;
        for(i32 j = 0; j < accessorElementComponents; j++)
        {
            f32 accessorElement = *((f32*)bufferCursor);
            tempArray.Push(accessorElement);
            bufferCursor += sizeof(f32);
        }

        if(bufferViewStride)
        {
            bufferCursor = bufferCursorElementStart + bufferViewStride;
        }
    }
    result.len = accessorElementCount * accessorElementComponents;

    return result;
}

Range LoadModelGLTF_LoadAccessorIndicesToTempArray(JsonObject* gltfJson, u32 accessorIndex, byte** buffers, DArray<u32>& tempArray)
{
    ASSERT(accessorIndex < TY_GLTF_MAX_ACCESSOR_RANGES);
    Range result = {};

    JsonArray& accessorsJson = *gltfJson->GetArrayValue("accessors");
    JsonArray& bufferViewsJson = *gltfJson->GetArrayValue("bufferViews");

    ASSERT(accessorIndex < accessorsJson.count);
    JsonObject* accessorJson = accessorsJson[accessorIndex].AsObject();
    JsonObject* bufferViewJson = bufferViewsJson[accessorJson->GetNumberValue("bufferView")].AsObject();

    byte* bufferCursor = buffers[(u32)bufferViewJson->GetNumberValue("buffer")];
    u32 offset = 0;
    if(bufferViewJson->GetNumberValue("byteOffset", &offset))
    {
        bufferCursor += offset;
    }
    if(accessorJson->GetNumberValue("byteOffset", &offset))
    {
        bufferCursor += offset;
    }

    u64 bufferViewStride = 0;
    bufferViewJson->GetNumberValue("byteStride", &bufferViewStride);

    u32 accessorElementCount = 0;
    accessorJson->GetNumberValue("count", &accessorElementCount);
    ASSERT(accessorElementCount != 0);

    result.start = tempArray.count;
    for(i32 i = 0; i < accessorElementCount; i++)
    {
        u32 accessorElement = *((u32*)bufferCursor);
        tempArray.Push(accessorElement);
        bufferCursor += sizeof(u32);
        bufferCursor += bufferViewStride;
    }
    result.len = accessorElementCount;

    return result;
}

DArray<f32> LoadModelGLTF_LoadAttributeFromPrimitiveAccessors(Context* ctx, JsonObject* gltfJson, String attrName, Range* accessorRanges, byte** buffers)
{
    DArray<f32> result = MakeDArray<f32>(ctx->tempArena);
    JsonArray& meshesJson = *gltfJson->GetArrayValue("meshes");
    for(i32 i = 0; i < meshesJson.count; i++)
    {
        JsonObject* meshJson = meshesJson[i].AsObject();
        JsonArray primitivesJson = *meshJson->GetArrayValue("primitives");
        for(i32 j = 0; j < primitivesJson.count; j++)
        {
            JsonObject* primitiveJson = primitivesJson[j].AsObject();
            JsonObject* attributeJson = primitiveJson->GetObjectValue("attributes");
            u32 accessorIndex;
            if(attributeJson->GetNumberValue(attrName, &accessorIndex))
            {
                // Primitive contains accessor for attribute. 
                // In this case, get the range and load it if not already loaded.
                if(!accessorRanges[accessorIndex].IsValid())
                {
                    // Load accessor to temp array and cache the accessor range.
                    Range accessorRange = LoadModelGLTF_LoadAccessorAttributeToTempArray(gltfJson, accessorIndex, buffers, result);
                    accessorRanges[accessorIndex] = accessorRange;
                }
            }
        }
    }

    return result;
}

DArray<u32> LoadModelGLTF_LoadIndicesFromPrimitiveAccessors(Context* ctx, JsonObject* gltfJson, Range* accessorRanges, byte** buffers)
{
    DArray<u32> result = MakeDArray<u32>(ctx->tempArena);
    JsonArray& meshesJson = *gltfJson->GetArrayValue("meshes");
    for(i32 i = 0; i < meshesJson.count; i++)
    {
        JsonObject* meshJson = meshesJson[i].AsObject();
        JsonArray primitivesJson = *meshJson->GetArrayValue("primitives");
        for(i32 j = 0; j < primitivesJson.count; j++)
        {
            JsonObject* primitiveJson = primitivesJson[j].AsObject();
            u32 accessorIndex;
            if(primitiveJson->GetNumberValue("indices", &accessorIndex))
            {
                // Primitive contains accessor for attribute. 
                // In this case, get the range and load it if not already loaded.
                if(!accessorRanges[accessorIndex].IsValid())
                {
                    // Load accessor to temp array and cache the accessor range.
                    Range accessorRange = LoadModelGLTF_LoadAccessorIndicesToTempArray(gltfJson, accessorIndex, buffers, result);
                    accessorRanges[accessorIndex] = accessorRange;
                }
            }
        }
    }

    return result;
}

SArray<f32> LoadModelGLTF_LoadAttributeArray(Context* ctx, JsonObject* gltfJson, String attrName, Range* accessorRanges, byte** buffers)
{
    u64 tempArenaOffset = ctx->tempArena->offset;
    DArray<f32> tempArray = LoadModelGLTF_LoadAttributeFromPrimitiveAccessors(ctx, gltfJson, attrName, accessorRanges, buffers);
    if(!tempArray.count)
    {
        return {};
    }

    SArray<f32> result = MakeSArray<f32>(ctx->arena, tempArray.count);
    for(i32 i = 0; i < tempArray.count; i++)
    {
        result.Push(tempArray[i]);
    }
    mem::ArenaFallback(ctx->tempArena, tempArenaOffset);
    return result;
}

SArray<u32> LoadModelGLTF_LoadIndexArray(Context* ctx, JsonObject* gltfJson, Range* accessorRanges, byte** buffers)
{
    u64 tempArenaOffset = ctx->tempArena->offset;
    DArray<u32> tempArray = LoadModelGLTF_LoadIndicesFromPrimitiveAccessors(ctx, gltfJson, accessorRanges, buffers);
    SArray<u32> result = MakeSArray<u32>(ctx->arena, tempArray.count);
    for(i32 i = 0; i < tempArray.count; i++)
    {
        result.Push(tempArray[i]);
    }
    mem::ArenaFallback(ctx->tempArena, tempArenaOffset);
    return result;
}

SArray<GltfMesh> LoadModelGLTF_LoadMeshes(Context* ctx, JsonObject* gltfJson, Range* accessorRanges)
{
    JsonArray& meshesJson = *gltfJson->GetArrayValue("meshes");
    SArray<GltfMesh> result = MakeSArray<GltfMesh>(ctx->arena, meshesJson.count);

    for(i32 i = 0; i < meshesJson.count; i++)
    {
        JsonObject* meshJson = meshesJson[i].AsObject();
        GltfMesh mesh = {};

        String name;
        if(meshJson->GetStringValue("name", &name))
        {
            mesh.name = Str(ctx->arena, name);
        }

        JsonArray primitivesJson = *meshJson->GetArrayValue("primitives");
        mesh.primitives = MakeSArray<GltfPrimitive>(ctx->arena, primitivesJson.count, primitivesJson.count, {});
        for(i32 j = 0; j < primitivesJson.count; j++)
        {
            JsonObject* primitiveJson = primitivesJson[j].AsObject();
            mesh.primitives[j].hMaterial = primitiveJson->GetNumberValue("material");
            mesh.primitives[j].rangeIndices = accessorRanges[(u32)primitiveJson->GetNumberValue("indices")];

            JsonObject* attributesJson = primitiveJson->GetObjectValue("attributes");
            u32 attributeAccessorIndex = 0;
            if(attributesJson->GetNumberValue("POSITION", &attributeAccessorIndex))
            {
                mesh.primitives[j].rangePositions = accessorRanges[attributeAccessorIndex];
            }
            if(attributesJson->GetNumberValue("NORMAL", &attributeAccessorIndex))
            {
                mesh.primitives[j].rangeNormals = accessorRanges[attributeAccessorIndex];
            }
            if(attributesJson->GetNumberValue("TEXCOORD_0", &attributeAccessorIndex))
            {
                mesh.primitives[j].rangeTexCoords0 = accessorRanges[attributeAccessorIndex];
            }
            if(attributesJson->GetNumberValue("TEXCOORD_1", &attributeAccessorIndex))
            {
                mesh.primitives[j].rangeTexCoords1 = accessorRanges[attributeAccessorIndex];
            }
            if(attributesJson->GetNumberValue("TEXCOORD_2", &attributeAccessorIndex))
            {
                mesh.primitives[j].rangeTexCoords2 = accessorRanges[attributeAccessorIndex];
            }
        }

        result.Push(mesh);
    }

    return result;
}

SArray<GltfNode> LoadModelGLTF_LoadNodes(Context* ctx, JsonObject* gltfJson)
{
    JsonArray& nodesJson = *gltfJson->GetArrayValue("nodes");
    SArray<GltfNode> result = MakeSArray<GltfNode>(ctx->arena, nodesJson.count + 1);    // +1 to include an empty root node, which acts as the start of the model scene hierarchy.

    for(i32 i = 0; i < nodesJson.count; i++)
    {
        JsonObject* nodeJson = nodesJson[i].AsObject();
        GltfNode node = {};

        u32 mesh;
        if(nodeJson->GetNumberValue("mesh", &mesh))
        {
            node.hMesh = mesh;
        }

        JsonArray matrixJson;
        m4f nodeTransform = math::Identity();
        if(nodeJson->GetArrayValue("matrix", &matrixJson))
        {
            nodeTransform.m00 = matrixJson[0].AsNumber();
            nodeTransform.m01 = matrixJson[1].AsNumber();
            nodeTransform.m02 = matrixJson[2].AsNumber();
            nodeTransform.m03 = matrixJson[3].AsNumber();
            nodeTransform.m10 = matrixJson[4].AsNumber();
            nodeTransform.m11 = matrixJson[5].AsNumber();
            nodeTransform.m12 = matrixJson[6].AsNumber();
            nodeTransform.m13 = matrixJson[7].AsNumber();
            nodeTransform.m20 = matrixJson[8].AsNumber();
            nodeTransform.m21 = matrixJson[9].AsNumber();
            nodeTransform.m22 = matrixJson[10].AsNumber();
            nodeTransform.m23 = matrixJson[11].AsNumber();
            nodeTransform.m30 = matrixJson[12].AsNumber();
            nodeTransform.m31 = matrixJson[13].AsNumber();
            nodeTransform.m32 = matrixJson[14].AsNumber();
            nodeTransform.m33 = matrixJson[15].AsNumber();
        }
        else
        {
            m4f translation = math::Identity();
            m4f rotation = math::Identity();
            m4f scale = math::Identity();

            JsonArray transformPropertyJson;
            if(nodeJson->GetArrayValue("translation", &transformPropertyJson))
            {
                translation = math::TranslationMatrix({
                        (f32)transformPropertyJson[0].AsNumber(),
                        (f32)transformPropertyJson[1].AsNumber(),
                        (f32)transformPropertyJson[2].AsNumber()});
            }
            if(nodeJson->GetArrayValue("rotation", &transformPropertyJson))
            {
                ASSERT(0);  // TODO(caio): To add support for rotation in GLTF I need support for quaternions in core math first.
            }
            if(nodeJson->GetArrayValue("scale", &transformPropertyJson))
            {
                scale = math::ScaleMatrix({
                        (f32)transformPropertyJson[0].AsNumber(),
                        (f32)transformPropertyJson[1].AsNumber(),
                        (f32)transformPropertyJson[2].AsNumber()});
            }

            nodeTransform = translation * rotation * scale;
        }
        node.transform = nodeTransform;

        JsonArray childrenList;
        if(nodeJson->GetArrayValue("children", &childrenList))
        {
            node.hChildren = MakeSArray<handle>(ctx->arena, childrenList.count, childrenList.count, {});
            for(i32 j = 0; j < childrenList.count; j++)
            {
                node.hChildren[j] = (handle)childrenList[j].AsNumber();
            }
        }

        result.Push(node);
    }

    return result;
}

handle LoadModelGLTF_LoadSceneRoot(Context* ctx, JsonObject* gltfJson, SArray<GltfNode>& gltfNodes)
{
    u32 defaultSceneIndex = gltfJson->GetNumberValue("scene");
    JsonArray scenesJson = *gltfJson->GetArrayValue("scenes");
    JsonObject* sceneJson = scenesJson[defaultSceneIndex].AsObject();
    JsonArray sceneNodesJson = *sceneJson->GetArrayValue("nodes");

    GltfNode rootNode = {};
    rootNode.hChildren = MakeSArray<handle>(ctx->arena, sceneNodesJson.count, sceneNodesJson.count, HANDLE_INVALID);
    for(i32 i = 0; i < sceneNodesJson.count; i++)
    {
        rootNode.hChildren[i] = sceneNodesJson[i].AsNumber();
    }

    gltfNodes.Push(rootNode);
    return gltfNodes.count - 1; // Index pointing to root node
}

handle LoadModelGLTF(Context* ctx, String assetPath)
{
    if(IsLoaded(ctx, assetPath))
    {
        return ctx->loadedAssets[assetPath];
    }
    mem::ArenaClear(ctx->tempArena);

    // Load GLTF Json
    JsonObject* gltfJson = MakeJsonFromFile(ctx->tempArena, assetPath);

    GltfModel model = {};

    // Create and populate temporary buffers
    byte* buffers[TY_GLTF_MAX_BUFFERS];
    memset(buffers, NULL, TY_GLTF_MAX_BUFFERS * sizeof(byte*));
    LoadModelGLTF_LoadBuffers(ctx, gltfJson, assetPath, buffers);

    // Load GLTF textures and materials
    model.textures = LoadModelGLTF_LoadTextures(ctx, gltfJson, assetPath);
    model.materials = LoadModelGLTF_LoadMaterials(ctx, gltfJson, assetPath, model.textures);

    // Load vertex attributes (required before filling in mesh primitives).
    Range accessorRanges[TY_GLTF_MAX_ACCESSOR_RANGES];
    DEFAULT_ARRAY(accessorRanges, TY_GLTF_MAX_ACCESSOR_RANGES);
    // TODO(caio): This iterates through all meshes for every separate attribute. This is inefficient, but might not be a problem.
    model.vPositions   = LoadModelGLTF_LoadAttributeArray(ctx, gltfJson, "POSITION", accessorRanges, buffers);
    model.vNormals     = LoadModelGLTF_LoadAttributeArray(ctx, gltfJson, "NORMAL", accessorRanges, buffers);
    model.vTexCoords0  = LoadModelGLTF_LoadAttributeArray(ctx, gltfJson, "TEXCOORD_0", accessorRanges, buffers);
    model.vTexCoords1  = LoadModelGLTF_LoadAttributeArray(ctx, gltfJson, "TEXCOORD_1", accessorRanges, buffers);
    model.vTexCoords2  = LoadModelGLTF_LoadAttributeArray(ctx, gltfJson, "TEXCOORD_2", accessorRanges, buffers);
    model.indices      = LoadModelGLTF_LoadIndexArray(ctx, gltfJson, accessorRanges, buffers);

    // Load meshes and primitives using accessor ranges
    model.meshes = LoadModelGLTF_LoadMeshes(ctx, gltfJson, accessorRanges);

    // Load mesh nodes and scene hierarchy
    model.nodes = LoadModelGLTF_LoadNodes(ctx, gltfJson);
    model.hRootNode = LoadModelGLTF_LoadSceneRoot(ctx, gltfJson, model.nodes);

    model.path = Str(ctx->arena, assetPath);
    handle result = ctx->modelsGLTF.Push(model);
    ctx->loadedAssets.Insert(assetPath, result);
    return result;
}

}   // namespace asset
}   // namespace ty
