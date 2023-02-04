#include "engine/renderer/renderer.hpp"

namespace Ty
{

// Default renderer resource data
f32 screenQuadVertices[] =
{
    // position    // UVs
    -1.f, -1.f,    0.f, 0.f,
     1.f, -1.f,    1.f, 0.f,
     1.f,  1.f,    1.f, 1.f,
    -1.f,  1.f,    0.f, 1.f,
};

u32 screenQuadIndices[] =
{
    0, 1, 2,
    0, 2, 3,
};

// TODO(caio)#RENDER: Add normals and texture coords here when needed.
f32 defaultCubeVertices[] =
{
    -1, -1, -1,
    -1, -1,  1,
    -1,  1,  1,
    -1,  1, -1,
     1, -1,  1,
     1, -1, -1,
     1,  1, -1,
     1,  1,  1,
};

u32 defaultCubeIndices[] =
{
    // Left
    0, 2, 1, 0, 3, 2,
    // Right
    4, 6, 5, 4, 7, 6,
    // Top
    2, 6, 7, 2, 3, 6,
    // Bottom
    0, 4, 5, 0, 1, 4,
    // Back
    1, 7, 4, 1, 2, 7,
    // Front
    5, 3, 0, 5, 6, 3,
};

u32 defaultWhiteTextureData = 0xFFFFFFFF;

const char* screenQuadVSSrc =
"#version 460 core\n"
"layout (location = 0) in vec2 vIn_position;\n"
"layout (location = 1) in vec2 vIn_texcoord;\n"
"\n"
"out vec2 vOut_texcoord;\n"
"\n"
"void main()\n"
"{\n"
"    gl_Position = vec4(vIn_position.xy, 0.0, 1.0);\n"
"    vOut_texcoord = vIn_texcoord;\n"
"}\n\0";

const char* screenQuadPSSrc =
"#version 460 core\n"
"\n"
"in vec2 vOut_texcoord;\n"
"\n"
"layout (binding = 0) uniform sampler2D u_inputTexture;\n"
"\n"
"out vec4 pOut_color;\n"
"\n"
"void main()\n"
"{\n"
"    pOut_color = texture(u_inputTexture, vOut_texcoord);\n"
"}\n\0";

void GLAPIENTRY
GLMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
        const GLchar* message, const void* userParam)
{
    ASSERTF(type != GL_DEBUG_TYPE_ERROR, "[OPENGL ERROR]: %s", message);
}

m4f Camera::GetView()
{
    return LookAtMatrix(position, position + front, up);
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
    // Rotate front along axis
    m4f rotationMatrix = RotationMatrix(rotationAngle, rotationAxis);
    v4f newFront = rotationMatrix * v4f{front.x, front.y, front.z, 0};
    if(ABS(TO_DEG(asinf(newFront.y))) >= 85.f) return;  // Preventing too much yaw rotation
    front = Normalize(v3f{newFront.x, newFront.y, newFront.z});

    // Recalculate camera basis after rotation
    right = Normalize(Cross(front, {0.f, 1.f, 0.f}));
    //up = Normalize(Cross(right, front));
}

void Camera::RotateYaw(f32 angle)
{
    Rotate(angle, right);
}

void Camera::RotatePitch(f32 angle)
{
    Rotate(angle, up);
}

Handle<Buffer> Renderer_CreateBuffer(BufferType type, u64 count, u64 stride, u8* data)
{
    APIHandle glHandle = API_HANDLE_INVALID;
    glGenBuffers(1, &glHandle);
    ASSERT(glHandle != API_HANDLE_INVALID);

    GLuint glBindType;
    switch(type)
    {
        case BUFFER_TYPE_VERTEX: glBindType = GL_ARRAY_BUFFER; break;
        case BUFFER_TYPE_INDEX:  glBindType = GL_ELEMENT_ARRAY_BUFFER; break;
        default: ASSERT(0);
    }

    glBindVertexArray(0);
    glBindBuffer(glBindType, glHandle);
    glBufferData(glBindType, count * stride, data, GL_STATIC_DRAW);

    Buffer* buffer = new Buffer();
    *buffer =
    {
        glHandle, type, count, stride, data
    };

    renderResourceTable.bufferResources.push_back(buffer);
    return { (u32)renderResourceTable.bufferResources.size() - 1 };
}

