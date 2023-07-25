#include "../core/time.hpp"
#include "./asset.hpp"

namespace ty
{

void TestAssets()
{
    time::Init();
    asset::Init();

    // Test image asset loading
    file::Path assetPath = file::MakePath(IStr("./resources/textures/checkers.png"));

    LOGLF("ASSET", "Asset memory before loading: %llu bytes", asset::assetHeap.used);

    Handle<asset::Image> h_checkerImage = asset::LoadImageFile(assetPath);

    LOGLF("ASSET", "Asset memory after loading: %llu bytes", asset::assetHeap.used);

    ASSERT(h_checkerImage.IsValid());
    asset::Image& checkerImage = asset::images[h_checkerImage];
    ASSERT(checkerImage.data);
    ASSERT(checkerImage.width == 1024);
    ASSERT(checkerImage.height == 1024);
    ASSERT(checkerImage.channels == 3);

    Handle<asset::Image> h_checkerImage2 = asset::LoadImageFile(assetPath);
    ASSERT(h_checkerImage2 == h_checkerImage);

    // //TODO(caio): Redo shader testing
    // // Test binary data loading (e.g. shader bytecode)
    // assetPath = file::MakePath(IStr("./resources/shaders/test.spv"));

    // LOGLF("ASSET", "Asset memory before loading: %llu bytes", asset::assetHeap.used);

    // Handle<asset::BinaryData> h_shaderBytecode = asset::LoadBinaryFile(assetPath);

    // LOGLF("ASSET", "Asset memory after loading: %llu bytes", asset::assetHeap.used);

    // ASSERT(h_shaderBytecode.IsValid());
    // asset::BinaryData& shaderBytecode = asset::binaryDatas[h_shaderBytecode];
    // ASSERT(shaderBytecode.size);
    // ASSERT(shaderBytecode.data);

    // Handle<asset::BinaryData> h_shaderBytecode2 = asset::LoadBinaryFile(assetPath);
    // ASSERT(h_shaderBytecode2 == h_shaderBytecode);

    // Test model loading
    time::Timer timer = {};
    assetPath = file::MakePath(IStr("./resources/models/sponza/sponza.obj"));
    //assetPath = file::MakePath(IStr("./resources/models/room/breakfast_room.obj"));
    timer.Start();
    Handle<asset::Model> h_sponzaModel = asset::LoadModelOBJ(assetPath);
    timer.Stop();
    f32 loadTime = (f32)timer.GetElapsedS();
    LOGF("Model load took %.4f seconds", loadTime);
    LOGLF("ASSET", "Asset memory after loading: %llu bytes", asset::assetHeap.used);
    Handle<asset::Model> h_sponzaModel2 = asset::LoadModelOBJ(assetPath);
    ASSERT(h_sponzaModel == h_sponzaModel2);
}

}
