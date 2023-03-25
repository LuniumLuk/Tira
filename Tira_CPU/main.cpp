//
// Created by Ziyi.Lu 2022/12/05
//

#include <iostream>
#include <filesystem>
#include <tira.h>

// The executable file will be placed at root directory as default.
#ifndef ROOT_DIR
#define ROOT_DIR "./"
#endif

using namespace tira;

static std::string generate_output_filename(int spp, int w, int h, bool mis, std::string const& scene_name);

int main(int argc, char* argv[]) {
    Scene scene;
    BidirectionalIntegrator integrator;
    //MonteCarloIntegrator integrator;

    // Tira_CPU.exe scene_name
    // your obj file and xml file is supporsed to be:
    //  - ROOT_DIT/Asset/scene_name/scene_name.obj
    //  - ROOT_DIT/Asset/scene_name/scene_name.xml
    //std::string scene_name = DEFAULT_SCENE;
    std::string scene_name = "CornellBox-Sphere";
    if (argc > 1) {
        scene_name = std::string(argv[1]);
        std::cout << "[Tira_CPU] Using Input Scene: " << scene_name << "\n";
    }

    scene.load(
        std::string(ROOT_DIR) + "Asset/" + scene_name + "/" + scene_name + ".obj",
        std::string(ROOT_DIR) + "Asset/" + scene_name + "/" + scene_name + ".xml",
        Scene::MaterialType::BlinnPhong);

    auto spp = scene.integrator_info.spp;
    auto w = scene.scr_w;
    auto h = scene.scr_h;

    integrator.max_depth = scene.integrator_info.max_bounce;
    integrator.use_mis = scene.integrator_info.use_mis;

    Image image(w, h);
    integrator.render(image, scene, spp);
    
    image.write_PNG(generate_output_filename(spp, w, h, scene.integrator_info.use_mis, scene_name));

    return EXIT_SUCCESS;
}

std::string generate_output_filename(int spp, int w, int h, bool mis, std::string const& scene_name) {
    std::string output_folder = std::string(ROOT_DIR) + "Output";
    std::filesystem::create_directory(std::filesystem::path(output_folder));
    std::string filename = output_folder + "/";
    filename += scene_name + "_";
    filename += std::to_string(spp) + "SPP_";
    filename += std::to_string(w) + "X" + std::to_string(h) + "_";
    filename += mis ? "_MIS" : "";
    filename += ".png";
    return filename;
}
