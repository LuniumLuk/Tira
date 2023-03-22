//
// Created by Ziyi.Lu 2022/12/05
//

#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <vector>
#include <stack>
#include <cmath>
#include <sstream>

#include "convert.h"
#include <tira.h>
#include "glwrapper.h"
#include "threadpool.h"

// [TODO] To run in local windows debugger, change the following ASSET_DIR to "../Asset/"
#ifndef ASSET_DIR
#define ASSET_DIR "./Asset/"
#endif

// [TODO] To run in local windows debugger, change the following SHADER_DIR to "./Shader/"
#ifndef SHADER_DIR
#define SHADER_DIR "./Tira_GPU/Shader/"
#endif

#define DISPLAY_H 720
#define USE_TILING
#define MAX_MATERIAL_TEXTURE_COUNT 30

std::string sceneName = "CornellBox-Original";
std::string generateOutputFilename(int SPP);
std::string generateShaderMacros(bool useMIS, bool acceptIntersectionCloseToLight, int textureCount, std::string const& additional);
int maxDepth = 8;
int SPP = 1;
int kernelSize = 32;
int samplesPerFrame = 1;
double startTime;
double endTime;
tira::Scene scene;
int imageW;
int imageH;
bool useMIS = false;
bool acceptIntersectionCloseToLight = true;
std::vector<tira::Texture*> textures;

float quadVertices[] = {
    -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
     1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
     1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
     1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
};

struct Tile {
    int x, y;
    int w, h;
    int SPP;
};

uint32_t quadIndices[] = { 0, 1, 2, 3, 4, 5 };

using GL::Application;
using GL::Root;
using GL::FrameBuffer;

struct App : public Application {
    virtual auto init() noexcept -> void override;
    virtual auto update(double deltaTime) noexcept -> void override;
    virtual auto quit() noexcept -> void override;

    double mFPS;
    double mFPSTimer = 0.0;
    float mTime = 0.0;
    int mFrame = 0;
    glm::mat3 mScreenToRaster{ 1.0f };
    glm::vec3 mCameraPos{ 0.0f };
    std::stack<Tile> mTiles;
    int mCurrentFrame = 0;
    std::vector<GL::Texture2D> mTextures;
    bool mEnableSun = false;
    glm::vec3 mSunDirection{ 0.0f };
    float mSunSolidAngle = 0.0f;
    glm::vec3 mSunRadiance{ 0.0f };
    int mTotalTiles = 0;
    std::unique_ptr<GL::Texture2D> mEnvmap = nullptr;
    tira::Timer mTimer;
    std::unique_ptr<GL::Texture2D> mEnvmapWeight = nullptr;
};

