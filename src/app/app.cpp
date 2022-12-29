#include "app/app.hpp"
#include "engine/renderer/renderer.hpp"

namespace Ty
{

std::vector<MeshVertex> sponzaVertices;
std::vector<u32> sponzaIndices;
std::vector<MeshVertex> bunnyVertices;
std::vector<u32> bunnyIndices;

std::string srcVS_Default;
std::string srcPS_Default;

ResourceHandle h_Renderable_Sponza = RESOURCE_INVALID;
ResourceHandle h_Renderable_Bunny = RESOURCE_INVALID;

#define RESOURCE_PATH "../resources/"
#define SHADER_PATH RESOURCE_PATH"shaders/"
#define TEXTURE_PATH RESOURCE_PATH"textures/"
#define MODELS_PATH RESOURCE_PATH"models/"

void App_Init(u32 windowWidth, u32 windowHeight, const char* appTitle)
{
    // Initializing common engine systems
    Time_Init();

    // Initializing renderer
    Renderer_Init(windowWidth, windowHeight, appTitle, &appWindow);
    Camera mainCamera = {};
    mainCamera.Move({0, 2, 3});
    Renderer_SetCamera(mainCamera);

    // Creating app resources and renderables
    LoadOBJModel(MODELS_PATH"sponza/sponza.obj", &sponzaVertices, &sponzaIndices);
    LoadOBJModel(MODELS_PATH"bunny/bunny.obj", &bunnyVertices, &bunnyIndices);
    ResourceHandle h_SponzaVertexBuffer = Renderer_CreateBuffer((u8*)sponzaVertices.data(), sponzaVertices.size(), sizeof(MeshVertex), BUFFER_TYPE_VERTEX);
    ResourceHandle h_SponzaIndexBuffer = Renderer_CreateBuffer((u8*)sponzaIndices.data(), sponzaIndices.size(), sizeof(u32), BUFFER_TYPE_INDEX);
    ResourceHandle h_SponzaMesh = Renderer_CreateMesh(h_SponzaVertexBuffer, h_SponzaIndexBuffer);
    ResourceHandle h_BunnyVertexBuffer = Renderer_CreateBuffer((u8*)bunnyVertices.data(), bunnyVertices.size(), sizeof(MeshVertex), BUFFER_TYPE_VERTEX);
    ResourceHandle h_BunnyIndexBuffer = Renderer_CreateBuffer((u8*)bunnyIndices.data(), bunnyIndices.size(), sizeof(u32), BUFFER_TYPE_INDEX);
    ResourceHandle h_BunnyMesh = Renderer_CreateMesh(h_BunnyVertexBuffer, h_BunnyIndexBuffer);

    srcVS_Default = ReadFile_Str(SHADER_PATH"default_vertex.vs");
    srcPS_Default = ReadFile_Str(SHADER_PATH"default_pixel.ps");
    ResourceHandle h_VS_Default = Renderer_CreateShader(srcVS_Default, SHADER_TYPE_VERTEX);
    ResourceHandle h_PS_Default = Renderer_CreateShader(srcPS_Default, SHADER_TYPE_PIXEL);
    ResourceHandle h_Shader_Default = Renderer_CreateShaderPipeline(h_VS_Default, h_PS_Default);

    // TODO(caio)#APP: Still no materials for rendering since I haven't loaded textures off the models yet.

    h_Renderable_Sponza = Renderer_CreateRenderable(h_SponzaMesh, h_Shader_Default, RESOURCE_INVALID);
    m4f sponzaWorld = RotationMatrix(TO_RAD(90.f), {0.f, 1.f, 0.f}) * ScaleMatrix({0.01f, 0.01f, 0.01f});
    Renderer_GetRenderable(h_Renderable_Sponza).u_Model = sponzaWorld;
    
    h_Renderable_Bunny = Renderer_CreateRenderable(h_BunnyMesh, h_Shader_Default, RESOURCE_INVALID);
}

void App_Update()
{
    // Update common systems
    Input_UpdateState();

    static f32 bunnyAngle = 0.f;
    bunnyAngle += 0.5f;
    if(bunnyAngle > 360.f) bunnyAngle -= 360.f;
    m4f bunnyWorld = RotationMatrix(TO_RAD(bunnyAngle), {0.f, 1.f, 0.f});
    Renderer_GetRenderable(h_Renderable_Bunny).u_Model = bunnyWorld;
}

void App_Render()
{
    Renderer_RenderFrame();
}

void App_Destroy()
{
    // Destroy app window
    Window_Destroy(appWindow);
}

} // namespace Ty
