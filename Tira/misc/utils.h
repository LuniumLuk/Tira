//
// Created by Ziyi.Lu 2022/12/05
//

#ifndef UTILS_H
#define UTILS_H

#include <cmath>
#include <vector>
#include <random>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <macro.h>
#include <math/vector.h>
#include <math/matrix.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#ifdef ENABLE_SIMD
#include <math/simd.h>
#endif

#define SETUP_FPS()             \
float fixed_delta = 0.16f;      \
float from_last_fixed = 0.0f;   \
int frame_since_last_fixed = 0

#define UPDATE_FPS() do {                                               \
t.update();                                                             \
from_last_fixed += t.delta_time();                                       \
++frame_since_last_fixed;                                               \
if (from_last_fixed > fixed_delta) {                                    \
    int fps = std::round(frame_since_last_fixed / from_last_fixed);     \
    std::string title = "Viewer @ LuGL FPS: " + std::to_string(fps);    \
    setWindowTitle(window, title.c_str());                              \
    from_last_fixed = 0.0f;                                             \
    frame_since_last_fixed = 0;                                         \
}                                                                       \
} while(0) 
        
// Generate random float from 0 to 1.
#define rnd() (static_cast<float>(rand())/static_cast<float>(RAND_MAX))

namespace tira {

    // Convert float value in scene to integer value in image space.
    inline int ftoi(float x) {
        // Difference of (int)std::floor(x) and static_cast<int>(x):
        // - (int)std::floor(x) cast x toward -INFINITY
        // - static_cast<int>(x) cast x toward 0
        // Although we ignore negative value in image space, -0.5 will still be cast to 0
        // which is not correct theoretically.
        return std::floor(x);
    }

    // Convert integer value in image space to float value in scene.
    inline float itof(int x) {
        return x + 0.5f;
    }

    template<typename T, typename U, typename V>
    inline T clamp(T x, U min, V max) {
        x = x < min ? min : x;
        x = x > max ? max : x;
        return x;
    }

    template<typename T>
    inline T lerp(T x, T y, float a) {
        return x * (1.f - a) + y * a;
    }

    constexpr float FLOAT_MAX = std::numeric_limits<float>::max();
    constexpr float FLOAT_MIN = std::numeric_limits<float>::min();
    constexpr float PI = 3.14159265;
    constexpr float INV_PI = 1 / PI;
    constexpr float INV_TWO_PI = 0.5 / PI;
    constexpr float PI_DIV_TWO = PI / 2;
    constexpr float PI_DIV_THREE = PI / 3;
    constexpr float PI_DIV_FOUR = PI / 4;
    constexpr float TWO_PI = PI * 2;
    constexpr float EPSILON = 1e-6;
    constexpr float sEPSILON = 1e-3; // just a larger epsilon
    constexpr float rEPSILON = 1e-10;    // epsilon for ray origin offset
    constexpr float GAMMA = 2.2;
    constexpr float ONE_DIV_GAMMA = 1 / 2.2;

    inline bool isglass(float ior, float3 const& transmittance) {
        return !(std::abs(1 - ior) < EPSILON || transmittance.max_component() < EPSILON);
    }

    inline bool isfinite(float3 const& v) {
        return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
    }

    inline float deg2rad(float d) {
        return d * PI / 180.f;
    }

    inline float rad2deg(float r) {
        return r * 180.f * INV_PI;
    }

    constexpr unsigned int HAMMERSLEY_N = 1024;

    inline float2 hammersley(float i) {
        unsigned int b = (unsigned int)(i * HAMMERSLEY_N);

        b = (b << 16u) | (b >> 16u);
        b = ((b & 0x55555555u) << 1u) | ((b & 0xAAAAAAAAu) >> 1u);
        b = ((b & 0x33333333u) << 2u) | ((b & 0xCCCCCCCCu) >> 2u);
        b = ((b & 0x0F0F0F0Fu) << 4u) | ((b & 0xF0F0F0F0u) >> 4u);
        b = ((b & 0x00FF00FFu) << 8u) | ((b & 0xFF00FF00u) >> 8u);

        float radicalInverseVDC = float(b) * 2.3283064365386963e-10;

        return { i, radicalInverseVDC };
    }

    inline float random_float() {
        static std::random_device dev;
        static std::mt19937 rng(dev());
        std::uniform_real_distribution<float> dist(0.f, 1.f);

        return dist(rng);
    }

    inline float2 random_float2() {
        return { random_float(), random_float() };
        // return hammersley(random_float());
    }

    inline float2 concentric_sample_dist(float2 const& u) {
        auto offset = u * 2.f - 1.f;
        if (offset.x == 0 && offset.y == 0) return float2::zero();

        float theta, r;
        if (std::abs(offset.x) > std::abs(offset.y)) {
            r = offset.x;
            theta = PI_DIV_FOUR * (offset.y / offset.x);
        }
        else {
            r = offset.y;
            theta = PI_DIV_TWO - PI_DIV_FOUR * (offset.x / offset.y);
        }
        return { std::cos(theta) * r, std::sin(theta) * r };
    }

