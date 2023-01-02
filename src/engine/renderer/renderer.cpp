#include "stb_image.h"
#include "fast_obj.h"

#include "engine/renderer/renderer.hpp"

namespace Ty
{

RendererData rendererData = {};
AssetDatabase assetDatabase = {};

void GLAPIENTRY
GLMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
        const GLchar* message, const void* userParam)
{
    ASSERTF(type != GL_DEBUG_TYPE_ERROR, "[OPENGL ERROR]: %s", message);
}

ResourceHandle Renderer_GetAsset(std::string_view assetPath)
{
    // TODO(caio)#ASSET: I'm sure this can be improved a lot. For instance,
    // This will return invalid if the paths don't match but point to the same file.
    // Deal with this when making a more robust path/asset system integrated with engine.
    std::string assetPath_str(assetPath);
    if(!assetDatabase.loadedAssets.count(assetPath_str)) return RESOURCE_INVALID;
    return assetDatabase.loadedAssets[assetPath_str];
}

ResourceHandle Renderer_LoadTextureAsset(std::string_view assetPath)
{
    ASSERT(PathExists(assetPath) && !IsDirectory(assetPath));
    ResourceHandle result = Renderer_GetAsset(assetPath);
    if(result != RESOURCE_INVALID) return result;

    i32 textureWidth = 0, textureHeight = 0, textureChannels = 0;

    // TODO(caio)#ASSET: No support yet for 2-channel textures. Alpha maps that
    // are 2-channel are actually loaded as 1-channel for now.
    i32 stbiInfoResult = stbi_info(assetPath.data(), NULL, NULL, &textureChannels);
    ASSERT(stbiInfoResult);
    i32 desiredChannels = textureChannels == 2 ? 1 : 0;
    stbi_set_flip_vertically_on_load(1);
    u8* textureData = stbi_load(assetPath.data(), &textureWidth, &textureHeight, &textureChannels, desiredChannels);
    
    TextureFormat textureFormat;
    switch(textureChannels)
    {
        case 1:
        case 2:
            {
                textureFormat = TEXTURE_FORMAT_R8;
            }; break;
        case 3:
            {
                textureFormat = TEXTURE_FORMAT_R8G8B8;
            }; break;
        case 4:
            {
                textureFormat = TEXTURE_FORMAT_R8G8B8A8;
            }; break;
        default: ASSERT(0);
    }

    TextureParams textureParams;

    result = Renderer_CreateTexture(textureData, textureWidth, textureHeight, textureFormat, textureParams);
    return result;
}

// Model loading with materials new API pass draft:
//      void Renderer_CreateRenderablesFromModel(assetPath, shader)
//          >> This function
//              > Loads OBJ model
//              > Creates each material and loads the relevant textures
//              > Parses each face grouped by material
//              > Creates a different mesh and renderable for each material