Handle<Texture> Renderer_CreateTexture(TextureFormat format, TextureParams parameters, u32 width, u32 height, u8* data)
{
    APIHandle glHandle = API_HANDLE_INVALID;
    glGenTextures(1, &glHandle);
    ASSERT(glHandle != API_HANDLE_INVALID);
    glBindTexture(GL_TEXTURE_2D, glHandle);

    GLuint glWrapS, glWrapT, glFilterMin, glFilterMag;
    GLuint glInternalFormat, glFormat, glDataType;
    switch(parameters.wrapMode)
    {
        case TEXTURE_WRAP_REPEAT: glWrapS = glWrapT = GL_REPEAT; break;
        case TEXTURE_WRAP_CLAMP: glWrapS = glWrapT = GL_CLAMP_TO_EDGE; break;
        default: ASSERT(0);
    }

    switch(parameters.filterMode_Min)
    {
        case TEXTURE_FILTER_LINEAR: glFilterMin = parameters.useMips ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR; break;
        case TEXTURE_FILTER_NEAREST: glFilterMin = parameters.useMips ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST; break;
        default: ASSERT(0);
    }
    
    switch(parameters.filterMode_Max)
    {
        case TEXTURE_FILTER_LINEAR: glFilterMag = GL_LINEAR; break;
        case TEXTURE_FILTER_NEAREST: glFilterMag = GL_NEAREST; break;
        default: ASSERT(0);
    }
    
    switch(format)
    {
        case TEXTURE_FORMAT_R8:
            {
                glInternalFormat = GL_R8;
                glFormat = GL_RED;
                glDataType = GL_UNSIGNED_BYTE;
            }; break;
        case TEXTURE_FORMAT_RG8:
            {
                glInternalFormat = GL_RG8;
                glFormat = GL_RG;
                glDataType = GL_UNSIGNED_BYTE;
            }; break;
        case TEXTURE_FORMAT_RGB8:
            {
                glInternalFormat = GL_RGB8;
                glFormat = GL_RGB;
                glDataType = GL_UNSIGNED_BYTE;
            }; break;
        case TEXTURE_FORMAT_RGBA8:
            {
                glInternalFormat = GL_RGBA8;
                glFormat = GL_RGBA;
                glDataType = GL_UNSIGNED_BYTE;
            }; break;
        case TEXTURE_FORMAT_RGBA16F:
            {
                glInternalFormat = GL_RGBA16F;
                glFormat = GL_RGBA;
                glDataType = GL_FLOAT;
            } break;
        case TEXTURE_FORMAT_D32:
            {
                glInternalFormat = GL_DEPTH_COMPONENT32;
                glFormat = GL_DEPTH_COMPONENT;
                glDataType = GL_FLOAT;
            } break;
        default: ASSERT(0);
    }


    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glWrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glWrapT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glFilterMin);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glFilterMag);

    glTexImage2D(GL_TEXTURE_2D, 0, glInternalFormat, width, height, 0, glFormat, glDataType, data);

    if(parameters.useMips)
        glGenerateMipmap(GL_TEXTURE_2D);

    Texture* texture = new Texture();
    *texture =
    {
        glHandle, format, parameters, width, height, data
    };

    renderResourceTable.textureResources.push_back(texture);
    return { (u32)renderResourceTable.textureResources.size() - 1 };
}

