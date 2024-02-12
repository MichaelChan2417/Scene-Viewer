#include "scene_config.hpp"

namespace sconfig {

    inline float orgDistanceToPlane(const cglm::Vec3f& point, const std::shared_ptr<Plane>& plane) {
        return cglm::dot(plane->normal, point);
    }

    void Camera::update_planes() {
        cglm::Vec3f camera_pos = this->position;
        cglm::Vec3f view_point = this->position + this->dir;
        this->lookAt_mat = cglm::lookAt(camera_pos, view_point, this->up);
        // proj matrix will not change, since parameter are fixed
        cglm::Mat44f inv_vp = cglm::inverse(this->proj_mat * this->lookAt_mat);

        // we also compute 6 M-1'd view planes with all normal points inside the frustum
        this->bounds.clear();
        cglm::Vec3f front_o = camera_pos + this->dir * this->near;
        cglm::Vec3f back_o = camera_pos + this->dir * this->far;
        cglm::Vec3f dy0 = this->up * (this->near * tan(this->vfov / 2));
        cglm::Vec3f dx0 = cglm::normalize(cglm::cross(this->dir, dy0)) * this->near * this->aspect * tan(this->vfov / 2);
        cglm::Vec3f dy1 = this->up * (this->far * tan(this->vfov / 2));
        cglm::Vec3f dx1 = cglm::normalize(cglm::cross(this->dir, dy1)) * this->far * this->aspect * tan(this->vfov / 2);

        // cglm::Vec3f p0 = cglm::transform_point(front_o - dx0 + dy0, inv_vp);
        // cglm::Vec3f p1 = cglm::transform_point(front_o + dx0 + dy0, inv_vp);
        // cglm::Vec3f p2 = cglm::transform_point(front_o + dx0 - dy0, inv_vp);
        // cglm::Vec3f p3 = cglm::transform_point(front_o - dx0 - dy0, inv_vp);
        // cglm::Vec3f p4 = cglm::transform_point(back_o - dx1 + dy1, inv_vp);
        // cglm::Vec3f p5 = cglm::transform_point(back_o + dx1 + dy1, inv_vp);
        // cglm::Vec3f p6 = cglm::transform_point(back_o + dx1 - dy1, inv_vp);
        cglm::Vec3f p0 = front_o - dx0 + dy0;
        cglm::Vec3f p1 = front_o + dx0 + dy0;
        cglm::Vec3f p2 = front_o + dx0 - dy0;
        cglm::Vec3f p3 = front_o - dx0 - dy0;
        cglm::Vec3f p4 = back_o - dx1 + dy1;
        cglm::Vec3f p5 = back_o + dx1 + dy1;
        cglm::Vec3f p6 = back_o + dx1 - dy1;

        // std::cout << this->name << " Camera pos: " << camera_pos << " Looking at " << this->dir;
        // std::cout << " Up is " << this->up << std::endl;
        // std::cout << "p0 " << p0 << std::endl;
        // std::cout << "p1 " << p1 << std::endl;
        // std::cout << "p2 " << p2 << std::endl;
        // std::cout << "p3 " << p3 << std::endl;
        // std::cout << "p4 " << p4 << std::endl;
        // std::cout << "p5 " << p5 << std::endl;
        // std::cout << "p6 " << p6 << std::endl;
        // front
        std::shared_ptr<Plane> front = std::make_shared<Plane>();
        front->normal = cglm::normalize(cglm::cross(p1 - p0, p2 - p1));
        front->d = orgDistanceToPlane(p0, front);
        this->bounds.push_back(front);
        // back
        std::shared_ptr<Plane> back = std::make_shared<Plane>();
        back->normal = cglm::normalize(cglm::cross(p4 - p5, p6 - p5));
        back->d = orgDistanceToPlane(p4, back);
        this->bounds.push_back(back);
        // left
        std::shared_ptr<Plane> left = std::make_shared<Plane>();
        left->normal = cglm::normalize(cglm::cross(p0 - p4, p3 - p0));
        left->d = orgDistanceToPlane(p0, left);
        this->bounds.push_back(left);
        // right
        std::shared_ptr<Plane> right = std::make_shared<Plane>();
        right->normal = cglm::normalize(cglm::cross(p1 - p2, p5 - p1));
        right->d = orgDistanceToPlane(p1, right);
        this->bounds.push_back(right);
        // top
        std::shared_ptr<Plane> top = std::make_shared<Plane>();
        top->normal = cglm::normalize(cglm::cross(p1 - p0, p0 - p4));
        top->d = orgDistanceToPlane(p0, top);
        this->bounds.push_back(top);
        // bottom
        std::shared_ptr<Plane> bottom = std::make_shared<Plane>();
        bottom->normal = cglm::normalize(cglm::cross(p2 - p3, p6 - p2));
        bottom->d = orgDistanceToPlane(p2, bottom);
        this->bounds.push_back(bottom);

        // print out all normals to check
        // for (auto& plane : this->bounds) {
        //     std::cout << "Plane " << plane->normal << " " << plane->d << std::endl;
        // }
    }

