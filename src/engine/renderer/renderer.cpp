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

        default: ASSERT(0);
    }


    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, glWrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, glWrapT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glFilterMin);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glFilterMag);

    glBindTexture(GL_TEXTURE_2D, glHandle);
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

    const char* shaderSrcCstr = src.data();
    glShaderSource(glHandle, 1, &shaderSrcCstr, NULL);
    glCompileShader(glHandle);

    // Checking for compile errors
    i32 ret;
    glGetShaderiv(glHandle, GL_COMPILE_STATUS, &ret);
    if(!ret)
    {
        char errorLog[1024];
        glGetShaderInfoLog(glHandle, 1024, NULL, errorLog);
        ASSERTF(0, "%s shader compilation error: %s",
                type == SHADERSTAGE_TYPE_VERTEX ? "Vertex" : "Pixel",
                errorLog);
    }

    ShaderStage* shaderStage = new ShaderStage();
    *shaderStage =
    {
        glHandle, type
    };

    renderResourceTable.shaderStageResources.push_back(shaderStage);
    return { (u32)renderResourceTable.shaderStageResources.size() - 1 };
}

Handle<Shader> Renderer_CreateShader(Handle<ShaderStage> h_vertexShader, Handle<ShaderStage> h_pixelShader)
{
    APIHandle glHandle = API_HANDLE_INVALID;
    glHandle = glCreateProgram();
    ASSERT(glHandle != API_HANDLE_INVALID);
    ShaderStage* vertexShader = Renderer_GetShaderStage(h_vertexShader);
    ShaderStage* pixelShader = Renderer_GetShaderStage(h_pixelShader);

    glAttachShader(glHandle, Renderer_GetShaderStage(h_vertexShader)->apiHandle);
    glAttachShader(glHandle, Renderer_GetShaderStage(h_pixelShader)->apiHandle);
    glLinkProgram(glHandle);
    i32 ret;
    glGetProgramiv(glHandle, GL_LINK_STATUS, &ret);
    if(!ret)
    {
        char errorLog[1024];
        glGetProgramInfoLog(glHandle, 1024, NULL, errorLog);
        ASSERTF(0, "Shader program linking error: %s",
                errorLog);
    }

    Shader* shader = new Shader();
    *shader =
    {
        glHandle, h_vertexShader, h_pixelShader
    };

    renderResourceTable.shaderResources.push_back(shader);
    return { (u32)renderResourceTable.shaderResources.size() - 1 };
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

    // Creating render target resource
    APIHandle glHandle = API_HANDLE_INVALID;
    glGenFramebuffers(1, &glHandle);
    ASSERT(glHandle != API_HANDLE_INVALID);
    glBindFramebuffer(GL_FRAMEBUFFER, glHandle);

    // TODO(caio)#RENDER: This doesn't support stencil buffer yet.
    APIHandle glDepthStencilHandle = API_HANDLE_INVALID;
    glGenRenderbuffers(1, &glDepthStencilHandle);
    ASSERT(glDepthStencilHandle != API_HANDLE_INVALID);
    glBindRenderbuffer(GL_RENDERBUFFER, glDepthStencilHandle);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, glDepthStencilHandle);

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
    renderTarget->depthStencilAPIHandle = glDepthStencilHandle;
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
    renderState.h_activeRenderTarget    = { HANDLE_INVALID };
    renderState.h_activeShader          = { HANDLE_INVALID };
    renderState.h_activeMaterial        = { HANDLE_INVALID };
    renderState.h_activeMesh            = { HANDLE_INVALID };

    // Creating default resources
    RenderTargetOutputDesc renderTargetDefaultOutputsDesc[] =
    {
        { TEXTURE_FORMAT_RGBA8, TEXTURE_WRAP_CLAMP, TEXTURE_FILTER_NEAREST, TEXTURE_FILTER_NEAREST },
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
