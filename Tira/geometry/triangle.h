//
// Created by Ziyi.Lu 2022/12/05
//

#ifndef TRIANGLE_H
#define TRIANGLE_H

#include <misc/utils.h>
#include <scene/scene.h>
#include <geometry/object.h>

namespace tira {

    inline float4 to_ndc(float3 const& v, float4x4 const& transform) {
        float4 ndc = float4(v, 1.f);
        ndc = transform * ndc;
        ndc.w = 1 / ndc.w;
        ndc.x *= ndc.w;
        ndc.y *= ndc.w;
        ndc.z *= ndc.w;
        return ndc;
    }

    struct Triangle : Object {
        float3 pos[3];
        float3 normal;
        float3 center;
        float3 tangent;
        float3 bitangent;
        float3 vn[3];
        float2 vt[3];
        float3 e01, e02;
        Bound3f bound;
        bool has_vn = false;
        bool has_vt = false;
        float area = 0.0f;

        Triangle() {}
        virtual ~Triangle() {}

        virtual void intersect(Ray const& ray, Intersection& isect) const override {
            auto pvec = ray.direction.cross(e02);
            auto det = e01.dot(pvec);
            // Parallel to triangle.
            if (fabs(det) < EPSILON) {
                return;
            }

            auto det_inv = 1 / det;
            auto tvec = ray.origin - pos[0];
            auto u = tvec.dot(pvec) * det_inv;
            if (u < 0 || u > 1) {
                return;
            }

            auto qvec = tvec.cross(e01);
            auto v = ray.direction.dot(qvec) * det_inv;
            if (v < 0 || u + v > 1) {
                return;
            }

            auto t = e02.dot(qvec) * det_inv;

            if (t < ray.t_min || t > ray.t_max) {
                return;
            }

#ifdef TRIANGLE_TOLERATE_LIGHT_CLOSE_TO_SURFACE
            float diff = t - isect.distance;
            if (isect.material && isect.material->emissive) {
                if (diff > -EPSILON) {
                    return;
                }
            }
            else if (material->emissive) {
                if (diff > EPSILON) {
                    return;
                }
            }
            else {
                if (diff > 0) {
                    return;
                }
            }
#else
            if (t > isect.distance) {
                return;
            }
#endif

            isect.hit = true;
            isect.object = (Object*)this;
            isect.material = material;
            isect.distance = t;

            if (ray.shadow_ray) {
                return;
            }

            isect.back_face = ray.direction.dot(normal) > 0;
            isect.position = ray.at(t);
            if (has_vn) {
                isect.normal = normalize(vn[0] * (1 - u - v) + vn[1] * u + vn[2] * v);
            }
            else {
                isect.normal = normal;
            }
            isect.uv = vt[0] * (1 - u - v) + vt[1] * u + vt[2] * v;
            isect.tangent = tangent;
            isect.bitangent = bitangent;
        }

        virtual void draw_wireframe(Image& image, float4x4 const& transform, colorf const& color) const override {
            auto v0 = to_ndc(pos[0], transform);
            auto v1 = to_ndc(pos[1], transform);
            auto v2 = to_ndc(pos[2], transform);

            // Back-face culling.
            auto e01 = float3(v1) - float3(v0);
            auto e02 = float3(v2) - float3(v0);
            auto N = e01.cross(e02);
            if (N.z < 0) return;

            // Screen mapping.
            int2 v0s = {
                ftoi((v0.x * .5f + .5f) * image.width),
                ftoi((v0.y * .5f + .5f) * image.height),
            };
            int2 v1s = {
                ftoi((v1.x * .5f + .5f) * image.width),
                ftoi((v1.y * .5f + .5f) * image.height),
            };
            int2 v2s = {
                ftoi((v2.x * .5f + .5f) * image.width),
                ftoi((v2.y * .5f + .5f) * image.height),
            };

            if (v0.w > 0 && v1.w > 0)
                image.draw_line(v0s, v1s, color);
            if (v1.w > 0 && v2.w > 0)
                image.draw_line(v1s, v2s, color);
            if (v2.w > 0 && v0.w > 0)
                image.draw_line(v2s, v0s, color);
        }

        virtual void sample(Intersection& isect, float& pdf) const override {
            float2 u0 = random_float2();
            float x = std::sqrt(u0.x);
            float y = u0.y;
            float u = x * (1 - y);
            float v = x * y;

            isect.hit = true;
            isect.position = pos[0] * (1 - x) + pos[1] * (x * (1 - y)) + pos[2] * (x * y);
            if (has_vn) {
                isect.normal = vn[0] * (1 - u - v) + vn[1] * u + vn[2] * v;
            }
            else {
                isect.normal = normal;
            }
            isect.uv.u = u;
            isect.uv.v = v;
            isect.material = material;
            isect.object = (Object*)this;
            isect.tangent = tangent;
            isect.bitangent = bitangent;

            pdf = 1 / area;
        }

        void calc_tangent() {
            if (has_vt) {
                auto dUV01 = vt[1] - vt[0];
                auto dUV02 = vt[2] - vt[0];

                float f = 1 / (dUV01.x * dUV02.y - dUV02.x * dUV01.y);

                tangent.x = f * (dUV02.y * e01.x - dUV01.y * e02.x);
                tangent.y = f * (dUV02.y * e01.y - dUV01.y * e02.y);
                tangent.z = f * (dUV02.y * e01.z - dUV01.y * e02.z);

                bitangent.x = f * (-dUV02.x * e01.x + dUV01.x * e02.x);
                bitangent.y = f * (-dUV02.x * e01.y + dUV01.x * e02.y);
                bitangent.z = f * (-dUV02.x * e01.z + dUV01.x * e02.z);
            }
            else {
                auto const& N = normal;
                bitangent = float3::zero();
                if (std::fabs(N.x) > std::fabs(N.y)) {
                    float len_inv = 1 / std::sqrt(N.x * N.x + N.z * N.z);
                    bitangent.x = N.z * len_inv;
                    bitangent.z = -N.x * len_inv;
                }
                else {
                    float len_inv = 1 / std::sqrt(N.y * N.y + N.z * N.z);
                    bitangent.y = N.z * len_inv;
                    bitangent.z = -N.y * len_inv;
                }
                tangent = bitangent.cross(N);
            }
        }

        void calc_center() {
            center = (pos[0] + pos[1] + pos[2]) / 3;
        }

        void calc_bound() {
            bound.min = float3::min(pos[0], float3::min(pos[1], pos[2]));
            bound.max = float3::max(pos[0], float3::max(pos[1], pos[2]));
        }

        void calc_area() {
            area = e01.cross(e02).norm() * .5f;
        }

        virtual float3 get_center() const override {
            return center;
        }

        virtual float3 get_min() const override {
            return bound.min;
        }

        virtual float3 get_max() const override {
            return bound.max;
        }

        virtual Bound3f get_bound() const override {
            return bound;
        }

        virtual float get_area() const override {
            return area;
        }

    };

} // namespace tira

#endif