std::vector<ResourceHandle> Renderer_CreateRenderablesFromModel(std::string_view assetPath, ResourceHandle h_Shader)
{
    // TODO(caio)#ASSET: Not really convinced this should return void.
    // Maybe an array with handles to all created renderables.
    ASSERT(PathExists(assetPath) && !IsDirectory(assetPath));

    fastObjMesh* objData = fast_obj_read(assetPath.data());

    // Loading and creating materials and material textures
    ResourceHandle h_Materials[objData->material_count];
    for(u64 m = 0; m < objData->material_count; m++)
    {
        auto objMat = objData->materials[m];

        ResourceHandle h_Textures[MATERIAL_MAX_TEXTURES];
        for(u32 i = 0; i < MATERIAL_MAX_TEXTURES; i++) h_Textures[i] = RESOURCE_INVALID;

        char* objTexturePaths[] =
        {
            objMat.map_Ka.path,         // Ambient map
            objMat.map_Kd.path,         // Diffuse map
            objMat.map_Ks.path,         // Specular map
            objMat.map_d.path,          // Alpha Mask
            objMat.map_bump.path,       // Bump map
        };

        for(u32 i = 0; i < ArrayCount(objTexturePaths); i++)
        {
            char* assetPath = objTexturePaths[i];
            if(!assetPath) continue;

            h_Textures[i] = Renderer_LoadTextureAsset(assetPath);
        }

        h_Materials[m] = Renderer_CreateMaterial(h_Textures, ArrayCount(objTexturePaths));
    }

    // Loading vertex and index buffers (one submesh per model material)
    std::vector<MeshVertex>* modelVertices = new std::vector<MeshVertex>();
    std::vector<std::vector<u32>>* modelIndices = new std::vector<std::vector<u32>>(objData->material_count);

    u64 currentIndex = 0;
    // Iterate on every group
    for(u64 g = 0; g < objData->group_count; g++)
    {
        u64 g_FaceCount = objData->groups[g].face_count;
        u64 g_FaceOffset = objData->groups[g].face_offset;
        u64 g_IndexOffset = objData->groups[g].index_offset;

        u64 currentFaceOffset = 0;  // Cursor that always points to current face (this is needed because
                                    // faces can have more than 3 sides).
        // Then every face
        for(u64 f = g_FaceOffset; f < g_FaceOffset + g_FaceCount; f++)
        {
            u32 faceMaterial = objData->face_materials[f];
            // Then every triangle of face (OBJ does not enforce triangulated meshes)
            for(u64 t = 0; t < objData->face_vertices[f] - 2; t++)
            {
                u64 t_Indices[] = {0, t + 1, t + 2};    // Fan triangulation for regular polygons
                // Then every vertex of triangle
                for(u64 v = 0; v < 3; v++)
                {
                    u64 i = (currentFaceOffset + t_Indices[v]) + g_IndexOffset;

                    u64 i_Position = objData->indices[i].p;
                    ASSERT(i_Position);
                    u64 i_Normal = objData->indices[i].n;
                    ASSERT(i_Normal);
                    u64 i_Texcoord = objData->indices[i].t;
                    ASSERT(i_Texcoord);

                    v3f v_Position =
                    {
                        objData->positions[i_Position * 3 + 0],
                        objData->positions[i_Position * 3 + 1],
                        objData->positions[i_Position * 3 + 2],
                    };

                    v3f v_Normal =
                    {
                        objData->normals[i_Normal * 3 + 0],
                        objData->normals[i_Normal * 3 + 1],
                        objData->normals[i_Normal * 3 + 2],
                    };
                    
                    v2f v_Texcoord =
                    {
                        objData->texcoords[i_Texcoord * 2 + 0],
                        objData->texcoords[i_Texcoord * 2 + 1],
                    };

                    //outVertices->push_back({v_Position, v_Normal, v_Texcoord});
                    //outIndices->push_back(currentIndex++);
                    (*modelVertices).push_back({v_Position, v_Normal, v_Texcoord});
                    (*modelIndices)[faceMaterial].push_back(currentIndex++);
                }
            }

            currentFaceOffset += objData->face_vertices[f];
        }
    }

    // Creating mesh and buffer resources, and merging resources to renderables
    std::vector<ResourceHandle> result;

    ResourceHandle h_VertexBuffer = Renderer_CreateBuffer((u8*)modelVertices->data(), modelVertices->size(), sizeof(MeshVertex), BUFFER_TYPE_VERTEX);
    for(i32 i = 0; i < objData->material_count; i++)
    {
        std::vector<u32>& submeshIndices = (*modelIndices)[i];
        ResourceHandle h_SubmeshIndexBuffer = Renderer_CreateBuffer((u8*)submeshIndices.data(), submeshIndices.size(), sizeof(u32), BUFFER_TYPE_INDEX);
        ResourceHandle h_Submesh = Renderer_CreateMesh(h_VertexBuffer, h_SubmeshIndexBuffer);
        ResourceHandle h_SubmeshRenderable = Renderer_CreateRenderable(h_Submesh, h_Shader, h_Materials[i]);

        // Check if the material has an alpha mask
        Material& submeshMaterial = rendererData.materials[h_Materials[i]];
        Renderer_GetRenderable(h_SubmeshRenderable).u_UseAlphaMask = submeshMaterial.h_Textures[3] == RESOURCE_INVALID ? 0 : 1;

        result.push_back(h_SubmeshRenderable);
    }

    // Freeing allocated obj parse data
    fast_obj_destroy(objData);

    // TODO(caio)#ASSET: Maybe I should treat models as loaded assets as well.
    // So if I already loaded an OBJ I don't load it again, and just create copies of the renderables
    // previously created... No idea how...
    return result;
}

