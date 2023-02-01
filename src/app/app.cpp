#include "app/app.hpp"
#include "engine/common/asset.hpp"
#include "engine/renderer/renderer.hpp"
#include "engine/renderer/gui.hpp"

#define APP_RESOURCE_PATH           "../resources/"
#define APP_RESOURCE_SHADERS_PATH   APP_RESOURCE_PATH"shaders/"
#define APP_RESOURCE_MODELS_PATH    APP_RESOURCE_PATH"models/"
#define APP_RESOURCE_FONTS_PATH     APP_RESOURCE_PATH"fonts/"

namespace Ty
{

struct RenderUnit
{
    Handle<Material>    h_material;
    Handle<Mesh>        h_mesh;

    v3f u_ambientColor = {};
    v3f u_diffuseColor = {};
    v3f u_specularColor = {};
    f32 u_specularWeight = 0.f;
};

struct RenderObjectProperties
{
    m4f u_world         = Identity();
    v3f u_baseColor     = {1,1,1};
};

struct RenderObject
{
    std::vector<RenderUnit> renderUnits;
    RenderObjectProperties properties = {};
};

void BindRenderUnitProperties(RenderUnit* unit)
{
    Renderer_BindUniform_v3f("u_ambientColor", unit->u_ambientColor);
    Renderer_BindUniform_v3f("u_diffuseColor", unit->u_diffuseColor);
    Renderer_BindUniform_v3f("u_specularColor", unit->u_specularColor);
    Renderer_BindUniform_f32("u_specularWeight", unit->u_specularWeight);
}

void BindRenderObjectProperties(RenderObject* object)
{
    Renderer_BindUniform_m4f("u_world", object->properties.u_world);
    Renderer_BindUniform_v3f("u_baseColor", object->properties.u_baseColor);
}

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

void RecompileShaders()
{
    // Recompile all shader stages from asset cache
    for(auto& cacheElement : shaderStageFromAssetCache)
    {
        // Update shader asset source data
        AssetShader* assetShader = Asset_GetShader(cacheElement.first);
        assetShader->src = ReadFile_Str(assetShader->path);
        // Recompile renderer resource
        Renderer_CompileShaderStage(cacheElement.second, assetShader->src);
    }

    // Link all shaders from renderer resource cache
    for(u32 i = 0; i < renderResourceTable.shaderResources.size(); i++)
    {
        Renderer_LinkShader({i});
    }

    // Rebind currently bound shader to not invalidate pipeline (like on ImGui draw)
    Renderer_BindShader(renderState.h_activeShader);
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
        case 2: format = TEXTURE_FORMAT_RG8; break;
        case 3: format = TEXTURE_FORMAT_RGB8; break;
        case 4: format = TEXTURE_FORMAT_RGBA8; break;
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
        RenderObject* roSrc = renderObjects[renderObjectFromModelCache[h_asset].value];
        ro->renderUnits = roSrc->renderUnits;
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
                    5, { VERTEX_ATTRIBUTE_VEC3, VERTEX_ATTRIBUTE_VEC3, VERTEX_ATTRIBUTE_VEC3, VERTEX_ATTRIBUTE_VEC3, VERTEX_ATTRIBUTE_VEC2 }
                });

        ro->renderUnits.push_back(
                {
                    h_objectMaterial,
                    h_objectMesh,
                    assetModelObject.ambientColor,
                    assetModelObject.diffuseColor,
                    assetModelObject.specularColor,
                    assetModelObject.specularWeight,
                });
    }

    renderObjects.push_back(ro);
    Handle<RenderObject> h_model = { (u32)renderObjects.size() - 1 };
    renderObjectFromModelCache[h_asset] = h_model;
    return h_model;
}

Handle<RenderTarget> h_gbufferRenderTarget;
Handle<Shader> h_modelGeometryPassShader;
Handle<Shader> h_modelLightingPassShader;

Handle<RenderObject> h_sponzaObject;
Handle<RenderObject> h_backpackObject;
std::vector<Handle<RenderObject>> h_bunnyObjects;
struct
{
    v3f dir = {0,1,0};
    v3f color = {1,1,1};
    f32 ambientIntensity = 0.1f;
    f32 specularIntensity = 0.5f;
} directionalLight;
bool inEditorMode = false;

