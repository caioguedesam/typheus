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

// enum GltfAttribute
// {
//     GLTF_ATTRIBUTE_POSITION,
//     GLTF_ATTRIBUTE_NORMAL,
//     GLTF_ATTRIBUTE_TEXCOORD0,
//     GLTF_ATTRIBUTE_TEXCOORD1,
//     GLTF_ATTRIBUTE_TEXCOORD2,
//     GLTF_ATTRIBUTE_COUNT,
// };
// 
// struct GltfAccessor
// {
//     enum Type : u8
//     {
//         SCALAR,
//         VEC2,
//         VEC3,
//         VEC4,
//     };
// 
//     i32 bufferView = -1;
//     u64 offset = 0;
//     Type type;
//     u32 componentType;
//     u64 count = 0;
// };
// 
// struct GltfBufferView
// {
//     i32 buffer = 0;
//     u64 offset = 0;
//     u64 len = 0;
//     u64 stride = 0;
// };
// 
// GltfBufferView LoadGltf_GetBufferView(List<JsonValue>& jsonBufferViewList, i32 index)
// {
//     ASSERT(index < jsonBufferViewList.count);
// 
//     JsonObject* jsonBufferView = jsonBufferViewList[index].AsObject();
//     GltfBufferView result = {};
// 
//     jsonBufferView->GetNumberValue("buffer", &result.buffer);
//     jsonBufferView->GetNumberValue("byteLength", &result.len);
//     jsonBufferView->GetNumberValue("byteOffset", &result.offset);
//     jsonBufferView->GetNumberValue("byteStride", &result.stride);
// 
//     return result;
// }
// 
// GltfAccessor LoadGltf_GetAccessor(List<JsonValue>& jsonAccessorList, i32 index)
// {
//     ASSERT(index < jsonAccessorList.count);
// 
//     JsonObject* jsonAccessor = jsonAccessorList[index].AsObject();
//     GltfAccessor result = {};
// 
//     jsonAccessor->GetNumberValue("bufferView", &result.bufferView);
//     jsonAccessor->GetNumberValue("byteOffset", &result.offset);
// 
//     JsonValue* valueType = jsonAccessor->GetValue("type");
//     if(valueType)
//     {
//         String value = valueType->AsString();
//         if(value == "SCALAR")
//         {
//             result.type = GltfAccessor::SCALAR;
//         }
//         else if (value == "VEC2")
//         {
//             result.type = GltfAccessor::VEC2;
//         }
//         else if (value == "VEC3")
//         {
//             result.type = GltfAccessor::VEC3;
//         }
//         else if (value == "VEC4")
//         {
//             result.type = GltfAccessor::VEC4;
//         }
//         else
//         {
//             ASSERT(0);  // ?
//         }
//     }
// 
//     jsonAccessor->GetNumberValue("componentType", &result.componentType);
//     jsonAccessor->GetNumberValue("count", &result.count);
// 
//     return result;
// }
// 
// // Pushes the accessor's contents onto the attribute list and returns the index
// // where the new contents start.
// Range LoadGltf_GetAccessorAttributes(u8** buffers, List<JsonValue>& bufferViewList, List<JsonValue>& accessorList, Range* loadedAccessors, i64 accIndex, GltfAttribute attr, List<f32>* out)
// {
// 
//     if(loadedAccessors[accIndex].IsValid())
//     {
//         return loadedAccessors[accIndex];
//     }
//     GltfAccessor accessor = LoadGltf_GetAccessor(accessorList, accIndex);
// 
//     ASSERT(out);
//     GltfBufferView bufferView = LoadGltf_GetBufferView(bufferViewList, accessor.bufferView);
//     u8* accessorBuffer = buffers[bufferView.buffer];
//     u8* accessorBufferStart = accessorBuffer + bufferView.offset + accessor.offset;
// 
//     u32 elementCount = 1;
//     switch (attr)
//     {
//         case GLTF_ATTRIBUTE_POSITION:
//         case GLTF_ATTRIBUTE_NORMAL:
//         {
//             elementCount = 3;
//         } break;
//         case GLTF_ATTRIBUTE_TEXCOORD0:
//         case GLTF_ATTRIBUTE_TEXCOORD1:
//         case GLTF_ATTRIBUTE_TEXCOORD2:
//         {
//             elementCount = 2;
//         } break;
//         default: ASSERT(0);
//     }
//     
//     u64 firstAttrIndex = out->count;
//     u8* bufferCursor = accessorBufferStart;
//     for(i64 i = 0; i < accessor.count; i++)
//     {
//         u8* elementCursor = bufferCursor;
//         for(i64 j = 0; j < elementCount; j++)
//         {
//             f32 element = *(f32*)elementCursor;
//             out->Push(element);
//             elementCursor += sizeof(f32);
//         }
// 
//         if(bufferView.stride)
//         {
//             bufferCursor += bufferView.stride;
//         }
//         else
//         {
//             bufferCursor += elementCount * sizeof(f32);
//         }
//     }
//     
//     Range result = { .start = (i64)firstAttrIndex, .len = (i64)(accessor.count * elementCount) };
//     loadedAccessors[accIndex] = result;
//     return result;
// }
// 
// Range LoadGltf_GetAccessorIndices(u8** buffers, List<JsonValue>& bufferViewList, List<JsonValue>& accessorList, Range* loadedAccessors, i64 accIndex, List<u32>* out)
// {
//     if(loadedAccessors[accIndex].IsValid())
//     {
//         return loadedAccessors[accIndex];
//     }
//     GltfAccessor accessor = LoadGltf_GetAccessor(accessorList, accIndex);
// 
//     ASSERT(out);
// 
//     GltfBufferView bufferView = LoadGltf_GetBufferView(bufferViewList, accessor.bufferView);
//     u8* accessorBuffer = buffers[bufferView.buffer];
//     u8* accessorBufferStart = accessorBuffer + bufferView.offset + accessor.offset;
// 
//     u64 firstAttrIndex = out->count;
//     u8* cursor = accessorBufferStart;
//     for(i64 i = 0; i < accessor.count; i++)
//     {
//         u32 element = *(u32*)cursor;
//         out->Push(element);
// 
//         if(bufferView.stride)
//         {
//             cursor += bufferView.stride;
//         }
//         else
//         {
//             cursor += sizeof(u32);
//         }
//     }
// 
//     Range result = { .start = (i64)firstAttrIndex, .len = (i64)(accessor.count) };
//     loadedAccessors[accIndex] = result;
//     return result;
// }
// 
// void LoadGltf_LoadBuffers(JsonObject* jsonGltf, file::Path assetPath, u8** out)
// {
//     List<JsonValue>& gltfJsonBuffers = *jsonGltf->GetArrayValue("buffers");
//     ASSERT(gltfJsonBuffers.count <= TY_GLTF_MAX_BUFFERS);
//     for(i32 i = 0; i < gltfJsonBuffers.count; i++)
//     {
//         JsonObject* jsonBuffer = gltfJsonBuffers[i].AsObject();
//         SStr(bufferPathStr, MAX_PATH);
//         str::Append(bufferPathStr, assetPath.FileDir());
//         str::Append(bufferPathStr, jsonBuffer->GetStringValue("uri"));
//         out[i] = file::ReadFileToBuffer(file::MakePath(bufferPathStr));
//     }
// }
// 
// void LoadGltf_LoadImages(JsonObject* jsonGltf, List<JsonValue>& jsonImages, file::Path assetPath, GltfModel* model, Handle<Image>* out)
// {
//     for(i32 i = 0; i < jsonImages.count; i++)
//     {
//         JsonObject* jsonImage = jsonImages[i].AsObject();
//         SStr(imagePathStr, MAX_PATH);
//         str::Append(imagePathStr, assetPath.FileDir());
//         str::Append(imagePathStr, jsonImage->GetStringValue("uri"));
//         out[i] = LoadImageFile(file::MakePath(imagePathStr));
//         model->hImages.Push(out[i]);
//     }
// }
// 
// void LoadGltf_LoadSamplers(JsonObject* jsonGltf, List<JsonValue>& jsonSamplers, GltfModel* model, Handle<GltfSampler>* out)
// {
//     for(i32 i = 0; i < jsonSamplers.count; i++)
//     {
//         JsonObject* jsonSampler = jsonSamplers[i].AsObject();
// 
//         GltfSampler sampler = {};
//         sampler.minFilter = (u32)jsonSampler->GetNumberValue("minFilter");
//         sampler.magFilter = (u32)jsonSampler->GetNumberValue("magFilter");
//         sampler.wrapS = (u32)jsonSampler->GetNumberValue("wrapS");
//         sampler.wrapT = (u32)jsonSampler->GetNumberValue("wrapT");
//         out[i] = gltfSamplers.Insert(sampler);
//         model->hSamplers.Push(out[i]);
//     }
// }
// 
// void LoadGltf_LoadTextures(JsonObject* jsonGltf, List<JsonValue>& jsonTextures, Handle<Image>* images, Handle<GltfSampler>* samplers, GltfTexture* out)
// {
//     for(i32 i = 0; i < jsonTextures.count; i++)
//     {
//         JsonObject* jsonTexture = jsonTextures[i].AsObject();
// 
//         out[i] = {};
//         out[i].hImage = images[(u32)jsonTexture->GetNumberValue("source")];
//         out[i].hSampler = samplers[(u32)jsonTexture->GetNumberValue("sampler")];
//     }
// }
// 
// GltfMaterial LoadGltf_ParseMaterial(JsonObject* jsonMaterial, GltfTexture* textures)
// {
//     GltfMaterial result = {};
// 
//     String jsonMaterialName;
//     if(jsonMaterial->GetStringValue("name", &jsonMaterialName))
//     {
//         MStr(materialName, jsonMaterialName.len + 1);
//         str::Append(materialName, jsonMaterialName);
//         result.name = materialName;
//     }
// 
//     JsonObject jsonPBR;
//     if(jsonMaterial->GetObjectValue("pbrMetallicRoughness", &jsonPBR))
//     {
//         jsonPBR.GetNumberValue("metallicFactor", &result.fMetallic);
//         jsonPBR.GetNumberValue("roughnessFactor", &result.fRoughness);
// 
//         JsonValue* jsonValueBaseColorFactor = jsonPBR.GetValue("baseColorFactor");
//         if(jsonValueBaseColorFactor)
//         {
//             List<JsonValue>& jsonColor = *jsonValueBaseColorFactor->AsArray();
//             result.fBaseColor.x = jsonColor[0].AsNumber();
//             result.fBaseColor.y = jsonColor[1].AsNumber();
//             result.fBaseColor.z = jsonColor[2].AsNumber();
//             result.fBaseColor.w = jsonColor[3].AsNumber();
//         }
// 
//         JsonObject jsonBaseColorTexture;
//         if(jsonPBR.GetObjectValue("baseColorTexture", &jsonBaseColorTexture))
//         {
//             result.texBaseColor = textures[(u32)jsonBaseColorTexture.GetNumberValue("index")];
//             jsonBaseColorTexture.GetNumberValue("texCoord", &result.texBaseColor.texCoordIndex);
//         }
//     }
//     
//     JsonObject jsonNormal;
//     if(jsonMaterial->GetObjectValue("normalTexture", &jsonNormal))
//     {
//         jsonNormal.GetNumberValue("scale", &result.fNormalScale);
//         result.texNormal = textures[(u32)jsonNormal.GetNumberValue("index")];
//         jsonNormal.GetNumberValue("texCoord", &result.texNormal.texCoordIndex);
//     }
// 
//     JsonObject jsonOcclusion;
//     if(jsonMaterial->GetObjectValue("occlusionTexture", &jsonNormal))
//     {
//         jsonOcclusion.GetNumberValue("strength", &result.fOcclusionStrength);
//         result.texOcclusion = textures[(u32)jsonOcclusion.GetNumberValue("index")];
//         jsonOcclusion.GetNumberValue("texCoord", &result.texOcclusion.texCoordIndex);
//     }
// 
//     JsonObject jsonEmissive;
//     if(jsonMaterial->GetObjectValue("emissiveTexture", &jsonNormal))
//     {
//         result.texEmissive = textures[(u32)jsonEmissive.GetNumberValue("index")];
//         jsonEmissive.GetNumberValue("texCoord", &result.texEmissive.texCoordIndex);
//     }
// 
//     JsonValue* jsonValueEmissiveFactor = jsonMaterial->GetValue("emissiveFactor");
//     if(jsonValueEmissiveFactor)
//     {
//         List<JsonValue>& jsonColor = *jsonValueEmissiveFactor->AsArray();
//         result.fEmissive.x = jsonColor[0].AsNumber();
//         result.fEmissive.y = jsonColor[1].AsNumber();
//         result.fEmissive.z = jsonColor[2].AsNumber();
//     }
// 
//     return result;
// }
// 
// GltfPrimitive LoadGltf_ParseMeshPrimitive(JsonObject* jsonPrimitive, 
//         GltfModel* model,
//         u8** buffers,
//         List<JsonValue>& jsonBufferViewList,
//         List<JsonValue>& jsonAccessorList,
//         Range* loadedAccessors,
//         Handle<GltfMaterial>* materials)
// {
//     GltfPrimitive result = {};
// 
//     i64 primitiveMaterial = 0;
//     if(jsonPrimitive->GetNumberValue("material", &primitiveMaterial))
//     {
//         result.hMaterial = materials[primitiveMaterial];
//     }
// 
//     JsonObject* jsonAttributes = jsonPrimitive->GetObjectValue("attributes");
//     i64 accIndex = 0;
//     if(jsonAttributes->GetNumberValue("POSITION", &accIndex))
//     {
//         result.rangePositions = LoadGltf_GetAccessorAttributes(buffers, jsonBufferViewList, jsonAccessorList, loadedAccessors, accIndex, GLTF_ATTRIBUTE_POSITION, &model->vPositions);
//     }
//     if(jsonAttributes->GetNumberValue("NORMAL", &accIndex))
//     {
//         result.rangeNormals = LoadGltf_GetAccessorAttributes(buffers, jsonBufferViewList, jsonAccessorList, loadedAccessors, accIndex, GLTF_ATTRIBUTE_NORMAL, &model->vNormals);
//     }
//     if(jsonAttributes->GetNumberValue("TEXCOORD_0", &accIndex))
//     {
//         result.rangeTexCoords0 = LoadGltf_GetAccessorAttributes(buffers, jsonBufferViewList, jsonAccessorList, loadedAccessors, accIndex, GLTF_ATTRIBUTE_TEXCOORD0, &model->vTexCoords0);
//     }
//     if(jsonAttributes->GetNumberValue("TEXCOORD_1", &accIndex))
//     {
//         result.rangeTexCoords1 = LoadGltf_GetAccessorAttributes(buffers, jsonBufferViewList, jsonAccessorList, loadedAccessors, accIndex, GLTF_ATTRIBUTE_TEXCOORD1, &model->vTexCoords1);
//     }
//     if(jsonAttributes->GetNumberValue("TEXCOORD_2", &accIndex))
//     {
//         result.rangeTexCoords2 = LoadGltf_GetAccessorAttributes(buffers, jsonBufferViewList, jsonAccessorList, loadedAccessors, accIndex, GLTF_ATTRIBUTE_TEXCOORD2, &model->vTexCoords2);
//     }
// 
//     jsonPrimitive->GetNumberValue("indices", &accIndex);
//     result.rangeIndices = LoadGltf_GetAccessorIndices(buffers, jsonBufferViewList, jsonAccessorList, loadedAccessors, accIndex, &model->indices);
// 
//     return result;
// }
// 
// 
// GltfMesh LoadGltf_ParseMesh(JsonObject* jsonMesh, 
//         GltfModel* model,
//         u8** buffers,
//         List<JsonValue>& jsonBufferViewList,
//         List<JsonValue>& jsonAccessorList,
//         Range* loadedAccessors,
//         Handle<GltfMaterial>* materials)
// {
//     GltfMesh result = {};
// 
//     JsonValue* jsonValueName = jsonMesh->GetValue("name");
//     if(jsonValueName)
//     {
//         String jsonMeshName = jsonValueName->AsString();
//         MStr(meshName, jsonMeshName.len + 1);
//         str::Append(meshName, jsonMeshName);
//         result.name = meshName;
//     }
// 
//     result.primitives = MakeList<GltfPrimitive>();
//     List<JsonValue>& jsonMeshPrimitives = *jsonMesh->GetArrayValue("primitives");
//     for(i32 i = 0; i < jsonMeshPrimitives.count; i++)
//     {
//         JsonObject* jsonPrimitive = jsonMeshPrimitives[i].AsObject();
//         GltfPrimitive primitive = LoadGltf_ParseMeshPrimitive(jsonPrimitive, model, buffers, jsonBufferViewList, jsonAccessorList, loadedAccessors, materials);
//         result.primitives.Push(primitive);
//     }
// 
//     return result;
// }
// 
// GltfNode LoadGltf_ParseNodeContents(JsonObject* jsonNode,
//         Handle<GltfMesh>* meshes)
// {
//     GltfNode result = {};
//     
//     i64 meshIndex;
//     if(jsonNode->GetNumberValue("mesh", &meshIndex))
//     {
//         result.hMesh = meshes[meshIndex];
//     }
// 
//     JsonValue* jsonMatrixValue = jsonNode->GetValue("matrix");
//     if(jsonMatrixValue)
//     {
//         List<JsonValue>& jsonMatrix = *jsonMatrixValue->AsArray();
//         ASSERT(jsonMatrix.count == 16);
//         result.transform.m00 = jsonMatrix[0].AsNumber();
//         result.transform.m01 = jsonMatrix[1].AsNumber();
//         result.transform.m02 = jsonMatrix[2].AsNumber();
//         result.transform.m03 = jsonMatrix[3].AsNumber();
// 
//         result.transform.m10 = jsonMatrix[4].AsNumber();
//         result.transform.m11 = jsonMatrix[5].AsNumber();
//         result.transform.m12 = jsonMatrix[6].AsNumber();
//         result.transform.m13 = jsonMatrix[7].AsNumber();
// 
//         result.transform.m20 = jsonMatrix[8].AsNumber();
//         result.transform.m21 = jsonMatrix[9].AsNumber();
//         result.transform.m22 = jsonMatrix[10].AsNumber();
//         result.transform.m23 = jsonMatrix[11].AsNumber();
// 
//         result.transform.m30 = jsonMatrix[12].AsNumber();
//         result.transform.m31 = jsonMatrix[13].AsNumber();
//         result.transform.m32 = jsonMatrix[14].AsNumber();
//         result.transform.m33 = jsonMatrix[15].AsNumber();
//     }
// 
//     //TODO(caio): Add support for vector transform along with matrix transform
//     ASSERT(     !jsonNode->GetValue("rotation")
//             &&  !jsonNode->GetValue("transform")
//             &&  !jsonNode->GetValue("scale"));
// 
//     return result;
// }
// 
// void LoadGltf_ParseNodeChildren(JsonObject* jsonNode,
//         Handle<GltfNode> hNode,
//         Handle<GltfNode>* nodes)
// {
//     JsonValue* jsonChildrenListValue = jsonNode->GetValue("children");
//     if(!jsonChildrenListValue) return;
// 
//     GltfNode& node = gltfNodes[hNode];
//     List<JsonValue>& jsonChildren = *jsonChildrenListValue->AsArray();
//     node.hChildren = MakeList<Handle<GltfNode>>(jsonChildren.count);
//     for(i64 i = 0; i < jsonChildren.count; i++)
//     {
//         i64 childIndex = jsonChildren[i].AsNumber();
//         node.hChildren.Push(nodes[childIndex]);
//     }
// }


