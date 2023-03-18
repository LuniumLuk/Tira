//
// Created by Ziyi.Lu 2022/12/05
//

#ifndef CAMERA_H
#define CAMERA_H

#include <ray.h>

namespace tira {

    struct Camera {
        Camera() {}
        Camera(float3 const& _eye
            , float3 const& _at
            , float3 const& _up
            , float _fov
            , float _aspect)
            : eye(_eye)
            , at(_at)
            , up(_up)
            , fov(_fov)
            , aspect(_aspect) {}

        float4x4 get_proj_view() const;
        float4x4 get_proj() const;
        float4x4 get_view() const;
        void rotate(float2 const& pitchYaw);
        void move(float2 const& movement);

        Ray get_ray_pinhole(int x, int y, int w, int h, float2 const& u0) const;
        Ray get_ray_thin_lens(int x, int y, int w, int h, float2 const& u0, float2 const& u1) const;

        float3x3 get_screen_to_raster() const;
        float3x3 get_raster_to_screen() const;

        float3 eye = { 0,  0, -1 };
        float3 at = { 0,  0,  0 };
        float3 up = { 0,  1,  0 };
        float fov = PI_DIV_THREE;
        float near = 0.01f;
        float far = 100.f;
        float aspect = 1.f;

        // Thin lens camera parameters.
        float focus_length = 4.f;
        float aperature = .1f;

        float move_speed = 0.5f;
        float view_speed = 0.001f;
    };

    inline float4x4 Camera::get_proj_view() const {
        return transform::perspective(fov, aspect, near, far) * transform::lookAt(eye, at, up);
    }

    inline float4x4 Camera::get_proj() const {
        return transform::perspective(fov, aspect, near, far);
    }

    inline float4x4 Camera::get_view() const {
        return transform::lookAt(eye, at, up);
    }

    inline void Camera::rotate(float2 const& pitchYaw) {
        auto transform = float4x4::identity();
        auto forward = at - eye;
        auto right = forward.cross(up);
        transform = transform::rotate(up, -pitchYaw.x * view_speed) * transform;
        transform = transform::rotate(right, pitchYaw.y * view_speed) * transform;

        forward = float3(transform * float4(forward, 1.0f)).normalized();
        float cos = forward.dot(up);
        if (1.0f - fabs(cos) < sEPSILON) {
            return;
        }

        at = eye + forward;
    }

    inline void Camera::move(float2 const& movement) {
        float3 f = (at - eye).normalized();
        float3 r = f.cross(up).normalized();
        float3 shift = (f * movement.y + r * movement.x) * move_speed;

        at = at + shift;
        eye = eye + shift;
    }

    inline  float3x3 Camera::get_screen_to_raster() const {
        // In camera space, the image plane is at z = 1
        auto vh = tanf(fov * .5f);
        auto vw = vh * aspect;

        auto forward = (at - eye).normalized();
        auto right = forward.cross(up).normalized();
        auto up = right.cross(forward);

        // [TODO] cache the result and calculate the inverse of this matrix
        return float3x3(right * vw, up * vh, forward);

        // auto dir = forward
        //          + right * vw * u
        //          + up * vh * v;
    }

    inline  float3x3 Camera::get_raster_to_screen() const {
        return get_screen_to_raster().inversed();
    }

    inline Ray Camera::get_ray_pinhole(int x, int y, int w, int h, float2 const& u0) const {
        auto u = (itof(x) + u0.x) / w * 2 - 1;
        auto v = (itof(y) + u0.y) / h * 2 - 1;

        auto screen_to_raster = get_screen_to_raster();
        auto dir = screen_to_raster * float3(u, v, 1);

        return Ray(eye, dir);
    }

    inline Ray Camera::get_ray_thin_lens(int x, int y, int w, int h, float2 const& u0, float2 const& u1) const {
        auto u = (itof(x) + u0.x) / w * 2 - 1;
        auto v = (itof(y) + u0.y) / h * 2 - 1;
        auto vh = tanf(fov * .5f);
        auto vw = vh * aspect;

        auto forward = (at - eye).normalized();
        auto right = forward.cross(up).normalized();
        auto up = right.cross(forward);

        forward = forward * focus_length;
        right = right * focus_length * vw;
        up = up * focus_length * vh;

        auto focus_offset = right * (u1.x * 2 - 1)
            + up * (u1.y * 2 - 1);
        focus_offset = focus_offset * aperature / 2;

        auto dir = forward
            + right * u
            + up * v;

        return Ray(eye + focus_offset, dir - focus_offset);
    }

} // namespace tira

#endif
