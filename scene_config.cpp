#include "scene_config.hpp"

namespace sconfig {

    std::shared_ptr<Camera> SceneConfig::generateCamera(const mcjp::Object* obj) {
        std::shared_ptr<Camera> camera = std::make_shared<Camera>();
        camera->name = std::get<std::string>(obj->contents.at("name"));
        auto& perspective = std::get<mcjp::Object*>(obj->contents.at("perspective"));

        camera->aspect = static_cast<float>(std::get<double>(perspective->contents.at("aspect")));
        camera->vfov = static_cast<float>(std::get<double>(perspective->contents.at("vfov")));
        camera->near = static_cast<float>(std::get<double>(perspective->contents.at("near")));
        camera->far = static_cast<float>(std::get<double>(perspective->contents.at("far")));
        return camera;
    }

    std::shared_ptr<Mesh> SceneConfig::generateMesh(const mcjp::Object* obj) {
        std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();
        mesh->name = std::get<std::string>(obj->contents.at("name"));
        mesh->topology = std::get<std::string>(obj->contents.at("topology"));
        mesh->vertex_count = static_cast<size_t>(std::get<int>(obj->contents.at("count")));

        // TODO: handle optional indices
        if (obj->contents.find("indices") != obj->contents.end()) {
            throw std::runtime_error("Indices not supported yet!");
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
            throw std::runtime_error("Failed to Open SubScene File!");
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

        // create a bounding sphere // TODO: bounding spherer may not be useful for mesh
        mesh->center = { (mesh->positions[0] + mesh->positions[3]) / 2.0f, (mesh->positions[1] + mesh->positions[4]) / 2.0f, (mesh->positions[2] + mesh->positions[5]) / 2.0f };
        mesh->radius = cglm::length(cglm::Vec3f{ mesh->positions[0] - mesh->center[0], mesh->positions[1] - mesh->center[1], mesh->positions[2] - mesh->center[2] });

        for (size_t i=2; i<mesh->vertex_count; i++) {
            cglm::Vec3f pos = { mesh->positions[i*3], mesh->positions[i*3+1], mesh->positions[i*3+2] };
            float dist = cglm::length(pos - mesh->center);
            // update the center and radius, called ritter's algorithm
            if (dist > mesh->radius) {
                float alpha = cglm::length(pos - mesh->center) / (2.0f * mesh->radius);
                float beta = 1 - alpha;
                mesh->center = pos * alpha + mesh->center * beta;
                mesh->radius = (dist + mesh->radius) / 2.0f;
            }
        }

        return mesh;
    }


    std::shared_ptr<Node> SceneConfig::generateNode(const mcjp::Object* obj) {
        std::shared_ptr<Node> node = std::make_shared<Node>();
        node->name = std::get<std::string>(obj->contents.at("name"));
        node->children = std::get<std::vector<int>>(obj->contents.at("children"));
        node->mesh = std::get<std::vector<int>>(obj->contents.at("mesh"));
        node->camera = std::get<int>(obj->contents.at("camera"));

        std::vector<double> translation = std::get<std::vector<double>>(obj->contents.at("translation"));
        std::vector<double> rotation = std::get<std::vector<double>>(obj->contents.at("rotation"));
        std::vector<double> scale = std::get<std::vector<double>>(obj->contents.at("scale"));

        cglm::Mat44f translate_m = cglm::translation(cglm::Vec3f{ static_cast<float>(translation[0]), static_cast<float>(translation[1]), static_cast<float>(translation[2]) });
        cglm::Mat44f rotation_m = cglm::rotation(cglm::Vec4f{ static_cast<float>(rotation[0]), static_cast<float>(rotation[1]), static_cast<float>(rotation[2]), static_cast<float>(rotation[3]) });
        cglm::Mat44f scale_m = cglm::scale(cglm::Vec3f{ static_cast<float>(scale[0]), static_cast<float>(scale[1]), static_cast<float>(scale[2]) });

        node->transform = translate_m * rotation_m * scale_m;
        cglm::Mat44f norm_trans = cglm::transpose(cglm::inverse(node->transform));

        // get total size of positions, normals, colors
        size_t total_size = 0;
        for (auto& id : node->mesh) {
            total_size += id2mesh[id]->vertex_count;
        }
        for (auto& id : node->children) {
            total_size += id2node[id]->vertex_count;
        }

        // allocate memory
        node->positions.resize(total_size * 3);
        node->normals.resize(total_size * 3);
        node->colors.resize(total_size * 3);
        // copy & transform data
        size_t offset = 0;

        // for direct meshes
        for (auto& id : node->mesh) {
            std::shared_ptr<Mesh> mesh = id2mesh[id];
            for (size_t i = 0; i < mesh->vertex_count; i++) {
                cglm::Vec4f pos = { mesh->positions[i * 3], mesh->positions[i * 3 + 1], mesh->positions[i * 3 + 2], 1.0f };
                cglm::Vec3f normal = { mesh->normals[i * 3], mesh->normals[i * 3 + 1], mesh->normals[i * 3 + 2] };
                cglm::Vec3f color = { mesh->colors[i * 3], mesh->colors[i * 3 + 1], mesh->colors[i * 3 + 2] };

                cglm::Vec4f mid_pos = node->transform * pos;
                cglm::Vec3f new_pos = { mid_pos[0]/mid_pos[3], mid_pos[1]/mid_pos[3], mid_pos[2]/mid_pos[3] };
                cglm::Vec3f new_normal = norm_trans * normal;

                node->positions[offset * 3] = new_pos[0]; node->positions[offset * 3 + 1] = new_pos[1]; node->positions[offset * 3 + 2] = new_pos[2];
                node->normals[offset * 3] = new_normal[0]; node->normals[offset * 3 + 1] = new_normal[1]; node->normals[offset * 3 + 2] = new_normal[2];
                node->colors[offset * 3] = color[0]; node->colors[offset * 3 + 1] = color[1]; node->colors[offset * 3 + 2] = color[2];

                offset++;
            }
        }
        for (auto& id : node->children) {
            std::shared_ptr<Node> child = id2node[id];
            for (size_t i = 0; i < child->vertex_count; i++) {
                cglm::Vec4f pos = { child->positions[i * 3], child->positions[i * 3 + 1], child->positions[i * 3 + 2], 1.0f };
                cglm::Vec3f normal = { child->normals[i * 3], child->normals[i * 3 + 1], child->normals[i * 3 + 2] };
                cglm::Vec3f color = { child->colors[i * 3], child->colors[i * 3 + 1], child->colors[i * 3 + 2] };

                cglm::Vec4f mid_pos = node->transform * pos;
                cglm::Vec3f new_pos = { mid_pos[0] / mid_pos[3], mid_pos[1] / mid_pos[3], mid_pos[2] / mid_pos[3] };
                cglm::Vec3f new_normal = norm_trans * normal;

                node->positions[offset * 3] = new_pos[0]; node->positions[offset * 3 + 1] = new_pos[1]; node->positions[offset * 3 + 2] = new_pos[2];
                node->normals[offset * 3] = new_normal[0]; node->normals[offset * 3 + 1] = new_normal[1]; node->normals[offset * 3 + 2] = new_normal[2];
                node->colors[offset * 3] = color[0]; node->colors[offset * 3 + 1] = color[1]; node->colors[offset * 3 + 2] = color[2];

                offset++;
            }
        }

        return node;
    }

    std::shared_ptr<Scene> SceneConfig::generateScene(const mcjp::Object* obj) {
        std::shared_ptr<Scene> scene = std::make_shared<Scene>();
        scene->name = std::get<std::string>(obj->contents.at("name"));
        scene->children = std::get<std::vector<int>>(obj->contents.at("roots"));
        return scene;
    }
    
    void SceneConfig::load_scene(const std::string& scene_file_name) {
        mcjp::Result result = mcjp::load(scene_file_name);
        std::vector<mcjp::Object*> objects;

        if (std::holds_alternative<std::vector<mcjp::Object*>>(result)) {
            objects = std::get<std::vector<mcjp::Object*>>(result);
        }
        else {
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
                id2mesh[i] = meshPtr;
                // for (int i=0; i<meshPtr->positions.size(); i+=3) {
                //     std::cout << meshPtr->positions[i] << "," << meshPtr->positions[i+1] << "," << meshPtr->positions[i+2] << std::endl;
                // }
            }
            else if (type == "node" || type == "NODE") {
                std::shared_ptr<Node> nodePtr = generateNode(obj);
                id2node[i] = nodePtr;
            }
            else if (type == "scene" || type == "SCENE") {
                scene = generateScene(obj);
            }

            if (cameras["debug"] == nullptr) {
                // we create a default camera
                std::shared_ptr<Camera> cameraPtr = std::make_shared<Camera>();
                cameraPtr->name = "debug";
                cameraPtr->aspect = 1.777f;
                cameraPtr->vfov = 1.04719f;
                cameraPtr->near = 0.1f;
                cameraPtr->far = 10.0f;
                cameraPtr->position = { 0.0f, 0.0f, 4.0f };
                cameraPtr->up = { 0.0f, 1.0f, 0.0f };
                cameraPtr->dir = normalize(-cameraPtr->position);
                cameras["debug"] = cameraPtr;
                this->cur_camera = "debug";
            }
            if (cameras["user"] == nullptr) {
                // we create a default camera
                std::shared_ptr<Camera> cameraPtr = std::make_shared<Camera>();
                cameraPtr->name = "user";
                cameraPtr->aspect = 1.777f;
                cameraPtr->vfov = 1.04719f;
                cameraPtr->near = 0.1f;
                cameraPtr->far = 10.0f;
                cameras["user"] = cameraPtr;
            }

            // cleanups
            for (auto obj : objects) {
                obj->cleanup();
            }
        }
    }

    size_t SceneConfig::get_total_vertex_count() {
        size_t total = 0;
        throw std::runtime_error("Total Vertex Count Not Implemented Yet!");
        for (auto& [id, mesh] : id2mesh) {
            total += mesh->vertex_count;
        }
        return total;
    }

} // namespace sconfig