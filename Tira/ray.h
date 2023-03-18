//
// Created by Ziyi.Lu 2022/12/07
//

#ifndef RAY_H
#define RAY_H

#include <utils.h>
#include <material.h>

namespace tira {

    struct Object;

    struct Ray {
#ifdef ENABLE_SIMD
        union {
            float3 origin;
            __m128 o4;
        };
        union {
            float3 direction;
            __m128 d4;
        };
        union {
            float3 inv_dir;
            __m128 id4;
        };
#else
        float3 origin;
        // Do not modify direction directly, call set_direction instead.
        float3 direction;
        float3 inv_dir;
#endif
        int3 sign;
        float const t_min = sEPSILON; // Use larger epsilon to avoid seam in mesh.
        float const t_max = std::numeric_limits<float>::max();
        bool shadow_ray = false;
        unsigned int depth = 0; // Depth of the ray, used in Integrator3, BDPT.
        bool is_delta = false;

        Ray() = delete;

        Ray(float3 const& o, float3 const& d) : origin(o) {
            set_direction(d);
        }

        void set_origin(float3 const& o) {
            origin = o;
        }

        void set_direction(float3 const& d) {
            direction = d.normalized();

            inv_dir.x = 1 / direction.x;
            inv_dir.y = 1 / direction.y;
            inv_dir.z = 1 / direction.z;

            sign.x = (inv_dir.x < 0);
            sign.y = (inv_dir.y < 0);
            sign.z = (inv_dir.z < 0);
        }

        float3 at(float t) const {
            return origin + direction * t;
        }
    };

    struct Intersection {
        bool hit = false;
        bool back_face = false;
        float3 position;
        float3 normal;
        float3 tangent;
        float3 bitangent;
        float distance = std::numeric_limits<float>::max();
        float2 uv;
        Material* material = nullptr;
        Object* object = nullptr;
    };

} // namespace tira

#endif