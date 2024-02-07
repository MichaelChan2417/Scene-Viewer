#include "scene_config.hpp"

namespace sconfig {

    std::shared_ptr<Camera> generateCamera(const mcjp::Object* obj) {
        std::shared_ptr<Camera> camera = std::make_shared<Camera>();
        camera->name = std::get<std::string>(obj->contents.at("name"));
        auto& perspective = std::get<mcjp::Object*>(obj->contents.at("perspective"));

        camera->aspect = static_cast<float>(std::get<double>(perspective->contents.at("aspect")));
        camera->vfov = static_cast<float>(std::get<double>(perspective->contents.at("vfov")));
        camera->near = static_cast<float>(std::get<double>(perspective->contents.at("near")));
        camera->far = static_cast<float>(std::get<double>(perspective->contents.at("far")));
        return camera;
    }

    std::shared_ptr<Mesh> generateMesh(const mcjp::Object* obj) {
        std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();
        mesh->name = std::get<std::string>(obj->contents.at("name"));
        mesh->topology = std::get<std::string>(obj->contents.at("topology"));
        mesh->vertex_count = std::get<int>(obj->contents.at("count"));

        // TODO: handle optional indices
        if (obj->contents.find("indices") != obj->contents.end()) {
            
        }

        // reading files to get vertex data
        mcjp::Object* attributes = std::get<mcjp::Object*>(obj->contents.at("attributes"));
        mcjp::Object* position = std::get<mcjp::Object*>(attributes->contents.at("POSITION"));
        mcjp::Object* normal = std::get<mcjp::Object*>(attributes->contents.at("NORMAL"));
        mcjp::Object* color = std::get<mcjp::Object*>(attributes->contents.at("COLOR"));

        mesh->position_format = std::get<std::string>(position->contents.at("format"));
        mesh->normal_format = std::get<std::string>(normal->contents.at("format"));
        mesh->color_format = std::get<std::string>(color->contents.at("format"));

        std::string file_name = std::get<std::string>(position->contents.at("src"));

        int stride = std::get<int>(position->contents.at("stride"));
        int pos_offset = std::get<int>(position->contents.at("offset"));
        int normal_offset = std::get<int>(normal->contents.at("offset"));
        int color_offset = std::get<int>(color->contents.at("offset"));

        std::ifstream file(file_name, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to Open File!");
        }

        // read vertex_count times, each time read stride bytes
        for (int i = 0; i < mesh->vertex_count; i++) {
            file.seekg(pos_offset + i * stride);
            float x, y, z;
            file.read(reinterpret_cast<char*>(&x), sizeof(float));
            file.read(reinterpret_cast<char*>(&y), sizeof(float));
            file.read(reinterpret_cast<char*>(&z), sizeof(float));
            mesh->positions.push_back(x);
            mesh->positions.push_back(y);
            mesh->positions.push_back(z);

            file.seekg(normal_offset + i * stride);
            float nx, ny, nz;
            file.read(reinterpret_cast<char*>(&nx), sizeof(float));
            file.read(reinterpret_cast<char*>(&ny), sizeof(float));
            file.read(reinterpret_cast<char*>(&nz), sizeof(float));
            mesh->normals.push_back(nx);
            mesh->normals.push_back(ny);
            mesh->normals.push_back(nz);

            file.seekg(color_offset + i * stride);
            uint8_t r, g, b, a;
            file.read(reinterpret_cast<char*>(&r), sizeof(uint8_t));
            file.read(reinterpret_cast<char*>(&g), sizeof(uint8_t));
            file.read(reinterpret_cast<char*>(&b), sizeof(uint8_t));
            file.read(reinterpret_cast<char*>(&a), sizeof(uint8_t));
            mesh->colors.push_back(static_cast<float>(r) / 255.0f);
            mesh->colors.push_back(static_cast<float>(g) / 255.0f);
            mesh->colors.push_back(static_cast<float>(b) / 255.0f);
        }

        return mesh;
    }

    
    void SceneConfig::load_scene(const std::string& scene_file_name) {
        mcjp::Result result = mcjp::load(scene_file_name);
        std::vector<mcjp::Object*> objects;

        if (std::holds_alternative<std::vector<mcjp::Object*>>(result)) {
            objects = std::get<std::vector<mcjp::Object*>>(result);
        } else {
            throw std::runtime_error("Failed to Load Scene File!");
        }

        size_t n = objects.size();
        for (size_t i = 1; i < n; i++) {
            mcjp::Object* obj = objects[i];
            std::string type = std::get<std::string>(obj->contents["type"]);
            if (type == "camera" || type == "CAMERA") {
                std::shared_ptr<Camera> cameraPtr = generateCamera(obj);
                cameras[cameraPtr->name] = cameraPtr;
            }
            else if (type == "mesh" || type == "MESH") {
                std::shared_ptr<Mesh> meshPtr = generateMesh(obj);
                id2mesh[meshPtr->id] = meshPtr;
                // for (int i=0; i<meshPtr->positions.size(); i+=3) {
                //     std::cout << meshPtr->positions[i] << "," << meshPtr->positions[i+1] << "," << meshPtr->positions[i+2] << std::endl;
                // }
            }
            else if (type == "node" || type == "NODE") {
            }
        }
        
        if (cameras["default"] == nullptr) {
            // we create a default camera
            std::shared_ptr<Camera> cameraPtr = std::make_shared<Camera>();
            cameraPtr->name = "default";
            cameraPtr->aspect = 1.777f;
            cameraPtr->vfov = 1.04719f;
            cameraPtr->near = 0.1f;
            cameraPtr->far = 10.0f;

            cameraPtr->position = { 0.0f, 4.0f, 0.0f };

            cameras["default"] = cameraPtr;
            this->cur_camera = "default";
        }
        
        // cleanups
        for (auto obj : objects) {
            obj->cleanup();
        }
    }

    size_t SceneConfig::get_total_vertex_count() {
        size_t total = 0;
        for (auto& [id, mesh] : id2mesh) {
            total += mesh->vertex_count;
        }
        return total;
    }

} // namespace sconfig