Handle<Cubemap> Renderer_CreateCubemap(TextureFormat format, TextureParams parameters, u32 width, u32 height, u8* data[CUBEMAP_FACES])
{
    APIHandle glHandle = API_HANDLE_INVALID;
    glGenTextures(1, &glHandle);
    ASSERT(glHandle != API_HANDLE_INVALID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, glHandle);

    GLuint glWrapS, glWrapT, glWrapR, glFilterMin, glFilterMag;
    GLuint glInternalFormat, glFormat, glDataType;
    switch(parameters.wrapMode)
    {
        case TEXTURE_WRAP_REPEAT: glWrapS = glWrapT = glWrapR = GL_REPEAT; break;
        case TEXTURE_WRAP_CLAMP: glWrapS = glWrapT = glWrapR = GL_CLAMP_TO_EDGE; break;
        default: ASSERT(0);
    }

    switch(parameters.filterMode_Min)
    {
        case TEXTURE_FILTER_LINEAR: glFilterMin = parameters.useMips ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR; break;
        case TEXTURE_FILTER_NEAREST: glFilterMin = parameters.useMips ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST; break;
        default: ASSERT(0);
    }
    
    switch(parameters.filterMode_Max)
    {
        case TEXTURE_FILTER_LINEAR: glFilterMag = GL_LINEAR; break;
        case TEXTURE_FILTER_NEAREST: glFilterMag = GL_NEAREST; break;
        default: ASSERT(0);
    }
    
    switch(format)
    {
        case TEXTURE_FORMAT_R8:
            {
                glInternalFormat = GL_R8;
                glFormat = GL_RED;
                glDataType = GL_UNSIGNED_BYTE;
            }; break;
        case TEXTURE_FORMAT_RG8:
            {
                glInternalFormat = GL_RG8;
                glFormat = GL_RG;
                glDataType = GL_UNSIGNED_BYTE;
            }; break;
        case TEXTURE_FORMAT_RGB8:
            {
                glInternalFormat = GL_RGB8;
                glFormat = GL_RGB;
                glDataType = GL_UNSIGNED_BYTE;
            }; break;
        case TEXTURE_FORMAT_RGBA8:
            {
                glInternalFormat = GL_RGBA8;
                glFormat = GL_RGBA;
                glDataType = GL_UNSIGNED_BYTE;
            }; break;
        case TEXTURE_FORMAT_RGBA16F:
            {
                glInternalFormat = GL_RGBA16F;
                glFormat = GL_RGBA;
                glDataType = GL_FLOAT;
            } break;

        default: ASSERT(0);
    }


    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, glWrapS);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, glWrapT);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, glWrapR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, glFilterMin);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, glFilterMag);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0);

    // https://www.khronos.org/opengl/wiki/Cubemap_Texture
    // Cubemap orientation is very confusing...
    for(i32 i = 0; i < CUBEMAP_FACES; i++)
    {
        u8* faceData = data[i];
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, glInternalFormat, width, height, 0, glFormat, glDataType, faceData);
    }

    // TODO(caio)#RENDER: Implement support for mipmaps on cubemaps.
    //if(parameters.useMips)
        //glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    Cubemap* cubemap = new Cubemap();
    *cubemap =
    {
        glHandle, format, parameters, width, height
    };
    for(i32 i = 0; i < CUBEMAP_FACES; i++)
    {
        u8* faceData = data[i];
        cubemap->data[i] = faceData;
    }

    renderResourceTable.cubemapResources.push_back(cubemap);
    return { (u32)renderResourceTable.cubemapResources.size() - 1 };
}

Handle<Mesh> Renderer_CreateMesh(Handle<Buffer> h_vertexBuffer, Handle<Buffer> h_indexBuffer, VertexLayout vertexLayout)
{
    APIHandle glHandle = API_HANDLE_INVALID;
    glGenVertexArrays(1, &glHandle);
    ASSERT(glHandle != API_HANDLE_INVALID);

    glBindVertexArray(glHandle);
    glBindBuffer(GL_ARRAY_BUFFER, Renderer_GetBuffer(h_vertexBuffer)->apiHandle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Renderer_GetBuffer(h_indexBuffer)->apiHandle);

    // Finding vertex layout stride first
    u64 vertexLayoutStride = 0;
    for(i32 i = 0; i < vertexLayout.count; i++)
    {
        VertexAttributeType attr = vertexLayout.attributes[i];
        switch(attr)
        {
            case VERTEX_ATTRIBUTE_FLOAT: vertexLayoutStride += sizeof(f32); break;
            case VERTEX_ATTRIBUTE_VEC2: vertexLayoutStride += sizeof(v2f); break;
            case VERTEX_ATTRIBUTE_VEC3: vertexLayoutStride += sizeof(v3f); break;
            default: ASSERT(0);
        }
    }
    // Now properly define vertex layout
    u64 currentLayoutOffset = 0;
    for(i32 i = 0; i < vertexLayout.count; i++)
    {
        VertexAttributeType attr = vertexLayout.attributes[i];
        switch(attr)
        {
            case VERTEX_ATTRIBUTE_FLOAT:
                {
                    glVertexAttribPointer(i, 1, GL_FLOAT, GL_FALSE, vertexLayoutStride, (void*)currentLayoutOffset);
                    currentLayoutOffset += sizeof(f32);
                } break;
            case VERTEX_ATTRIBUTE_VEC2:
                {
                    glVertexAttribPointer(i, 2, GL_FLOAT, GL_FALSE, vertexLayoutStride, (void*)currentLayoutOffset);
                    currentLayoutOffset += sizeof(v2f);
                } break;
            case VERTEX_ATTRIBUTE_VEC3:
                {
                    glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, vertexLayoutStride, (void*)currentLayoutOffset);
                    currentLayoutOffset += sizeof(v3f);
                } break;
            default: ASSERT(0);
        }
        glEnableVertexAttribArray(i);
    }

    Mesh* mesh = new Mesh();
    *mesh =
    {
        glHandle, h_vertexBuffer, h_indexBuffer
    };

    renderResourceTable.meshResources.push_back(mesh);
    return { (u32)renderResourceTable.meshResources.size() - 1 };
}

