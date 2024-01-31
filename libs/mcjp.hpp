#pragma once

#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <variant>
#include <stdexcept>

namespace mcjp {
    struct Object {
        using Data = std::variant<int, double, std::string, Object*,
            std::vector<int>, std::vector<std::string>, std::vector<double>,
            std::vector<Object*>, std::monostate>;

        // internals
        std::unordered_map < std::string, Data > contents;

        // constructor
        Object() = default;

        // interface
        void cleanup();
    };

    using Result = std::variant<Object*, std::vector<Object*>>;

    Result load(const std::string& filename);
    Result parse(const std::string& str);

    // below should be hide after debug
    Result buildObject(const std::string& str, int startIdx, int endIdx);
    Result buildVector(const std::string& str, int startIdx, int endIdx);
    void filter(const std::string& str, std::string& res);

    std::ostream& operator<<(std::ostream& os, const Object& obj);

} // namespace mcjp
