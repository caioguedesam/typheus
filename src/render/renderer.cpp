#include "glad/glad.h"

#include "stb_image.h"
#include "fast_obj.h"
//#include "cgltf.h"

#include "core/file.hpp"
#include "render/renderer.hpp"
#include "app.hpp"

#define RESOURCE_PATH "../resources/"
#define SHADER_PATH RESOURCE_PATH"shaders/"
#define TEXTURE_PATH RESOURCE_PATH"textures/"
#define MODELS_PATH RESOURCE_PATH"models/"

namespace Ty
{

MemArena memArena_Shader;
MemArena memArena_Texture;
MemArena memArena_Mesh;

u32 vsHandle = MAX_U32;
u32 psHandle = MAX_U32;
u32 shaderProgramHandle = MAX_U32;

u32 textureHandle = MAX_U32;

u32 quadVBO = MAX_U32;
u32 quadEBO = MAX_U32;
u32 quadVAO = MAX_U32;

f32 quadVertices[] =
{
   -0.5f,  -0.5f,   0.f,    0.f,    0.f,
    0.5f,  -0.5f,   0.f,    0.f,    1.f,
    0.5f,   0.5f,   0.f,    1.f,    1.f,
   -0.5f,   0.5f,   0.f,    1.f,    0.f,
};

u32 quadIndices[] =
{
    0, 1, 2,
    0, 2, 3,
};

Array<MeshVertex> sponzaVertices;
Array<u32> sponzaIndices;
u32 sponzaVAO;
u32 sponzaVBO;
u32 sponzaEBO;

Array<MeshVertex> bunnyVertices;
Array<u32> bunnyIndices;
u32 bunnyVAO;
u32 bunnyVBO;
u32 bunnyEBO;

m4f modelMatrix = {};
m4f viewMatrix = {};
m4f projMatrix = {};

void GLAPIENTRY
GLMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
        const GLchar* message, const void* userParam)
{
    ASSERTF(type != GL_DEBUG_TYPE_ERROR, "[OPENGL ERROR]: %s", message);
}

void LoadOBJModel(FilePath assetPath, Array<MeshVertex>* outVertices, Array<u32>* outIndices)
{
    ASSERT(outVertices && outIndices);

    fastObjMesh* objData = fast_obj_read(assetPath.str.ToCStr());

    *outVertices = ArrayAlloc<MeshVertex>(
            &memArena_Mesh,
            objData->index_count * 3
            );
    *outIndices = ArrayAlloc<u32>(
            &memArena_Mesh,
            objData->index_count * 3
            );

    u64 currentIndex = 0;
    for(u64 g = 0; g < objData->group_count; g++)
    {
        u64 groupFaceCount = objData->groups[g].face_count;
        u64 groupFaceOffset = objData->groups[g].face_offset;
        u64 groupIndexOffset = objData->groups[g].index_offset;

        u64 currentFaceOffset = 0;  // Cursor that always points to current face (this is needed because
                                    // faces can have more than 3 sides).

        for(u64 f = groupFaceOffset; f < groupFaceOffset + groupFaceCount; f++)
        {
            if(objData->face_vertices[f] == 3)
            {
                for(u64 v = 0; v < 3; v++)
                {
                    u64 i = (currentFaceOffset + v) + groupIndexOffset;
                    u64 i_position = objData->indices[i].p;
                    ASSERT(i_position);
                    u64 i_normal = objData->indices[i].n;
                    ASSERT(i_normal);
                    u64 i_texcoord = objData->indices[i].t;
                    ASSERT(i_texcoord);

                    v3f vertexPos =
                    {
                        objData->positions[i_position * 3 + 0],
                        objData->positions[i_position * 3 + 1],
                        objData->positions[i_position * 3 + 2],
                    };

                    v3f vertexNormal =
                    {
                        objData->normals[i_normal * 3 + 0],
                        objData->normals[i_normal * 3 + 1],
                        objData->normals[i_normal * 3 + 2],
                    };
                    
                    v2f vertexTexcoord =
                    {
                        objData->texcoords[i_texcoord * 2 + 0],
                        objData->texcoords[i_texcoord * 2 + 1],
                    };

                    outVertices->Push({vertexPos, vertexNormal, vertexTexcoord});
                    outIndices->Push(currentIndex++);
                }
            }
            else
            {
                // For non-triangle faces, triangulate then parse
                // N sided polygon has N-2 triangles
                for(u64 point = 1; point < objData->face_vertices[f] - 1; point++)
                {
                    u64 triIndices[] = {0, point, point + 1};
                    for(u64 v = 0; v < 3; v++)
                    {
                        u64 i = (currentFaceOffset + triIndices[v]) + groupIndexOffset;

                        u64 i_position = objData->indices[i].p;
                        ASSERT(i_position);
                        u64 i_normal = objData->indices[i].n;
                        ASSERT(i_normal);
                        u64 i_texcoord = objData->indices[i].t;
                        ASSERT(i_texcoord);

                        v3f vertexPos =
                        {
                            objData->positions[i_position * 3 + 0],
                            objData->positions[i_position * 3 + 1],
                            objData->positions[i_position * 3 + 2],
                        };

                        v3f vertexNormal =
                        {
                            objData->normals[i_normal * 3 + 0],
                            objData->normals[i_normal * 3 + 1],
                            objData->normals[i_normal * 3 + 2],
                        };
                        
                        v2f vertexTexcoord =
                        {
                            objData->texcoords[i_texcoord * 2 + 0],
                            objData->texcoords[i_texcoord * 2 + 1],
                        };

                        outVertices->Push({vertexPos, vertexNormal, vertexTexcoord});
                        outIndices->Push(currentIndex++);
                    }
                }
            }
            currentFaceOffset += objData->face_vertices[f];
        }
    }

    fast_obj_destroy(objData);
}

