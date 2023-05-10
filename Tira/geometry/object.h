//
// Created by Ziyi.Lu 2022/12/06
//

#ifndef OBJECT_H
#define OBJECT_H

#include <misc/utils.h>
#include <misc/image.h>
#include <geometry/ray.h>
#include <scene/material.h>
#ifdef ENABLE_SIMD
#include <math/simd.h>
#endif

namespace tira {

    struct Bound3f {
#ifdef ENABLE_SIMD
        union {
            float3 min;
            __m128 min4;
        };
        union {
            float3 max;
            __m128 max4;
        };
#else
        float3 min;
        float3 max;
#endif

        Bound3f()
            : min(float3::max())
            , max(float3::min()) {}

        Bound3f(float3 const& _min, float3 const& _max)
            : min(_min)
            , max(_max) {}

        float3 const& operator[] (int idx) const {
            switch (idx) {
            case 0:
                return min;
            case 1:
                return max;
            default:
                return min;
            }
        }

        void operator+= (Bound3f const& other) {
            min = float3::min(min, other.min);
            max = float3::max(max, other.max);
        }

        void operator+= (float3 const& point) {
            min = float3::min(min, point);
            max = float3::max(max, point);
        }

        float3 get_extent() const {
            return max - min;
        }

        float3 get_center() const {
            return (min + max) * .5f;
        }

        float get_surface_area() const {
            auto extent = get_extent();
            return (extent.x * extent.y + extent.y * extent.z + extent.z * extent.x) * 2;
        }

        float distant_to(float3 const& point) const {
            float dist = 0;

            for (int i = 0; i < 3; ++i) {
                float value = 0;
                if (point[i] < min[i])
                    value = min[i] - point[i];
                else if (point[i] > max[i])
                    value = point[i] - max[i];
                dist += value * value;
            }

            return std::sqrt(dist);
        }

        float distant_to(Bound3f const& other) const {
            float dist = 0;

            for (int i = 0; i < 3; ++i) {
                float value = 0;
                if (other.max[i] < min[i])
                    value = min[i] - other.max[i];
                else if (other.min[i] > max[i])
                    value = other.min[i] - max[i];
                dist += value * value;
            }

            return std::sqrt(dist);
        }

        void draw_wireframe(Image& image, float4x4 const& transform, colorf const& color) const {
            const int lines[12][2] = {
                {0, 1}, {2, 3}, {0, 2}, {1, 3},
                {0, 4}, {1, 5}, {2, 6}, {3, 7},
                {4, 5}, {6, 7}, {4, 6}, {5, 7},
            };

            float4 v[8] = {
                { min.x, min.y, min.z, 1 },
                { max.x, min.y, min.z, 1 },
                { min.x, max.y, min.z, 1 },
                { max.x, max.y, min.z, 1 },
                { min.x, min.y, max.z, 1 },
                { max.x, min.y, max.z, 1 },
                { min.x, max.y, max.z, 1 },
                { max.x, max.y, max.z, 1 },
            };

            int2 vs[8];

            for (int i = 0; i < 8; ++i) {
                v[i] = transform * v[i];
                v[i].w = 1 / v[i].w;
                v[i].x *= v[i].w;
                v[i].y *= v[i].w;
                v[i].z *= v[i].w;

                v[i].x = ftoi((v[i].x * .5f + .5f) * image.width);
                v[i].y = ftoi((v[i].y * .5f + .5f) * image.height);

                vs[i] = int2{ (int)v[i].x, (int)v[i].y };
            }

            for (int i = 0; i < 12; ++i) {
                if (v[lines[i][0]].w < 0 || v[lines[i][1]].w < 0) {
                    continue;
                }
                image.draw_line(vs[lines[i][0]], vs[lines[i][1]], color);
            }
        }

#ifdef ENABLE_SIMD
        float intersect(Ray const& ray) const {
            static __m128 mask = _mm_cmpeq_ps(_mm_setzero_ps(), _mm_set_ps(1, 0, 0, 0));
            __m128 t1 = _mm_mul_ps(_mm_sub_ps(_mm_and_ps(min4, mask), ray.o4), ray.id4);
            __m128 t2 = _mm_mul_ps(_mm_sub_ps(_mm_and_ps(max4, mask), ray.o4), ray.id4);
            __m128 vmax4 = _mm_max_ps(t1, t2);
            __m128 vmin4 = _mm_min_ps(t1, t2);
            float tmax = std::min(_access_m128f(&vmax4, 0), std::min(_access_m128f(&vmax4, 1), _access_m128f(&vmax4, 2)));
            float tmin = std::max(_access_m128f(&vmin4, 0), std::max(_access_m128f(&vmin4, 1), _access_m128f(&vmin4, 2)));
            if (tmax >= tmin && tmax > ray.t_min && tmin < ray.t_max) return tmin;
            return FLOAT_MAX;
        }
#else
        float intersect(Ray const& ray) const {
            float tmin, tmax, tymin, tymax, tzmin, tzmax;

            tmin = ((*this)[ray.sign.x].x - ray.origin.x) * ray.inv_dir.x;
            tmax = ((*this)[1 - ray.sign.x].x - ray.origin.x) * ray.inv_dir.x;
            tymin = ((*this)[ray.sign.y].y - ray.origin.y) * ray.inv_dir.y;
            tymax = ((*this)[1 - ray.sign.y].y - ray.origin.y) * ray.inv_dir.y;

            if ((tmin > tymax) || (tymin > tmax)) return FLOAT_MAX;

            if (tymin > tmin) tmin = tymin;
            if (tymax < tmax) tmax = tymax;

            tzmin = ((*this)[ray.sign.z].z - ray.origin.z) * ray.inv_dir.z;
            tzmax = ((*this)[1 - ray.sign.z].z - ray.origin.z) * ray.inv_dir.z;

            if ((tmin > tzmax) || (tzmin > tmax)) return FLOAT_MAX;

            return std::max(tzmin, tmin);
        }
#endif
    };

    inline std::ostream& operator<<(std::ostream& os, Bound3f const& b) {
        os << "min: " << b.min << "\n"
            << "max: " << b.max;
        return os;
    }

    struct Object {
        Material* material = nullptr;

        Object() {}
        virtual ~Object() {}

        virtual void draw_wireframe(Image& image, float4x4 const& transform, colorf const& color) const = 0;
        virtual float3 get_center() const = 0;
        virtual float3 get_min() const = 0;
        virtual float3 get_max() const = 0;
        virtual Bound3f get_bound() const = 0;
        virtual float get_area() const = 0;

        virtual void intersect(Ray const& ray, Intersection& isect) const = 0;
        virtual void sample(Intersection& isect, float& pdf) const = 0;
    };

} // namespace tira

#endif
