//
// Created by Ziyi.Lu 2022/12/08
//

#include <octree.h>
#include <utils.h>
#include <algorithm>

namespace tira {

    using CompareNode = std::pair<OctreeNode*, float>;

    void OctreeNode::intersect(Ray& ray, Intersection& isect) const {
        if (bound.intersect(ray) == FLOAT_MAX) return;

        if (!is_leaf()) {
            std::vector<CompareNode> nodes;

            for (auto c : children) {
                nodes.push_back({ c, c->bound.distant_to(ray.origin) });
            }

            std::sort(nodes.begin(), nodes.end(), [](CompareNode lhs, CompareNode rhs) {
                return lhs.second < rhs.second;
                });

            for (auto n : nodes) {
                n.first->intersect(ray, isect);

                if (isect.hit) break;
            }
        }

        for (auto& o : objects) {
            o->intersect(ray, isect);
        }
    }

    void OctreeNode::insert(Object* object, int max_objs) {
        if (objects.size() < max_objs) {
            objects.push_back(object);
            return;
        }
        if (is_leaf()) {
            // Leaf node that contains neighter data nor child,
            // subdivide and insert data into corresponding child.
            auto quater_extent = bound.get_extent() * .25f;
            auto center = bound.get_center();
            for (int i = 0; i < 8; ++i) {
                auto c = center;
                c.x += quater_extent.x * (i & 4 ? 1 : -1);
                c.y += quater_extent.y * (i & 2 ? 1 : -1);
                c.z += quater_extent.z * (i & 1 ? 1 : -1);
                auto b = Bound3f(c - quater_extent, c + quater_extent);
                children[i] = new OctreeNode(b);
            }
        }
        int child = get_child_containing_object(object);
        if (child == 8) {
            objects.push_back(object);
        }
        else {
            children[child]->insert(object, max_objs);
        }
    }

    void OctreeNode::draw_wireframe(Image& image, float4x4 const& transform, colorf const& color) const {
        bound.draw_wireframe(image, transform, color);

        if (!is_leaf()) for (auto c : children) c->draw_wireframe(image, transform, color);
    }

    /**
     * Octree children are defined as follow:
     *   0 1 2 3 4 5 6 7
     * x - - - - + + + + (w.r.t center of parent node)
     * y - - + + - - + +
     * z - + - + - + - +
     */
    int OctreeNode::get_child_containing_object(Object* object) const {
        int child = 0;
        auto b = object->get_bound();
        auto c = bound.get_center();
        if (b.max.x > c.x) child |= 0b100000;
        if (b.max.y > c.y) child |= 0b010000;
        if (b.max.z > c.z) child |= 0b001000;
        if (b.min.x > c.x) child |= 0b000100;
        if (b.min.y > c.y) child |= 0b000010;
        if (b.min.z > c.z) child |= 0b000001;

        switch (child) {
        case 0b000000: return 0;
        case 0b001001: return 1;
        case 0b010010: return 2;
        case 0b011011: return 3;
        case 0b100100: return 4;
        case 0b101101: return 5;
        case 0b110110: return 6;
        case 0b111111: return 7;
        default:       return 8;
        }
    }

    void OctreeAccel::build(std::vector<Object*>&& _objects) {
        objects = std::move(_objects);

        auto bound = Bound3f();
        for (auto& o : objects) {
            bound += o->get_bound();
        }

        root = new OctreeNode(bound);

        for (auto& o : objects) {
            root->insert(o, max_objs);
        }
    }

    void OctreeAccel::intersect(Ray& ray, Intersection& isect) {
        root->intersect(ray, isect);
    }

    void OctreeAccel::draw_wireframe(Image& image, float4x4 const& transform, colorf const& color) const {
        root->draw_wireframe(image, transform, color);
    }

} // namespace tira