void InitRenderer(Window* window)
{
    InitGLContext(window);

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

    // Resource memory
    MemArenaInit(&memArena_Shader, KB(256));
    MemArenaInit(&memArena_Texture, MB(1));
    MemArenaInit(&memArena_Mesh, MB(256));

    // Shader resources
    // TODO(caio)#RENDER: Change this when starting support for multiple shaders + hot reload
    String vsSrc = ReadFileToStr(&memArena_Shader, MakePath(SHADER_PATH"default_vertex.vs"));
    String psSrc = ReadFileToStr(&memArena_Shader, MakePath(SHADER_PATH"default_pixel.ps"));
    const char* vsCStr = vsSrc.ToCStr();
    const char* psCStr = psSrc.ToCStr();
    
    i32 ret = 0;
    vsHandle = glCreateShader(GL_VERTEX_SHADER);
    ASSERT(vsHandle);
    glShaderSource(vsHandle, 1, &vsCStr, NULL);
    glCompileShader(vsHandle);
    glGetShaderiv(vsHandle, GL_COMPILE_STATUS, &ret);
    if(!ret)
    {
        String infoLog = StrAllocZero(&memArena_Frame, 512);
        glGetShaderInfoLog(vsHandle, 512, NULL, infoLog.ToCStr());
        ASSERTF(0, "Vertex shader compilation failed: %s", infoLog.ToCStr());
    }

    psHandle = glCreateShader(GL_FRAGMENT_SHADER);
    ASSERT(psHandle);
    glShaderSource(psHandle, 1, &psCStr, NULL);
    glCompileShader(psHandle);
    glGetShaderiv(psHandle, GL_COMPILE_STATUS, &ret);
    if(!ret)
    {
        String infoLog = StrAllocZero(&memArena_Frame, 512);
        glGetShaderInfoLog(psHandle, 512, NULL, infoLog.ToCStr());
        ASSERTF(0, "Pixel shader compilation failed: %s", infoLog.ToCStr());
    }

    shaderProgramHandle = glCreateProgram();
    glAttachShader(shaderProgramHandle, vsHandle);
    glAttachShader(shaderProgramHandle, psHandle);
    glLinkProgram(shaderProgramHandle);
    glGetProgramiv(shaderProgramHandle, GL_LINK_STATUS, &ret);
    if(!ret)
    {
        String infoLog = StrAllocZero(&memArena_Frame, 512);
        glGetProgramInfoLog(shaderProgramHandle, 512, NULL, infoLog.ToCStr());
        ASSERTF(0, "Shader program compilation failed: %s", infoLog.ToCStr());
    }

    // TODO(caio)#RENDER: This is temporary code before setting up proper object drawing.
    // Create quad object
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glGenBuffers(1, &quadEBO);

    glBindVertexArray(quadVAO);

    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, ArraySize(quadVertices), quadVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, ArraySize(quadIndices), quadIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(f32), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(f32), (void*)(3 * sizeof(f32)));
    glEnableVertexAttribArray(1);

    // Load quad texture
    i32 textureWidth, textureHeight, textureChannels;
    u8* stbiData = stbi_load(TEXTURE_PATH"test.png", &textureWidth, &textureHeight, &textureChannels, 3);
    ASSERT(stbiData);
    ASSERT(textureWidth);
    ASSERT(textureHeight);
    ASSERT(textureChannels);

    // TODO(caio)#RENDER: This allocates without using arenas because of stbi. Proper support would need an arena realloc implementation.
    u8* textureData = (u8*)MemAlloc(&memArena_Texture, textureWidth * textureHeight * textureChannels);
    memcpy(textureData, stbiData, textureWidth * textureHeight * 3);
    stbi_image_free(stbiData);

    glGenTextures(1, &textureHandle);
    glBindTexture(GL_TEXTURE_2D, textureHandle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureWidth, textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Loading OBJ model
    LoadOBJModel(MakePath(MODELS_PATH"sponza/sponza.obj"), &sponzaVertices, &sponzaIndices);
    LoadOBJModel(MakePath(MODELS_PATH"bunny/bunny.obj"), &bunnyVertices, &bunnyIndices);

    // Creating OpenGL structures for sponza model
    glGenVertexArrays(1, &sponzaVAO);
    glGenBuffers(1, &sponzaVBO);
    glGenBuffers(1, &sponzaEBO);

    glBindVertexArray(sponzaVAO);

    glBindBuffer(GL_ARRAY_BUFFER, sponzaVBO);
    glBufferData(GL_ARRAY_BUFFER, sponzaVertices.Size(), sponzaVertices.data, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sponzaEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sponzaIndices.Size(), sponzaIndices.data, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)StructOffset(MeshVertex, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)StructOffset(MeshVertex, normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)StructOffset(MeshVertex, texcoord));
    glEnableVertexAttribArray(2);

    // Creating OpenGL structures for bunny model
    glGenVertexArrays(1, &bunnyVAO);
    glGenBuffers(1, &bunnyVBO);
    glGenBuffers(1, &bunnyEBO);

    glBindVertexArray(bunnyVAO);

    glBindBuffer(GL_ARRAY_BUFFER, bunnyVBO);
    glBufferData(GL_ARRAY_BUFFER, bunnyVertices.Size(), bunnyVertices.data, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bunnyEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, bunnyIndices.Size(), bunnyIndices.data, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)StructOffset(MeshVertex, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)StructOffset(MeshVertex, normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)StructOffset(MeshVertex, texcoord));
    glEnableVertexAttribArray(2);

    // Temporary transformation matrices
    v3f cameraCenter = {0,2,3};
    v3f cameraDir = {0,0,-1};
    f32 cameraFov = 45.f;
    f32 cameraAspectRatio = 1280.f/720.f;
    f32 cameraNear = 0.1f;
    f32 cameraFar = 1000.f;
    viewMatrix = LookAtMatrix(
            cameraCenter,
            cameraCenter + cameraDir,
            {0,1,0});
    projMatrix = PerspectiveProjectionMatrix(
            cameraFov,
            cameraAspectRatio,
            cameraNear,
            cameraFar);

    i32 a = 10;
}

