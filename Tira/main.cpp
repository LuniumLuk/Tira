//
// Created by Ziyi.Lu 2022/12/05
//

#include <iostream>
#include <vector>
#include <cmath>
#include <macro.h>
#include <utils.h>
#include <platform.h>
#include <timer.h>
#include <image.h>
#include <gui.h>
#include <scene.h>
#include <integrator.h>
#include <integrator2.h>
#include <integrator3.h>
#include <texture.h>

#ifndef ASSET_DIR
#define ASSET_DIR "./Asset/"
#endif

// A simple window API that support frame buffer swapping.
// I transported this window API from my renderer project:
// https://github.com/LuniumLuk/soft-renderer
using namespace LuGL;

using namespace tira;

static void keyboardEventCallback(AppWindow *window, KEY_CODE key, bool pressed);
static void mouseButtonEventCallback(AppWindow *window, MOUSE_BUTTON button, bool pressed, float x, float y);
static void mouseScrollEventCallback(AppWindow *window, float offset);
static void mouseDragEventCallback(AppWindow *window, float x, float y);

std::string generate_output_filename();

// GUI gui(SCR_WIDTH, SCR_HEIGHT, 10, 10, 1);
// Scene scene(SCR_WIDTH, SCR_HEIGHT);

GUI* gui;
Scene scene;

int SPP = 1;
int SCR_WIDTH = 1024;
int SCR_HEIGHT = 1024;
int const display_interval = 1;
Scene::MaterialType material_type = Scene::MaterialType::BlinnPhong;
// Scene::MaterialType material_type = Scene::MaterialType::DisneyBSDF;
Integrator::SamplingScheme sampling_scheme = Integrator::SamplingScheme::Importance;
Integrator::RenderMode render_mode = Integrator::RenderMode::Normal;
Integrator integrator;
Integrator2 integrator2;
Integrator3 integrator3;
std::string scene_name = DEFAULT_SCENE;

static AppWindow *window;
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Using Default Scene: " << scene_name << "\n";
    }
    else {
        scene_name = std::string(argv[1]);
        std::cout << "Using Input Scene: " << scene_name << "\n";
    }

    scene.load(
        ASSET_DIR + scene_name + "/" + scene_name + ".obj",
        ASSET_DIR + scene_name + "/" + scene_name + ".xml",
        material_type);

    SPP = scene.integrator_info.spp;
    integrator2.max_depth = scene.integrator_info.max_bounce;
    integrator2.use_mis = scene.integrator_info.use_mis;
    std::cout << "Max Bounce: " << integrator2.max_depth << ", Use MIS: " << (integrator2.use_mis ? "true" : "false") << "\n";
    gui = new GUI(scene.scr_w, scene.scr_h, 10, 10, 1);
    SCR_WIDTH = scene.scr_w;
    SCR_HEIGHT = scene.scr_h;

    Image image(SCR_WIDTH, SCR_HEIGHT);

#if GUI_MODE == 0 // Render offscreen.
    image.fill({0, 0, 0});
#if INTEGRATOR_VER == 1
    integrator.render(
        image,
        scene,
        render_mode,
        SPP,
        sampling_scheme);
#elif INTEGRATOR_VER == 2
    integrator2.render(image, scene, SPP);
#elif INTEGRATOR_VER == 3
    integrator3.render(image, scene, SPP);
#endif
    image.write_PNG(generate_output_filename());
    return 0;
#endif

    initializeApplication();

    const char * title = "RaysTracer [CPU]";
    window = createWindow(title, SCR_WIDTH, SCR_HEIGHT, image.data);

    setKeyboardCallback(window, keyboardEventCallback);
    setMouseButtonCallback(window, mouseButtonEventCallback);
    setMouseScrollCallback(window, mouseScrollEventCallback);
    setMouseDragCallback(window, mouseDragEventCallback);

////////////////////////////////////////////////////////////////////////
////// GUI Setup.
////////////////////////////////////////////////////////////////////////

#if GUI_MODE == 1 // Render with scene preview.
    bool show_render = false;
#endif

#if GUI_MODE == 2 // Update window every sample.
    ImageFloat render_target(SCR_WIDTH, SCR_HEIGHT);
    image.fill({0, 0, 0});
    render_target.fill({0, 0, 0});
    double render_elapsed_time = 0;
    Timer render_timer;
    int integrated_spp = 0;
