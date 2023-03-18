//
// Created by Ziyi.Lu 2022/12/06
//

#include <array>
#include <bvh.h>

namespace tira {

    void BVHAccel::build(std::vector<Object*>&& _objects) {
        objects = std::move(_objects);
        max_height = 0;

        auto obj_num = objects.size();
        nodes.reserve(obj_num * 2 - 1);

        BVHNode root;
        root.left = 0;
        root.right = 0;
        root.first_prim = 0;
        root.prim_count = obj_num;
        root.height = 0;
        root.hit_idx = -1;
        root.miss_idx = -1;
        nodes.push_back(root);

        update_node_bound(0);
        subdivide(0);

        bound = nodes[0].bound;
    }

    void BVHAccel::intersect(Ray& ray, Intersection& isect) {
        if (bound.intersect(ray) == FLOAT_MAX) {
            return;
        }

#ifdef TRAVERSE_ITERATIVE
#ifdef TRAVERSE_ITERATIVE_STACK
        BVHNode* stack[64];
        int ptr = 0;
        stack[ptr++] = &nodes[0];
        BVHNode* node;

        while (ptr > 0) {
            node = stack[--ptr];

            if (node->is_leaf()) {
                for (int i = 0; i < node->prim_count; ++i) {
                    objects[node->first_prim + i]->intersect(ray, isect);
                }

                continue;
            }

            auto c0 = &nodes[node->left];
            auto c1 = &nodes[node->right];

            float dist0 = c0->bound.intersect(ray);
            float dist1 = c1->bound.intersect(ray);

            if (dist0 > dist1) {
                std::swap(dist0, dist1);
                std::swap(c0, c1);
            }

            if (dist0 != FLOAT_MAX) {
                stack[ptr++] = c0;
                if (dist1 != FLOAT_MAX) {
                    stack[ptr++] = c1;
                }
            }
        }
#else
        int idx = 0;
        while (idx >= 0) {
            auto& node = nodes[idx];

            if (node.bound.intersect(ray) == FLOAT_MAX) {
                idx = node.miss_idx;
                continue;
            }

            if (node.is_leaf()) {
                for (int i = 0; i < node.prim_count; ++i) {
                    objects[node.first_prim + i]->intersect(ray, isect);
                }
                idx = node.miss_idx;
            }
            else {
                idx = node.hit_idx;
            }
        }
#endif
#else
        intersect_node(ray, isect, 0);
#endif
    }

    void BVHAccel::intersect_node(Ray& ray, Intersection& isect, int idx) const {
        auto& node = nodes[idx];

        if (node.bound.intersect(ray) == FLOAT_MAX) {
            return;
        }

        if (node.is_leaf()) {
            for (int i = 0; i < node.prim_count; ++i) {
                objects[node.first_prim + i]->intersect(ray, isect);
            }
        }
        else {
            intersect_node(ray, isect, node.left);
            intersect_node(ray, isect, node.right);
        }
    }

    void BVHAccel::draw_wireframe(Image& image, float4x4 const& transform, colorf const& color) const {
        draw_wireframeNode(image, transform, color, 0);
    }

    void BVHAccel::draw_wireframeNode(Image& image, float4x4 const& transform, colorf const& color, int idx) const {
        auto& node = nodes[idx];
        node.bound.draw_wireframe(image, transform, color);
        if (node.left) {
            draw_wireframeNode(image, transform, color, node.left);
        }
        if (node.right) {
            draw_wireframeNode(image, transform, color, node.right);
        }
    }


    static std::array<int, 3> getSortedAxis(float3 const& extent) {
        std::array<float, 3> ext = { extent.x, extent.y, extent.z };
        std::array<int, 3> res = { 0, 1, 2 };
        if (ext[2] > ext[1]) {
            std::swap(ext[2], ext[1]);
            std::swap(res[2], res[1]);
        }
        if (ext[1] > ext[0]) {
            std::swap(ext[1], ext[0]);
            std::swap(res[1], res[0]);
        }
        if (ext[2] > ext[1]) {
            std::swap(ext[2], ext[1]);
            std::swap(res[2], res[1]);
        }
        return res;
    }

