#pragma once

#if defined(_MSC_VER)
// type conversion warning
#pragma warning(disable : 4305)
#endif

#include <glm/glm.hpp>
#include <OpenGL/buffer.h>

namespace GL {

struct Camera {
    Camera();
    Camera(
        glm::vec3 eye,
        glm::vec3 at,
        glm::vec3 up,
        float verticalFov = 90.0f,
        float aspectRatio = 1.0f,
        float aperture = 0.1f,
        float focalLenght = 1.0f);

    glm::vec3 mEye = glm::vec3(0.0, 0.0, -1.0);
    glm::vec3 mAt = glm::vec3(0.0, 0.0, 0.0);
    glm::vec3 mUp = glm::vec3(0.0, 1.0, 0.0);
    glm::vec3 mHorizontal = glm::vec3(0.0, 0.0, 0.0);
    glm::vec3 mVertical = glm::vec3(0.0, 0.0, 0.0);
    glm::vec3 mLowerLeftCorner = glm::vec3(0.0, 0.0, 0.0);
    glm::vec3 mU = glm::vec3(0.0, 0.0, 0.0);
    glm::vec3 mV = glm::vec3(0.0, 0.0, 0.0);

    float mFocalLength = 1.0;
    float mLensRadius = 1.0;
    float mShutterSpeed = 1.0;
    float mViewportHeight = 1.0;
    float mViewportWidth = 1.0;

    // camera motion
    float mVelocity = 4.0;
    float mMouseSpeed = 0.1;
    bool mFirstMouseCall = true;
    glm::vec2 mMouseLastPos = glm::vec2(0.0, 0.0);

    auto processInput() -> void;
    auto update() -> void;
    auto yaw(float degree) -> void;
    auto pitch(float degree) -> void;
    auto rotate(glm::vec2 const& pitchYaw) -> void;
    auto move(glm::vec2 const& movement) -> void;
    auto updateStorageBuffer(StorageBuffer * buffer) -> void;
};

}
