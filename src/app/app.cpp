#include "app/app.hpp"
#include "engine/common/asset.hpp"
#include "engine/renderer/renderer.hpp"

#define APP_RESOURCE_PATH           "../resources/"
#define APP_RESOURCE_SHADERS_PATH   APP_RESOURCE_PATH"shaders/"
#define APP_RESOURCE_MODELS_PATH    APP_RESOURCE_PATH"models/"

namespace Ty
{

struct RenderUnit
{
    Handle<Material>    h_material;
    Handle<Mesh>        h_mesh;
};

struct RenderObjectProperties
{
    m4f u_world         = Identity();
    v3f u_baseColor     = {1,1,1};
};
void BindRenderObjectProperties(RenderObjectProperties properties)
{
    Renderer_BindUniform_m4f("u_world", properties.u_world);
    Renderer_BindUniform_v3f("u_baseColor", properties.u_baseColor);
}

struct RenderObject
{
    std::vector<RenderUnit> renderUnits;
    RenderObjectProperties properties = {};
};

std::vector<RenderObject*> renderObjects;
std::unordered_map<Handle<AssetShader>, Handle<ShaderStage>, HandleHash<AssetShader>> shaderStageFromAssetCache;
std::unordered_map<Handle<AssetTexture>, Handle<Texture>, HandleHash<AssetTexture>> textureFromAssetCache;
std::unordered_map<Handle<AssetModel>, Handle<RenderObject>, HandleHash<AssetModel>> renderObjectFromModelCache;

Handle<ShaderStage> GetShaderFromAsset(Handle<AssetShader> h_asset, ShaderStageType type)
{
    if(!h_asset.IsValid())              return { HANDLE_INVALID };
    if(shaderStageFromAssetCache.count(h_asset))  return shaderStageFromAssetCache[h_asset];

    Handle<ShaderStage> h_shaderStage = Renderer_CreateShaderStage(type, Asset_GetShader(h_asset)->src);
    shaderStageFromAssetCache[h_asset] = h_shaderStage;
    return h_shaderStage;
}

Handle<Texture> GetTextureFromAsset(Handle<AssetTexture> h_asset)
{
    if(!h_asset.IsValid())          return { HANDLE_INVALID };
    if(textureFromAssetCache.count(h_asset))  return textureFromAssetCache[h_asset];

    AssetTexture* assetTexture = Asset_GetTexture(h_asset);
    TextureFormat format = {};
    TextureParams params = {};
    switch(assetTexture->channels)
    {
        case 1: format = TEXTURE_FORMAT_R8; break;
        case 2: format = TEXTURE_FORMAT_R8G8; break;
        case 3: format = TEXTURE_FORMAT_R8G8B8; break;
        case 4: format = TEXTURE_FORMAT_R8G8B8A8; break;
        default: ASSERT(0);
    }
    params.useMips = true;
    params.wrapMode = TEXTURE_WRAP_REPEAT;
    params.filterMode_Min = TEXTURE_FILTER_LINEAR;
    params.filterMode_Max = TEXTURE_FILTER_LINEAR;

    Handle<Texture> h_texture = Renderer_CreateTexture(format, params, assetTexture->width, assetTexture->height, assetTexture->data);
    textureFromAssetCache[h_asset] = h_texture;
    return h_texture;
}

Handle<RenderObject> CreateRenderObjectFromAsset(Handle<AssetModel> h_asset)
{
    if(!h_asset.IsValid())                  return { HANDLE_INVALID };

    RenderObject* ro = new RenderObject();
    if(renderObjectFromModelCache.count(h_asset))
    {
        // Create new render object with same data as previously created one,
        // to avoid duplicating render resource data.
        ro->renderUnits = renderObjects[renderObjectFromModelCache[h_asset].value]->renderUnits;
        renderObjects.push_back(ro);
        return { (u32)renderObjects.size() - 1 };
    }

    // If it's a new asset, then create and add the first render object created from the model asset to cache
    AssetModel* modelAsset = Asset_GetModel(h_asset);

    Handle<Buffer> h_modelVertexBuffer = Renderer_CreateBuffer(BUFFER_TYPE_VERTEX, modelAsset->vertices.size(), sizeof(f32), (u8*)modelAsset->vertices.data());
    for(i32 i = 0; i < modelAsset->objects.size(); i++)
    {
        AssetModelObject& assetModelObject = modelAsset->objects[i];
        Handle<Texture> h_objectMaterialTextures[] =
        {
            GetTextureFromAsset(assetModelObject.textureAmbient),
            GetTextureFromAsset(assetModelObject.textureDiffuse),
            GetTextureFromAsset(assetModelObject.textureSpecular),
            GetTextureFromAsset(assetModelObject.textureAlphaMask),
            GetTextureFromAsset(assetModelObject.textureBump),
        };
        Handle<Material> h_objectMaterial = Renderer_CreateMaterial(5, h_objectMaterialTextures);
        Handle<Buffer> h_objectIndexBuffer = Renderer_CreateBuffer(BUFFER_TYPE_INDEX, assetModelObject.indices.size(), sizeof(u32), (u8*)assetModelObject.indices.data());
        Handle<Mesh> h_objectMesh = Renderer_CreateMesh(h_modelVertexBuffer, h_objectIndexBuffer,
                {
                    3, { VERTEX_ATTRIBUTE_VEC3, VERTEX_ATTRIBUTE_VEC3, VERTEX_ATTRIBUTE_VEC2 }
                });

        ro->renderUnits.push_back(
                {
                    h_objectMaterial,
                    h_objectMesh,
                });
    }

    renderObjects.push_back(ro);
    Handle<RenderObject> h_model = { (u32)renderObjects.size() - 1 };
    renderObjectFromModelCache[h_asset] = h_model;
    return h_model;
}

Handle<Shader> h_modelShader;

Handle<RenderObject> h_sponzaObject;
std::vector<Handle<RenderObject>> h_bunnyObjects;

void App_Init(u32 windowWidth, u32 windowHeight, const char* appTitle)
{
    // Initializing common engine systems
    Time_Init();

    // Initializing renderer
    Renderer_Init(windowWidth, windowHeight, appTitle, &appWindow);
    Camera mainCamera = {};
    mainCamera.Move({0, 2, 3});
    Renderer_SetCamera(mainCamera);

    Handle<AssetShader> h_assetDefaultVS = Asset_LoadShader(APP_RESOURCE_SHADERS_PATH"default_vertex.vs");
    Handle<AssetShader> h_assetDefaultPS = Asset_LoadShader(APP_RESOURCE_SHADERS_PATH"default_pixel.ps");
    h_modelShader = Renderer_CreateShader(
                GetShaderFromAsset(h_assetDefaultVS, SHADERSTAGE_TYPE_VERTEX),
                GetShaderFromAsset(h_assetDefaultPS, SHADERSTAGE_TYPE_PIXEL)
            );
    Handle<AssetModel> h_assetSponzaModel = Asset_LoadModel_OBJ(APP_RESOURCE_MODELS_PATH"sponza/sponza.obj");
    h_sponzaObject = CreateRenderObjectFromAsset(h_assetSponzaModel);
    renderObjects[h_sponzaObject.value]->properties.u_world =
        RotationMatrix(TO_RAD(90.f), {0.f, 1.f, 0.f}) * ScaleMatrix(v3f{1.f, 1.f, 1.f} * 0.01f);
    Handle<AssetModel> h_assetBunnyModel = Asset_LoadModel_OBJ(APP_RESOURCE_MODELS_PATH"bunny/bunny.obj");
    for(i32 i = 0; i < 10; i++)
    {
        Handle<RenderObject> h_bunnyObject = CreateRenderObjectFromAsset(h_assetBunnyModel);
        h_bunnyObjects.push_back(h_bunnyObject);
        RenderObjectProperties& bunnyProperties = renderObjects[h_bunnyObject.value]->properties;

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
        bunnyProperties.u_world = TranslationMatrix(pos) * RotationMatrix(angle, axis) * ScaleMatrix({scale, scale, scale});
        bunnyProperties.u_baseColor =
        {
            RandomRange(0.f, 1.f),
            RandomRange(0.f, 1.f),
            RandomRange(0.f, 1.f)
        };
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
    Renderer_BeginFrame();
    Handle<RenderTarget> h_defaultRenderTarget = Renderer_GetDefaultRenderTarget();

    // Prepare
    {
        Renderer_BindRenderTarget(h_defaultRenderTarget);
        Renderer_Clear({1.f, 0.5f, 0.f, 1.f});
    }

    // Render models
    {
        Renderer_BindShader(h_modelShader);
        Renderer_BindUniform_m4f("u_view", Renderer_GetCamera().GetView());
        Renderer_BindUniform_m4f("u_proj", Renderer_GetCamera().GetProjection(*renderState.window));
        for(i32 i = 0; i < renderObjects.size(); i++)
        {
            RenderObject* ro = renderObjects[i];
            for(i32 o = 0; o < ro->renderUnits.size(); o++)
            {
                RenderUnit& renderUnit = ro->renderUnits[o];
                Renderer_BindMaterial(renderUnit.h_material);
                Renderer_BindMesh(renderUnit.h_mesh);
                BindRenderObjectProperties(ro->properties);

                Renderer_DrawMesh();
            }
        }
    }

    // Copy to backbuffer
    {
        Renderer_GenerateMipsForRenderTarget(h_defaultRenderTarget);
        Renderer_CopyRenderTargetOutputToBackbuffer(h_defaultRenderTarget, 0);
    }
    
    Renderer_EndFrame();
}

void App_Destroy()
{
    // Destroy app window
    Window_Destroy(appWindow);
}

} // namespace Ty
