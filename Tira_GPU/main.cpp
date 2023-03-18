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
#include "tira.h"
#include "glwrapper.h"

#define DISPLAY_H 720
#define USE_TILING

std::string sceneName = "CornellBox-Original";
std::string generateOutputFilename(int SPP);
std::string generateShaderMacros(bool useMIS, bool acceptIntersectionCloseToLight, int textureCount);
int SPP = 1;
// Benchmark (sponza SPP 8, spf = 8)
// tile size | 64   32   16   8
//   tpi = 1 | 
//   tpi = 4 |
//  tpi = 16 |
//  tpi = 64 |      33s  70s
// tpi = 128 |           70s
int tileSize = 32;
int samplesPerFrame = 8;
// Benchmark (sponza SPP 8, tile size = 32)
// tiles per invoke | 4   8   16  32  64
//          spf = 1 |                 48s
//          spf = 4 | 72s 44s 44s 42s 40s
//          spf = 8 | 69s 42s 39s 38s 33s
int tilesPerFrame = 16;
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
    glm::mat3 mScreenToRaster;
    glm::vec3 mCameraPos;
    std::stack<Tile> mTiles;
    int mCurrentFrame = 0;
    std::vector<GL::GLTexture2D> mTextures;
    bool mEnableSun;
    glm::vec3 mSunDirection;
    float mSunSolidAngle;
    glm::vec3 mSunRadiance;
    int mTotalTiles;
};

