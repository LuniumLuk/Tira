//
// Created by Ziyi.Lu 2022/12/06
//

#ifndef BVH_H
#define BVH_H

#include <vector>
#include <scene/accel.h>
#include <misc/utils.h>
#include <misc/image.h>
#include <geometry/object.h>

namespace tira {

    struct BVHNode {
        Bound3f bound;
        int left, right;
        int first_prim, prim_count;
        // For the benefit of traversing tree in GPU.
        int height;
        int miss_idx;
        int hit_idx;

        bool is_leaf() const { return prim_count > 0; }
    };

    struct BVHAccel : Accelerator {
        enum struct SplitMethod { NAIVE, SAH };
        std::vector<BVHNode> nodes;
        SplitMethod split_method;
        int max_objs;
        int max_height;

#ifdef BVH_WITH_SAH
        BVHAccel(int _max_objs = 2, SplitMethod _split_method = SplitMethod::SAH)
#else
        BVHAccel(int _max_objs = 2, SplitMethod _split_method = SplitMethod::NAIVE)
#endif
            : max_objs(_max_objs)
            , split_method(_split_method) {}
        ~BVHAccel() {}

        virtual void build(std::vector<Object*>&& objects) override;
        virtual void intersect(Ray& ray, Intersection& isect) override;
        virtual void draw_wireframe(Image& image, float4x4 const& transform, colorf const& color) const override;

    private:
        void intersect_node(Ray& ray, Intersection& isect, int idx) const;
        void subdivide(int idx);
        void update_node_bound(int idx);
        void draw_wireframeNode(Image& image, float4x4 const& transform, colorf const& color, int idx) const;
    };

} // namespace tira

#endif