void Renderer_CompileShaderStage(Handle<ShaderStage> h_shaderStage, std::string_view src)
{
    ShaderStage* shaderStage = Renderer_GetShaderStage(h_shaderStage);
    ASSERT(shaderStage->apiHandle != API_HANDLE_INVALID);
    const char* shaderSrcCstr = src.data();
    glShaderSource(shaderStage->apiHandle, 1, &shaderSrcCstr, NULL);
    glCompileShader(shaderStage->apiHandle);

    // Checking for compile errors
    i32 ret;
    glGetShaderiv(shaderStage->apiHandle, GL_COMPILE_STATUS, &ret);
    if(!ret)
    {
        char errorLog[1024];
        glGetShaderInfoLog(shaderStage->apiHandle, 1024, NULL, errorLog);
        ASSERTF(0, "%s shader compilation error: %s",
                shaderStage->type == SHADERSTAGE_TYPE_VERTEX ? "Vertex" : "Pixel",
                errorLog);
    }
}

Handle<ShaderStage> Renderer_CreateShaderStage(ShaderStageType type, std::string_view src)
{
    APIHandle glHandle = API_HANDLE_INVALID;
    switch(type)
    {
        case SHADERSTAGE_TYPE_VERTEX: glHandle = glCreateShader(GL_VERTEX_SHADER); break;
        case SHADERSTAGE_TYPE_PIXEL: glHandle = glCreateShader(GL_FRAGMENT_SHADER); break;
        default: ASSERT(0);
    }
    ASSERT(glHandle != API_HANDLE_INVALID);

    ShaderStage* shaderStage = new ShaderStage();
    *shaderStage =
    {
        glHandle, type
    };
    renderResourceTable.shaderStageResources.push_back(shaderStage);
    Handle<ShaderStage> h_shaderStage = { (u32)renderResourceTable.shaderStageResources.size() - 1 };

    // Compilation is done after inserting into resource array for recompilation purposes
    Renderer_CompileShaderStage(h_shaderStage, src);

    return h_shaderStage;
}

void Renderer_LinkShader(Handle<Shader> h_shader)
{    
    // OpenGL linking requires new shader programs to be created every time.
    Shader* shader = Renderer_GetShader(h_shader);
    
    // Delete previous program if existing
    if(shader->apiHandle != API_HANDLE_INVALID)
    {
        glDeleteProgram(shader->apiHandle);
    }

    // Create and link new shader program
    APIHandle glHandle = glCreateProgram();
    ASSERT(glHandle != API_HANDLE_INVALID);
    shader->apiHandle = glHandle;

    ShaderStage* h_vs = Renderer_GetShaderStage(shader->h_VS);
    ShaderStage* h_ps = Renderer_GetShaderStage(shader->h_PS);

    glAttachShader(shader->apiHandle, h_vs->apiHandle);
    glAttachShader(shader->apiHandle, h_ps->apiHandle);
    glLinkProgram(shader->apiHandle);
    i32 ret;
    glGetProgramiv(shader->apiHandle, GL_LINK_STATUS, &ret);
    if(!ret)
    {
        char errorLog[1024];
        glGetProgramInfoLog(shader->apiHandle, 1024, NULL, errorLog);
        ASSERTF(0, "Shader program linking error: %s",
                errorLog);
    }
}