auto App::init() noexcept -> void {
    tileSize = scene.tiling_info.size;
    tilesPerFrame = scene.tiling_info.num;
    samplesPerFrame = scene.tiling_info.spf;

    SPP = scene.integrator_info.spp;
    imageW = scene.scr_w;
    imageH = scene.scr_h;
    useMIS = scene.integrator_info.use_mis;
    acceptIntersectionCloseToLight = scene.integrator_info.robust_light;

    mEnableSun = scene.sun_enabled;
    mSunDirection = { scene.sun_direction.x, scene.sun_direction.y, scene.sun_direction.z };
    mSunSolidAngle = scene.sun_solid_angle;
    mSunRadiance = { scene.sun_radiance.x, scene.sun_radiance.y, scene.sun_radiance.z };

    // Generate tiles
    int xTiles = (imageW + tileSize - 1) / tileSize;
    int yTiles = (imageH + tileSize - 1) / tileSize;

    for (int y = 0; y < yTiles; ++y) for (int x = xTiles - 1; x >= 0; --x) {
        Tile tile{};
        tile.x = x * tileSize;
        tile.y = y * tileSize;
        tile.w = tileSize;
        tile.h = tileSize;
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
    if (textures.size() > 32) {
        std::cout << "[GL] Do not support more than 32 textures\n";
    }

    Root::get()->assetManager->LoadShaderProgramCompute("rt_compute_shader", "rt.comp", generateShaderMacros(useMIS, acceptIntersectionCloseToLight, textures.size()));
    Root::get()->assetManager->LoadShaderProgramVF("rt_screen_shader", "screen.vert", "screen.frag");
    Root::get()->assetManager->LoadShaderProgramVF("rt_mix_shader", "screen.vert", "mix.frag");

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
        if (mTextures.size() == 32) break;
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
    glViewport(0, 0, imageW, imageH);
    auto quadMesh = Root::get()->assetManager->GetMesh("rt_quad_mesh");

    Root::get()->assetManager->GetFrameBuffer("rt_render_target")->bind();
    FrameBuffer::ClearColor(0.0f, 0.0f, 0.0f, 1.0f);

#ifdef USE_TILING
    std::vector<Tile> tiles;
    while (tiles.size() < tilesPerFrame && !mTiles.empty()) {
        tiles.push_back(mTiles.top());
        mTiles.pop();
    }

    Root::get()->assetManager->GetShader("rt_compute_shader")->use();
    Root::get()->assetManager->GetShader("rt_compute_shader")->setInt("uSPP", SPP);
    Root::get()->assetManager->GetShader("rt_compute_shader")->setVec3("uCameraPos", mCameraPos);
    Root::get()->assetManager->GetShader("rt_compute_shader")->setMat3("uScreenToRaster", mScreenToRaster);
    Root::get()->assetManager->GetShader("rt_compute_shader")->setFloat("uLightTotalArea", scene.lights_total_area);
    Root::get()->assetManager->GetShader("rt_compute_shader")->setBool("uEnableSun", mEnableSun);
    Root::get()->assetManager->GetShader("rt_compute_shader")->setVec3("uSunDirection", mSunDirection);
    Root::get()->assetManager->GetShader("rt_compute_shader")->setFloat("uSunSolidAngle", mSunSolidAngle);
    Root::get()->assetManager->GetShader("rt_compute_shader")->setVec3("uSunRadiance", mSunRadiance);
    Root::get()->assetManager->GetShader("rt_compute_shader")->setInt("uSamplesPerFrame", samplesPerFrame);
    for (int i = 0; i < mTextures.size(); ++i) mTextures[i].bind(i);

    glBindImageTexture(0, Root::get()->assetManager->GetFrameBuffer("rt_render_target")->colorAttachments[0].handle, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glBindImageTexture(1, Root::get()->assetManager->GetTexture("rt_sample_counter")->handle, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);

    for (auto& tile : tiles) {
        Root::get()->assetManager->GetShader("rt_compute_shader")->setInt("uCurrentFrame", tile.SPP);
        Root::get()->assetManager->GetShader("rt_compute_shader")->setInt("uOffsetX", tile.x);
        Root::get()->assetManager->GetShader("rt_compute_shader")->setInt("uOffsetY", tile.y);
        tile.SPP += samplesPerFrame;
        glDispatchCompute(tile.w / 4, tile.h / 4, 1);
    }
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    for (auto tile : tiles) {
        if (tile.SPP < SPP) mTiles.push(tile);
    }
#else
    Root::get()->assetManager->GetShader("rt_compute_shader")->use();
    Root::get()->assetManager->GetShader("rt_compute_shader")->setInt("uSPP", SPP);
    Root::get()->assetManager->GetShader("rt_compute_shader")->setVec3("uCameraPos", mCameraPos);
    Root::get()->assetManager->GetShader("rt_compute_shader")->setMat3("uScreenToRaster", mScreenToRaster);
    Root::get()->assetManager->GetShader("rt_compute_shader")->setFloat("uLightTotalArea", scene.lights_total_area);
    Root::get()->assetManager->GetShader("rt_compute_shader")->setBool("uEnableSun", mEnableSun);
    Root::get()->assetManager->GetShader("rt_compute_shader")->setVec3("uSunDirection", mSunDirection);
    Root::get()->assetManager->GetShader("rt_compute_shader")->setFloat("uSunSolidAngle", mSunSolidAngle);
    Root::get()->assetManager->GetShader("rt_compute_shader")->setVec3("uSunRadiance", mSunRadiance);
    Root::get()->assetManager->GetShader("rt_compute_shader")->setInt("uSamplesPerFrame", samplesPerFrame);
    for (int i = 0; i < mTextures.size(); ++i) mTextures[i].bind(i);

    glBindImageTexture(0, Root::get()->assetManager->GetFrameBuffer("rt_render_target")->colorAttachments[0].handle, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glBindImageTexture(1, Root::get()->assetManager->GetTexture("rt_sample_counter")->handle, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);

    Root::get()->assetManager->GetShader("rt_compute_shader")->setInt("uCurrentFrame", mCurrentFrame);
    Root::get()->assetManager->GetShader("rt_compute_shader")->setInt("uOffsetX", 0);
    Root::get()->assetManager->GetShader("rt_compute_shader")->setInt("uOffsetY", 0);
    mCurrentFrame += samplesPerFrame;
    glDispatchCompute(imageW, imageH, 1);

    glMemoryBarrier(GL_ALL_BARRIER_BITS);
#endif

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
#ifdef USE_TILING
    float tilesPerSecond = mFPS * tiles.size() * samplesPerFrame / SPP;
    ImGui::Text((std::string("Tile size: ") + std::to_string(tileSize)).c_str());
    ImGui::Text((std::string("Samples per frame: ") + std::to_string(samplesPerFrame)).c_str());
    ImGui::Text((std::string("Tiles per frame: ") + std::to_string(tilesPerFrame)).c_str());
    ImGui::Text((std::string("SPP for current tiles: ") + std::to_string(tiles[0].SPP) + "/" + std::to_string(SPP)).c_str());
    ImGui::Text((std::string("Tiles: ") + std::to_string(mTotalTiles - mTiles.size()) + "/" + std::to_string(mTotalTiles)).c_str());
    ImGui::Text((std::string("Tiles/s: ") + std::to_string(tilesPerSecond)).c_str());
    ImGui::Text((std::string("Estimated time left: ") + std::to_string(mTiles.size() / tilesPerSecond / 60) + "min").c_str());
#endif
    if (ImGui::Button("Terminate")) {
        terminate();
    }

    ImGui::End();

#ifdef USE_TILING
    if (mTiles.empty()) terminate();
#else
    if (mCurrentFrame >= SPP) terminate();
#endif
}

auto App::quit() noexcept -> void {
    endTime = Root::get()->timer->totalTime();

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

    float scale = (float)samplesPerFrame / SPP;
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

    std::cout << "[GL] Total render time: " << (endTime - startTime) << "s\n";
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
        "./Asset/" + sceneName + "/" + sceneName + ".obj",
        "./Asset/" + sceneName + "/" + sceneName + ".xml",
        tira::Scene::MaterialType::BlinnPhong);

    GL::WindowDesc desc{};
    desc.name = "RaysTracer [GPU Compute]";
    desc.width = (float)DISPLAY_H * scene.scr_w / scene.scr_h;
    desc.height = DISPLAY_H;
    desc.resizable = false;
    Root root{ desc };
    Root::get()->shaderRoot = "./Tira_GPU/Shader";
    Root::get()->assetRoot = "./Asset";
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

std::string generateShaderMacros(bool useMIS, bool acceptIntersectionCloseToLight, int textureCount) {
    std::string macros;
    if (useMIS) {
        macros += "#define MIS\n";
    }
    if (acceptIntersectionCloseToLight) {
        macros += "#define ACCEPT_INTERSECTION_CLOSE_TO_LIGHT\n";
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

    return macros + textures.str();
}