auto App::init() noexcept -> void {
    kernelSize = scene.kernel_info.size;

    SPP = scene.integrator_info.spp;
    maxDepth = scene.integrator_info.max_bounce;
    imageW = scene.scr_w;
    imageH = scene.scr_h;
    useMIS = scene.integrator_info.use_mis;
    acceptIntersectionCloseToLight = scene.integrator_info.robust_light;

    mEnableSun = scene.sun_enabled;
    mSunDirection = { scene.sun_direction.x, scene.sun_direction.y, scene.sun_direction.z };
    mSunSolidAngle = scene.sun_solid_angle;
    mSunRadiance = { scene.sun_radiance.x, scene.sun_radiance.y, scene.sun_radiance.z };

    // Generate tiles
    int xTiles = (imageW + kernelSize - 1) / kernelSize;
    int yTiles = (imageH + kernelSize - 1) / kernelSize;

    for (int y = 0; y < yTiles; ++y) for (int x = xTiles - 1; x >= 0; --x) {
        Tile tile{};
        tile.x = x * kernelSize;
        tile.y = y * kernelSize;
        tile.w = kernelSize;
        tile.h = kernelSize;
        tile.SPP = 0;
        mTiles.push(tile);
    }
    mTotalTiles = mTiles.size();

    auto triangleList  = getTriangleList(scene);
    auto materialList  = getMaterialList(scene, textures);
    auto BVHNodeList   = getBVHNodeList(scene);
    auto areaLightList = getAreaLights(scene);

    // [TODO] Use 2D Texture Array instead.
    std::cout << "[GL] Load " << textures.size() << " textures\n";
    if (textures.size() > MAX_MATERIAL_TEXTURE_COUNT) {
        std::cout << "[GL] Do not support more than " << MAX_MATERIAL_TEXTURE_COUNT << " textures\n";
    }

    Root::get()->assetManager->LoadShaderProgramCompute("rt_compute_shader", "rt.comp", generateShaderMacros(useMIS, acceptIntersectionCloseToLight, textures.size(), scene.kernel_info.macro));
    Root::get()->assetManager->LoadShaderProgramVF("rt_screen_shader", "screen.vert", "screen.frag");
    Root::get()->assetManager->LoadShaderProgramVF("rt_mix_shader", "screen.vert", "mix.frag");

    if (scene.envmap) {
        mEnvmap = std::make_unique<GL::Texture2D>(scene.envmap->width, scene.envmap->height, GL::InternalFormat::FloatRGB, scene.envmap->data);
        mEnvmapWeight = std::make_unique<GL::Texture2D>(scene.envmap->weight_grid_size, scene.envmap->weight_grid_size, GL::InternalFormat::FloatRED, scene.envmap->weight);
    }

    Root::get()->assetManager->LoadTexture2D("rt_sample_counter", imageW, imageH, GL::InternalFormat::FloatRED);
    Root::get()->assetManager->LoadMeshData("rt_quad_mesh", &quadVertices[0], sizeof(quadVertices), { {GL::DataType::Float3}, {GL::DataType::Float2} }, &quadIndices[0], sizeof(quadIndices));

    // Setup camera.
    tira::float3x3 screenToRaster = scene.camera.get_screen_to_raster();
    mScreenToRaster = {
        screenToRaster[0][0], screenToRaster[0][1], screenToRaster[0][2],
        screenToRaster[1][0], screenToRaster[1][1], screenToRaster[1][2],
        screenToRaster[2][0], screenToRaster[2][1], screenToRaster[2][2],
    };

    for (auto tex : textures) {
        auto tex2D = reinterpret_cast<tira::Texture2D*>(tex);
        mTextures.emplace_back(tex2D->width, tex2D->height, GL::InternalFormat::FloatRGBA, tex2D->data);
        if (mTextures.size() == MAX_MATERIAL_TEXTURE_COUNT) break;
    }

    mCameraPos = {
        scene.camera.eye.x, scene.camera.eye.y, scene.camera.eye.z, 
    };

    // Load storage buffers.
    Root::get()->assetManager->LoadStorageBuffer(
        "rt_triangles", triangleList.data(), sizeof(GL::Triangle) * triangleList.size(), GL::UsageType::Dynamic);
    Root::get()->assetManager->GetStorageBuffer("rt_triangles")->bind(2);
    Root::get()->assetManager->LoadStorageBuffer(
        "rt_materials", materialList.data(), sizeof(GL::Material) * materialList.size(), GL::UsageType::Dynamic);
    Root::get()->assetManager->GetStorageBuffer("rt_materials")->bind(3);
    Root::get()->assetManager->LoadStorageBuffer(
        "rt_bvh_nodes", BVHNodeList.data(), sizeof(GL::BVHNode) * BVHNodeList.size(), GL::UsageType::Dynamic);
    Root::get()->assetManager->GetStorageBuffer("rt_bvh_nodes")->bind(4);
    Root::get()->assetManager->LoadStorageBuffer(
        "rt_area_lights", areaLightList.data(), sizeof(int) * areaLightList.size(), GL::UsageType::Dynamic);
    Root::get()->assetManager->GetStorageBuffer("rt_area_lights")->bind(5);

    // Load framebuffers.
    Root::get()->assetManager->LoadFrameBuffer("rt_render_target", {
        {GL::AttachmentType::Color, GL::InternalFormat::FloatRGBA},
    }, imageW, imageH);
    Root::get()->assetManager->LoadFrameBuffer("rt_transfer", {
        {GL::AttachmentType::Color, GL::InternalFormat::FloatRGBA},
    }, imageW, imageH);
    Root::get()->assetManager->LoadFrameBuffer("rt_store", {
        {GL::AttachmentType::Color, GL::InternalFormat::FloatRGBA},
    }, imageW, imageH);

    Root::get()->assetManager->GetFrameBuffer("rt_transfer")->bind();
    FrameBuffer::ClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    Root::get()->assetManager->GetFrameBuffer("rt_store")->bind();
    FrameBuffer::ClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    FrameBuffer::BindDefault();
    startTime = Root::get()->timer->totalTime();
}