Handle<Shader> Renderer_CreateShader(Handle<ShaderStage> h_vertexShader, Handle<ShaderStage> h_pixelShader)
{
    Shader* shader = new Shader();
    *shader =
    {
        API_HANDLE_INVALID, h_vertexShader, h_pixelShader
    };

    renderResourceTable.shaderResources.push_back(shader);
    Handle<Shader> h_shader = { (u32)renderResourceTable.shaderResources.size() - 1 };

    // Linking is done after inserting to resource array, for recompilation purposes.
    Renderer_LinkShader(h_shader);

    return h_shader;
};

Handle<Material> Renderer_CreateMaterial(u8 texturesCount, Handle<Texture>* h_textures)
{
    Material* material = new Material();
    material->count = texturesCount;
    for(i32 i = 0; i < texturesCount; i++)
    {
        material->h_Textures[i] = h_textures[i];
    }

    renderResourceTable.materialResources.push_back(material);
    return { (u32)renderResourceTable.materialResources.size() - 1 };
}

Handle<RenderTarget> Renderer_CreateRenderTarget(u32 width, u32 height, u8 outputsCount, RenderTargetOutputDesc* outputsDesc)
{
    Handle<Texture> outputs[RENDER_TARGET_MAX_OUTPUTS];
    for(i32 i = 0; i < outputsCount; i++)
    {
        outputs[i] = Renderer_CreateTexture(outputsDesc[i].format, outputsDesc[i].params, width, height, NULL);
        ASSERT(outputs[i].IsValid());
    }

    // TODO(caio)#RENDER: Not sure if passing parameters other than these for depth textures makes sense,
    // so leaving it like this for now. If needed, add option to pass depth parameters.
    Handle<Texture> depthOutput = Renderer_CreateTexture(TEXTURE_FORMAT_D32, 
            { TEXTURE_WRAP_CLAMP, TEXTURE_FILTER_NEAREST, TEXTURE_FILTER_NEAREST, false }, width, height, NULL);
    ASSERT(depthOutput.IsValid());

    // Creating render target resource
    APIHandle glHandle = API_HANDLE_INVALID;
    glGenFramebuffers(1, &glHandle);
    ASSERT(glHandle != API_HANDLE_INVALID);
    glBindFramebuffer(GL_FRAMEBUFFER, glHandle);

    // TODO(caio)#RENDER: Support depth-only render targets, without color attachments.
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, Renderer_GetTexture(depthOutput)->apiHandle, 0);

    GLenum glAttachments[outputsCount];
    for(i32 i = 0; i < outputsCount; i++)
    {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, Renderer_GetTexture(outputs[i])->apiHandle, 0);
        glAttachments[i] = GL_COLOR_ATTACHMENT0 + i;
    }
    glDrawBuffers(outputsCount, glAttachments);

    ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    RenderTarget* renderTarget = new RenderTarget();
    renderTarget->apiHandle = glHandle;
    renderTarget->depthOutput = depthOutput;
    renderTarget->width = width;
    renderTarget->height = height;
    renderTarget->outputsCount = outputsCount;
    for(i32 i = 0; i < outputsCount; i++)
    {
        renderTarget->outputs[i] = outputs[i];
    }

    renderResourceTable.renderTargetResources.push_back(renderTarget);
    return { (u32)renderResourceTable.renderTargetResources.size() - 1 };
}

Handle<RenderTarget> Renderer_GetDefaultRenderTarget()
{
    ASSERT(h_defaultRenderTarget.IsValid());
    return h_defaultRenderTarget;
}

void Renderer_BindUniform_i32(std::string_view name, i32 value)
{
    ASSERT(renderState.h_activeShader.IsValid());
    GLuint shaderAPIHandle = Renderer_GetShader(renderState.h_activeShader)->apiHandle;
    GLint location = glGetUniformLocation(shaderAPIHandle, name.data());
    ASSERT(location != -1);
    glUniform1i(location, value);
}

void Renderer_BindUniform_u32(std::string_view name, u32 value)
{
    ASSERT(renderState.h_activeShader.IsValid());
    GLuint shaderAPIHandle = Renderer_GetShader(renderState.h_activeShader)->apiHandle;
    GLint location = glGetUniformLocation(shaderAPIHandle, name.data());
    ASSERT(location != -1);
    glUniform1ui(location, value);
}

