//
// Created by Ziyi.Lu 2022/12/07
//

#ifndef OCTREE_H
#define OCTREE_H

#include <vector>
#include <accel.h>
#include <image.h>
#include <object.h>
#include <utils.h>

namespace tira {

    struct OctreeNode {
        Bound3f bound;
        std::vector<Object*> objects;
        mutable OctreeNode* children[8] = { nullptr };
        float dist; // When traverse child nodes, record dist to ray origin.

        OctreeNode() = delete;
        OctreeNode(Bound3f const& _bound) : bound(_bound) {}

        ~OctreeNode() { for (auto c : children) delete c; }

        void intersect(Ray& ray, Intersection& isect) const;
        bool is_leaf() const { return children[0] == nullptr; }
        void insert(Object* object, int max_objs = 4);
        void draw_wireframe(Image& image, float4x4 const& transform, colorf const& color) const;

    private:
        int get_child_containing_object(Object* object) const;
    };

    struct OctreeAccel : Accelerator {
        OctreeNode* root;
        int max_objs;

        OctreeAccel(int _max_objs = 4)
            : max_objs(_max_objs) {}

        OctreeAccel(const OctreeAccel&) = delete;
        OctreeAccel(OctreeAccel&&) = delete;

        virtual ~OctreeAccel() {}

        virtual void build(std::vector<Object*>&& objects) override;
        virtual void intersect(Ray& ray, Intersection& isect) override;
        virtual void draw_wireframe(Image& image, float4x4 const& transform, colorf const& color) const override;
    };

} // namespace tira

#endif
