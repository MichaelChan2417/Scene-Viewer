#include "scene_viewer.hpp"
#include "headless.hpp"

// this is the file we are going to implement headless mode

void SceneViewer::run_headless(std::string& events) {
    std::vector<std::shared_ptr<Event>> evs;
    parseEvents(events, evs);

    
}