// TODO(caio)#RENDER: Move this function from here, this is not a rendering function.
void LoadOBJModel(std::string_view assetPath, std::vector<MeshVertex>* outVertices, std::vector<u32>* outIndices)
{
    // TODO(caio)#RENDER: This does not support material loading yet.
    // The time will come when trying to render textured meshes and with an actual render resource system.
    ASSERT(outVertices && outIndices);

    fastObjMesh* objData = fast_obj_read(assetPath.data());

    outVertices->reserve(objData->index_count * 3);
    outIndices->reserve(objData->index_count * 3);

    u64 currentIndex = 0;
    // Iterate on every group
    for(u64 g = 0; g < objData->group_count; g++)
    {
        u64 g_FaceCount = objData->groups[g].face_count;
        u64 g_FaceOffset = objData->groups[g].face_offset;
        u64 g_IndexOffset = objData->groups[g].index_offset;

        u64 currentFaceOffset = 0;  // Cursor that always points to current face (this is needed because
                                    // faces can have more than 3 sides).
        // Then every face
        for(u64 f = g_FaceOffset; f < g_FaceOffset + g_FaceCount; f++)
        {
            // Then every triangle of face (OBJ does not enforce triangulated meshes)
            for(u64 t = 0; t < objData->face_vertices[f] - 2; t++)
            {
                u64 t_Indices[] = {0, t + 1, t + 2};    // Fan triangulation for regular polygons
                // Then every vertex of triangle
                for(u64 v = 0; v < 3; v++)
                {
                    u64 i = (currentFaceOffset + t_Indices[v]) + g_IndexOffset;

                    u64 i_Position = objData->indices[i].p;
                    ASSERT(i_Position);
                    u64 i_Normal = objData->indices[i].n;
                    ASSERT(i_Normal);
                    u64 i_Texcoord = objData->indices[i].t;
                    ASSERT(i_Texcoord);

                    v3f v_Position =
                    {
                        objData->positions[i_Position * 3 + 0],
                        objData->positions[i_Position * 3 + 1],
                        objData->positions[i_Position * 3 + 2],
                    };

                    v3f v_Normal =
                    {
                        objData->normals[i_Normal * 3 + 0],
                        objData->normals[i_Normal * 3 + 1],
                        objData->normals[i_Normal * 3 + 2],
                    };
                    
                    v2f v_Texcoord =
                    {
                        objData->texcoords[i_Texcoord * 2 + 0],
                        objData->texcoords[i_Texcoord * 2 + 1],
                    };

                    outVertices->push_back({v_Position, v_Normal, v_Texcoord});
                    outIndices->push_back(currentIndex++);
                }
            }

            currentFaceOffset += objData->face_vertices[f];
        }
    }

    fast_obj_destroy(objData);
}

m4f Camera::GetView()
{
    return LookAtMatrix(position, position + target, up);
}

m4f Camera::GetProjection(const Window& window)
{
    f32 aspectRatio = (f32)window.width/(f32)window.height;
    return PerspectiveProjectionMatrix(fov, aspectRatio, nearPlane, farPlane);
}

void Camera::Move(v3f newPosition)
{
    position = newPosition;
}

void Camera::Rotate(f32 rotationAngle, v3f rotationAxis)
{
    m4f rotationMatrix = RotationMatrix(rotationAngle, rotationAxis);
    v4f newTarget = rotationMatrix * v4f{target.x, target.y, target.z, 0};
    target = v3f{newTarget.x, newTarget.y, newTarget.z};
}

void Renderer_SetCamera(Camera camera)
{
    rendererData.camera = camera;
}

void Renderer_SetViewport(RenderViewport viewport)
{
    rendererData.viewport = viewport;
}

