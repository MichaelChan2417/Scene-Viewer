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

    struct Bound_Sphere {
        cglm::Vec3f center;
        float radius;
    };

    struct Instance {
        // self unique id
        int id;         // this is unique over all instances
        // reference to the mesh
        int mesh_id;
        cglm::Mat44f transform;
    };

    struct Camera {
        std::string name;
        float aspect;
        float vfov;
        float near;
        float far;

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
        int inner_id;
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

        std::shared_ptr<Bound_Sphere> bound_sphere;
    };

    struct Node {
        int id;
        std::string name;
        cglm::Mat44f transform;             // this is for current node transformation
        cglm::Mat44f animation_transform;   // this is for animation transformation
        std::vector<int> children;
        std::vector<int> parents;
        std::vector<int> mesh;

        int camera;
        int vertex_count;

        std::vector<std::shared_ptr<Instance>> instances;
    };

    struct Scene {
        std::string name;
        std::vector<int> children;
    };

    struct SceneConfig {
        std::unordered_map<std::string, std::shared_ptr<Camera>> cameras;
        std::unordered_map<int, std::string> id2camera_name;
        std::unordered_map<int, std::shared_ptr<Mesh>> id2mesh;
        std::unordered_map<int, int> innerId2meshId;
        std::unordered_map<int, std::shared_ptr<Instance>> id2instance;
        std::unordered_map<int, std::shared_ptr<Node>> id2node;
        std::shared_ptr<Scene> scene;

        std::string cur_camera;
        int cur_instance;
        int cur_mesh;

        void load_scene(const std::string& scene_file_name);
        size_t get_total_vertex_count();
        size_t get_mesh_vertex_count();

        // parser
        std::shared_ptr<Camera> generateCamera(const mcjp::Object* obj);
        std::shared_ptr<Mesh> generateMesh(const mcjp::Object* obj);
        std::shared_ptr<Node> generateNode(const mcjp::Object* obj, size_t node_id);
        std::shared_ptr<Scene> generateScene(const mcjp::Object* obj);
    };


}  // namespace sconfig