void App_Init(u32 windowWidth, u32 windowHeight, const char* appTitle)
{
    // Initializing common engine systems
    Time_Init();

    // Initializing renderer
    Renderer_Init(windowWidth, windowHeight, appTitle, &appWindow);
    Camera mainCamera = {};
    mainCamera.Move({0, 2, 3});
    Renderer_SetCamera(mainCamera);

    GUI_Init(&appWindow, APP_RESOURCE_FONTS_PATH"cascadia_code/CascadiaMono.ttf", 16);

    // Render targets
    RenderTargetOutputDesc gbufferOutputDesc[] =
    {
        { TEXTURE_FORMAT_RGBA8, TEXTURE_WRAP_REPEAT, TEXTURE_FILTER_LINEAR, TEXTURE_FILTER_LINEAR },      // Diffuse + specular color output
        { TEXTURE_FORMAT_RGBA16F, TEXTURE_WRAP_REPEAT, TEXTURE_FILTER_LINEAR, TEXTURE_FILTER_LINEAR },    // Position output // TODO(caio)#RENDER: This can be removed and reconstructed via depth.
        { TEXTURE_FORMAT_RGBA16F, TEXTURE_WRAP_CLAMP, TEXTURE_FILTER_LINEAR, TEXTURE_FILTER_LINEAR },    // Normals output
    };
    h_gbufferRenderTarget = Renderer_CreateRenderTarget(1920, 1080, 3, gbufferOutputDesc);

    // Shader resources
    Handle<AssetShader> h_assetModelGeometryVS = Asset_LoadShader(APP_RESOURCE_SHADERS_PATH"model_geometry_pass.vert");
    Handle<AssetShader> h_assetModelGeometryPS = Asset_LoadShader(APP_RESOURCE_SHADERS_PATH"model_geometry_pass.frag");
    h_modelGeometryPassShader = Renderer_CreateShader(
                GetShaderFromAsset(h_assetModelGeometryVS, SHADERSTAGE_TYPE_VERTEX),
                GetShaderFromAsset(h_assetModelGeometryPS, SHADERSTAGE_TYPE_PIXEL)
            );
    Handle<AssetShader> h_assetModelLightingPS = Asset_LoadShader(APP_RESOURCE_SHADERS_PATH"model_lighting_pass.frag");
    h_modelLightingPassShader = Renderer_CreateShader(
            h_screenQuadVS,
            GetShaderFromAsset(h_assetModelLightingPS, SHADERSTAGE_TYPE_PIXEL)
            );

    // In-game models
    Handle<AssetModel> h_assetSponzaModel = Asset_LoadModel_OBJ(APP_RESOURCE_MODELS_PATH"sponza/sponza.obj");
    h_sponzaObject = CreateRenderObjectFromAsset(h_assetSponzaModel);
    Handle<AssetModel> h_assetBackpackModel = Asset_LoadModel_OBJ(APP_RESOURCE_MODELS_PATH"backpack/backpack.obj", true);
    h_backpackObject = CreateRenderObjectFromAsset(h_assetBackpackModel);

    renderObjects[h_sponzaObject.value]->properties.u_world =
        RotationMatrix(TO_RAD(90.f), {0.f, 1.f, 0.f}) * ScaleMatrix(v3f{1.f, 1.f, 1.f} * 0.01f);
    renderObjects[h_backpackObject.value]->properties.u_world = ScaleMatrix(v3f{1.f, 1.f, 1.f} * 0.5f);
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

    if(!inEditorMode)
    {
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

    // App controls
    if(Input_IsKeyJustDown(KEY_ESCAPE))
    {
        appWindow.shouldClose = true;
    }
    if(Input_IsKeyJustDown(KEY_P))
    {
        Input_ToggleLockMouse();
        Input_ToggleShowMouse();
        inEditorMode = !inEditorMode;
    }
}

void App_Render()
{
    Renderer_BeginFrame();

    // Prepare
    {
        Renderer_BindRenderTarget(h_gbufferRenderTarget);
        Renderer_Clear({0.f, 0.f, 0.f, 0.f});
    }

    // G-Buffer geometry pass
    {
        Renderer_BindShader(h_modelGeometryPassShader);
        Renderer_BindUniform_m4f("u_view", Renderer_GetCamera().GetView());
        Renderer_BindUniform_m4f("u_proj", Renderer_GetCamera().GetProjection(*renderState.window));
        for(i32 i = 0; i < renderObjects.size(); i++)
        {
            RenderObject* ro = renderObjects[i];
            BindRenderObjectProperties(ro);
            for(i32 o = 0; o < ro->renderUnits.size(); o++)
            {
                RenderUnit& renderUnit = ro->renderUnits[o];
                Renderer_BindMaterial(renderUnit.h_material);
                Renderer_BindMesh(renderUnit.h_mesh);
                BindRenderUnitProperties(&renderUnit);

                Renderer_DrawMesh();
            }
        }
    }

    // G-Buffer lighting pass
    {
        Renderer_BindRenderTarget(h_defaultRenderTarget);
        Renderer_Clear({0.f,0.f,0.f,0.f});

        Renderer_BindShader(h_modelLightingPassShader);
        Renderer_BindUniform_m4f("u_view", Renderer_GetCamera().GetView());
        Renderer_BindUniform_v3f("u_lightDir", directionalLight.dir);
        Renderer_BindUniform_v3f("u_lightColor", directionalLight.color);
        Renderer_BindUniform_f32("u_lightIntensityAmbient", directionalLight.ambientIntensity);
        Renderer_BindUniform_f32("u_lightIntensitySpecular", directionalLight.specularIntensity);
        RenderTarget* gbufferRenderTarget = Renderer_GetRenderTarget(h_gbufferRenderTarget);
        Renderer_BindTexture(gbufferRenderTarget->outputs[0], 0);   // Diffuse + specular
        Renderer_BindTexture(gbufferRenderTarget->outputs[1], 1);   // View space positions
        Renderer_BindTexture(gbufferRenderTarget->outputs[2], 2);   // View space normals
        Renderer_BindMesh(h_screenQuadMesh);

        Renderer_DrawMesh();
    }

    // Copy to backbuffer
    static bool showGbufferOutput = false;
    static i32 showGbufferOutputTarget = 0;
    {
        if(showGbufferOutput)
        {
            Renderer_GenerateMipsForRenderTarget(h_gbufferRenderTarget);
            Renderer_CopyRenderTargetOutputToBackbuffer(h_gbufferRenderTarget, showGbufferOutputTarget);
        }
        else
        {
            Renderer_GenerateMipsForRenderTarget(h_defaultRenderTarget);
            Renderer_CopyRenderTargetOutputToBackbuffer(h_defaultRenderTarget, 0);
        }
    }

    // Application GUI
    if(inEditorMode)
    {
        GUI_BeginFrame();

        {
            if(ImGui::Button("Recompile shaders")) RecompileShaders();
            ImGui::Text("Final output");
            ImGui::Checkbox("Show GBuffer output", &showGbufferOutput);
            if(showGbufferOutput)
            {
                ImGui::RadioButton("Diffuse", &showGbufferOutputTarget, 0); ImGui::SameLine();
                ImGui::RadioButton("Position", &showGbufferOutputTarget, 1); ImGui::SameLine();
                ImGui::RadioButton("Normal", &showGbufferOutputTarget, 2);
            }
            ImGui::DragFloat3("Light direction", &directionalLight.dir.x, 0.005f, -1.f, 1.f);
            ImGui::ColorEdit3("Light color", &directionalLight.color.x);
            ImGui::DragFloat("Ambient intensity", &directionalLight.ambientIntensity, 0.005f, 0.f);
            ImGui::DragFloat("Specular intensity", &directionalLight.specularIntensity, 0.005f, 0.f);
        }

        GUI_EndFrame();
    }
    
    Renderer_EndFrame();
}

void App_Destroy()
{
    Window_Destroy(appWindow);
}

} // namespace Ty