    inline float3 random_float3_on_unit_hemisphere() {
        float2 u = random_float2();
        auto phi = u.y * TWO_PI;
        auto z = std::abs(1 - 2 * u.x);
        auto r = std::sqrt(1 - z * z);
        auto x = r * std::cos(phi);
        auto y = r * std::sin(phi);
        return { x, y, z };
    }

    inline float3 cosine_sample_hemisphere(float2 const& u) {
        float2 d = concentric_sample_dist(u);
        float z = std::sqrt(std::max(0.f, 1.f - d.x * d.x - d.y * d.y));
        return { d.x, d.y, z };
    }

    inline float3 random_unit_float3() {
        float2 u = random_float2();
        auto x = std::cos(TWO_PI * u.x) * 2 * std::sqrt(u.y * (1 - u.y));
        auto y = std::sin(TWO_PI * u.x) * 2 * std::sqrt(u.y * (1 - u.y));
        auto z = 1 - 2 * u.y;
        return { x, y, z };
    }

    inline float3 random_float3_in_unit_sphere() {
        return random_unit_float3() * random_float();
    }

    inline float3 local_to_world(float3 const& dir, float3 const& N) {
        float3 B;
        if (std::fabs(N.x) > std::fabs(N.y)) {
            float len_inv = 1 / std::sqrt(N.x * N.x + N.z * N.z);
            B.x = N.z * len_inv;
            B.z = -N.x * len_inv;
        }
        else {
            float len_inv = 1 / std::sqrt(N.y * N.y + N.z * N.z);
            B.y = N.z * len_inv;
            B.z = -N.y * len_inv;
        }
        float3 T = B.cross(N);
        return T * dir.x + B * dir.y + N * dir.z;
    }

    inline float3 spherical_to_cartesian(float theta, float phi) {
        auto sin_theta = std::sin(theta);
        auto cos_theta = std::cos(theta);
        return {
            sin_theta * std::cos(phi),
            sin_theta * std::sin(phi),
            cos_theta,
        };
    }

    inline float3 spherical_to_cartesian(float sin_theta, float cos_theta, float phi) {
        return {
            sin_theta * std::cos(phi),
            sin_theta * std::sin(phi),
            cos_theta,
        };
    }

    inline float pow2(float x) {
        return x * x;
    }

    inline bool same_hemisphere(float3 const& wo, float3 const& wi, float3 const& N) {
        return wo.dot(N) * wi.dot(N) > 0.f;
    }

    // Tone mapping & Gamma correction.

    inline colorf reinhard_tone_mapping(colorf const& color) {
        return color / (color + 1);
    }

    inline colorf ACES_tone_mapping(colorf color) {
        float const A = 2.51f;
        float const B = 0.03f;
        float const C = 2.43f;
        float const D = 0.59f;
        float const E = 0.14f;

        return saturate((color * (color * A + B)) / (color * (color * C + D) + E));
    }

    inline colorf gamma_transform(colorf const& color) {
        return {
            std::pow(color.r, GAMMA),
            std::pow(color.g, GAMMA),
            std::pow(color.b, GAMMA),
        };
    }

    inline colorf gamma_correction(colorf const& color) {
        return {
            std::pow(color.r, ONE_DIV_GAMMA),
            std::pow(color.g, ONE_DIV_GAMMA),
            std::pow(color.b, ONE_DIV_GAMMA),
        };
    }

    inline float color_to_luminance(colorf const& color) {
        return 0.3f * color.r + 0.59f * color.g + 0.11f * color.b;
    }

    inline void print_progress2(int s, int spp, float delta_time, float total_time, int bar_width = 50) {
        float progress = (s + 1) / (float)spp;
        int second = total_time * (spp - s - 1) / (s + 1);
        int minute = second / 60;
        int hour = minute / 60;
        std::cout << "[";
        int pos = bar_width * progress;
        for (int i = 0; i < bar_width; ++i) {
            if (i < pos) std::cout << "=";
            else if (i == pos) std::cout << ">";
            else std::cout << " ";
        }
        std::cout << "] " << int(progress * 100.f) << "% (" << (s + 1) << "/" << spp << ") Estimated Time Left: "
            << std::setfill('0') << std::setw(2) << hour << ":"
            << std::setfill('0') << std::setw(2) << minute % 60 << ":"
            << std::setfill('0') << std::setw(2) << second % 60 << "\r";
        std::cout.flush();
    }

    inline void print_progress(float progress, float estimated_time, int bar_width = 50) {
        std::cout << "[";
        int pos = bar_width * progress;
        for (int i = 0; i < bar_width; ++i) {
            if (i < pos) std::cout << "=";
            else if (i == pos) std::cout << ">";
            else std::cout << " ";
        }
        std::cout << "] " << int(progress * 100.f) << "% Estimated Time Left: "
            << estimated_time << "s\r";
        std::cout.flush();
    }

} // namespace tira

#endif
