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

        cglm::Vec3f p0 = front_o - dx0 + dy0;
        cglm::Vec3f p1 = front_o + dx0 + dy0;
        cglm::Vec3f p2 = front_o + dx0 - dy0;
        cglm::Vec3f p3 = front_o - dx0 - dy0;
        cglm::Vec3f p4 = back_o - dx1 + dy1;
        cglm::Vec3f p5 = back_o + dx1 + dy1;
        cglm::Vec3f p6 = back_o + dx1 - dy1;

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
        left->d = orgDistanceToPlane(p4, left);
        this->bounds.push_back(left);
        // right
        std::shared_ptr<Plane> right = std::make_shared<Plane>();
        right->normal = cglm::normalize(cglm::cross(p1 - p2, p5 - p1));
        right->d = orgDistanceToPlane(p5, right);
        this->bounds.push_back(right);
        // top
        std::shared_ptr<Plane> top = std::make_shared<Plane>();
        top->normal = cglm::normalize(cglm::cross(p1 - p0, p0 - p4));
        top->d = orgDistanceToPlane(p4, top);
        this->bounds.push_back(top);
        // bottom
        std::shared_ptr<Plane> bottom = std::make_shared<Plane>();
        bottom->normal = cglm::normalize(cglm::cross(p2 - p3, p6 - p2));
        bottom->d = orgDistanceToPlane(p6, bottom);
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

        camera->update_planes();

        return camera;
    }

    void generateBoundingSphere(std::shared_ptr<Mesh>& mesh) {
        float minx, miny, minz, maxx, maxy, maxz;
        minx = maxx = mesh->positions[0][0];
        miny = maxy = mesh->positions[0][1];
        minz = maxz = mesh->positions[0][2];

        for (auto& pos : mesh->positions) {
            minx = std::min(minx, pos[0]);
            miny = std::min(miny, pos[1]);
            minz = std::min(minz, pos[2]);
            maxx = std::max(maxx, pos[0]);
            maxy = std::max(maxy, pos[1]);
            maxz = std::max(maxz, pos[2]);
        }

        cglm::Vec3f center = { (minx + maxx) / 2, (miny + maxy) / 2, (minz + maxz) / 2 };
        float radius = cglm::length(cglm::Vec3f{maxx, maxy, maxz} - center);
        mesh->bound_sphere = std::make_shared<Bound_Sphere>();
        mesh->bound_sphere->center = center;
        mesh->bound_sphere->radius = radius;
    }


    /**
     * Material Generator
    */
    std::shared_ptr<Material> SceneConfig::generateMaterial(const mcjp::Object* obj) {
        std::shared_ptr<Material> material = std::make_shared<Material>();
        material->name = std::get<std::string>(obj->contents.at("name"));

        // normal map
        if (obj->contents.find("normalMap") != obj->contents.end()) {
            mcjp::Object* nm_src = std::get<mcjp::Object*>(obj->contents.at("normalMap"));
            material->normal_map = std::get<std::string>(nm_src->contents.at("src"));
        }

        // pbr, lambertian, mirror, environment, simple, should only be one of them
        if (obj->contents.find("pbr") != obj->contents.end()) {
            material->matetial_type = MaterialType::pbr;
            mcjp::Object* pbrb = std::get<mcjp::Object*>(obj->contents.at("pbr"));
            auto& albedo = pbrb->contents.at("albedo");
            auto& roughness = pbrb->contents.at("roughness");
            auto& metalness = pbrb->contents.at("metalness");

            std::shared_ptr<Pbr> pbr_data = std::make_shared<Pbr>();
            pbr_data->albedo_type = TextureType::texture2D;
            pbr_data->roughness_type = TextureType::texture2D;
            pbr_data->metalness_type = TextureType::texture2D;

            // albedo
            if (std::holds_alternative<std::vector<double>>(albedo)) {
                pbr_data->albedo = std::get<std::vector<double>>(albedo);
            }
            else {
                auto& src_obj = std::get<mcjp::Object*>(albedo);
                pbr_data->albedo = std::get<std::string>(src_obj->contents.at("src"));
                if (src_obj->contents.find("type") != src_obj->contents.end()) {
                    std::string tp = std::get<std::string>(src_obj->contents.at("type"));
                    if (tp == "cube") {
                        pbr_data->albedo_type = TextureType::textureCube;
                    }
                }
            }
            // roughness
            if (std::holds_alternative<double>(roughness)) {
                pbr_data->roughness = std::get<double>(roughness);
            }
            else {
                auto& src_obj = std::get<mcjp::Object*>(roughness);
                pbr_data->roughness = std::get<std::string>(src_obj->contents.at("src"));
                if (src_obj->contents.find("type") != src_obj->contents.end()) {
                    std::string tp = std::get<std::string>(src_obj->contents.at("type"));
                    if (tp == "cube") {
                        pbr_data->roughness_type = TextureType::textureCube;
                    }
                }
            }
            // metalness
            if (std::holds_alternative<double>(metalness)) {
                pbr_data->metalness = std::get<double>(metalness);
            }
            else {
                auto& src_obj = std::get<mcjp::Object*>(metalness);
                pbr_data->metalness = std::get<std::string>(src_obj->contents.at("src"));
                if (src_obj->contents.find("type") != src_obj->contents.end()) {
                    std::string tp = std::get<std::string>(src_obj->contents.at("type"));
                    if (tp == "cube") {
                        pbr_data->metalness_type = TextureType::textureCube;
                    }
                }
            }

            material->matetial_detail = pbr_data;
        }

        else if (obj->contents.find("lambertian") != obj->contents.end()) {
            material->matetial_type = MaterialType::lambertian;
            mcjp::Object* lamb = std::get<mcjp::Object*>(obj->contents.at("lambertian"));
            auto& albedo = lamb->contents.at("albedo");

            std::shared_ptr<Lambertian> lab_data = std::make_shared<Lambertian>();
            lab_data->albedo_type = TextureType::texture2D;

            // if the case with single values
            if (std::holds_alternative<std::vector<double>>(albedo)) {
                lab_data->albedo = std::get<std::vector<double>>(albedo);
            }
            else {
                auto& src_obj = std::get<mcjp::Object*>(albedo);
                lab_data->albedo = std::get<std::string>(src_obj->contents.at("src"));
                if (src_obj->contents.find("type") != src_obj->contents.end()) {
                    std::string tp = std::get<std::string>(src_obj->contents.at("type"));
                    if (tp == "cube") {
                        lab_data->albedo_type = TextureType::textureCube;
                    }
                }
            }

            material->matetial_detail = lab_data;
        }
        else if (obj->contents.find("mirror") != obj->contents.end()) {
            material->matetial_type = MaterialType::mirror;
        }
        else if (obj->contents.find("environment") != obj->contents.end()) {
            material->matetial_type = MaterialType::environment;
        }
        else if (obj->contents.find("simple") != obj->contents.end()) {
            material->matetial_type = MaterialType::simple;
        }

        return material;
    }


    /**
     * Environment Generator
    */
    
    std::shared_ptr<Environment> SceneConfig::generateEnvironment(const mcjp::Object* obj) {
        std::shared_ptr<Environment> environment = std::make_shared<Environment>();
        environment->name = std::get<std::string>(obj->contents.at("name"));
        mcjp::Object* radiance = std::get<mcjp::Object*>(obj->contents.at("radiance"));

        environment->texture_src = std::get<std::string>(radiance->contents.at("src"));
        std::string env_tp = std::get<std::string>(radiance->contents.at("type"));
        if (env_tp == "cube") {
            environment->env_type = TextureType::textureCube;
        }
        else if (env_tp == "2D") {
            environment->env_type = TextureType::texture2D;
        }

        environment->texture_format = std::get<std::string>(radiance->contents.at("format"));

        return environment;
    }

    
    /**
     * Mesh Generator 
    */
    std::shared_ptr<Mesh> SceneConfig::generateMesh(const mcjp::Object* obj) {
        std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();
        mesh->inner_id = cur_mesh++;
        mesh->name = std::get<std::string>(obj->contents.at("name"));
        mesh->topology = std::get<std::string>(obj->contents.at("topology"));
        mesh->vertex_count = static_cast<size_t>(std::get<double>(obj->contents.at("count")));
        mesh->material_id = -1;
        if (obj->contents.find("material") != obj->contents.end()) {
            mesh->material_id = static_cast<int>(std::get<double>(obj->contents.at("material")));
        }

        // handle optional indices
        if (obj->contents.find("indices") != obj->contents.end()) {
            throw std::runtime_error("Indices not supported yet!");
        }

        // reading files to get vertex data
        mcjp::Object* attributes = std::get<mcjp::Object*>(obj->contents.at("attributes"));
        mcjp::Object* position = std::get<mcjp::Object*>(attributes->contents.at("POSITION"));
        mcjp::Object* normal = std::get<mcjp::Object*>(attributes->contents.at("NORMAL"));
        mcjp::Object* color = std::get<mcjp::Object*>(attributes->contents.at("COLOR"));

        // tangent and texcoord are optional
        mcjp::Object* tangent;
        if (attributes->contents.find("TANGENT") == attributes->contents.end()) {
            tangent = nullptr;
        }
        else {
            tangent = std::get<mcjp::Object*>(attributes->contents.at("TANGENT"));
        }

        mcjp::Object* texcoord;
        if (attributes->contents.find("TEXCOORD") == attributes->contents.end()) {
            texcoord = nullptr;
        }
        else {
            texcoord = std::get<mcjp::Object*>(attributes->contents.at("TEXCOORD"));
        }

        mesh->position_format = std::get<std::string>(position->contents.at("format"));
        mesh->normal_format = std::get<std::string>(normal->contents.at("format"));
        mesh->color_format = std::get<std::string>(color->contents.at("format"));

        std::string file_name = std::get<std::string>(position->contents.at("src"));

        int stride = std::get<double>(position->contents.at("stride"));
        int pos_offset = std::get<double>(position->contents.at("offset"));
        int normal_offset = std::get<double>(normal->contents.at("offset"));
        int tangent_offset = -1;
        if (tangent != nullptr) {
            tangent_offset = std::get<double>(tangent->contents.at("offset"));
        }
        int texcoord_offset = -1;
        if (texcoord != nullptr) {
            texcoord_offset = std::get<double>(texcoord->contents.at("offset"));
        }
        int color_offset = std::get<double>(color->contents.at("offset"));

        std::ifstream file(file_name, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to Open SubScene File!");
        }

        // read vertex_count times, each time read stride bytes
        for (int i = 0; i < mesh->vertex_count; i++) {
            // positions
            file.seekg(pos_offset + i * stride);
            float x, y, z;
            file.read(reinterpret_cast<char*>(&x), sizeof(float));
            file.read(reinterpret_cast<char*>(&y), sizeof(float));
            file.read(reinterpret_cast<char*>(&z), sizeof(float));
            mesh->positions.push_back(cglm::Vec3f{ x, y, z });

            // normals
            file.seekg(normal_offset + i * stride);
            float nx, ny, nz;
            file.read(reinterpret_cast<char*>(&nx), sizeof(float));
            file.read(reinterpret_cast<char*>(&ny), sizeof(float));
            file.read(reinterpret_cast<char*>(&nz), sizeof(float));
            mesh->normals.push_back(cglm::Vec3f{ nx, ny, nz });

            // tangents
            if (tangent_offset != -1) {
                file.seekg(tangent_offset + i * stride);
                float tx, ty, tz, tw;
                file.read(reinterpret_cast<char*>(&tx), sizeof(float));
                file.read(reinterpret_cast<char*>(&ty), sizeof(float));
                file.read(reinterpret_cast<char*>(&tz), sizeof(float));
                file.read(reinterpret_cast<char*>(&tw), sizeof(float));
                mesh->tangents.push_back(cglm::Vec4f{ tx, ty, tz, tw });
            }

            // texcoords
            if (texcoord_offset != -1) {
                file.seekg(texcoord_offset + i * stride);
                float u, v;
                file.read(reinterpret_cast<char*>(&u), sizeof(float));
                file.read(reinterpret_cast<char*>(&v), sizeof(float));
                mesh->texcoords.push_back(cglm::Vec2f{ u, v });
            }
        
            // if (mesh->name == "Cube" && mesh->material_id == 40.0) {
            //     std::cout << x << " " << y << " " << z << " => ";
            //     std::cout << mesh->texcoords.back()[0] << " " << mesh->texcoords.back()[1] << std::endl;
            // }

            // colors
            file.seekg(color_offset + i * stride);
            uint8_t r, g, b, a;
            file.read(reinterpret_cast<char*>(&r), sizeof(uint8_t));
            file.read(reinterpret_cast<char*>(&g), sizeof(uint8_t));
            file.read(reinterpret_cast<char*>(&b), sizeof(uint8_t));
            file.read(reinterpret_cast<char*>(&a), sizeof(uint8_t));
            mesh->colors.emplace_back(cglm::Vec3f{ static_cast<float>(r) / 255.0f, static_cast<float>(g) / 255.0f, static_cast<float>(b) / 255.0f });
        }

        // close file
        file.close();

        // we also generate a bounding shpere togther with the mesh
        generateBoundingSphere(mesh);

        return mesh;
    }


    /**
     * Node Generator
    */
    std::shared_ptr<Node> SceneConfig::generateNode(const mcjp::Object* obj, size_t id) {
        std::shared_ptr<Node> node = std::make_shared<Node>();
        node->name = std::get<std::string>(obj->contents.at("name"));
        node->id = static_cast<int>(id);
        node->vertex_count = 0;

        // below are optional
        if (obj->contents.find("children") != obj->contents.end()) {
            std::vector<double> dv = std::get<std::vector<double>>(obj->contents.at("children"));
            node->children = { dv.begin(), dv.end() };
            for (auto& child : node->children) {
                id2node[child]->parents.push_back(id);
            }
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

        // retrieve transformation & rotation & scale, for current node
        std::vector<double> translation = std::get<std::vector<double>>(obj->contents.at("translation"));
        std::vector<double> rotation = std::get<std::vector<double>>(obj->contents.at("rotation"));
        std::vector<double> scale = std::get<std::vector<double>>(obj->contents.at("scale"));

        cglm::Mat44f translate_m = cglm::translation(cglm::Vec3f{ static_cast<float>(translation[0]), static_cast<float>(translation[1]), static_cast<float>(translation[2]) });
        cglm::Mat44f rotation_m = cglm::rotation(cglm::Vec4f{ static_cast<float>(rotation[0]), static_cast<float>(rotation[1]), static_cast<float>(rotation[2]), static_cast<float>(rotation[3]) });
        cglm::Mat44f scale_m = cglm::scale(cglm::Vec3f{ static_cast<float>(scale[0]), static_cast<float>(scale[1]), static_cast<float>(scale[2]) });

        // transformation could be applied to camera
        if (obj->contents.find("camera") != obj->contents.end()) {
            node->camera = static_cast<int>(std::get<double>(obj->contents.at("camera")));
            std::shared_ptr<Camera> camera = cameras[id2camera_name[node->camera]];
            cglm::Vec4f npos = translate_m * cglm::Vec4f{ camera->position, 1.0f };
            camera->position = { npos[0] / npos[3], npos[1] / npos[3], npos[2] / npos[3] };
            camera->dir = rotation_m * camera->dir;
            camera->up = rotation_m * camera->up;
            camera->update_planes();
            return node;
        }

        // then the rest are all cases to instances
        // node->transform = translate_m * rotation_m * scale_m;
        // node->animation_transform = cglm::identity(1.0f);
        node->translation = translate_m;
        node->rotation = rotation_m;
        node->scale = scale_m;

        // inherit information will not change, we create instances here
        // all children's instances are directly my instances
        for (auto& child : node->children) {
            node->vertex_count += id2node[child]->vertex_count;
            for (auto& instance : id2node[child]->instances) {
                std::shared_ptr<Instance> inst(instance);
                // here no need increase id, since we are not creating new instance
                node->instances.push_back(inst);
            }
        }
        // but for direct meshes, we need to create instances
        for (auto& mesh_id : node->mesh) {
            node->vertex_count += id2mesh[mesh_id]->vertex_count;
            std::shared_ptr<Mesh> mesh = id2mesh[mesh_id];
            std::shared_ptr<Instance> inst = std::make_shared<Instance>();
            inst->id = cur_instance++;
            inst->mesh_id = mesh_id;
            node->instances.push_back(inst);
            id2instance[inst->id] = inst;
        }

        return node;
    }

    std::shared_ptr<Scene> SceneConfig::generateScene(const mcjp::Object* obj) {
        std::shared_ptr<Scene> scene = std::make_shared<Scene>();
        scene->name = std::get<std::string>(obj->contents.at("name"));
        std::vector<double> rv = std::get<std::vector<double>>(obj->contents.at("roots"));
        scene->children = { rv.begin(), rv.end() };

        return scene;
    }

    std::shared_ptr<Driver> SceneConfig::generateDriver(const mcjp::Object* obj) {
        std::shared_ptr<Driver> driver = std::make_shared<Driver>();
        driver->name = std::get<std::string>(obj->contents.at("name"));
        driver->node = static_cast<int>(std::get<double>(obj->contents.at("node")));
        driver->useful = true;
        std::string prev_name = id2node[driver->node]->driver_name;
        std::cout << "Driver " << driver->name << " " << driver->node << " " << prev_name << std::endl;
        std::cout << "Prev name " << prev_name << std::endl;
        if (!prev_name.empty()) {
            name2driver[prev_name]->useful = false;
        }
        id2node[driver->node]->driver_name = driver->name;
        driver->channel = std::get<std::string>(obj->contents.at("channel"));
        driver->times = std::get<std::vector<double>>(obj->contents.at("times"));
        driver->values = std::get<std::vector<double>>(obj->contents.at("values"));
        driver->interpolation = std::get<std::string>(obj->contents.at("interpolation"));

        return driver;
    }

    cglm::Mat44f Driver::getCurrentTransform(double dtime) {
        double rtime = fmod(dtime, times.back());
        auto it = std::lower_bound(times.begin(), times.end(), rtime);
        int idx = std::distance(times.begin(), it);
        // very occasional case, we need to handle it, just land on the first key frame
        if (idx == 0) {
            if (channel == "rotation") {
                cglm::Vec4f vr = cglm::Vec4f{ static_cast<float>(values[idx*4]), static_cast<float>(values[idx*4+1]), static_cast<float>(values[idx*4+2]), static_cast<float>(values[idx*4+3]) };
                return cglm::rotation(vr);
            }
            if (channel == "translation") {
                cglm::Vec3f vt = cglm::Vec3f{ static_cast<float>(values[idx*3]), static_cast<float>(values[idx*3+1]), static_cast<float>(values[idx*3+2]) };
                return cglm::translation(vt);
            }
            if (channel == "scale") {
                cglm::Vec3f vs = cglm::Vec3f{ static_cast<float>(values[idx*3]), static_cast<float>(values[idx*3+1]), static_cast<float>(values[idx*3+2]) };
                return cglm::scale(vs);
            }
        }

        --idx;
        if (interpolation == "STEP") {
            if (channel == "rotation") {
                cglm::Vec4f vr = cglm::Vec4f{ static_cast<float>(values[idx*4]), static_cast<float>(values[idx*4+1]), static_cast<float>(values[idx*4+2]), static_cast<float>(values[idx*4+3]) };
                return cglm::rotation(vr);
            }
            if (channel == "translation") {
                cglm::Vec3f vt = cglm::Vec3f{ static_cast<float>(values[idx*3]), static_cast<float>(values[idx*3+1]), static_cast<float>(values[idx*3+2]) };
                return cglm::translation(vt);
            }
            if (channel == "scale") {
                cglm::Vec3f vs = cglm::Vec3f{ static_cast<float>(values[idx*3]), static_cast<float>(values[idx*3+1]), static_cast<float>(values[idx*3+2]) };
                return cglm::scale(vs);
            }
        }
        else if (interpolation == "LINEAR") {
            double d1 = rtime - times[idx], d2 = times[idx + 1] - rtime;
            float p1 = static_cast<float> (d1 / (d1+d2)), p2 = static_cast<float> (d2 / (d1+d2));
            if (channel == "rotation") {
                cglm::Vec4f vr1 = cglm::Vec4f{ static_cast<float>(values[idx*4]), static_cast<float>(values[idx*4+1]), static_cast<float>(values[idx*4+2]), static_cast<float>(values[idx*4+3]) };
                cglm::Vec4f vr2 = cglm::Vec4f{ static_cast<float>(values[(idx+1)*4]), static_cast<float>(values[(idx+1)*4+1]), static_cast<float>(values[(idx+1)*4+2]), static_cast<float>(values[(idx+1)*4+3]) };
                return cglm::rotation(vr1 * p2 + vr2 * p1);
            }
            if (channel == "translation") {
                cglm::Vec3f vt1 = cglm::Vec3f{ static_cast<float>(values[idx*3]), static_cast<float>(values[idx*3+1]), static_cast<float>(values[idx*3+2]) };
                cglm::Vec3f vt2 = cglm::Vec3f{ static_cast<float>(values[(idx+1)*3]), static_cast<float>(values[(idx+1)*3+1]), static_cast<float>(values[(idx+1)*3+2]) };
                return cglm::translation(vt1 * p2 + vt2 * p1);
            }
            if (channel == "scale") {
                cglm::Vec3f vs1 = cglm::Vec3f{ static_cast<float>(values[idx*3]), static_cast<float>(values[idx*3+1]), static_cast<float>(values[idx*3+2]) };
                cglm::Vec3f vs2 = cglm::Vec3f{ static_cast<float>(values[(idx+1)*3]), static_cast<float>(values[(idx+1)*3+1]), static_cast<float>(values[(idx+1)*3+2]) };
                return cglm::scale(vs1 * p2 + vs2 * p1);
            }
        }
        else {
            // case for SLERP
            double d1 = rtime - times[idx], d2 = times[idx + 1] - rtime;
            float t = static_cast<float> (d1 / (d1 + d2));
            if (channel == "rotation") {
                cglm::Vec4f vr1 = cglm::Vec4f{ static_cast<float>(values[idx*4]), static_cast<float>(values[idx*4+1]), static_cast<float>(values[idx*4+2]), static_cast<float>(values[idx*4+3]) };
                cglm::Vec4f vr2 = cglm::Vec4f{ static_cast<float>(values[(idx+1)*4]), static_cast<float>(values[(idx+1)*4+1]), static_cast<float>(values[(idx+1)*4+2]), static_cast<float>(values[(idx+1)*4+3]) };
                return cglm::rotation(cglm::slerp(vr1, vr2, t));
            }
            if (channel == "translation") {
                cglm::Vec3f vt1 = cglm::Vec3f{ static_cast<float>(values[idx*3]), static_cast<float>(values[idx*3+1]), static_cast<float>(values[idx*3+2]) };
                cglm::Vec3f vt2 = cglm::Vec3f{ static_cast<float>(values[(idx+1)*3]), static_cast<float>(values[(idx+1)*3+1]), static_cast<float>(values[(idx+1)*3+2]) };
                std::cout << "Do you really want SLERP for translation?" << std::endl;
                return cglm::translation(vt1 * (1 - t) + vt2 * t);
            }
            if (channel == "scale") {
                cglm::Vec3f vs1 = cglm::Vec3f{ static_cast<float>(values[idx*3]), static_cast<float>(values[idx*3+1]), static_cast<float>(values[idx*3+2]) };
                cglm::Vec3f vs2 = cglm::Vec3f{ static_cast<float>(values[(idx+1)*3]), static_cast<float>(values[(idx+1)*3+1]), static_cast<float>(values[(idx+1)*3+2]) };
                std::cout << "Do you really want SLERP for scale?" << std::endl;
                return cglm::scale(vs1 * (1 - t) + vs2 * t);
            }
        }

        std::cout << "Unknown Channel Type!" << std::endl;
        return cglm::identity(1.0f);
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

        // initialize parameters
        this->cur_instance = 0;
        this->cur_mesh = 0;

        // default material is simple
        std::shared_ptr<Material> materialPtr = std::make_shared<Material>();
        materialPtr->name = "_default_simple";
        materialPtr->matetial_type = MaterialType::simple;
        id2material[-1] = materialPtr;

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
                meshPtr->id = static_cast<int>(i);
                id2mesh[i] = meshPtr;
                innerId2meshId[meshPtr->inner_id] = meshPtr->id;
                // std::cout << "Mesh " << meshPtr->id << " " << meshPtr->name << " has vertex count " << meshPtr->vertex_count << std::endl;
            }
            else if (type == "node" || type == "NODE") {
                std::shared_ptr<Node> nodePtr = generateNode(obj, i);
                id2node[i] = nodePtr;
                std::cout << "Node " << nodePtr->name << " has vertex count " << nodePtr->vertex_count << std::endl;
            }
            else if (type == "scene" || type == "SCENE") {
                scene = generateScene(obj);
            }
            else if (type == "driver" || type == "DRIVER") {
                std::shared_ptr<Driver> driverPtr = generateDriver(obj);
                name2driver[driverPtr->name] = driverPtr;
            }
            else if (type == "material" || type == "MATERIAL") {
                std::shared_ptr<Material> materialPtr = generateMaterial(obj);
                materialPtr->idx = static_cast<int>(i);
                id2material[i] = materialPtr;
            }
            else if (type == "environment" || type == "ENVIRONMENT") {
                environment = generateEnvironment(obj);
            }

            if (cameras["debug"] == nullptr) {
                // we create a default camera
                std::shared_ptr<Camera> cameraPtr = std::make_shared<Camera>();
                cameraPtr->name = "debug";
                cameraPtr->aspect = 1.777f;
                cameraPtr->vfov = 0.47109f;
                cameraPtr->near = 0.1f;
                cameraPtr->far = 1000.0f;
                cameraPtr->position = { -8.0f, 2.0f, 10.0f };
                cameraPtr->dir = { 0.536329f, -0.086282f, -0.839587f };
                cameraPtr->up = {0.0f, 1.0f, 0.0f};
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
                cameraPtr->far = 100.0f;
                cameraPtr->position = { 0.0f, 0.0f, 4.0f };
                cameraPtr->dir = {0.0f, 0.0f, -1.0f};
                cameraPtr->up = { 0.0f, 1.0f, 0.0f };
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

    size_t SceneConfig::get_mesh_vertex_count() {
        size_t total = 0;
        for (auto& [id, mesh] : id2mesh) {
            total += mesh->vertex_count;
        }
        return total;
    }

} // namespace sconfig