    std::shared_ptr<Camera> SceneConfig::generateCamera(const mcjp::Object* obj) {
        std::shared_ptr<Camera> camera = std::make_shared<Camera>();
        camera->name = std::get<std::string>(obj->contents.at("name"));
        auto& perspective = std::get<mcjp::Object*>(obj->contents.at("perspective"));

        camera->aspect = static_cast<float>(std::get<double>(perspective->contents.at("aspect")));
        camera->vfov = static_cast<float>(std::get<double>(perspective->contents.at("vfov")));
        camera->near = static_cast<float>(std::get<double>(perspective->contents.at("near")));
        camera->far = static_cast<float>(std::get<double>(perspective->contents.at("far")));
        camera->boundary_view = static_cast<float>(sin(atan(sqrt(1 + pow(camera->aspect, 2)) * tan(camera->vfov / 2))));

        camera->update_planes();

        return camera;
    }

    std::shared_ptr<Mesh> SceneConfig::generateMesh(const mcjp::Object* obj) {
        std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();
        mesh->name = std::get<std::string>(obj->contents.at("name"));
        mesh->topology = std::get<std::string>(obj->contents.at("topology"));
        mesh->vertex_count = static_cast<size_t>(std::get<double>(obj->contents.at("count")));

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

        int stride = std::get<double>(position->contents.at("stride"));
        int pos_offset = std::get<double>(position->contents.at("offset"));
        int normal_offset = std::get<double>(normal->contents.at("offset"));
        int color_offset = std::get<double>(color->contents.at("offset"));

        std::ifstream file(file_name, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to Open SubScene File!");
        }

        // cglm::Vec3f white{ 0.0f, 0.0f, 0.0f };
        // cglm::Vec3f black{ 1.0f, 1.0f, 1.0f };
        // read vertex_count times, each time read stride bytes
        for (int i = 0; i < mesh->vertex_count; i++) {
            file.seekg(pos_offset + i * stride);
            float x, y, z;
            file.read(reinterpret_cast<char*>(&x), sizeof(float));
            file.read(reinterpret_cast<char*>(&y), sizeof(float));
            file.read(reinterpret_cast<char*>(&z), sizeof(float));
            mesh->positions.push_back(cglm::Vec3f{ x, y, z });

            file.seekg(normal_offset + i * stride);
            float nx, ny, nz;
            file.read(reinterpret_cast<char*>(&nx), sizeof(float));
            file.read(reinterpret_cast<char*>(&ny), sizeof(float));
            file.read(reinterpret_cast<char*>(&nz), sizeof(float));
            mesh->normals.push_back(cglm::Vec3f{ nx, ny, nz });

            file.seekg(color_offset + i * stride);
            uint8_t r, g, b, a;
            file.read(reinterpret_cast<char*>(&r), sizeof(uint8_t));
            file.read(reinterpret_cast<char*>(&g), sizeof(uint8_t));
            file.read(reinterpret_cast<char*>(&b), sizeof(uint8_t));
            file.read(reinterpret_cast<char*>(&a), sizeof(uint8_t));
            mesh->colors.emplace_back(cglm::Vec3f{ static_cast<float>(r) / 255.0f, static_cast<float>(g) / 255.0f, static_cast<float>(b) / 255.0f });

            // color change
            // cglm::Vec3f light = cglm::mix(white, black, dot(mesh->normals[i], { 0.0f, 0.0f, 1.0f }) * 0.5f + 0.5f);
            // mesh->colors[i] = light * mesh->colors[i];
        }

        // close file
        file.close();

        return mesh;
    }


