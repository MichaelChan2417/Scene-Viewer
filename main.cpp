#include "scene_viewer.hpp"
#include "libs/mcjp.hpp"

// #define NDEBUG 1

ASSIGN_RESULT load_scene_file(const std::string& scene_file_name, std::vector<Vertex>& static_vertices);

ASSIGN_RESULT assign_values(SceneViewer& sv, int w, int h, std::string scene_file, std::string camera_name,
    std::string device_name, std::string events);

void parse_arguments(int argc, char* argv[], int& w, int& h, std::string& scene_file, std::string& camera_name,
    std::string& device_name, std::string& events, bool& list_devices);


int main(int argc, char* argv[]) {

    // handling arguments
    int w, h;
    std::string scene_file, camera_name, device_name, events;
    bool list_devices = false;
    parse_arguments(argc, argv, w, h, scene_file, camera_name, device_name, events, list_devices);

    SceneViewer scene_viewer;

    if (list_devices) {
        scene_viewer.list_physical_devices();
        return EXIT_SUCCESS;
    }

    // assigning values to scene_viewer
    if (assign_values(scene_viewer, w, h, scene_file, camera_name, device_name, events) != SUCCESS) {
        return EXIT_FAILURE;
    }

    // loading scene_file
    if (load_scene_file(scene_file, static_vertices) != SUCCESS) {
        return EXIT_FAILURE;
    }

    // main call
    try {
        scene_viewer.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

struct FloatData {
    float x, y, z;
};

struct RgbaData {
    uint8_t r, g, b, a;
};

ASSIGN_RESULT load_scene_file(const std::string& scene_file_name, std::vector<Vertex>& static_vertices) {

    mcjp::Result result = mcjp::load(scene_file_name);
    std::vector<mcjp::Object*> objects;

    if (std::holds_alternative<std::vector<mcjp::Object*>>(result)) {
        objects = std::get<std::vector<mcjp::Object*>>(result);
    } else {
        std::cerr << "Error: " << scene_file_name << " is not a valid scene file" << std::endl;
        return FAILURE;
    }

    mcjp::Object mesh = *objects[2];
    int cnt = std::get<int>(mesh.contents["count"]);
    mcjp::Object attributes = *std::get<mcjp::Object*>(mesh.contents["attributes"]);
    mcjp::Object pos = *std::get<mcjp::Object*>(attributes.contents["POSITION"]);
    int stride = std::get<int>(pos.contents["stride"]);
    std::string binary_path = "resources/heart/heart.Heart.b72";

    // there are cnt vertex in total, each vertex has 3 float and stride of pos is stride
    std::ifstream bfile(binary_path, std::ios::binary);

    FloatData float_data;
    RgbaData rgba_data;

    while (bfile.read(reinterpret_cast<char*>(&float_data), sizeof(FloatData))) {
        Vertex vtx;
        vtx.pos = { float_data.x, float_data.y, float_data.z };

        bfile.seekg(12, std::ios::cur);

        bfile.read(reinterpret_cast<char*>(&rgba_data), sizeof(RgbaData));
        vtx.color = { static_cast<float>(rgba_data.r) / 255.0f, static_cast<float>(rgba_data.g) / 255.0f, static_cast<float>(rgba_data.b) / 255.0f };
        static_vertices.push_back(vtx);
    }

    // for (auto& vtx : static_vertices) {
    //     std::cout << "pos: " << vtx.pos.x << " " << vtx.pos.y << " " << vtx.pos.z << std::endl;
    //     std::cout << "color: " << vtx.color.r << " " << vtx.color.g << " " << vtx.color.b << std::endl;
    // }

    return SUCCESS;
}


// implement the assign_values function
ASSIGN_RESULT assign_values(SceneViewer& sv, int w, int h, std::string scene_file, std::string camera_name,
                            std::string device_name, std::string events) {
    if (w != 0 && h != 0) {
        sv.window_width = w;
        sv.window_height = h;
    }

    if (!scene_file.empty()) {
        sv.scene_file = scene_file;
    }

    if (!camera_name.empty()) {
        sv.camera_name = camera_name;
    }

    if (!device_name.empty()) {
        sv.device_name = device_name;
    }

    if (!events.empty()) {
        sv.events = events;
    }

    return SUCCESS;
}

// TODO: culling left
// TODO: there is a bug here: enter name with space, it will not work
void parse_arguments(int argc, char* argv[], int& w, int& h, std::string& scene_file, std::string& camera_name,
                    std::string& device_name, std::string& events, bool& list_devices) {
    if (argc == 1) {
        return;
    }

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--scene") {
            scene_file = argv[i + 1];
            ++i;
        }
        else if (arg == "--camera") {
            camera_name = argv[i + 1];
            ++i;
        }
        else if (arg == "--physical-device") {
            device_name = argv[i + 1];
            ++i;
        }
        else if (arg == "--drawing-size") {
            w = std::stoi(argv[i + 1]);
            h = std::stoi(argv[i + 2]);
            i += 2;
        }
        else if (arg == "--list-physical-devices") {
            list_devices = true;
        }
        else if (arg == "--headless") {
            events = argv[i + 1];
            ++i;
        }
    }
}