// Handle<GltfModel> LoadGltfModel(file::Path assetPath)
// {
//     if(IsLoaded(assetPath)) return Handle<GltfModel>(loadedAssets[assetPath.str]);
// 
//     mem::SetContext(&assetHeap);
// 
//     // Reading .gltf file
//     JsonObject* pGltfJson = MakeJson(assetPath);
//     JsonObject& gltfJson = *pGltfJson;
// 
//     // Parsing gltf file
//     GltfModel model = {};
//     model.indices       = MakeList<u32>();
//     model.vPositions    = MakeList<f32>(); 
//     model.vNormals      = MakeList<f32>(); 
//     model.vTexCoords0   = MakeList<f32>();
//     model.vTexCoords1   = MakeList<f32>();
//     model.vTexCoords2   = MakeList<f32>();
//     model.hImages       = MakeList<Handle<Image>>();
//     model.hSamplers     = MakeList<Handle<GltfSampler>>();
//     model.hMaterials    = MakeList<Handle<GltfMaterial>>();
// 
//     // Buffers
//     u8* buffers[TY_GLTF_MAX_BUFFERS];
//     for(i32 i = 0; i < TY_GLTF_MAX_BUFFERS; i++)
//     {
//         buffers[i] = NULL;
//     }
//     LoadGltf_LoadBuffers(&gltfJson, assetPath, buffers);
// 
//     // Images
//     List<JsonValue>& gltfJsonImages = *gltfJson.GetArrayValue("images");
//     Handle<Image> gltfImages[gltfJsonImages.count];
//     LoadGltf_LoadImages(&gltfJson, gltfJsonImages, assetPath, &model, gltfImages);
// 
//     // Samplers
//     List<JsonValue>& gltfJsonSamplers = *gltfJson.GetArrayValue("samplers");
//     Handle<GltfSampler> samplers[gltfJsonSamplers.count];
//     LoadGltf_LoadSamplers(&gltfJson, gltfJsonSamplers, &model, samplers);
// 
//     // Textures (image + sampler)
//     List<JsonValue>& gltfJsonTextures = *gltfJson.GetArrayValue("textures");
//     GltfTexture textures[gltfJsonTextures.count];
//     LoadGltf_LoadTextures(&gltfJson, gltfJsonTextures, gltfImages, samplers, textures);
// 
//     // Materials
//     List<JsonValue>& gltfJsonMaterials = *gltfJson.GetArrayValue("materials");
//     Handle<GltfMaterial> materials[gltfJsonMaterials.count];
//     for(i32 i = 0; i < gltfJsonMaterials.count; i++)
//     {
//         JsonObject* jsonMaterial = gltfJsonMaterials[i].AsObject();
//         GltfMaterial material = LoadGltf_ParseMaterial(jsonMaterial, textures);
//         materials[i] = gltfMaterials.Insert(material);
//         model.hMaterials.Push(materials[i]);
//     }
// 
//     // Meshes
//     List<JsonValue>& gltfJsonBufferViews = *gltfJson.GetArrayValue("bufferViews");
//     List<JsonValue>& gltfJsonAccessors = *gltfJson.GetArrayValue("accessors");
//     List<JsonValue>& gltfJsonMeshes = *gltfJson.GetArrayValue("meshes");
//     Handle<GltfMesh> meshes[gltfJsonMeshes.count];
//     Range loadedAccessors[gltfJsonAccessors.count];
//     for(u64 i = 0; i < gltfJsonAccessors.count; i++)
//     {
//         loadedAccessors[i] = {};
//     }
// 
//     for(i32 i = 0; i < gltfJsonMeshes.count; i++)
//     {
//         JsonObject* jsonMesh = gltfJsonMeshes[i].AsObject();
//         GltfMesh mesh = LoadGltf_ParseMesh(jsonMesh, &model, buffers, gltfJsonBufferViews, gltfJsonAccessors, loadedAccessors, materials);
//         meshes[i] = gltfMeshes.Insert(mesh);
//     }
// 
//     // Nodes
//     List<JsonValue>& gltfJsonNodes = *gltfJson.GetArrayValue("nodes");
//     Handle<GltfNode> nodes[gltfJsonNodes.count];
//     for(i32 i = 0; i < gltfJsonNodes.count; i++)
//     {
//         JsonObject* jsonNode = gltfJsonNodes[i].AsObject();
//         GltfNode node = LoadGltf_ParseNodeContents(jsonNode, meshes);
//         nodes[i] = gltfNodes.Insert(node);
//     }
//     for(i32 i = 0; i < gltfJsonNodes.count; i++)
//     {
//         JsonObject* jsonNode = gltfJsonNodes[i].AsObject();
//         LoadGltf_ParseNodeChildren(jsonNode, nodes[i], nodes);
//     }
// 
//     // Scene
//     // Here a scene is considered as just an empty root node
//     List<JsonValue>& jsonScenes = *gltfJson.GetArrayValue("scenes");
//     i64 rootSceneIndex = -1;
//     if(!gltfJson.GetNumberValue("scene", &rootSceneIndex))
//     {
//         ASSERT(0);  // Only supports gltf with root scene set
//     }
//     JsonObject* jsonRootScene = jsonScenes[rootSceneIndex].AsObject();
//     List<JsonValue>& jsonRootSceneNodes = *jsonRootScene->GetArrayValue("nodes");
//     ASSERT(jsonRootSceneNodes.count);
// 
//     GltfNode rootNode = {};
//     rootNode.hChildren = MakeList<Handle<GltfNode>>(jsonRootSceneNodes.count);
//     for(i32 i = 0; i < jsonRootSceneNodes.count; i++)
//     {
//         i64 nodeIndex = jsonRootSceneNodes[i].AsNumber();
//         rootNode.hChildren.Push(nodes[nodeIndex]);
//     }
//     Handle<GltfNode> hRoot = gltfNodes.Insert(rootNode);
//     model.root = hRoot;
//     model.path = assetPath;
// 
//     Handle<GltfModel> result = gltfModels.Insert(model);
//     loadedAssets.Insert(assetPath.str, result.GetData());
// 
//     DestroyJson(pGltfJson);
//     for(i32 i = 0; i < TY_GLTF_MAX_BUFFERS; i++)
//     {
//         if(buffers[i])
//         {
//             mem::Free(buffers[i]);
//         }
//     }
// 
//     return result;
// }
// 
// void UnloadGltf_DestroySampler(Handle<GltfSampler> hSampler)
// {
//     ASSERT(hSampler.IsValid());
//     gltfSamplers.Remove(hSampler);
// }
// 
// void UnloadGltf_DestroyMaterial(Handle<GltfMaterial> hMaterial)
// {
//     GltfMaterial& material = gltfMaterials[hMaterial];
// 
//     if(IS_MUTABLE(material.name))
//     {
//         FreeMStr(&material.name);
//     }
// 
//     gltfMaterials.Remove(hMaterial);
// }
// 
// void UnloadGltf_DestroyMesh(Handle<GltfMesh> hMesh)
// {
//     GltfMesh& mesh = gltfMeshes[hMesh];
// 
//     if(IS_MUTABLE(mesh.name))
//     {
//         FreeMStr(&mesh.name);
//     }
//     DestroyList(&mesh.primitives);
// 
//     gltfMeshes.Remove(hMesh);
// }
// 
// void UnloadGltf_DestroyNode(Handle<GltfNode> hNode)
// {
//     ASSERT(hNode.IsValid());
//     GltfNode& node = gltfNodes[hNode];
// 
//     // Destroy all children
//     for(i32 i = 0; i < node.hChildren.count; i++)
//     {
//         UnloadGltf_DestroyNode(node.hChildren[i]);
//     }
// 
//     // Destroy this node's components
//     if(node.hChildren.count)
//     {
//         DestroyList(&node.hChildren);
//     }
//     if(node.hMesh.IsValid())
//     {
//         UnloadGltf_DestroyMesh(node.hMesh);
//     }
// 
//     gltfNodes.Remove(hNode);
// }
// 
// void UnloadGltfModel(Handle<GltfModel> hModel)
// {
//     //TODO(caio): Double check this for missing frees.
//     ASSERT(hModel.IsValid());
//     mem::SetContext(&assetHeap);
// 
//     GltfModel& model = gltfModels[hModel];
// 
//     UnloadGltf_DestroyNode(model.root);
// 
//     DestroyList(&model.indices);
//     DestroyList(&model.vPositions);
//     DestroyList(&model.vNormals);
//     DestroyList(&model.vTexCoords0);
//     DestroyList(&model.vTexCoords1);
//     DestroyList(&model.vTexCoords2);
// 
//     for(i32 i = 0; i < model.hImages.count; i++)
//     {
//         UnloadImage(model.hImages[i]);
//     }
//     DestroyList(&model.hImages);
//     for(i32 i = 0; i < model.hSamplers.count; i++)
//     {
//         UnloadGltf_DestroySampler(model.hSamplers[i]);
//     }
//     DestroyList(&model.hSamplers);
//     for(i32 i = 0; i < model.hMaterials.count; i++)
//     {
//         UnloadGltf_DestroyMaterial(model.hMaterials[i]);
//     }
//     DestroyList(&model.hMaterials);
// 
//     loadedAssets.Remove(model.path.CStr());
//     gltfModels.Remove(hModel);
// }
