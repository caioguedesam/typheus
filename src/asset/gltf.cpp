#include "../core/memory.hpp"
#include "../core/debug.hpp"
#include "../core/file.hpp"
#include "./json.hpp"
#include "./asset.hpp"
#include "src/core/ds.hpp"
#include "src/core/string.hpp"

namespace ty
{
namespace asset
{

enum GltfAttribute
{
    GLTF_ATTRIBUTE_POSITION,
    GLTF_ATTRIBUTE_NORMAL,
    GLTF_ATTRIBUTE_TEXCOORD0,
    GLTF_ATTRIBUTE_TEXCOORD1,
    GLTF_ATTRIBUTE_TEXCOORD2,
};


struct GltfAccessor
{
    enum Type : u8
    {
        SCALAR,
        VEC2,
        VEC3,
        VEC4,
    };

    i32 bufferView = -1;
    u64 offset = 0;
    Type type;
    u32 componentType;
    u64 count = 0;
};

struct GltfBufferView
{
    i32 buffer = 0;
    u64 offset = 0;
    u64 len = 0;
    u64 stride = 0;
};

GltfBufferView LoadGltf_GetBufferView(List<JsonValue>& jsonBufferViewList, i32 index)
{
    ASSERT(index < jsonBufferViewList.count);

    JsonObject* jsonBufferView = jsonBufferViewList[index].AsObject();
    GltfBufferView result = {};

    jsonBufferView->GetNumberValue("buffer", &result.buffer);
    jsonBufferView->GetNumberValue("byteLength", &result.len);
    jsonBufferView->GetNumberValue("byteOffset", &result.offset);
    jsonBufferView->GetNumberValue("byteStride", &result.stride);

    return result;
}

GltfAccessor LoadGltf_GetAccessor(List<JsonValue>& jsonAccessorList, i32 index)
{
    ASSERT(index < jsonAccessorList.count);

    JsonObject* jsonAccessor = jsonAccessorList[index].AsObject();
    GltfAccessor result = {};

    jsonAccessor->GetNumberValue("bufferView", &result.bufferView);
    jsonAccessor->GetNumberValue("byteOffset", &result.offset);

    JsonValue* valueType = jsonAccessor->GetValue("type");
    if(valueType)
    {
        String value = valueType->AsString();
        if(value == "SCALAR")
        {
            result.type = GltfAccessor::SCALAR;
        }
        else if (value == "VEC2")
        {
            result.type = GltfAccessor::VEC2;
        }
        else if (value == "VEC3")
        {
            result.type = GltfAccessor::VEC3;
        }
        else if (value == "VEC4")
        {
            result.type = GltfAccessor::VEC4;
        }
        else
        {
            ASSERT(0);  // ?
        }
    }

    jsonAccessor->GetNumberValue("componentType", &result.componentType);
    jsonAccessor->GetNumberValue("count", &result.count);

    return result;
}

// Pushes the accessor's contents onto the attribute list and returns the index
// where the new contents start.
Range LoadGltf_GetAccessorAttributes(u8** buffers, List<JsonValue>& bufferViewList, List<JsonValue>& accessorList, Range* loadedAccessors, i64 accIndex, GltfAttribute attr, List<f32>* out)
{

    if(loadedAccessors[accIndex].IsValid())
    {
        return loadedAccessors[accIndex];
    }
    GltfAccessor accessor = LoadGltf_GetAccessor(accessorList, accIndex);

    ASSERT(out);
    GltfBufferView bufferView = LoadGltf_GetBufferView(bufferViewList, accessor.bufferView);
    u8* accessorBuffer = buffers[bufferView.buffer];
    u8* accessorBufferStart = accessorBuffer + bufferView.offset + accessor.offset;

    u32 elementCount = 1;
    switch (attr)
    {
        case GLTF_ATTRIBUTE_POSITION:
        case GLTF_ATTRIBUTE_NORMAL:
        {
            elementCount = 3;
        } break;
        case GLTF_ATTRIBUTE_TEXCOORD0:
        case GLTF_ATTRIBUTE_TEXCOORD1:
        case GLTF_ATTRIBUTE_TEXCOORD2:
        {
            elementCount = 2;
        } break;
        default: ASSERT(0);
    }
    
    u64 firstAttrIndex = out->count;
    u8* bufferCursor = accessorBufferStart;
    for(i64 i = 0; i < accessor.count; i++)
    {
        u8* elementCursor = bufferCursor;
        for(i64 j = 0; j < elementCount; j++)
        {
            f32 element = *(f32*)elementCursor;
            out->Push(element);
            elementCursor += sizeof(f32);
        }

        if(bufferView.stride)
        {
            bufferCursor += bufferView.stride;
        }
        else
        {
            bufferCursor += elementCount * sizeof(f32);
        }
    }
    
    Range result = { .start = (i64)firstAttrIndex, .len = (i64)(accessor.count * elementCount) };
    loadedAccessors[accIndex] = result;
    return result;
}

Range LoadGltf_GetAccessorIndices(u8** buffers, List<JsonValue>& bufferViewList, List<JsonValue>& accessorList, Range* loadedAccessors, i64 accIndex, List<u32>* out)
{
    if(loadedAccessors[accIndex].IsValid())
    {
        return loadedAccessors[accIndex];
    }
    GltfAccessor accessor = LoadGltf_GetAccessor(accessorList, accIndex);

    ASSERT(out);

    GltfBufferView bufferView = LoadGltf_GetBufferView(bufferViewList, accessor.bufferView);
    u8* accessorBuffer = buffers[bufferView.buffer];
    u8* accessorBufferStart = accessorBuffer + bufferView.offset + accessor.offset;

    u64 firstAttrIndex = out->count;
    u8* cursor = accessorBufferStart;
    for(i64 i = 0; i < accessor.count; i++)
    {
        u32 element = *(u32*)cursor;
        out->Push(element);

        if(bufferView.stride)
        {
            cursor += bufferView.stride;
        }
        else
        {
            cursor += sizeof(u32);
        }
    }

    Range result = { .start = (i64)firstAttrIndex, .len = (i64)(accessor.count) };
    loadedAccessors[accIndex] = result;
    return result;
}

void LoadGltf_LoadBuffers(JsonObject* jsonGltf, file::Path assetPath, u8** out)
{
    List<JsonValue>& gltfJsonBuffers = *jsonGltf->GetArrayValue("buffers");
    ASSERT(gltfJsonBuffers.count <= TY_GLTF_MAX_BUFFERS);
    for(i32 i = 0; i < gltfJsonBuffers.count; i++)
    {
        JsonObject* jsonBuffer = gltfJsonBuffers[i].AsObject();
        SStr(bufferPathStr, MAX_PATH);
        str::Append(bufferPathStr, assetPath.FileDir());
        str::Append(bufferPathStr, jsonBuffer->GetStringValue("uri"));
        out[i] = file::ReadFileToBuffer(file::MakePath(bufferPathStr));
    }
}

void LoadGltf_LoadImages(JsonObject* jsonGltf, List<JsonValue>& jsonImages, file::Path assetPath, GltfModel* model, Handle<Image>* out)
{
    for(i32 i = 0; i < jsonImages.count; i++)
    {
        JsonObject* jsonImage = jsonImages[i].AsObject();
        SStr(imagePathStr, MAX_PATH);
        str::Append(imagePathStr, assetPath.FileDir());
        str::Append(imagePathStr, jsonImage->GetStringValue("uri"));
        out[i] = LoadImageFile(file::MakePath(imagePathStr));
        model->hImages.Push(out[i]);
    }
}

void LoadGltf_LoadSamplers(JsonObject* jsonGltf, List<JsonValue>& jsonSamplers, GltfModel* model, Handle<GltfSampler>* out)
{
    for(i32 i = 0; i < jsonSamplers.count; i++)
    {
        JsonObject* jsonSampler = jsonSamplers[i].AsObject();

        GltfSampler sampler = {};
        sampler.minFilter = (u32)jsonSampler->GetNumberValue("minFilter");
        sampler.magFilter = (u32)jsonSampler->GetNumberValue("magFilter");
        sampler.wrapS = (u32)jsonSampler->GetNumberValue("wrapS");
        sampler.wrapT = (u32)jsonSampler->GetNumberValue("wrapT");
        out[i] = gltfSamplers.Insert(sampler);
        model->hSamplers.Push(out[i]);
    }
}

void LoadGltf_LoadTextures(JsonObject* jsonGltf, List<JsonValue>& jsonTextures, Handle<Image>* images, Handle<GltfSampler>* samplers, GltfTexture* out)
{
    for(i32 i = 0; i < jsonTextures.count; i++)
    {
        JsonObject* jsonTexture = jsonTextures[i].AsObject();

        out[i] = {};
        out[i].hImage = images[(u32)jsonTexture->GetNumberValue("source")];
        out[i].hSampler = samplers[(u32)jsonTexture->GetNumberValue("sampler")];
    }
}

GltfMaterial LoadGltf_ParseMaterial(JsonObject* jsonMaterial, GltfTexture* textures)
{
    GltfMaterial result = {};

    String jsonMaterialName;
    if(jsonMaterial->GetStringValue("name", &jsonMaterialName))
    {
        MStr(materialName, jsonMaterialName.len + 1);
        str::Append(materialName, jsonMaterialName);
        result.name = materialName;
    }

    JsonObject jsonPBR;
    if(jsonMaterial->GetObjectValue("pbrMetallicRoughness", &jsonPBR))
    {
        jsonPBR.GetNumberValue("metallicFactor", &result.fMetallic);
        jsonPBR.GetNumberValue("roughnessFactor", &result.fRoughness);

        JsonValue* jsonValueBaseColorFactor = jsonPBR.GetValue("baseColorFactor");
        if(jsonValueBaseColorFactor)
        {
            List<JsonValue>& jsonColor = *jsonValueBaseColorFactor->AsArray();
            result.fBaseColor.x = jsonColor[0].AsNumber();
            result.fBaseColor.y = jsonColor[1].AsNumber();
            result.fBaseColor.z = jsonColor[2].AsNumber();
            result.fBaseColor.w = jsonColor[3].AsNumber();
        }

        JsonObject jsonBaseColorTexture;
        if(jsonPBR.GetObjectValue("baseColorTexture", &jsonBaseColorTexture))
        {
            result.texBaseColor = textures[(u32)jsonBaseColorTexture.GetNumberValue("index")];
            jsonBaseColorTexture.GetNumberValue("texCoord", &result.texBaseColor.texCoordIndex);
        }
    }
    
    JsonObject jsonNormal;
    if(jsonMaterial->GetObjectValue("normalTexture", &jsonNormal))
    {
        jsonNormal.GetNumberValue("scale", &result.fNormalScale);
        result.texNormal = textures[(u32)jsonNormal.GetNumberValue("index")];
        jsonNormal.GetNumberValue("texCoord", &result.texNormal.texCoordIndex);
    }

    JsonObject jsonOcclusion;
    if(jsonMaterial->GetObjectValue("occlusionTexture", &jsonNormal))
    {
        jsonOcclusion.GetNumberValue("strength", &result.fOcclusionStrength);
        result.texOcclusion = textures[(u32)jsonOcclusion.GetNumberValue("index")];
        jsonOcclusion.GetNumberValue("texCoord", &result.texOcclusion.texCoordIndex);
    }

    JsonObject jsonEmissive;
    if(jsonMaterial->GetObjectValue("emissiveTexture", &jsonNormal))
    {
        result.texEmissive = textures[(u32)jsonEmissive.GetNumberValue("index")];
        jsonEmissive.GetNumberValue("texCoord", &result.texEmissive.texCoordIndex);
    }

    JsonValue* jsonValueEmissiveFactor = jsonMaterial->GetValue("emissiveFactor");
    if(jsonValueEmissiveFactor)
    {
        List<JsonValue>& jsonColor = *jsonValueEmissiveFactor->AsArray();
        result.fEmissive.x = jsonColor[0].AsNumber();
        result.fEmissive.y = jsonColor[1].AsNumber();
        result.fEmissive.z = jsonColor[2].AsNumber();
    }

    return result;
}

GltfPrimitive LoadGltf_ParseMeshPrimitive(JsonObject* jsonPrimitive, 
        GltfModel* model,
        u8** buffers,
        List<JsonValue>& jsonBufferViewList,
        List<JsonValue>& jsonAccessorList,
        Range* loadedAccessors,
        Handle<GltfMaterial>* materials)
{
    GltfPrimitive result = {};

    i64 primitiveMaterial = 0;
    if(jsonPrimitive->GetNumberValue("material", &primitiveMaterial))
    {
        result.hMaterial = materials[primitiveMaterial];
    }

    JsonObject* jsonAttributes = jsonPrimitive->GetObjectValue("attributes");
    i64 accIndex = 0;
    if(jsonAttributes->GetNumberValue("POSITION", &accIndex))
    {
        result.rangePositions = LoadGltf_GetAccessorAttributes(buffers, jsonBufferViewList, jsonAccessorList, loadedAccessors, accIndex, GLTF_ATTRIBUTE_POSITION, &model->vPositions);
    }
    if(jsonAttributes->GetNumberValue("NORMAL", &accIndex))
    {
        result.rangeNormals = LoadGltf_GetAccessorAttributes(buffers, jsonBufferViewList, jsonAccessorList, loadedAccessors, accIndex, GLTF_ATTRIBUTE_NORMAL, &model->vNormals);
    }
    if(jsonAttributes->GetNumberValue("TEXCOORD_0", &accIndex))
    {
        result.rangeTexCoords0 = LoadGltf_GetAccessorAttributes(buffers, jsonBufferViewList, jsonAccessorList, loadedAccessors, accIndex, GLTF_ATTRIBUTE_TEXCOORD0, &model->vTexCoords0);
    }
    if(jsonAttributes->GetNumberValue("TEXCOORD_1", &accIndex))
    {
        result.rangeTexCoords1 = LoadGltf_GetAccessorAttributes(buffers, jsonBufferViewList, jsonAccessorList, loadedAccessors, accIndex, GLTF_ATTRIBUTE_TEXCOORD1, &model->vTexCoords1);
    }
    if(jsonAttributes->GetNumberValue("TEXCOORD_2", &accIndex))
    {
        result.rangeTexCoords2 = LoadGltf_GetAccessorAttributes(buffers, jsonBufferViewList, jsonAccessorList, loadedAccessors, accIndex, GLTF_ATTRIBUTE_TEXCOORD2, &model->vTexCoords2);
    }

    jsonPrimitive->GetNumberValue("indices", &accIndex);
    result.rangeIndices = LoadGltf_GetAccessorIndices(buffers, jsonBufferViewList, jsonAccessorList, loadedAccessors, accIndex, &model->indices);

    return result;
}


GltfMesh LoadGltf_ParseMesh(JsonObject* jsonMesh, 
        GltfModel* model,
        u8** buffers,
        List<JsonValue>& jsonBufferViewList,
        List<JsonValue>& jsonAccessorList,
        Range* loadedAccessors,
        Handle<GltfMaterial>* materials)
{
    GltfMesh result = {};

    JsonValue* jsonValueName = jsonMesh->GetValue("name");
    if(jsonValueName)
    {
        String jsonMeshName = jsonValueName->AsString();
        MStr(meshName, jsonMeshName.len + 1);
        str::Append(meshName, jsonMeshName);
        result.name = meshName;
    }

    result.primitives = MakeList<GltfPrimitive>();
    List<JsonValue>& jsonMeshPrimitives = *jsonMesh->GetArrayValue("primitives");
    for(i32 i = 0; i < jsonMeshPrimitives.count; i++)
    {
        JsonObject* jsonPrimitive = jsonMeshPrimitives[i].AsObject();
        GltfPrimitive primitive = LoadGltf_ParseMeshPrimitive(jsonPrimitive, model, buffers, jsonBufferViewList, jsonAccessorList, loadedAccessors, materials);
        result.primitives.Push(primitive);
    }

    return result;
}

GltfNode LoadGltf_ParseNodeContents(JsonObject* jsonNode,
        Handle<GltfMesh>* meshes)
{
    GltfNode result = {};
    
    i64 meshIndex;
    if(jsonNode->GetNumberValue("mesh", &meshIndex))
    {
        result.hMesh = meshes[meshIndex];
    }

    JsonValue* jsonMatrixValue = jsonNode->GetValue("matrix");
    if(jsonMatrixValue)
    {
        List<JsonValue>& jsonMatrix = *jsonMatrixValue->AsArray();
        ASSERT(jsonMatrix.count == 16);
        result.transform.m00 = jsonMatrix[0].AsNumber();
        result.transform.m01 = jsonMatrix[1].AsNumber();
        result.transform.m02 = jsonMatrix[2].AsNumber();
        result.transform.m03 = jsonMatrix[3].AsNumber();

        result.transform.m10 = jsonMatrix[4].AsNumber();
        result.transform.m11 = jsonMatrix[5].AsNumber();
        result.transform.m12 = jsonMatrix[6].AsNumber();
        result.transform.m13 = jsonMatrix[7].AsNumber();

        result.transform.m20 = jsonMatrix[8].AsNumber();
        result.transform.m21 = jsonMatrix[9].AsNumber();
        result.transform.m22 = jsonMatrix[10].AsNumber();
        result.transform.m23 = jsonMatrix[11].AsNumber();

        result.transform.m30 = jsonMatrix[12].AsNumber();
        result.transform.m31 = jsonMatrix[13].AsNumber();
        result.transform.m32 = jsonMatrix[14].AsNumber();
        result.transform.m33 = jsonMatrix[15].AsNumber();
    }

    //TODO(caio): Add support for vector transform along with matrix transform
    ASSERT(     !jsonNode->GetValue("rotation")
            &&  !jsonNode->GetValue("transform")
            &&  !jsonNode->GetValue("scale"));

    return result;
}

void LoadGltf_ParseNodeChildren(JsonObject* jsonNode,
        Handle<GltfNode> hNode,
        Handle<GltfNode>* nodes)
{
    JsonValue* jsonChildrenListValue = jsonNode->GetValue("children");
    if(!jsonChildrenListValue) return;

    GltfNode& node = gltfNodes[hNode];
    List<JsonValue>& jsonChildren = *jsonChildrenListValue->AsArray();
    node.hChildren = MakeList<Handle<GltfNode>>(jsonChildren.count);
    for(i64 i = 0; i < jsonChildren.count; i++)
    {
        i64 childIndex = jsonChildren[i].AsNumber();
        node.hChildren.Push(nodes[childIndex]);
    }
}

Handle<GltfModel> LoadGltfModel(file::Path assetPath)
{
    if(IsLoaded(assetPath)) return Handle<GltfModel>(loadedAssets[assetPath.str]);

    mem::SetContext(&assetHeap);

    // Reading .gltf file
    JsonObject* pGltfJson = MakeJson(assetPath);
    JsonObject& gltfJson = *pGltfJson;

    // Parsing gltf file
    GltfModel model = {};
    model.indices       = MakeList<u32>();
    model.vPositions    = MakeList<f32>(); 
    model.vNormals      = MakeList<f32>(); 
    model.vTexCoords0   = MakeList<f32>();
    model.vTexCoords1   = MakeList<f32>();
    model.vTexCoords2   = MakeList<f32>();
    model.hImages       = MakeList<Handle<Image>>();
    model.hSamplers     = MakeList<Handle<GltfSampler>>();
    model.hMaterials    = MakeList<Handle<GltfMaterial>>();

    // Buffers
    u8* buffers[TY_GLTF_MAX_BUFFERS];
    for(i32 i = 0; i < TY_GLTF_MAX_BUFFERS; i++)
    {
        buffers[i] = NULL;
    }
    LoadGltf_LoadBuffers(&gltfJson, assetPath, buffers);

    // Images
    List<JsonValue>& gltfJsonImages = *gltfJson.GetArrayValue("images");
    Handle<Image> gltfImages[gltfJsonImages.count];
    LoadGltf_LoadImages(&gltfJson, gltfJsonImages, assetPath, &model, gltfImages);

    // Samplers
    List<JsonValue>& gltfJsonSamplers = *gltfJson.GetArrayValue("samplers");
    Handle<GltfSampler> samplers[gltfJsonSamplers.count];
    LoadGltf_LoadSamplers(&gltfJson, gltfJsonSamplers, &model, samplers);

    // Textures (image + sampler)
    List<JsonValue>& gltfJsonTextures = *gltfJson.GetArrayValue("textures");
    GltfTexture textures[gltfJsonTextures.count];
    LoadGltf_LoadTextures(&gltfJson, gltfJsonTextures, gltfImages, samplers, textures);

    // Materials
    List<JsonValue>& gltfJsonMaterials = *gltfJson.GetArrayValue("materials");
    Handle<GltfMaterial> materials[gltfJsonMaterials.count];
    for(i32 i = 0; i < gltfJsonMaterials.count; i++)
    {
        JsonObject* jsonMaterial = gltfJsonMaterials[i].AsObject();
        GltfMaterial material = LoadGltf_ParseMaterial(jsonMaterial, textures);
        materials[i] = gltfMaterials.Insert(material);
        model.hMaterials.Push(materials[i]);
    }

    // Meshes
    List<JsonValue>& gltfJsonBufferViews = *gltfJson.GetArrayValue("bufferViews");
    List<JsonValue>& gltfJsonAccessors = *gltfJson.GetArrayValue("accessors");
    List<JsonValue>& gltfJsonMeshes = *gltfJson.GetArrayValue("meshes");
    Handle<GltfMesh> meshes[gltfJsonMeshes.count];
    Range loadedAccessors[gltfJsonAccessors.count];
    for(u64 i = 0; i < gltfJsonAccessors.count; i++)
    {
        loadedAccessors[i] = {};
    }

    for(i32 i = 0; i < gltfJsonMeshes.count; i++)
    {
        JsonObject* jsonMesh = gltfJsonMeshes[i].AsObject();
        GltfMesh mesh = LoadGltf_ParseMesh(jsonMesh, &model, buffers, gltfJsonBufferViews, gltfJsonAccessors, loadedAccessors, materials);
        meshes[i] = gltfMeshes.Insert(mesh);
    }

    // Nodes
    List<JsonValue>& gltfJsonNodes = *gltfJson.GetArrayValue("nodes");
    Handle<GltfNode> nodes[gltfJsonNodes.count];
    for(i32 i = 0; i < gltfJsonNodes.count; i++)
    {
        JsonObject* jsonNode = gltfJsonNodes[i].AsObject();
        GltfNode node = LoadGltf_ParseNodeContents(jsonNode, meshes);
        nodes[i] = gltfNodes.Insert(node);
    }
    for(i32 i = 0; i < gltfJsonNodes.count; i++)
    {
        JsonObject* jsonNode = gltfJsonNodes[i].AsObject();
        LoadGltf_ParseNodeChildren(jsonNode, nodes[i], nodes);
    }

    // Scene
    // Here a scene is considered as just an empty root node
    List<JsonValue>& jsonScenes = *gltfJson.GetArrayValue("scenes");
    i64 rootSceneIndex = -1;
    if(!gltfJson.GetNumberValue("scene", &rootSceneIndex))
    {
        ASSERT(0);  // Only supports gltf with root scene set
    }
    JsonObject* jsonRootScene = jsonScenes[rootSceneIndex].AsObject();
    List<JsonValue>& jsonRootSceneNodes = *jsonRootScene->GetArrayValue("nodes");
    ASSERT(jsonRootSceneNodes.count);

    GltfNode rootNode = {};
    rootNode.hChildren = MakeList<Handle<GltfNode>>(jsonRootSceneNodes.count);
    for(i32 i = 0; i < jsonRootSceneNodes.count; i++)
    {
        i64 nodeIndex = jsonRootSceneNodes[i].AsNumber();
        rootNode.hChildren.Push(nodes[nodeIndex]);
    }
    Handle<GltfNode> hRoot = gltfNodes.Insert(rootNode);
    model.root = hRoot;
    model.path = assetPath;

    Handle<GltfModel> result = gltfModels.Insert(model);
    loadedAssets.Insert(assetPath.str, result.GetData());

    DestroyJson(pGltfJson);
    for(i32 i = 0; i < TY_GLTF_MAX_BUFFERS; i++)
    {
        if(buffers[i])
        {
            mem::Free(buffers[i]);
        }
    }

    return result;
}

void UnloadGltf_DestroySampler(Handle<GltfSampler> hSampler)
{
    ASSERT(hSampler.IsValid());
    gltfSamplers.Remove(hSampler);
}

void UnloadGltf_DestroyMaterial(Handle<GltfMaterial> hMaterial)
{
    GltfMaterial& material = gltfMaterials[hMaterial];

    if(IS_MUTABLE(material.name))
    {
        FreeMStr(&material.name);
    }

    gltfMaterials.Remove(hMaterial);
}

void UnloadGltf_DestroyMesh(Handle<GltfMesh> hMesh)
{
    GltfMesh& mesh = gltfMeshes[hMesh];

    if(IS_MUTABLE(mesh.name))
    {
        FreeMStr(&mesh.name);
    }
    DestroyList(&mesh.primitives);

    gltfMeshes.Remove(hMesh);
}

void UnloadGltf_DestroyNode(Handle<GltfNode> hNode)
{
    ASSERT(hNode.IsValid());
    GltfNode& node = gltfNodes[hNode];

    // Destroy all children
    for(i32 i = 0; i < node.hChildren.count; i++)
    {
        UnloadGltf_DestroyNode(node.hChildren[i]);
    }

    // Destroy this node's components
    if(node.hChildren.count)
    {
        DestroyList(&node.hChildren);
    }
    if(node.hMesh.IsValid())
    {
        UnloadGltf_DestroyMesh(node.hMesh);
    }

    gltfNodes.Remove(hNode);
}

void UnloadGltfModel(Handle<GltfModel> hModel)
{
    //TODO(caio): Double check this for missing frees.
    ASSERT(hModel.IsValid());
    mem::SetContext(&assetHeap);

    GltfModel& model = gltfModels[hModel];

    UnloadGltf_DestroyNode(model.root);

    DestroyList(&model.indices);
    DestroyList(&model.vPositions);
    DestroyList(&model.vNormals);
    DestroyList(&model.vTexCoords0);
    DestroyList(&model.vTexCoords1);
    DestroyList(&model.vTexCoords2);

    for(i32 i = 0; i < model.hImages.count; i++)
    {
        UnloadImage(model.hImages[i]);
    }
    DestroyList(&model.hImages);
    for(i32 i = 0; i < model.hSamplers.count; i++)
    {
        UnloadGltf_DestroySampler(model.hSamplers[i]);
    }
    DestroyList(&model.hSamplers);
    for(i32 i = 0; i < model.hMaterials.count; i++)
    {
        UnloadGltf_DestroyMaterial(model.hMaterials[i]);
    }
    DestroyList(&model.hMaterials);

    loadedAssets.Remove(model.path.CStr());
    gltfModels.Remove(hModel);
}

}   // namespace asset
}   // namespace ty
