#include <OpenGL/app.h>
#include <OpenGL/root.h>
#include <OpenGL/timer.h>

namespace GL {

auto Application::run() noexcept -> void {
    init();
    while (!shouldExit) {
        if (Root::get()->startFrame()) {
            terminate();
            break;
        }
        double deltaTime = Root::get()->timer->deltaTime();
        accumulatedTime += deltaTime;
        while (accumulatedTime > fixedUpdateDelta) {
            fixedUpdate();
            accumulatedTime -= fixedUpdateDelta;
        }
        update(deltaTime);
        Root::get()->endFrame();
    }
    quit();
}

auto Application::terminate() noexcept -> void {
    shouldExit = true;
}

}
