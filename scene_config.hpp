#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include <memory>
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

    struct Camera {
        std::string name;
        float aspect;
        float vfov;
        float near;
        float far;

        std::array<float, 3> position = { 0.0f, 2.0f, 0.0f };
    };

    struct Mesh {
        int id;
        std::string name;
        std::string topology;

        int vertex_count;
        std::vector<uint32_t> indices;      // this is optional, if not present, then the mesh is non-indexed

        // vertex data
        std::vector<float> positions;
        std::string position_format;
        std::vector<float> normals;
        std::string normal_format;
        std::vector<float> colors;
        std::string color_format;

        // create a bounding shpere
        cglm::Vec3f center = { 0.0f, 0.0f, 0.0f };
        float radius = 0.0f;
    };

    struct Node {
        int id;
        std::string name;
        std::array<int, 3> translation = { 0, 0, 0 };
        std::array<int, 4> rotation = { 0, 0, 0, 1 };
        std::array<int, 3> scale = { 1, 1, 1 };
        std::vector<int> children;
        int camera;
        int mesh;
    };

    struct SceneConfig {
        std::unordered_map<std::string, std::shared_ptr<Camera>> cameras;
        std::unordered_map<int, std::shared_ptr<Mesh>> id2mesh;
        std::unordered_map<int, std::shared_ptr<Node>> id2node;

        std::string cur_camera;
        int cur_node;

        void load_scene(const std::string& scene_file_name);
        size_t get_total_vertex_count();
    };


}  // namespace sconfig
