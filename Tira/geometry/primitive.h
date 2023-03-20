//
// Created by Ziyi.Lu 2022/12/05
//

#ifndef PRIMITIVE_H
#define PRIMITIVE_H

#include <misc/utils.h>
#include <scene/scene.h>
#include <geometry/object.h>

namespace tira {

    struct Sphere : Object {
        Sphere() {}
        virtual ~Sphere() {}

        float radius;
        float3 center;
        Bound3f bound;
        float area;

        virtual void intersect(Ray const& ray, Intersection& isect) const override {
            float3 oc = ray.origin - center;
            float a = dot(ray.direction, ray.direction);
            float half_b = dot(oc, ray.direction);
            float c = dot(oc, oc) - radius * radius;

            float discriminant = half_b * half_b - a * c;
            if (discriminant < 0) {
                return;
            }
            float sqrt_d = std::sqrt(discriminant);

            float t = (-half_b - sqrt_d) / a;
            if (t < ray.t_min) {
                t = (-half_b + sqrt_d) / a;
                if (t < ray.t_min) {
                    return;
                }
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

            isect.position = ray.at(t);
            isect.normal = (isect.position - center) / radius;
            isect.back_face = ray.direction.dot(isect.normal) > 0;
        }

        virtual void draw_wireframe(Image& image, float4x4 const& transform, colorf const& color) const override {
            // Pass.
        }

        virtual void sample(Intersection& isect, float& pdf) const override {
            float3 normal = random_float3_in_unit_sphere();
            float3 p = center + normal * radius;

            isect.hit = true;
            isect.position = p;
            isect.normal = normal;
            isect.material = material;
            isect.object = (Object*)this;

            pdf = 1 / area;
        }

        void calc_bound() {
            bound.min = center - radius;
            bound.max = center + radius;
        }

        void calc_area() {
            area = 4 * PI * radius * radius;
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
