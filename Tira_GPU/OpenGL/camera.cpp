#include <OpenGL/camera.h>
#include <OpenGL/root.h>
#include <OpenGL/window.h>
#include <OpenGL/timer.h>
#include <OpenGL/io.h>
#include <OpenGL/util.h>
#include <glm/gtc/quaternion.hpp>

namespace GL {

Camera::Camera()
    : mEye(glm::vec3(0.0, 0.0, -2.0))
    , mAt(glm::vec3(0.0, 0.0, 0.0))
    , mUp(glm::vec3(0.0, 1.0, 0.0))
    , mFocalLength(1.0) 
{
    float theta = DegreeToRadian(90.0);
    float h = tanf(theta * 0.5f);
    mViewportHeight = 2.0f * h;
    mViewportWidth = 1.0 * mViewportHeight;
    mLensRadius = 0.1 * 0.5f;

    update();
}


Camera::Camera(
    glm::vec3 eye,
    glm::vec3 at,
    glm::vec3 up,
    float verticalFov,
    float aspectRatio,
    float aperture,
    float focalLenght)
    : mEye(eye)
    , mAt(at)
    , mUp(glm::normalize(up))
    , mFocalLength(focalLenght)
{
    float theta = DegreeToRadian(verticalFov);
    float h = tanf(theta * 0.5f);
    mViewportHeight = 2.0f * h;
    mViewportWidth = aspectRatio * mViewportHeight;
    mLensRadius = aperture * 0.5f;

    update();
}

auto Camera::processInput() -> void {
    auto window = Root::get()->window;
    glm::vec2 movement(0.0, 0.0);
    if (window->queryInputPressed(KEY_D)) {
        movement.x += 1.0;
    }
    if (window->queryInputPressed(KEY_A)) {
        movement.x -= 1.0;
    }
    if (window->queryInputPressed(KEY_W)) {
        movement.y += 1.0;
    }
    if (window->queryInputPressed(KEY_S)) {
        movement.y -= 1.0;
    }

    movement *= mVelocity * Root::get()->timer->deltaTime();
    
    double xpos, ypos;
    window->queryMousePosition(&xpos, &ypos);
    glm::vec2 mousePos(xpos, ypos);

    if (mFirstMouseCall) {
        mMouseLastPos = mousePos;
        mFirstMouseCall = false;
    }

    glm::vec2 view = mousePos - mMouseLastPos;
    mMouseLastPos = mousePos;

    view *= mMouseSpeed * Root::get()->timer->deltaTime();

    yaw(-view.x);
    pitch(-view.y);
    move(movement);
}

auto Camera::update() -> void {
    glm::vec3 w = glm::normalize(mEye - mAt);
    mU = glm::normalize(glm::cross(mUp, w));
    mV = glm::cross(w, mU);

    mHorizontal = mFocalLength * mViewportWidth * mU;
    mVertical = mFocalLength * mViewportHeight * mV;
    mLowerLeftCorner = mEye - mHorizontal * 0.5f - mVertical * 0.5f - mFocalLength * w;
}

auto Camera::yaw(float degree) -> void {
    glm::mat4 transform(1.0);
    transform = glm::rotate(transform, degree, mUp);

    glm::vec3 forward = mAt - mEye;
    forward = glm::vec3(transform * glm::vec4(forward, 1.0));

    mAt = mEye + forward;

    update();
}

auto Camera::pitch(float degree) -> void {
    glm::mat4 transform(1.0);
    glm::vec3 forward = mAt - mEye;
    glm::vec3 right = glm::cross(forward, mUp);
    transform = glm::rotate(transform, degree, right);

    forward = glm::normalize(glm::vec3(transform * glm::vec4(forward, 1.0)));
    float cos = glm::dot(forward, mUp);
    if (1.0 - fabs(cos) < 0.0001) {
        return;
    }

    mAt = mEye + forward;

    update();
}

auto Camera::rotate(glm::vec2 const& pitchYaw) -> void {
    glm::mat4 transform(1.0);
    glm::vec3 forward = mAt - mEye;
    glm::vec3 right = glm::cross(forward, mUp);
    transform = glm::rotate(transform, pitchYaw.y, mUp);
    transform = glm::rotate(transform, pitchYaw.x, right);

    forward = glm::normalize(glm::vec3(transform * glm::vec4(forward, 1.0)));
    float cos = glm::dot(forward, mUp);
    if (1.0 - fabs(cos) < 0.0001) {
        return;
    }
    
    mAt = mEye + forward;
    update();
}

auto Camera::move(glm::vec2 const& movement) -> void {
    glm::vec3 f = glm::normalize(mAt - mEye);
    glm::vec3 r = glm::normalize(glm::cross(f, mUp));
    glm::vec3 shift = f * movement.y + r * movement.x;

    mAt += shift;
    mEye += shift;

    update();
}

/**
    layout(std430, binding = 3) buffer Camera
    {								// Offset
        vec3 CameraOrigin;			// 0
        vec3 CameraLowerLeftCorner;	// 16
        vec3 CameraHorizontal;		// 32
        vec3 CameraVertical;		// 48
        vec3 CameraU;				// 60
        vec3 CameraV;				// 76
        float CameraLensRadius;		// 92
        // Size: 108
    };
**/
auto Camera::updateStorageBuffer(StorageBuffer* buffer) -> void {
    buffer->update(&mEye.x, 0, 3 * sizeof(float));
    buffer->update(&mLowerLeftCorner.x, 16, 3 * sizeof(float));
    buffer->update(&mHorizontal.x, 32, 3 * sizeof(float));
    buffer->update(&mVertical.x, 48, 3 * sizeof(float));
    buffer->update(&mU.x, 60, 3 * sizeof(float));
    buffer->update(&mV.x, 76, 3 * sizeof(float));
    buffer->update(&mLensRadius, 92, sizeof(float));
}

}
