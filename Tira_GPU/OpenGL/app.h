#pragma once

namespace GL {

struct Application
{
public:
    auto run() noexcept -> void;
    auto terminate() noexcept -> void;
    double const fixedUpdateDelta = 0.016;

private:
    virtual auto init() noexcept -> void = 0;
    virtual auto update(double deltaTime) noexcept -> void = 0;
    virtual auto fixedUpdate() noexcept -> void {}
    virtual auto quit() noexcept -> void {}
    double accumulatedTime = 0.0;
    bool shouldExit = false;
};

}
