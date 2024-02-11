#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include <memory>
#include <algorithm>
#include <stdexcept>

#include "libs/cglm.hpp"
#include "libs/mcjp.hpp"

struct FloatData {
    float x, y, z;
};

struct RgbaData {
    uint8_t r, g, b, a;
};

// TODO: currently not include SCENE

namespace sconfig {

    struct Plane {
        cglm::Vec3f normal;
        float d;
    };

    struct Camera {
        std::string name;
        float aspect;
        float vfov;
        float near;
        float far;

        float boundary_view;

        cglm::Vec3f position = { 0.0f, 0.0f, 0.0f };
        cglm::Vec3f dir = { 0.0f, 0.0f, -1.0f };
        cglm::Vec3f up = { 0.0f, 1.0f, 0.0f };

        std::vector<std::shared_ptr<Plane>> bounds;
        cglm::Mat44f lookAt_mat;
        cglm::Mat44f proj_mat;

        void update_planes();
    };

    struct Mesh {
        int id;
        std::string name;
        std::string topology;

        size_t vertex_count;
        std::vector<uint32_t> indices;      // this is optional, if not present, then the mesh is non-indexed

        // vertex data
        std::vector<cglm::Vec3f> positions;
        std::string position_format;
        std::vector<cglm::Vec3f> normals;
        std::string normal_format;
        std::vector<cglm::Vec3f> colors;
        std::string color_format;

        // create a bounding shpere
        // cglm::Vec3f center = { 0.0f, 0.0f, 0.0f };
        // float radius = 0.0f;
    };

    struct Bound_Sphere {
        cglm::Vec3f center;
        float radius;
        size_t startIdx, endIdx; // [,)
    };

    struct Node {
        int id;
        size_t vertex_count;
        std::string name;
        cglm::Mat44f transform;
        std::vector<int> children;
        std::vector<int> mesh;

        std::vector<cglm::Vec3f> positions;
        std::vector<cglm::Vec3f> normals;
        std::vector<cglm::Vec3f> colors;

        std::vector<std::shared_ptr<Bound_Sphere>> bound_spheres;

        int camera;
    };

    struct Scene {
        std::string name;
        std::vector<int> children;
    };

    struct SceneConfig {
        std::unordered_map<std::string, std::shared_ptr<Camera>> cameras;
        std::unordered_map<int, std::shared_ptr<Mesh>> id2mesh;
        std::unordered_map<int, std::shared_ptr<Node>> id2node;
        std::shared_ptr<Scene> scene;

        std::string cur_camera;
        int cur_node;

        void load_scene(const std::string& scene_file_name);
        size_t get_total_vertex_count();

        // parser
        std::shared_ptr<Camera> generateCamera(const mcjp::Object* obj);
        std::shared_ptr<Mesh> generateMesh(const mcjp::Object* obj);
        std::shared_ptr<Node> generateNode(const mcjp::Object* obj);
        std::shared_ptr<Scene> generateScene(const mcjp::Object* obj);
    };


}  // namespace sconfig