ResourceHandle Renderer_CreateBuffer(u8* bufferData, u64 bufferCount, u64 bufferStride, BufferType bufferType)
{
    APIHandle glHandle = API_HANDLE_INVALID;
    glGenBuffers(1, &glHandle);
    ASSERT(glHandle != API_HANDLE_INVALID);

    GLuint glBindType;
    switch(bufferType)
    {
        case BUFFER_TYPE_VERTEX:
            {
                glBindType = GL_ARRAY_BUFFER;
            }; break;
        case BUFFER_TYPE_INDEX:
            {
                glBindType = GL_ELEMENT_ARRAY_BUFFER;
            }; break;

        default: ASSERT(0);
    }

    glBindVertexArray(0);
    glBindBuffer(glBindType, glHandle);
    glBufferData(glBindType, bufferCount * bufferStride, bufferData, GL_STATIC_DRAW);

    Buffer buffer =
    {
        bufferType,
        glHandle,
        bufferCount,
        bufferStride,
        bufferData
    };

    rendererData.buffers.push_back(buffer);
    return rendererData.buffers.size() - 1;
}

ResourceHandle Renderer_CreateTexture(u8* textureData, u32 textureWidth, u32 textureHeight, TextureFormat textureFormat, TextureParams textureParams)
{
    APIHandle glHandle = API_HANDLE_INVALID;
    glGenTextures(1, &glHandle);
    ASSERT(glHandle != API_HANDLE_INVALID);
    GLuint glWrapS, glWrapT, glFilterMin, glFilterMag;
    GLuint glInternalFormat, glFormat, glDataType;
    switch(textureParams.wrapMode)
    {
        case TEXTURE_WRAP_REPEAT:
            {
                glWrapS = glWrapT = GL_REPEAT;
            }; break;
        case TEXTURE_WRAP_CLAMP:
            {
                glWrapS = glWrapT = GL_CLAMP_TO_EDGE;
            }; break;
        default: ASSERT(0);
    }

    switch(textureParams.filterMode_Min)
    {
        case TEXTURE_FILTER_LINEAR:
            {
                glFilterMin = textureParams.useMips ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
            }; break;
        case TEXTURE_FILTER_NEAREST:
            {
                glFilterMin = textureParams.useMips ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST;
            }; break;
        default: ASSERT(0);
    }
    
    switch(textureParams.filterMode_Max)
    {
        case TEXTURE_FILTER_LINEAR:
            {
                glFilterMag = GL_LINEAR;
            }; break;
        case TEXTURE_FILTER_NEAREST:
            {
                glFilterMag = GL_NEAREST;
            }; break;
        default: ASSERT(0);
    }
    
    switch(textureFormat)
    {
        case TEXTURE_FORMAT_R8:
            {
                glInternalFormat = GL_R8;
                glFormat = GL_RED;
                glDataType = GL_UNSIGNED_BYTE;
            }; break;
        case TEXTURE_FORMAT_R8G8B8:
            {
                glInternalFormat = GL_RGB8;
                glFormat = GL_RGB;
                glDataType = GL_UNSIGNED_BYTE;
            }; break;
        case TEXTURE_FORMAT_R8G8B8A8:
            {
                glInternalFormat = GL_RGBA8;
                glFormat = GL_RGBA;
                glDataType = GL_UNSIGNED_BYTE;
            }; break;

        default: ASSERT(0);
    }


    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glWrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glWrapT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glFilterMin);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glFilterMag);

    glBindTexture(GL_TEXTURE_2D, glHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, glInternalFormat, textureWidth, textureHeight, 0, glFormat, glDataType, textureData);

    if(textureParams.useMips)
        glGenerateMipmap(GL_TEXTURE_2D);
    
    Texture texture =
    {
        textureFormat,
        textureParams,
        textureWidth,
        textureHeight,
        glHandle,
        textureData
    };
    
    rendererData.textures.push_back(texture);
    return rendererData.textures.size() - 1;
}

