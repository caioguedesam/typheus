#include "engine/common/asset.hpp"
#include "stb_image.h"
#include "fast_obj.h"
// TODO(caio)#ASSET: Include cgltf for loading gltf models
// TODO(caio)#ASSET: Support more asset types such as sound files
// TODO(caio)#ASSET: I think this should probably be separated from the common code
// and be its own module like renderer.

namespace Ty
{

Handle<AssetShader> Asset_LoadShader(const std::string& assetPath)
{
    if(Asset_IsLoaded(assetPath)) return { assetTable.loadedAssets[assetPath] };

    AssetShader* shader = new AssetShader();
    shader->path = assetPath;
    shader->src = ReadFile_Str(assetPath);

    assetTable.shaderAssets.push_back(shader);
    assetTable.loadedAssets[assetPath] = (u32)assetTable.shaderAssets.size() - 1;
    return { assetTable.loadedAssets[assetPath] };
}

Handle<AssetTexture> Asset_LoadTexture(const std::string& assetPath)
{
    if(Asset_IsLoaded(assetPath)) return { assetTable.loadedAssets[assetPath] };

    AssetTexture* texture = new AssetTexture();

    i32 width, height, channels;
    stbi_set_flip_vertically_on_load(1);
    u8* data = stbi_load(assetPath.c_str(), &width, &height, &channels, 0);

    texture->width = width;
    texture->height = height;
    texture->channels = channels;
    texture->data = data;

    assetTable.textureAssets.push_back(texture);
    assetTable.loadedAssets[assetPath] = (u32)assetTable.textureAssets.size() - 1;
    return { assetTable.loadedAssets[assetPath] };
}

Handle<AssetModel> Asset_LoadModel_OBJ(const std::string& assetPath, bool flipVerticalTexcoord)
{
    if(Asset_IsLoaded(assetPath)) return { assetTable.loadedAssets[assetPath] };

    AssetModel* model = new AssetModel();

    fastObjMesh* fastObjData = fast_obj_read(assetPath.c_str());
    model->objects = std::vector<AssetModelObject>(MAX(1, fastObjData->material_count));

    // Loading model material textures
    for(u64 m = 0; m < fastObjData->material_count; m++)
    {
        auto objMat = fastObjData->materials[m];
        AssetModelObject& modelObject = model->objects[m];
        if(objMat.map_Ka.path)
        {
            modelObject.textureAmbient = Asset_LoadTexture(objMat.map_Ka.path);
        }
        if(objMat.map_Kd.path)
        {
            modelObject.textureDiffuse = Asset_LoadTexture(objMat.map_Kd.path);
        }
        if(objMat.map_Ks.path)
        {
            modelObject.textureSpecular = Asset_LoadTexture(objMat.map_Ks.path);
        }
        if(objMat.map_d.path)
        {
            modelObject.textureAlphaMask = Asset_LoadTexture(objMat.map_d.path);
        }
        if(objMat.map_bump.path)
        {
            modelObject.textureBump = Asset_LoadTexture(objMat.map_bump.path);
        }
        modelObject.ambientColor = { objMat.Ka[0], objMat.Ka[1], objMat.Ka[2] };
        modelObject.diffuseColor = { objMat.Kd[0], objMat.Kd[1], objMat.Kd[2] };
        modelObject.specularColor = { objMat.Ks[0], objMat.Ks[1], objMat.Ks[2] };
        modelObject.specularWeight = objMat.Ns;
    }

    // Loading model vertex and index data
    struct AssetVertex
    {
        v3f position; v3f normal; v2f texcoord;
    }; 

    u64 currentIndex = 0;
    // Iterate on every group of OBJ
    for(u64 g = 0; g < fastObjData->group_count; g++)
    {
        auto objGroup = fastObjData->groups[g];
        u64 currentFaceOffset = 0;  // Cursor always pointing to current face (needed for faces that
                                    // have >3 sides).
        // Iterate on every face of group
        for(u64 f = objGroup.face_offset; f < objGroup.face_offset + objGroup.face_count; f++)
        {
            u32 faceMaterial = fastObjData->face_materials[f];
            // Iterate on every triangle of face (enforcing triangulation here)
            for(u64 t = 0; t < fastObjData->face_vertices[f] - 2; t++)
            {
                u64 triangleIndices[] = {0, t + 1, t + 2};
                AssetVertex triangleVertices[] = {{}, {}, {}};
                // Iterate on each triangle vertex
                for(u64 v = 0; v < 3; v++)
                {
                    u64 i = (currentFaceOffset + triangleIndices[v]) + objGroup.index_offset;
                    u64 iPosition   = fastObjData->indices[i].p;    ASSERT(iPosition);
                    u64 iNormal     = fastObjData->indices[i].n;    ASSERT(iNormal);
                    u64 iTexcoord   = fastObjData->indices[i].t;    ASSERT(iTexcoord);

                    //v3f vertexPosition =
                    triangleVertices[v].position = 
                    {
                        fastObjData->positions[iPosition * 3 + 0],
                        fastObjData->positions[iPosition * 3 + 1],
                        fastObjData->positions[iPosition * 3 + 2],
                    };

                    //v3f vertexNormal =
                    triangleVertices[v].normal = 
                    {
                        fastObjData->normals[iNormal * 3 + 0],
                        fastObjData->normals[iNormal * 3 + 1],
                        fastObjData->normals[iNormal * 3 + 2],
                    };
                    
                    //v2f vertexTexcoord =
                    triangleVertices[v].texcoord = 
                    {
                        fastObjData->texcoords[iTexcoord * 2 + 0],
                        fastObjData->texcoords[iTexcoord * 2 + 1],
                    };
                    if(flipVerticalTexcoord)
                    {
                        triangleVertices[v].texcoord.v = 1.f - triangleVertices[v].texcoord.v;
                    }

                    //model->vertices.push_back(vertexPosition.x);
                    //model->vertices.push_back(vertexPosition.y);
                    //model->vertices.push_back(vertexPosition.z);
                    //model->vertices.push_back(vertexNormal.x);
                    //model->vertices.push_back(vertexNormal.y);
                    //model->vertices.push_back(vertexNormal.z);
                    //model->vertices.push_back(vertexTexcoord.x);
                    //model->vertices.push_back(vertexTexcoord.y);

                    //model->objects[faceMaterial].indices.push_back(currentIndex++);
                }

                // Calculate tangent/bitangent for all 3 vertices
                v3f deltaPos0 = triangleVertices[1].position - triangleVertices[0].position;
                v3f deltaPos1 = triangleVertices[2].position - triangleVertices[0].position;
                v2f deltaUV0 = triangleVertices[1].texcoord - triangleVertices[0].texcoord;
                v2f deltaUV1 = triangleVertices[2].texcoord - triangleVertices[0].texcoord;
                f32 r = 1.f / (deltaUV0.x * deltaUV1.y - deltaUV0.y * deltaUV1.x);
                v3f tangent = Normalize((deltaPos0 * deltaUV1.y - deltaPos1 * deltaUV0.y) * r);
                v3f bitangent = Normalize((deltaPos1 * deltaUV0.x - deltaPos0 * deltaUV1.x) * r);

                // Send vertex data
                for(i32 v = 0; v < 3; v++)
                {
                    AssetVertex& av = triangleVertices[v];
                    model->vertices.push_back(av.position.x);
                    model->vertices.push_back(av.position.y);
                    model->vertices.push_back(av.position.z);
                    model->vertices.push_back(av.normal.x);
                    model->vertices.push_back(av.normal.y);
                    model->vertices.push_back(av.normal.z);
                    model->vertices.push_back(tangent.x);
                    model->vertices.push_back(tangent.y);
                    model->vertices.push_back(tangent.z);
                    model->vertices.push_back(bitangent.x);
                    model->vertices.push_back(bitangent.y);
                    model->vertices.push_back(bitangent.z);
                    model->vertices.push_back(av.texcoord.x);
                    model->vertices.push_back(av.texcoord.y);

                    model->objects[faceMaterial].indices.push_back(currentIndex++);
                }
            }

            currentFaceOffset += fastObjData->face_vertices[f];
        }
    }

    fast_obj_destroy(fastObjData);

    assetTable.modelAssets.push_back(model);
    assetTable.loadedAssets[assetPath] = (u32)assetTable.modelAssets.size() - 1;
    return { assetTable.loadedAssets[assetPath] };
}

}   // namespace Ty