auto App::update(double deltaTime) noexcept -> void {
    if (mTiles.empty()) {
        terminate();
        return;
    }
    auto& tile = mTiles.top();

    glViewport(0, 0, imageW, imageH);
    auto quadMesh = Root::get()->assetManager->GetMesh("rt_quad_mesh");

    Root::get()->assetManager->GetFrameBuffer("rt_render_target")->bind();
    FrameBuffer::ClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    Root::get()->assetManager->GetShader("rt_compute_shader")->use();
    Root::get()->assetManager->GetShader("rt_compute_shader")->setInt("uSPP", SPP);
    Root::get()->assetManager->GetShader("rt_compute_shader")->setVec3("uCameraPos", mCameraPos);
    Root::get()->assetManager->GetShader("rt_compute_shader")->setMat3("uScreenToRaster", mScreenToRaster);
    Root::get()->assetManager->GetShader("rt_compute_shader")->setFloat("uLightTotalArea", scene.lights_total_area);
    Root::get()->assetManager->GetShader("rt_compute_shader")->setBool("uEnableSun", mEnableSun);
    Root::get()->assetManager->GetShader("rt_compute_shader")->setVec3("uSunDirection", mSunDirection);
    Root::get()->assetManager->GetShader("rt_compute_shader")->setFloat("uSunSolidAngle", mSunSolidAngle);
    Root::get()->assetManager->GetShader("rt_compute_shader")->setVec3("uSunRadiance", mSunRadiance);
    Root::get()->assetManager->GetShader("rt_compute_shader")->setBool("uEnableEnvmap", mEnvmap != nullptr);
    Root::get()->assetManager->GetShader("rt_compute_shader")->setFloat("uEnvmapScale", scene.envmap_scale);
    Root::get()->assetManager->GetShader("rt_compute_shader")->setInt("uMaxDepth", maxDepth);
    Root::get()->assetManager->GetShader("rt_compute_shader")->setInt("uEnvmapWeightGridSize", scene.envmap->weight_grid_size);
    for (int i = 0; i < mTextures.size(); ++i) mTextures[i].bind(i);
    if (mEnvmap) {
        mEnvmap->bind(30);
        mEnvmapWeight->bind(31);
    }

    glBindImageTexture(0, Root::get()->assetManager->GetFrameBuffer("rt_render_target")->colorAttachments[0].handle, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glBindImageTexture(1, Root::get()->assetManager->GetTexture("rt_sample_counter")->handle, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);

    mTimer.update();
    Root::get()->assetManager->GetShader("rt_compute_shader")->setInt("uSamplesPerFrame", std::min(samplesPerFrame, SPP - tile.SPP));
    Root::get()->assetManager->GetShader("rt_compute_shader")->setInt("uCurrentFrame", tile.SPP);
    Root::get()->assetManager->GetShader("rt_compute_shader")->setInt("uOffsetX", tile.x);
    Root::get()->assetManager->GetShader("rt_compute_shader")->setInt("uOffsetY", tile.y);
    tile.SPP += samplesPerFrame;
    glDispatchCompute(tile.w / 4, tile.h / 4, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    double duration = mTimer.delta_time();

    // Screen Shader
    // Pass #1 mix with present buffer
    Root::get()->assetManager->GetShader("rt_mix_shader")->use();
    Root::get()->assetManager->GetFrameBuffer("rt_store")->bind();
    Root::get()->assetManager->GetFrameBuffer("rt_render_target")->bindColorAttachmentAsTexture(0, 0);
    Root::get()->assetManager->GetFrameBuffer("rt_transfer")->bindColorAttachmentAsTexture(0, 1);
    quadMesh->bind();
    quadMesh->draw();

    // Pass #2 update present with store
    FrameBuffer::BlitFrameBuffer(
        Root::get()->assetManager->GetFrameBuffer("rt_transfer"),
        Root::get()->assetManager->GetFrameBuffer("rt_store"));

    // Pass #3 render present to default buffer
    FrameBuffer::SetWindowViewport();
    FrameBuffer::BindDefault();
    Root::get()->assetManager->GetShader("rt_screen_shader")->use();
    Root::get()->assetManager->GetFrameBuffer("rt_store")->bindColorAttachmentAsTexture(0, 0);
    Root::get()->assetManager->GetTexture("rt_sample_counter")->bind(1);
    quadMesh->bind();
    quadMesh->draw();

    // FPS Display

    mFPSTimer += deltaTime;
    mTime += deltaTime;
    if (mFPSTimer > 1.0) {
        mFPS = 1.0 / deltaTime;
        mFPSTimer = 0.0;
    }
    // ImGui

    ImGui::Begin("Hello Dear ImGui");

    ImGui::Text(sceneName.c_str());
    ImGui::Text((std::string("SPP: ") + std::to_string(SPP)).c_str());
    ImGui::Text((std::to_string(imageW) + "x" + std::to_string(imageH)).c_str());
    ImGui::Text((std::string("FPS: ") + std::to_string(mFPS)).c_str());
    float tilesPerSecond = mFPS * samplesPerFrame / SPP;
    ImGui::Text((std::string("Kernel size: ") + std::to_string(kernelSize)).c_str());
    ImGui::Text((std::string("Samples per frame: ") + std::to_string(samplesPerFrame)).c_str());
    ImGui::Text((std::string("SPP for current tiles: ") + std::to_string(tile.SPP) + "/" + std::to_string(SPP)).c_str());
    ImGui::Text((std::string("Tiles: ") + std::to_string(mTotalTiles - mTiles.size()) + "/" + std::to_string(mTotalTiles)).c_str());
    ImGui::Text((std::string("Tiles/s: ") + std::to_string(tilesPerSecond)).c_str());
    ImGui::Text((std::string("Estimated time left: ") + std::to_string(std::ceil(mTiles.size() / tilesPerSecond / 60)) + " min").c_str());
    if (ImGui::Button("Terminate")) {
        terminate();
    }

    ImGui::End();

    if (duration < 0.05) {
        samplesPerFrame = std::min(256, samplesPerFrame + (samplesPerFrame + 1) / 2);
    }
    else {
        samplesPerFrame = std::max(1, samplesPerFrame - (samplesPerFrame + 1) / 2);
    }

    if (tile.SPP >= SPP) {
        mTiles.pop();
        samplesPerFrame = 1;
    }
}

auto App::quit() noexcept -> void {
    endTime = Root::get()->timer->totalTime();
    std::cout << "[GL] Total render time: " << (endTime - startTime) << "s\n";
    std::cout << "[GL] Writing image ... please wait ...\n";

    size_t w = imageW;
    size_t h = imageH;
    size_t imageSize = w * h * 3;
    std::vector<float> imageData(imageSize);

    Root::get()->assetManager->GetFrameBuffer("rt_store")->bind();
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_PACK_SKIP_ROWS, 0);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);
    glReadPixels(0, 0, w, h, GL_RGB, GL_FLOAT, imageData.data());

    float scale = (float)1 / SPP;
    tira::Image image(w, h);
    for (int x = 0; x < w; ++x) {
        for (int y = 0; y < h; ++y) {
            tira::colorf color;
            color.r = tira::clamp(std::pow(imageData[(y * w + x) * 3 + 0] * scale, 1.0f / 2.2f), 0.0f, 1.0f);
            color.g = tira::clamp(std::pow(imageData[(y * w + x) * 3 + 1] * scale, 1.0f / 2.2f), 0.0f, 1.0f);
            color.b = tira::clamp(std::pow(imageData[(y * w + x) * 3 + 2] * scale, 1.0f / 2.2f), 0.0f, 1.0f);
            image.set_pixel(x, y, color);
        }
    }
    image.write_PNG(generateOutputFilename(SPP));

}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Using Default Scene: " << sceneName << "\n";
    }
    else {
        sceneName = std::string(argv[1]);
        std::cout << "Using Input Scene: " << sceneName << "\n";
    }

    scene.load(
        ASSET_DIR + sceneName + "/" + sceneName + ".obj",
        ASSET_DIR + sceneName + "/" + sceneName + ".xml",
        tira::Scene::MaterialType::BlinnPhong);

    GL::WindowDesc desc{};
    desc.name = "RaysTracer [GPU Compute]";
    desc.width = (float)DISPLAY_H * scene.scr_w / scene.scr_h;
    desc.height = DISPLAY_H;
    desc.resizable = false;
    Root root{ desc };
    Root::get()->shaderRoot = SHADER_DIR;
    Root::get()->assetRoot = ASSET_DIR;
    App app;

    try {
        app.run();
    }
    catch (std::exception const& err) {
        std::cerr << "[FATAL] " << err.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

std::string generateOutputFilename(int SPP) {
    std::string filename = "./Output/";
    filename += sceneName + "_";
    filename += std::to_string(SPP) + "SPP_";
    filename += std::to_string(imageW) + "X" + std::to_string(imageH);
    if (useMIS) {
        filename += "_MIS";
    }
    filename += "_GPU_COMPUTE.png";
    return filename;
}

std::string generateShaderMacros(bool useMIS, bool acceptIntersectionCloseToLight, int textureCount, std::string const& additional) {
    std::string macros;
    if (useMIS) {
        macros += "#define MIS\n";
    }
    if (acceptIntersectionCloseToLight) {
        macros += "#define ACCEPT_INTERSECTION_CLOSE_TO_LIGHT\n";
    }
    size_t front = 0, end = 0;
    while ((end = additional.find(' ', front)) != std::string::npos) {
        macros += "#define " + additional.substr(front, end) + "\n";
        front = end + 1;
    }
    if (front < additional.length()) {
        macros += "#define " + additional.substr(front) + "\n";
    }
    std::stringstream textures;
    for (int i = 0; i < textureCount; ++i) {
        textures << "layout (binding = " << i << ") uniform sampler2D uDiffuseMap" << i << ";\n";
    }

    textures << "vec4 getDiffuse(int i, vec2 uv) {\n";
    for (int i = 0; i < textureCount; ++i) {
        textures << "if (i == " << i << ") return texture(uDiffuseMap" << i << ", uv);\n";
    }
    textures << "return vec4(1.0, 0.0, 1.0, 1.0);\n}\n";

    std::cout << "[GL] Shader macros:\n" << macros << "\n";

    return macros + textures.str();
}