    std::shared_ptr<Node> SceneConfig::generateNode(const mcjp::Object* obj) {
        std::shared_ptr<Node> node = std::make_shared<Node>();
        node->name = std::get<std::string>(obj->contents.at("name"));

        // below are optional
        if (obj->contents.find("children") != obj->contents.end()) {
            std::vector<double> dv = std::get<std::vector<double>>(obj->contents.at("children"));
            node->children = { dv.begin(), dv.end() };
        }
        if (obj->contents.find("mesh") != obj->contents.end()) {
            // mesh could either be a single id or a list of ids
            if (std::holds_alternative<double>(obj->contents.at("mesh"))) {
                node->mesh.push_back(std::get<double>(obj->contents.at("mesh")));
            } else if (std::holds_alternative<std::vector<double>>(obj->contents.at("mesh"))) {
                std::vector<double> mv = std::get<std::vector<double>>(obj->contents.at("mesh"));
                node->mesh = { mv.begin(), mv.end() };
            }
        }

        // they could be double or int
        std::vector<double> translation;
        if (std::holds_alternative<std::vector<double>>(obj->contents.at("translation"))) {
            translation = std::get<std::vector<double>>(obj->contents.at("translation"));
        } else if (std::holds_alternative<std::vector<int>>(obj->contents.at("translation"))) {
            std::vector<int> tmp = std::get<std::vector<int>>(obj->contents.at("translation"));
            translation = { static_cast<double>(tmp[0]), static_cast<double>(tmp[1]), static_cast<double>(tmp[2]) };
        }
        std::vector<double> rotation;
        if (std::holds_alternative<std::vector<double>>(obj->contents.at("rotation"))) {
            rotation = std::get<std::vector<double>>(obj->contents.at("rotation"));
        } else if (std::holds_alternative<std::vector<int>>(obj->contents.at("rotation"))) {
            std::vector<int> tmp = std::get<std::vector<int>>(obj->contents.at("rotation"));
            rotation = { static_cast<double>(tmp[0]), static_cast<double>(tmp[1]), static_cast<double>(tmp[2]) };
        }
        std::vector<double> scale;
        if (std::holds_alternative<std::vector<double>>(obj->contents.at("scale"))) {
            scale = std::get<std::vector<double>>(obj->contents.at("scale"));
        } else if (std::holds_alternative<std::vector<int>>(obj->contents.at("scale"))) {
            std::vector<int> tmp = std::get<std::vector<int>>(obj->contents.at("scale"));
            scale = { static_cast<double>(tmp[0]), static_cast<double>(tmp[1]), static_cast<double>(tmp[2]) };
        }

        cglm::Mat44f translate_m = cglm::translation(cglm::Vec3f{ static_cast<float>(translation[0]), static_cast<float>(translation[1]), static_cast<float>(translation[2]) });
        cglm::Mat44f rotation_m = cglm::rotation(cglm::Vec4f{ static_cast<float>(rotation[0]), static_cast<float>(rotation[1]), static_cast<float>(rotation[2]), static_cast<float>(rotation[3]) });
        cglm::Mat44f scale_m = cglm::scale(cglm::Vec3f{ static_cast<float>(scale[0]), static_cast<float>(scale[1]), static_cast<float>(scale[2]) });

        if (obj->contents.find("camera") != obj->contents.end()) {
            node->camera = static_cast<int>(std::get<double>(obj->contents.at("camera")));
            std::shared_ptr<Camera> camera = cameras[id2camera_name[node->camera]];
            cglm::Vec4f npos = translate_m * cglm::Vec4f{ camera->position, 1.0f };
            camera->position = { npos[0] / npos[3], npos[1] / npos[3], npos[2] / npos[3] };
            camera->dir = rotation_m * camera->dir;
            camera->up = rotation_m * camera->up;
            return node;
        }
        
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
        node->vertex_count = total_size;

        // allocate memory
        node->positions.resize(total_size);
        node->normals.resize(total_size);
        node->colors.resize(total_size);
        // copy & transform data
        size_t offset = 0;
        size_t prev = 0;

        // for direct meshes
        for (auto& id : node->mesh) {
            std::shared_ptr<Mesh> mesh = id2mesh[id];

            for (size_t i = 0; i < mesh->vertex_count; i++) {
                cglm::Vec4f pos{ mesh->positions[i], 1.0f };
                cglm::Vec3f normal{ mesh->normals[i] };

                cglm::Vec4f mid_pos = node->transform * pos;
                node->positions[offset] = { mid_pos[0]/mid_pos[3], mid_pos[1]/mid_pos[3], mid_pos[2]/mid_pos[3] };
                node->normals[offset] = norm_trans * normal;
                node->colors[offset] = mesh->colors[i];
                offset++;
            }

            // We don't actually generate it here, only generate it when root
            std::shared_ptr<Bound_Sphere> bs = std::make_shared<Bound_Sphere>();
            bs->startIdx = prev;
            bs->endIdx = prev + mesh->vertex_count;
            prev = bs->endIdx;
            bs->radius = -1.0f;
            node->bound_spheres.push_back(bs);
        }
        for (auto& id : node->children) {
            std::shared_ptr<Node> child = id2node[id];
            for (auto& bs : child->bound_spheres) {
                size_t start_ = bs->startIdx, end_ = bs->endIdx;
                for (size_t i = start_; i < end_; i++) {
                    cglm::Vec4f pos{ child->positions[i], 1.0f };
                    cglm::Vec3f normal{child->normals[i]};
                    cglm::Vec3f color{child->colors[i]};

                    cglm::Vec4f mid_pos = node->transform * pos;
                    node->positions[offset] = { mid_pos[0] / mid_pos[3], mid_pos[1] / mid_pos[3], mid_pos[2] / mid_pos[3] };
                    node->normals[offset] = norm_trans * normal;
                    node->colors[offset] = color;
                    offset++;
                }
                std::shared_ptr<Bound_Sphere> bs2 = std::make_shared<Bound_Sphere>();
                bs2->startIdx = prev;
                bs2->endIdx = prev + (end_ - start_);
                prev = bs2->endIdx;
                bs2->radius = -1.0f;
                node->bound_spheres.push_back(bs2);
            }
            
        }

        return node;
    }

