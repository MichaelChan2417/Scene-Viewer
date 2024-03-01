#pragma once

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <memory>
#include <stdexcept>
#include <fstream>

enum EventType {
    MARK,
    PLAY,
    AVAILABLE,
    SAVE
};

struct Event {
    long long timestamp;
    EventType type;
    double t;
    double rate;

    std::string args;
};

static void parseEvents(std::string& filename, std::vector<std::shared_ptr<Event>>& events) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Error: cannot open file " + filename);
    }
    events.clear();
    std::string line;
    // each line is a event
    while (std::getline(file, line)) {
        std::shared_ptr<Event> ev = std::make_shared<Event>();
        std::string::size_type pos = line.find_first_of(" ");
        ev->timestamp = std::stoll(line.substr(0, pos));
        line = line.substr(pos + 1);
        if (line[0] == 'M') {
            ev->type = MARK;
            ev->args = line.substr(5);
        }
        else if (line[0] == 'P') {
            ev->type = PLAY;
            pos = line.find_first_of(" ", 5);
            ev->t = std::stod(line.substr(5, pos - 5));
            ev->rate = std::stod(line.substr(pos + 1));
        }
        else if (line[0] == 'A') {
            ev->type = AVAILABLE;
        }
        else if (line[0] == 'S') {
            ev->type = SAVE;
            std::string ss;
            for (char c : line) {
                if (c != '\r' && c != '\n') {
                    ss += c;
                }
            }
            ev->args = ss.substr(5);
        }
        events.push_back(ev);
    }

    file.close();
};