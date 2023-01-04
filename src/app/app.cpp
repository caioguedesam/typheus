#include "app/app.hpp"
#include "engine/renderer/renderer.hpp"

namespace Ty
{

std::string srcVS_Default;
std::string srcPS_Default;

std::vector<Handle<MeshRenderable>> h_Renderables_Sponza;
std::vector<Handle<MeshRenderable>> h_Renderables_Bunny0;
std::vector<Handle<MeshRenderable>> h_Renderables_Bunny1;
std::vector<Handle<MeshRenderable>> h_Renderables_Bunny2;
std::vector<Handle<MeshRenderable>> h_Renderables_Bunny3;

struct GameObject
{
    std::vector<Handle<MeshRenderable>> h_Renderables;

    void SetTransform(m4f transformMatrix)
    {
        for(i32 i = 0; i < h_Renderables.size(); i++)
        {
            Renderer_GetMeshRenderable(h_Renderables[i]).u_Model = transformMatrix;
        }
    }
};

void SpawnRandomBunny(Handle<ShaderPipeline> h_Shader)
{
    GameObject bunny;
    bunny.h_Renderables = Renderer_CreateRenderablesFromModel(MODELS_PATH"bunny/bunny.obj", h_Shader);
    v3f pos =
    {
        RandomRange(-2.f, 2.f),
        RandomRange(-2.f, 2.f),
        RandomRange(-2.f, 2.f),
    };
    f32 angle = RandomRange(0.f, 360.f);
    v3f axis =
    {
        RandomRange(0.f, 1.f),
        RandomRange(0.f, 1.f),
        RandomRange(0.f, 1.f),
    };
    axis = Normalize(axis);
    f32 scale = RandomRange(0.2f, 1.5f);
    bunny.SetTransform(TranslationMatrix(pos) * RotationMatrix(angle, axis) * ScaleMatrix({scale, scale, scale}));
}

GameObject sponza;
std::vector<GameObject> bunnies;

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
    Handle<Shader> h_VS_Default = Renderer_CreateShader(srcVS_Default, SHADER_TYPE_VERTEX);
    Handle<Shader> h_PS_Default = Renderer_CreateShader(srcPS_Default, SHADER_TYPE_PIXEL);
    Handle<ShaderPipeline> h_Shader_Default = Renderer_CreateShaderPipeline(h_VS_Default, h_PS_Default);

    sponza.h_Renderables = Renderer_CreateRenderablesFromModel(MODELS_PATH"sponza/sponza.obj", h_Shader_Default);
    m4f sponzaWorld = RotationMatrix(TO_RAD(90.f), {0.f, 1.f, 0.f}) * ScaleMatrix({0.01f, 0.01f, 0.01f});
    sponza.SetTransform(sponzaWorld);

    for(i32 i = 0; i < 20; i++)
    {
        SpawnRandomBunny(h_Shader_Default);
    }

    // App settings
    Input_LockMouse(true);
    Input_ShowMouse(false);
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

    // App controls
    if(Input_IsKeyJustDown(KEY_ESCAPE))
    {
        appWindow.shouldClose = true;
    }
    if(Input_IsKeyJustDown(KEY_P))
    {
        Input_ToggleLockMouse();
        Input_ToggleShowMouse();
    }
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
