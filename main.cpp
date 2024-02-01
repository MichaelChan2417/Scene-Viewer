#include "scene_viewer.hpp"

#define NDEBUG 1

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

    ASSIGN_RESULT res = assign_values(scene_viewer, w, h, scene_file, camera_name, device_name, events);
    if (res != SUCCESS) {
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