ResourceHandle Renderer_CreateMesh(ResourceHandle h_VertexBuffer, ResourceHandle h_IndexBuffer)
{
    APIHandle glHandle = API_HANDLE_INVALID;
    glGenVertexArrays(1, &glHandle);
    ASSERT(glHandle != API_HANDLE_INVALID);

    glBindVertexArray(glHandle);

    Buffer& vertexBuffer = rendererData.buffers[h_VertexBuffer];
    Buffer& indexBuffer = rendererData.buffers[h_IndexBuffer];

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.apiHandle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer.apiHandle);

    // For meshes, Vertex Data is formatted as MeshVertex (v3f position - v3f normal - v2f texcoord)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)StructOffset(MeshVertex, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)StructOffset(MeshVertex, normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)StructOffset(MeshVertex, texcoord));
    glEnableVertexAttribArray(2);

    Mesh mesh =
    {
        h_VertexBuffer,
        h_IndexBuffer,
        glHandle
    };

    rendererData.meshes.push_back(mesh);
    return rendererData.meshes.size() - 1;
}

ResourceHandle Renderer_CreateShader(std::string_view shaderSrc, ShaderType shaderType)
{
    APIHandle glHandle = API_HANDLE_INVALID;
    switch(shaderType)
    {
        case SHADER_TYPE_VERTEX:
            {
                glHandle = glCreateShader(GL_VERTEX_SHADER);
            }; break;
        case SHADER_TYPE_PIXEL:
            {
                glHandle = glCreateShader(GL_FRAGMENT_SHADER);
            }; break;
        default: ASSERT(0);
    }
    ASSERT(glHandle != API_HANDLE_INVALID);

    const char* shaderSrcCstr = shaderSrc.data();
    glShaderSource(glHandle, 1, &shaderSrcCstr, NULL);
    glCompileShader(glHandle);

    Shader shader =
    {
        shaderType,
        glHandle,
        shaderSrc
    };
    rendererData.shaders.push_back(shader);
    return rendererData.shaders.size() - 1;
}

ResourceHandle Renderer_CreateShaderPipeline(ResourceHandle h_VS, ResourceHandle h_PS)
{
    APIHandle glHandle = API_HANDLE_INVALID;
    glHandle = glCreateProgram();
    ASSERT(glHandle != API_HANDLE_INVALID);
    Shader& vertexShader = rendererData.shaders[h_VS];
    Shader& pixelShader = rendererData.shaders[h_PS];

    glAttachShader(glHandle, vertexShader.apiHandle);
    glAttachShader(glHandle, pixelShader.apiHandle);
    glLinkProgram(glHandle);

    ShaderPipeline shaderPipeline =
    {
        h_VS,
        h_PS,
        glHandle
    };
    rendererData.shaderPipelines.push_back(shaderPipeline);
    return rendererData.shaderPipelines.size() - 1;
};

ResourceHandle Renderer_CreateMaterial(ResourceHandle* h_MaterialTextureArray, u8 materialTextureCount)
{
    Material material = {};
    material.count = materialTextureCount;
    for(i32 i = 0; i < material.count; i++)
    {
        material.h_Textures[i] = h_MaterialTextureArray[i];
    }
    rendererData.materials.push_back(material);
    return rendererData.materials.size() - 1;
}

ResourceHandle Renderer_CreateRenderable(ResourceHandle h_Mesh, ResourceHandle h_Shader, ResourceHandle h_Material)
{
    Renderable renderable =
    {
        h_Mesh,
        h_Shader,
        h_Material
    };

    // TODO(caio)#RENDER: IMPORTANT, push these to array by insertion sorting them later.
    rendererData.renderables.push_back(renderable);
    return rendererData.renderables.size() - 1;
}

Renderable& Renderer_GetRenderable(ResourceHandle h_Renderable)
{
    ASSERT(h_Renderable != RESOURCE_INVALID);
    return rendererData.renderables[h_Renderable];
}

void Renderer_BindMesh(ResourceHandle h_Mesh)
{
    Mesh& mesh = rendererData.meshes[h_Mesh];
    glBindVertexArray(mesh.apiHandle);
}

void Renderer_BindShaderPipeline(ResourceHandle h_ShaderPipeline)
{
    ShaderPipeline& shader = rendererData.shaderPipelines[h_ShaderPipeline];
    glUseProgram(shader.apiHandle);
}

