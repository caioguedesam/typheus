#include "app/app.hpp"
#include "engine/renderer/renderer.hpp"

namespace Ty
{

std::string srcVS_Default;
std::string srcPS_Default;

std::vector<ResourceHandle> h_Renderables_Sponza;

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
    srcVS_Default = ReadFile_Str(SHADER_PATH"default_vertex.vs");
    srcPS_Default = ReadFile_Str(SHADER_PATH"default_pixel.ps");
    ResourceHandle h_VS_Default = Renderer_CreateShader(srcVS_Default, SHADER_TYPE_VERTEX);
    ResourceHandle h_PS_Default = Renderer_CreateShader(srcPS_Default, SHADER_TYPE_PIXEL);
    ResourceHandle h_Shader_Default = Renderer_CreateShaderPipeline(h_VS_Default, h_PS_Default);

    h_Renderables_Sponza = Renderer_CreateRenderablesFromModel(MODELS_PATH"sponza/sponza.obj", h_Shader_Default);
    m4f sponzaWorld = RotationMatrix(TO_RAD(90.f), {0.f, 1.f, 0.f}) * ScaleMatrix({0.01f, 0.01f, 0.01f});
    for(i32 i = 0; i < h_Renderables_Sponza.size(); i++)
    {
        Renderer_GetRenderable(h_Renderables_Sponza[i]).u_Model = sponzaWorld;
    }
}

void App_Update()
{
    // Update common systems
    Input_UpdateState();

    // Camera controls
    Camera& mainCamera = Renderer_GetCamera();

    const f32 cameraRotationSpeed = 0.01f;
    v2f mouseDelta = Input_GetMouseDelta();
    mainCamera.RotateYaw(cameraRotationSpeed * -mouseDelta.y);
    mainCamera.RotatePitch(cameraRotationSpeed * -mouseDelta.x);

    const f32 cameraSpeed = 0.01f;
    v2f cameraMoveAmount = {};
    if      (Input_IsKeyDown(KEY_W)) cameraMoveAmount.y =  1.f * cameraSpeed;
    else if (Input_IsKeyDown(KEY_S)) cameraMoveAmount.y = -1.f * cameraSpeed;
    if      (Input_IsKeyDown(KEY_A)) cameraMoveAmount.x = -1.f * cameraSpeed;
    else if (Input_IsKeyDown(KEY_D)) cameraMoveAmount.x =  1.f * cameraSpeed;

    mainCamera.Move(mainCamera.position
            + mainCamera.front * cameraMoveAmount.y
            + mainCamera.right * cameraMoveAmount.x);
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