    void BVHAccel::subdivide(int idx) {
        BVHNode& node = nodes[idx];
        if (node.height > max_height) max_height = node.height;
        if (node.prim_count <= max_objs) return;

        float3 extent = node.bound.get_extent();
        auto axes = getSortedAxis(extent);

        if (split_method == SplitMethod::NAIVE) {
            for (auto axis : axes) {
                float pivot = node.bound.min[axis] + extent[axis] * .5f;

                int i = node.first_prim;
                int j = i + node.prim_count - 1;
                while (i <= j) {
                    // TODO: cache center of objects.
                    if (objects[i]->get_center()[axis] < pivot) {
                        ++i;
                    }
                    else {
                        std::swap(objects[i], objects[j]);
                        --j;
                    }
                }

                int left_count = i - node.first_prim;
                if (left_count == 0 || left_count == node.prim_count) continue;

                BVHNode left;
                left.first_prim = node.first_prim;
                left.prim_count = left_count;
                left.left = left.right = 0;
                left.height = node.height + 1;
                left.hit_idx = -1;
                left.miss_idx = -1;

                BVHNode right;
                right.first_prim = i;
                right.prim_count = node.prim_count - left_count;
                right.left = right.right = 0;
                right.height = node.height + 1;
                right.hit_idx = -1;
                right.miss_idx = -1;

                node.left = nodes.size();
                nodes.push_back(left);
                node.right = nodes.size();
                nodes.push_back(right);

                node.prim_count = 0;
                node.hit_idx = node.left;
                nodes[node.left].miss_idx = node.right;
                if (node.miss_idx >= 0) nodes[node.right].miss_idx = node.miss_idx;

                update_node_bound(node.left);
                update_node_bound(node.right);

                subdivide(node.left);
                subdivide(node.right);

                break;
            }
        }
        else {
            // Surface Area Heuristic
            int best_axis = 0;
            int axis = axes[0];
            float best_sah = std::numeric_limits<float>::max();
            int best_left_count;
            int step = 1;
            if (node.prim_count > SAH_MAX_SEARCH) {
                step = std::ceil((float)node.prim_count / SAH_MAX_SEARCH);
            }

            for (auto axis : axes) {
                std::sort(objects.begin() + node.first_prim, objects.begin() + (node.first_prim + node.prim_count), [=](Object const* x, Object const* y) {
                    return x->get_center()[axis] < y->get_center()[axis];
                    });

                int left_count = step;
                while (left_count < node.prim_count) {
                    Bound3f left_bound;
                    Bound3f right_bound;

                    for (int i = 0; i < node.prim_count; ++i) {
                        if (i < left_count)
                            left_bound += objects[node.first_prim + i]->get_bound();
                        else
                            right_bound += objects[node.first_prim + i]->get_bound();
                    }

                    // Calculate sah by SAH(left, right) = p(left) * nleft + p(right) * nright
                    // where p(a) is the probability of intersecting with left child and nleft is the primitive count of the left child
                    float sah = left_bound.get_surface_area() * left_count + right_bound.get_surface_area() * (node.prim_count - left_count);

                    if (sah < best_sah) {
                        best_sah = sah;
                        best_left_count = left_count;
                        best_axis = axis;
                    }

                    left_count += step;
                }
            }

            std::sort(objects.begin() + node.first_prim, objects.begin() + (node.first_prim + node.prim_count), [=](Object const* x, Object const* y) {
                return x->get_center()[best_axis] < y->get_center()[best_axis];
            });

            BVHNode left;
            left.first_prim = node.first_prim;
            left.prim_count = best_left_count;
            left.left = left.right = 0;
            left.height = node.height + 1;
            left.hit_idx = -1;
            left.miss_idx = -1;

            BVHNode right;
            right.first_prim = node.first_prim + best_left_count;
            right.prim_count = node.prim_count - best_left_count;
            right.left = right.right = 0;
            right.height = node.height + 1;
            right.hit_idx = -1;
            right.miss_idx = -1;

            node.left = nodes.size();
            nodes.push_back(left);
            node.right = nodes.size();
            nodes.push_back(right);

            node.prim_count = 0;
            node.hit_idx = node.left;
            nodes[node.left].miss_idx = node.right;
            if (node.miss_idx >= 0) nodes[node.right].miss_idx = node.miss_idx;

            update_node_bound(node.left);
            update_node_bound(node.right);

            subdivide(node.left);
            subdivide(node.right);
        }
    }

    void BVHAccel::update_node_bound(int idx) {
        auto& node = nodes[idx];
        Bound3f bound;
        for (int i = 0; i < node.prim_count; ++i) {
            bound += objects[node.first_prim + i]->get_bound();
        }
        node.bound = bound;
    }

} // namespace tira