void Renderer_BindUniform_f32(std::string_view name, f32 value)
{
    ASSERT(renderState.h_activeShader.IsValid());
    GLuint shaderAPIHandle = Renderer_GetShader(renderState.h_activeShader)->apiHandle;
    GLint location = glGetUniformLocation(shaderAPIHandle, name.data());
    ASSERT(location != -1);
    glUniform1f(location, value);
}

void Renderer_BindUniform_v2f(std::string_view name, v2f value)
{
    ASSERT(renderState.h_activeShader.IsValid());
    GLuint shaderAPIHandle = Renderer_GetShader(renderState.h_activeShader)->apiHandle;
    GLint location = glGetUniformLocation(shaderAPIHandle, name.data());
    ASSERT(location != -1);
    glUniform2f(location, value.x, value.y);
}

void Renderer_BindUniform_v3f(std::string_view name, v3f value)
{
    ASSERT(renderState.h_activeShader.IsValid());
    GLuint shaderAPIHandle = Renderer_GetShader(renderState.h_activeShader)->apiHandle;
    GLint location = glGetUniformLocation(shaderAPIHandle, name.data());
    ASSERT(location != -1);
    glUniform3f(location, value.x, value.y, value.z);
}

void Renderer_BindUniform_m4f(std::string_view name, m4f value)
{
    ASSERT(renderState.h_activeShader.IsValid());
    GLuint shaderAPIHandle = Renderer_GetShader(renderState.h_activeShader)->apiHandle;
    GLint location = glGetUniformLocation(shaderAPIHandle, name.data());
    ASSERT(location != -1);
    glUniformMatrix4fv(location, 1, GL_TRUE, &value.m00);
}


void Renderer_BindRenderTarget(Handle<RenderTarget> h_renderTarget)
{
    ASSERT(h_renderTarget.IsValid());
    RenderTarget* renderTarget = Renderer_GetRenderTarget(h_renderTarget);
    glBindFramebuffer(GL_FRAMEBUFFER, renderTarget->apiHandle);
    renderState.h_activeRenderTarget = h_renderTarget;
    Renderer_SetViewport({0, 0, renderTarget->width, renderTarget->height});
}

void Renderer_UnbindRenderTarget()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    renderState.h_activeRenderTarget = { HANDLE_INVALID };
    Renderer_SetViewport({0, 0, renderState.window->width, renderState.window->height});
}

void Renderer_BindMesh(Handle<Mesh> h_mesh)
{
    ASSERT(h_mesh.IsValid());
    glBindVertexArray(Renderer_GetMesh(h_mesh)->apiHandle);
    renderState.h_activeMesh = h_mesh;
}

void Renderer_BindShader(Handle<Shader> h_shader)
{
    ASSERT(h_shader.IsValid());
    glUseProgram(Renderer_GetShader(h_shader)->apiHandle);
    renderState.h_activeShader = h_shader;
}

void Renderer_BindTexture(Handle<Texture> h_texture, u32 slot)
{
    u32 bindTarget = h_texture.IsValid() 
        ? Renderer_GetTexture(h_texture)->apiHandle
        : Renderer_GetTexture(h_defaultWhiteTexture)->apiHandle;
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, bindTarget);
}

void Renderer_BindCubemap(Handle<Cubemap> h_cubemap, u32 slot)
{
    u32 bindTarget = h_cubemap.IsValid()
        ? Renderer_GetCubemap(h_cubemap)->apiHandle
        : Renderer_GetTexture(h_defaultWhiteTexture)->apiHandle;    // No idea if this works...
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_CUBE_MAP, bindTarget);
}

void Renderer_BindMaterial(Handle<Material> h_material)
{
    ASSERT(h_material.IsValid());
    Material* material = Renderer_GetMaterial(h_material);
    for(i32 i = 0; i < material->count; i++)
    {
        Renderer_BindTexture(material->h_Textures[i], i);
    }
    renderState.h_activeMaterial = h_material;
}

void Renderer_SetDepthCompare(DepthCompare func)
{
    GLenum depthFunc;
    switch(func)
    {
        case DEPTH_COMPARE_NEVER:   depthFunc = GL_NEVER; break;
        case DEPTH_COMPARE_ALWAYS:  depthFunc = GL_ALWAYS; break;
        case DEPTH_COMPARE_LT:      depthFunc = GL_LESS; break;
        case DEPTH_COMPARE_LE:      depthFunc = GL_LEQUAL; break;
        case DEPTH_COMPARE_GT:      depthFunc = GL_GREATER; break;
        case DEPTH_COMPARE_GE:      depthFunc = GL_GEQUAL; break;
        case DEPTH_COMPARE_E:       depthFunc = GL_EQUAL; break;
        case DEPTH_COMPARE_NE:      depthFunc = GL_NOTEQUAL; break;
        default: ASSERT(0);
    }
    glDepthFunc(depthFunc);
}