    std::shared_ptr<Scene> SceneConfig::generateScene(const mcjp::Object* obj) {
        std::shared_ptr<Scene> scene = std::make_shared<Scene>();
        scene->name = std::get<std::string>(obj->contents.at("name"));
        std::vector<double> rv = std::get<std::vector<double>>(obj->contents.at("roots"));
        scene->children = { rv.begin(), rv.end() };

        // there is only one scene, so we do bounding sphere here
        for (auto& id : scene->children) {
            std::shared_ptr<Node> node = id2node[id];
            // color modify
            cglm::Vec3f white{ 0.0f, 0.0f, 0.0f };
            cglm::Vec3f black{ 1.0f, 1.0f, 1.0f };
            for (size_t i = 0; i < node->vertex_count; i++) {
                cglm::Vec3f light = cglm::mix(white, black, dot(node->normals[i], { 0.0f, 0.0f, 1.0f }) * 0.5f + 0.5f);
                node->colors[i] = light * node->colors[i];
            }
            // std::cout << "Node " << id << " " << node->name << " has " << node->vertex_count << " vertices" << std::endl;
            for (auto& bs : node->bound_spheres) {
                size_t start_ = bs->startIdx, end_ = bs->endIdx;

                // we need correct center and radius
                cglm::Vec3f x = node->positions[start_];
                cglm::Vec3f y, z;
                float clen = 0.0f;
                for (size_t i = start_ + 1; i < end_; i++) {
                    if (cglm::length(node->positions[i] - x) > clen) {
                        y = node->positions[i];
                        clen = cglm::length(node->positions[i] - x);
                    }
                }
                clen = 0.0f;
                for (size_t i = start_; i < end_; i++) {
                    if (cglm::length(node->positions[i] - y) > clen) {
                        z = node->positions[i];
                        clen = cglm::length(node->positions[i] - y);
                    }
                }
                bs->center = (x + y + z) / 3.0f;
                bs->radius = std::max(std::max(cglm::length(x - bs->center), cglm::length(y - bs->center)), cglm::length(z - bs->center));

                for (size_t i = start_; i < end_; i++) {
                    float dist = cglm::length(node->positions[i] - bs->center);
                    if (dist > bs->radius) {
                        bs->radius = dist;
                        bs->center = (node->positions[i] + bs->center) / 2.0f;
                    }
                }

                // std::cout << "Bounding Sphere " << bs->center << " " << bs->radius << std::endl;
            }
        }
        return scene;
    }
    
