#include "glad/glad.h"
//#ifndef STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"
//#endif

#include "core/file.hpp"
#include "render/renderer.hpp"
#include "app.hpp"

namespace Ty
{

MemArena memArena_Shader;

u32 vsHandle = MAX_U32;
u32 psHandle = MAX_U32;
u32 shaderProgramHandle = MAX_U32;

u32 quadVBO = MAX_U32;
u32 quadEBO = MAX_U32;
u32 quadVAO = MAX_U32;

f32 quadVertices[] =
{
   -0.5f,  -0.5f,   0.f,
    0.5f,  -0.5f,   0.f,
    0.5f,   0.5f,   0.f,
   -0.5f,   0.5f,   0.f,
};

u32 quadIndices[] =
{
    0, 1, 2,
    0, 2, 3,
};

#define RESOURCE_PATH "../resources/"
#define SHADER_PATH RESOURCE_PATH"shaders/"
#define TEXTURE_PATH RESOURCE_PATH"textures/"
#define MODELS_PATH RESOURCE_PATH"models/"

void GLAPIENTRY
GLMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
        const GLchar* message, const void* userParam)
{
    ASSERTF(type != GL_DEBUG_TYPE_ERROR, "[OPENGL ERROR]: %s", message);
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
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glGenBuffers(1, &quadEBO);

    glBindVertexArray(quadVAO);

    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, ArraySize(quadVertices), quadVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, ArraySize(quadIndices), quadIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(f32), (GLvoid*)0);
    glEnableVertexAttribArray(0);
}

void RenderFrame()
{
    glClearColor(1.f, 0.5f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // TODO(caio)#RENDER: This is temporary code before setting up proper object drawing.
    glUseProgram(shaderProgramHandle);
    glBindVertexArray(quadVAO);

    glDrawElements(GL_TRIANGLES, ArrayCount(quadIndices), GL_UNSIGNED_INT, 0);
}

} // namespace Ty