void Renderer_SetCamera(Camera camera)
{
    renderState.camera = camera;
}

void Renderer_SetViewport(RenderViewport viewport)
{
    renderState.viewport = viewport;
    glViewport(
        renderState.viewport.bottomLeft.x,
        renderState.viewport.bottomLeft.y,
        renderState.viewport.width,
        renderState.viewport.height
        );
}

void Renderer_Clear(v4f clearColor)
{
    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer_GenerateMips(Handle<Texture> h_texture)
{
    ASSERT(h_texture.IsValid());
    Texture* texture = Renderer_GetTexture(h_texture);
    if(!texture->params.useMips) return;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture->apiHandle);
    glGenerateMipmap(GL_TEXTURE_2D);
}

void Renderer_GenerateMipsForRenderTarget(Handle<RenderTarget> h_renderTarget)
{
    ASSERT(h_renderTarget.IsValid());
    RenderTarget* renderTarget = Renderer_GetRenderTarget(h_renderTarget);
    for(i32 i = 0; i < renderTarget->outputsCount; i++)
    {
        Renderer_GenerateMips(renderTarget->outputs[i]);
    }
}

void Renderer_DrawMesh()
{
    ASSERT(renderState.h_activeMesh.IsValid());
    Mesh* mesh = Renderer_GetMesh(renderState.h_activeMesh);
    Buffer* ib = Renderer_GetBuffer(mesh->h_IndexBuffer);
    glDrawElements(GL_TRIANGLES, ib->count, GL_UNSIGNED_INT, 0);
}

void Renderer_CopyTextureToBackbuffer(Handle<Texture> h_texture)
{
    ASSERT(h_texture.IsValid());
    Renderer_UnbindRenderTarget();
    Renderer_Clear({1.f, 1.f, 1.f, 1.f});
    Renderer_SetViewport({0, 0, renderState.window->width, renderState.window->height});

    // TODO(caio)#RENDER: There's probably a better way to do this copy to backbuffer
    Material* material = Renderer_GetMaterial(h_screenQuadMaterial);
    material->count = 1;
    material->h_Textures[0] = h_texture;

    Renderer_BindShader(h_screenQuadShader);
    Renderer_BindMaterial(h_screenQuadMaterial);
    Renderer_BindMesh(h_screenQuadMesh);
    Renderer_DrawMesh();
}

void Renderer_CopyRenderTargetOutputToBackbuffer(Handle<RenderTarget> h_renderTarget, u8 outputIndex)
{
    ASSERT(h_renderTarget.IsValid());

    Handle<Texture> h_texture = Renderer_GetRenderTarget(h_renderTarget)->outputs[outputIndex];
    Renderer_CopyTextureToBackbuffer(h_texture);
}

void Renderer_CopyRenderTargetDepthOutputToBackbuffer(Handle<RenderTarget> h_renderTarget)
{
    ASSERT(h_renderTarget.IsValid());

    Handle<Texture> h_texture = Renderer_GetRenderTarget(h_renderTarget)->depthOutput;
    Renderer_CopyTextureToBackbuffer(h_texture);
}

void Renderer_CopyRenderTargetOutput(Handle<RenderTarget> h_src, Handle<RenderTarget> h_dest)
{
    RenderTarget* src = Renderer_GetRenderTarget(h_src);
    RenderTarget* dest = Renderer_GetRenderTarget(h_dest);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, src->apiHandle);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest->apiHandle);
    glBlitFramebuffer(
            0, 0, src->width, src->height,
            0, 0, dest->width, dest->height,
            GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void Renderer_CopyRenderTargetDepth(Handle<RenderTarget> h_src, Handle<RenderTarget> h_dest)
{
    RenderTarget* src = Renderer_GetRenderTarget(h_src);
    RenderTarget* dest = Renderer_GetRenderTarget(h_dest);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, src->apiHandle);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest->apiHandle);
    glBlitFramebuffer(
            0, 0, src->width, src->height,
            0, 0, dest->width, dest->height,
            GL_DEPTH_BUFFER_BIT, GL_NEAREST);
}

