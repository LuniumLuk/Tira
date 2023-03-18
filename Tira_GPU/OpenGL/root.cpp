#include <OpenGL/Root.h>
#include <OpenGL/timer.h>
#include <OpenGL/window.h>
#include <OpenGL/command.h>
#include <OpenGL/assetmanager.h>

namespace GL {

Root* Root::root = nullptr;

Root::Root() {
    root = this;
    timer = new Timer();
    window = new Window(WindowDesc{});
    assetManager = new AssetManager();
    commandList = new CommandList();
}

Root::Root(WindowDesc desc) {
    root = this;
    timer = new Timer();
    window = new Window(desc);
    assetManager = new AssetManager();
    commandList = new CommandList();
}

Root::~Root() {
    delete window;
    delete timer;
    delete assetManager;
    delete commandList;
}

auto Root::startFrame() noexcept -> bool {
    get()->window->preframe();
    get()->commandList->invoke();
    if (!get()->window->update())
        return true;
    get()->timer->update();
    return false;
}

auto Root::endFrame() noexcept -> bool {
    get()->window->endframe();
    return false;
}

}