#endif

    float test = 0.f;

    SETUP_FPS();
    Timer t;
    while (!windowShouldClose(window)) {

////////////////////////////////////////////////////////////////////////
////// Rendering Loop.
////////////////////////////////////////////////////////////////////////

#if GUI_MODE == 1 // Render with scene preview.
        gui->text(image, "Control Panel");
        if (!show_render) {
            image.fill({0, 0, 0});

            scene.draw_wireframe(image, {1, 1, 1});
            scene.accel->draw_wireframe(image, scene.get_transform(), {1, 0, 1});
        }

        if (show_render) {
            if (gui->button(image, "Preview Scene")) {
                show_render = !show_render;
            }
        }
        else {
            if (gui->button(image, "Render Scene")) {
                show_render = !show_render;
                image.fill({0, 0, 0});
#if INTEGRATOR_VER == 1
                integrator.render(
                    image,
                    scene,
                    render_mode,
                    SPP,
                    sampling_scheme);
#else
                integrator2.render(image, scene, SPP);
#endif

                image.write_PNG(generate_output_filename());
            }
        }
        gui->slider_float(image, &scene.camera.move_speed, .5f, 10.f);
#endif

#if GUI_MODE == 2 // Update window every sample.

        render_timer.update();
#if INTEGRATOR_VER == 1
        integrator.render_N_samples(
            render_target,
            scene,
            render_mode,
            display_interval,
            integrated_spp,
            sampling_scheme);
#else
        integrator2.render_N_samples(
            render_target,
            scene,
            display_interval,
            integrated_spp);
#endif
        integrated_spp += display_interval;
        render_timer.update();
        render_elapsed_time += render_timer.delta_time();

        for (int y = 0; y < image.height; ++y) {
#ifdef _OPENMP
#pragma omp parallel for
#endif
            for (int x = 0; x < image.width; ++x) {
                auto color = render_target.color_at(x, y);
                color = reinhard_tone_mapping(color);
                color = gamma_correction(color);
                image.set_pixel(x, y, color);
            }
        }

        if (integrated_spp >= SPP) {
            image.write_PNG(generate_output_filename());
            destroyWindow(window);
        }

        gui->text(image, ("SPP: " + std::to_string(integrated_spp) + "/" + std::to_string(SPP)).c_str());
        auto render_remaining_time = render_elapsed_time * (SPP - integrated_spp) / integrated_spp;
        gui->text(image, ("Elapsed time: " + std::to_string(render_elapsed_time) + "s").c_str());
        gui->text(image, ("Remaining time: " + std::to_string(render_remaining_time) + "s").c_str());
#endif

        gui->tick();

        UPDATE_FPS();
        swapBuffer(window);
        pollEvent();
    }

    delete gui;

    terminateApplication();
    return 0;
}

void keyboardEventCallback(AppWindow *window, KEY_CODE key, bool pressed) {
    __unused_variable(window);
    if (pressed) {
        switch (key) {
        case KEY_A:
            scene.camera.move({-1, 0});
            break;
        case KEY_S:
            scene.camera.move({0, -1});
            break;
        case KEY_D:
            scene.camera.move({1, 0});
            break;
        case KEY_W:
            scene.camera.move({0, 1});
            break;
        case KEY_ESCAPE:
            destroyWindow(window);
            break;
        case KEY_SPACE:
            break;
        default:
            return;
        }
    }
}
bool first_drag = false;
void mouseButtonEventCallback(AppWindow *window, MOUSE_BUTTON button, bool pressed, float x, float y) {
    __unused_variable(window);
    // GUI process mouse button.
    if (button == BUTTON_L && pressed) {
        first_drag = true;
    }
    gui->process_mouse_button_event(button, pressed, x, y);
}
void mouseScrollEventCallback(AppWindow *window, float offset) {
    __unused_variable(window);
    __unused_variable(offset);
}
float2 last_pos;
void mouseDragEventCallback(AppWindow *window, float x, float y) {
    __unused_variable(window);
    // GUI process mouse drag.
    if (first_drag) {
        last_pos.x = x;
        last_pos.y = y;
        first_drag = false;
    }
    else {
        auto delta = float2{ x, y } - last_pos;
        scene.camera.rotate(delta);
        last_pos.x = x;
        last_pos.y = y;
    }
    gui->process_mouse_drag_event(x, y);
}

std::string generate_output_filename() {
    std::string filename = "output/";
#ifdef USE_SIMPLE_SCENE
    filename += "Simple_";
#else
    filename += scene_name + "_";
#endif
    filename += std::to_string(SPP) + "SPP_";
    filename += std::to_string(SCR_WIDTH) + "X" + std::to_string(SCR_HEIGHT) + "_";
    switch (material_type) {
    case Scene::MaterialType::BlinnPhong:
        filename += "BlinnPhong_"; break;
    case Scene::MaterialType::DisneyBSDF:
        filename += "DisneyBSDF_"; break;
    }
#if INTEGRATOR_VER == 1
        filename += "VER_1";
#elif INTEGRATOR_VER == 2
    if (scene.integrator_info.use_mis)
        filename += "VER_2_MIS";
    else
        filename += "VER_2_AREA";
#elif INTEGRATOR_VER == 3
        filename += "VER_3";
#endif
    filename += ".png";
    return filename;
}