void Renderer_BindMaterial(ResourceHandle h_Material)
{
    Material& material = rendererData.materials[h_Material];
    for(i32 i = 0; i < material.count; i++)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        ResourceHandle h_Texture = material.h_Textures[i];
        if(h_Texture == RESOURCE_INVALID)
        {
            glBindTexture(GL_TEXTURE_2D, 0);    // Incomplete texture, sample returns (0,0,0,1) according to spec
        }
        else
        {
            Texture& texture = rendererData.textures[h_Texture];
            glBindTexture(GL_TEXTURE_2D, texture.apiHandle);
        }
    }
}

void Renderer_BindUniforms(const Renderable& renderable)
{
    ShaderPipeline& shader = rendererData.shaderPipelines[renderable.h_Shader];
    GLint location = -1;
    location = glGetUniformLocation(shader.apiHandle, "u_Model");
    ASSERT(location != -1);
    glUniformMatrix4fv(location, 1, GL_TRUE, &renderable.u_Model.m00);
    location = glGetUniformLocation(shader.apiHandle, "u_UseAlphaMask");
    ASSERT(location != -1);
    glUniform1f(location, renderable.u_UseAlphaMask);

    // TODO(caio)#RENDER: Move these to a global uniform buffer whenever adding UBO support.
    m4f viewMatrix = rendererData.camera.GetView();
    m4f projMatrix = rendererData.camera.GetProjection(*rendererData.window);
    location = glGetUniformLocation(shader.apiHandle, "u_View");
    ASSERT(location != -1);
    glUniformMatrix4fv(location, 1, GL_TRUE, &viewMatrix.m00);
    location = glGetUniformLocation(shader.apiHandle, "u_Proj");
    ASSERT(location != -1);
    glUniformMatrix4fv(location, 1, GL_TRUE, &projMatrix.m00);
}

void Renderer_Init(u32 windowWidth, u32 windowHeight, const char* windowName, Window* outWindow)
{
    ASSERT(outWindow);
    Window_Init(windowWidth, windowHeight, windowName, outWindow);
    Window_InitRenderContext(*outWindow);

    // OpenGL settings
    glEnable(GL_DEPTH_TEST);            // Depth testing
    glEnable(GL_CULL_FACE);             // Face culling
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
#if _DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(GLMessageCallback, 0);
#endif
    // TODO(caio)#RENDER: Enable MSAA

    rendererData.window = outWindow;
    RenderViewport viewport = {{0,0}, windowWidth, windowHeight};
    Renderer_SetViewport(viewport);

    Window_Show(*outWindow);
}

void Renderer_RenderFrame()
{
    // Preparing Render
    {
        // Clearing backbuffer
        glClearColor(1.f, 0.5f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Updating viewport (in case of window resizing)
        Renderer_SetViewport({0, 0, rendererData.window->width, rendererData.window->height});
        glViewport(
                rendererData.viewport.bottomLeft.x,
                rendererData.viewport.bottomLeft.y,
                rendererData.viewport.width,
                rendererData.viewport.height
                );
    }

    // Rendering 3D mesh renderables
    {
        ResourceHandle activeShader         = RESOURCE_INVALID;
        ResourceHandle activeMaterial       = RESOURCE_INVALID;
        ResourceHandle activeMesh           = RESOURCE_INVALID;

        for(i32 i = 0; i < rendererData.renderables.size(); i++)
        {
            Renderable& renderable = rendererData.renderables[i];
            // Bind resources
            if(renderable.h_Shader != activeShader)
            {
                activeShader = renderable.h_Shader;
                Renderer_BindShaderPipeline(activeShader);
            }
            if(renderable.h_Material != activeMaterial)
            {
                activeMaterial = renderable.h_Material;
                Renderer_BindMaterial(activeMaterial);
            }
            if(renderable.h_Mesh != activeMesh)
            {
                activeMesh = renderable.h_Mesh;
                Renderer_BindMesh(activeMesh);
            }
            Renderer_BindUniforms(renderable);

            // Draw
            Mesh& mesh = rendererData.meshes[renderable.h_Mesh];
            Buffer& indexBuffer = rendererData.buffers[mesh.h_IndexBuffer];
            glDrawElements(GL_TRIANGLES, indexBuffer.count, GL_UNSIGNED_INT, 0);
        }
    }
}

} // namespace Ty