void Renderer_Init(u32 windowWidth, u32 windowHeight, const char* windowName, Window* outWindow)
{
    // Initializing window to render on
    ASSERT(outWindow);
    Window_Init(windowWidth, windowHeight, windowName, outWindow);
    Window_InitRenderContext(*outWindow);
    Window_Show(*outWindow);

    // Render profiling hooks
    PROFILE_GPU_CONTEXT;

    // OpenGL settings
    glEnable(GL_DEPTH_TEST);            // Depth testing
    glEnable(GL_CULL_FACE);             // Face culling
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
#if _DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(GLMessageCallback, 0);
#endif

    // Initializing render state
    renderState.window = outWindow;
    Renderer_SetViewport({{0,0}, windowWidth, windowHeight});
    Renderer_SetDepthCompare(DEPTH_COMPARE_LT);
    renderState.h_activeRenderTarget    = { HANDLE_INVALID };
    renderState.h_activeShader          = { HANDLE_INVALID };
    renderState.h_activeMaterial        = { HANDLE_INVALID };
    renderState.h_activeMesh            = { HANDLE_INVALID };

    // Creating default resources
    RenderTargetOutputDesc renderTargetDefaultOutputsDesc[] =
    {
        { TEXTURE_FORMAT_RGBA8, TEXTURE_WRAP_CLAMP, TEXTURE_FILTER_LINEAR, TEXTURE_FILTER_NEAREST },
    };
    h_defaultRenderTarget = Renderer_CreateRenderTarget(1920, 1080, 1, renderTargetDefaultOutputsDesc);

    h_screenQuadVS = Renderer_CreateShaderStage(SHADERSTAGE_TYPE_VERTEX, screenQuadVSSrc);
    h_screenQuadPS = Renderer_CreateShaderStage(SHADERSTAGE_TYPE_PIXEL, screenQuadPSSrc);
    h_screenQuadShader = Renderer_CreateShader(h_screenQuadVS, h_screenQuadPS);

    h_screenQuadMaterial = Renderer_CreateMaterial(0, NULL);   // This material is dynamic and textures are set on CopyToBackbuffer command

    Handle<Buffer> h_screenQuadVB = Renderer_CreateBuffer(BUFFER_TYPE_VERTEX, ArrayCount(screenQuadVertices), sizeof(f32), (u8*)screenQuadVertices);
    Handle<Buffer> h_screenQuadIB = Renderer_CreateBuffer(BUFFER_TYPE_INDEX, ArrayCount(screenQuadIndices), sizeof(u32), (u8*)screenQuadIndices);
    h_screenQuadMesh = Renderer_CreateMesh(h_screenQuadVB, h_screenQuadIB,
            {2, { VERTEX_ATTRIBUTE_VEC2, VERTEX_ATTRIBUTE_VEC2 }});
    
    Handle<Buffer> h_defaultCubeVB = Renderer_CreateBuffer(BUFFER_TYPE_VERTEX, ArrayCount(defaultCubeVertices), sizeof(f32), (u8*)defaultCubeVertices);
    Handle<Buffer> h_defaultCubeIB = Renderer_CreateBuffer(BUFFER_TYPE_INDEX, ArrayCount(defaultCubeIndices), sizeof(u32), (u8*)defaultCubeIndices);
    h_defaultCubeMesh = Renderer_CreateMesh(h_defaultCubeVB, h_defaultCubeIB,
            {1, { VERTEX_ATTRIBUTE_VEC3 }});    // This will change later when default cube is more useful.

    h_defaultWhiteTexture = Renderer_CreateTexture(TEXTURE_FORMAT_RGBA8, {}, 1, 1, (u8*)&defaultWhiteTextureData);
}

void Renderer_BeginFrame()
{
    renderState.h_activeRenderTarget    = { HANDLE_INVALID };
    renderState.h_activeShader          = { HANDLE_INVALID };
    renderState.h_activeMaterial        = { HANDLE_INVALID };
    renderState.h_activeMesh            = { HANDLE_INVALID };
}

void Renderer_EndFrame()
{
    Window_SwapBuffers(*renderState.window);
    PROFILE_GPU_FRAME;
}

} // namespace Ty