void RenderFrame()
{
    glClearColor(1.f, 0.5f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    modelMatrix = ScaleMatrix({0.01f, 0.01f, 0.01f});
    modelMatrix = RotationMatrix(TO_RAD(90.f), {0.f, 1.f, 0.f}) * modelMatrix;

    // TODO(caio)#RENDER: This is temporary code before setting up proper object drawing.
    glUseProgram(shaderProgramHandle);
    //glBindTexture(GL_TEXTURE_2D, textureHandle);
    //glBindVertexArray(quadVAO);
    glBindVertexArray(sponzaVAO);

    GLint location = glGetUniformLocation(shaderProgramHandle, "u_Model");
    glUniformMatrix4fv(location, 1, GL_TRUE, &modelMatrix.m00);
    location = glGetUniformLocation(shaderProgramHandle, "u_View");
    glUniformMatrix4fv(location, 1, GL_TRUE, &viewMatrix.m00);
    location = glGetUniformLocation(shaderProgramHandle, "u_Proj");
    glUniformMatrix4fv(location, 1, GL_TRUE, &projMatrix.m00);

    glDrawElements(GL_TRIANGLES, sponzaIndices.count, GL_UNSIGNED_INT, 0);

    modelMatrix = Identity();

    glBindVertexArray(bunnyVAO);

    location = glGetUniformLocation(shaderProgramHandle, "u_Model");
    glUniformMatrix4fv(location, 1, GL_TRUE, &modelMatrix.m00);
    location = glGetUniformLocation(shaderProgramHandle, "u_View");
    glUniformMatrix4fv(location, 1, GL_TRUE, &viewMatrix.m00);
    location = glGetUniformLocation(shaderProgramHandle, "u_Proj");
    glUniformMatrix4fv(location, 1, GL_TRUE, &projMatrix.m00);

    glDrawElements(GL_TRIANGLES, bunnyIndices.count, GL_UNSIGNED_INT, 0);
}

} // namespace Ty
