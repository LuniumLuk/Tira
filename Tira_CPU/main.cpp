//
// Created by Ziyi.Lu 2022/12/05
//

#include <iostream>
#include <vector>
#include <cmath>
#include <tira.h>

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

static std::string generate_output_filename();

GUI* gui;
Scene scene;

int SPP = 1;
int image_width = 1024;
int image_height = 1024;
int const display_interval = 1;
WhittedIntegrator whittedIntegrator;
MonteCarloIntegrator monteCarloIntegrator;
BidirectionalIntegrator bidirectionalIntegrator;
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
        Scene::MaterialType::BlinnPhong);

    SPP = scene.integrator_info.spp;
    whittedIntegrator.max_depth = scene.integrator_info.max_bounce;
    monteCarloIntegrator.max_depth = scene.integrator_info.max_bounce;
    monteCarloIntegrator.use_mis = scene.integrator_info.use_mis;

    image_width = scene.scr_w;
    image_height = scene.scr_h;

    Image image(image_width, image_height);

#if 1
    //whittedIntegrator.render(image, scene, SPP);
    monteCarloIntegrator.render(image, scene, SPP);
    //bidirectionalIntegrator.render(image, scene, SPP);
    image.write_PNG(generate_output_filename());
    return 0;
#endif

#if 0
    image.fill({ 0, 0, 0 });

    initializeApplication();
    gui = new GUI(scene.scr_w, scene.scr_h, 10, 10, 1);

    const char* title = "Tira [CPU]";
    window = createWindow(title, image_width, image_height, image.data);

    setKeyboardCallback(window, keyboardEventCallback);
    setMouseButtonCallback(window, mouseButtonEventCallback);
    setMouseScrollCallback(window, mouseScrollEventCallback);
    setMouseDragCallback(window, mouseDragEventCallback);

    bool show_render = false;
    SETUP_FPS();
    Timer t;
    while (!windowShouldClose(window)) {

        gui->text(image, "Control Panel");
        if (!show_render) {
            image.fill({ 0, 0, 0 });

            scene.draw_wireframe(image, { 1, 1, 1 });
            scene.accel->draw_wireframe(image, scene.get_transform(), { 1, 0, 1 });
        }

        if (show_render) {
            if (gui->button(image, "Preview Scene")) {
                show_render = !show_render;
            }
        }
        else {
            if (gui->button(image, "Render Scene")) {
                show_render = !show_render;
                image.fill({ 0, 0, 0 });
                monteCarloIntegrator.render(image, scene, SPP);
                image.write_PNG(generate_output_filename());
            }
        }
        gui->slider_float(image, &scene.camera.move_speed, .5f, 10.f);
        gui->tick();

        UPDATE_FPS();
        swapBuffer(window);
        pollEvent();
    }
    delete gui;

    terminateApplication();
    return 0;
#endif
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
    filename += scene_name + "_";
    filename += std::to_string(SPP) + "SPP_";
    filename += std::to_string(image_width) + "X" + std::to_string(image_height) + "_";
    if (scene.integrator_info.use_mis)
        filename += "_MIS";
    else
        filename += "_AREA";
    filename += ".png";
    return filename;
}
