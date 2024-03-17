#pragma once

#include <unordered_map>
#include <map>
#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include <memory>
#include <algorithm>
#include <stdexcept>
#include <set>

#include "libs/cglm.hpp"
#include "libs/mcjp.hpp"

struct FloatData {
    float x, y, z;
};

struct RgbaData {
    uint8_t r, g, b, a;
};

enum class MaterialType {
    pbr,
    lambertian,
    mirror,
    environment,
    simple,
};

enum class TextureType {
    texture2D,
    textureCube,
};

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
        std::vector<cglm::Vec3f> normals;
        std::vector<cglm::Vec3f> colors;
        std::string position_format;
        std::string normal_format;
        std::string color_format;

        // texture data
        std::vector<cglm::Vec4f> tangents;
        std::vector<cglm::Vec2f> texcoords;

        int material_id;

        std::shared_ptr<Bound_Sphere> bound_sphere;
    };

    struct Node {
        int id;
        std::string name;
        // cglm::Mat44f transform;             // this is for current node transformation
        // cglm::Mat44f animation_transform;   // this is for animation transformation
        cglm::Mat44f translation;
        cglm::Mat44f rotation;
        cglm::Mat44f scale; 
        std::vector<int> children;
        std::vector<int> parents;
        std::vector<int> mesh;
        std::string driver_name;

        int camera;
        int vertex_count;

        std::vector<std::shared_ptr<Instance>> instances;
    };

    struct Driver {
        std::string name;
        int node;   // reference to the node
        std::string channel;   // channel could be "translation" or "rotation" or "scale"
        std::vector<double> times;
        std::vector<double> values;
        std::string interpolation;  // interpolation could be "STEP" or "LINEAR" or "SLERP"
        bool useful;

        cglm::Mat44f getCurrentTransform(double time);
    };


    struct Pbr {
        std::variant < std::vector<double>, std::string > albedo;
        TextureType albedo_type;
        std::variant < double, std::string > roughness;
        TextureType roughness_type;
        std::variant < double, std::string > metalness;
        TextureType metalness_type;
    };

    struct Lambertian {
        std::variant < std::vector<double>, std::string > albedo;
        TextureType albedo_type;
    };

    struct Material {
        int idx;
        std::string name;

        std::string normal_map;
        std::string displacement_map;

        MaterialType matetial_type;
        std::variant < std::shared_ptr<Pbr>, std::shared_ptr<Lambertian> > matetial_detail;
    };

    struct Environment {
        std::string name;

        std::string texture_src;
        TextureType env_type;
        std::string texture_format;
    };


    struct Sun {
        float angle;
        float strength;
    };
    struct Sphere {
        float radius;
        float power;
        float limit;
    };
    struct Spot {
        float radius;
        float power;
        float fov;
        float blend;
        float limit;
    };
    using LightData = std::variant<Sun, Sphere, Spot>;
    enum class LightType {
        SUN,
        SPHERE,
        SPOT
    };
    struct Light {
        LightType type;
        std::string name;
        std::array<float, 3> tint;
        LightData data;
        int shadow;

        cglm::Vec3f position;
        cglm::Vec3f direction;
        cglm::Vec3f up;
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

        std::unordered_map<std::string, std::shared_ptr<Driver>> name2driver;
        
        std::unordered_map<int, std::shared_ptr<Material>> id2material;

        std::unordered_map<std::string, int> texture2D2Idx;   // this is useful for descriptor set generate
        std::unordered_map<std::string, int> textureCube2Idx;   // this is useful for descriptor set generate

        std::shared_ptr<Scene> scene;
        std::shared_ptr<Environment> environment;

        std::map<int, std::shared_ptr<Light>> id2lights;

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
        std::shared_ptr<Driver> generateDriver(const mcjp::Object* obj);
        std::shared_ptr<Material> generateMaterial(const mcjp::Object* obj);
        std::shared_ptr<Environment> generateEnvironment(const mcjp::Object* obj);
        std::shared_ptr<Light> generateLight(const mcjp::Object* obj);
    };


}  // namespace sconfig