    void SceneConfig::load_scene(const std::string& scene_file_name) {
        if (scene_file_name.empty()) {
            throw std::runtime_error("Scene File Name is Empty!");
        }
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
                id2camera_name[i] = cameraPtr->name;
            }
            else if (type == "mesh" || type == "MESH") {
                std::shared_ptr<Mesh> meshPtr = generateMesh(obj);
                id2mesh[i] = meshPtr;
            }
            else if (type == "node" || type == "NODE") {
                std::shared_ptr<Node> nodePtr = generateNode(obj);
                id2node[i] = nodePtr;
                std::cout << "Node " << nodePtr->name << " has " << nodePtr->vertex_count << " vertices" << std::endl;
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
                cameraPtr->far = 50.0f;
                cameraPtr->position = { 0.0f, 0.0f, 4.0f };
                cameraPtr->up = { 0.0f, 1.0f, 0.0f };
                cameraPtr->dir = {0.0f, 0.0f, -1.0f};
                cameraPtr->boundary_view = static_cast<float>(sin(atan(sqrt(1 + pow(cameraPtr->aspect, 2)) * tan(cameraPtr->vfov / 2))));
                cameras["debug"] = cameraPtr;
                cameraPtr->update_planes();
                this->cur_camera = "debug";
                id2camera_name[-1] = "debug";
            }
            if (cameras["user"] == nullptr) {
                // we create a default camera
                std::shared_ptr<Camera> cameraPtr = std::make_shared<Camera>();
                cameraPtr->name = "user";
                cameraPtr->aspect = 1.777f;
                cameraPtr->vfov = 1.04719f;
                cameraPtr->near = 0.1f;
                cameraPtr->far = 10.0f;
                cameraPtr->position = { 0.0f, 0.0f, 4.0f };
                cameraPtr->up = { 0.0f, 1.0f, 0.0f };
                cameraPtr->dir = {0.0f, 0.0f, -1.0f};
                cameraPtr->boundary_view = static_cast<float>(sin(atan(sqrt(1 + pow(cameraPtr->aspect, 2)) * tan(cameraPtr->vfov / 2))));
                cameras["user"] = cameraPtr;
                cameraPtr->update_planes();
                id2camera_name[0] = "user";
            }
        }

        // cleanups
        for (auto obj : objects) {
            obj->cleanup();
        }
    }

    size_t SceneConfig::get_total_vertex_count() {
        size_t total = 0;
        for (int node_id : scene->children) {
            total += id2node[node_id]->vertex_count;
        }
        return total;
    }

} // namespace sconfig