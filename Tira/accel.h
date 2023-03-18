//
// Created by Ziyi.Lu 2022/12/07
//

#ifndef ACCEL_H
#define ACCEL_H

#include <ray.h>
#include <image.h>
#include <object.h>

namespace tira {

    struct Accelerator {
        std::vector<Object*> objects;
        Bound3f bound;

        Accelerator() {}

        virtual ~Accelerator() {
            for (auto& o : objects) delete o;
        }

        virtual void build(std::vector<Object*>&& objects) = 0;
        virtual void intersect(Ray& ray, Intersection& isect) = 0;
        virtual void draw_wireframe(Image& image, float4x4 const& transform, colorf const& color) const = 0;
    };

} // namespace tira

#endif
