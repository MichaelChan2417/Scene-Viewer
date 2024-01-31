#include "mcjp.hpp"

namespace mcjp {
    void filter(const std::string& str, std::string& res);

    Result buildObject(const std::string& str, int startIdx, int endIdx) {
        Object* res = new Object();
        // we need to find the boundary of each pair, str is start with '{' and end with '}'
        // std::cout << "POOOOOOOOOO" << str.substr(startIdx, endIdx - startIdx + 1) << std::endl;
        int previdx = startIdx;
        int lvl = 0;
        bool in_string = false;
        for (int i = startIdx+1; i < endIdx; i++) {
            if (str[i] == '"' || str[i] == '\'') {
                in_string = !in_string;
                continue;
            }
            if (str[i] == '[' || str[i] == '{') {
                ++lvl;
            }
            else if (str[i] == ']' || str[i] == '}') {
                --lvl;
            }

            if (str[i] == ',' && lvl == 0 && !in_string) {
                // we find a pair range (previdx, i)
                std::string pair = str.substr(previdx+1, i - previdx);
                // we need to find the boundary of key and value
                int colonIdx = -1;
                for (size_t j = 0; j < pair.size(); j++) {
                    if (pair[j] == ':') {
                        colonIdx = j;
                        break;
                    }
                }
                if (colonIdx == -1) {
                    // error
                    std::cout << "Error: no colon found in pair: " << pair << std::endl;
                    return res;
                }
                std::string key = pair.substr(1, colonIdx-2);   // key is always without quotes
                std::string value = pair.substr(colonIdx + 1, pair.size() - colonIdx - 2);  // make value without ','
                // std::cout << "PREV key: " << key << " value: " << value << std::endl;
                if (value[0] == '\"') {
                    // it's a string
                    std::string vv = value.substr(1, value.size() - 2);
                    // std::cout << "key: " << key << " value: " << vv << std::endl;
                    res->contents[key] = vv;
                }
                else if (value[0] == '{') {
                    // it's an object
                    Object* obj = std::get<Object*>(buildObject(value, 0, value.size() - 1));
                    res->contents[key] = obj;
                }
                else if (value[0] == '[') {
                    // it's a vector
                    if (value[1] == '\"') {
                        // std::vector < std::string> vec;
                        // TODO: this case seems not possible, need check
                        std::cout << "Error: vector of string not supported yet" << std::endl;
                    }
                    else if (value[1] == '{') {
                        // TODO: this case seems not possible, need check
                        std::cout << "Error: vector of object not supported yet" << std::endl;
                    }
                    else {
                        // it's a number, but could be int or double
                        value = value.substr(1, value.size() - 2);  // remove '[' and ']'
                        if (value.find('.') != std::string::npos) {
                            // it's a double
                            std::vector<double> nvec;
                            std::stringstream ss(value);
                            std::string token;
                            for (int i = 0; std::getline(ss, token, ','); i++) {
                                try {
                                    double doubleVal = std::stod(token);
                                    nvec.push_back(doubleVal);
                                }
                                catch (std::invalid_argument& ex) {
                                    std::cout << "Error: invalid number: " << token << std::endl;
                                }
                            }
                            res->contents[key] = nvec;
                        }
                        else {
                            // it's an int
                            std::vector<int> nvec;
                            std::stringstream ss(value);
                            std::string token;
                            for (int i = 0; std::getline(ss, token, ','); i++) {
                                try {
                                    int intVal = std::stoi(token);
                                    nvec.push_back(intVal);
                                }
                                catch (std::invalid_argument& ex) {
                                    std::cout << "Error: invalid number: " << token << std::endl;
                                }
                            }
                            res->contents[key] = nvec;
                        }
                    }
                }
                else {
                    // it's a number
                    try {
                        int intVal = std::stoi(value);
                        res->contents[key] = intVal;
                    }
                    catch (std::invalid_argument& ex) {
                        try {
                            double doubleVal = std::stod(value);
                            res->contents[key] = doubleVal;
                        }
                        catch (std::invalid_argument& ex) {
                            std::cout << "Error: invalid number: " << value << std::endl;
                        }
                    }
                }

                previdx = i;
            }
        }

        // due to no-end-comma, we need to handle the last pair
        std::string pair = str.substr(previdx + 1, endIdx - previdx);
        // we need to find the boundary of key and value
        int colonIdx = -1;
        for (size_t j = 0; j < pair.size(); j++) {
            if (pair[j] == ':') {
                colonIdx = j;
                break;
            }
        }
        if (colonIdx == -1) {
            // error
            std::cout << "Error: no colon found in pair: " << pair << std::endl;
            return res;
        }
        std::string key = pair.substr(1, colonIdx-2);   // key is always without quotes
        std::string value = pair.substr(colonIdx + 1, pair.size() - colonIdx - 2);  // make value without ','
        if (value[0] == '\"') {
            // it's a string
            std::string vv = value.substr(1, value.size() - 2);
            res->contents[key] = vv;
        }
        else if (value[0] == '{') {
            // it's an object
            Object* obj = std::get<Object*>(buildObject(value, 0, value.size() - 1));
            res->contents[key] = obj;
        }
        else if (value[0] == '[') {
            // it's a vector
            if (value[1] == '\"') {
                // std::vector < std::string> vec;
                // TODO: this case seems not possible, need check
                std::cout << "Error: vector of string not supported yet" << std::endl;
            }
            else if (value[1] == '{') {
                // TODO: this case seems not possible, need check
                std::cout << "Error: vector of object not supported yet" << std::endl;
            }
            else {
                // it's a number, but could be int or double
                value = value.substr(1, value.size() - 2);  // remove '[' and ']'
                if (value.find('.') != std::string::npos) {
                    // it's a double
                    std::vector<double> nvec;
                    std::stringstream ss(value);
                    std::string token;
                    for (int i = 0; std::getline(ss, token, ','); i++) {
                        try {
                            double doubleVal = std::stod(token);
                            nvec.push_back(doubleVal);
                        }
                        catch (std::invalid_argument& ex) {
                            std::cout << "Error: invalid number: " << token << std::endl;
                        }
                    }
                    res->contents[key] = nvec;
                }
                else {
                    // it's an int
                    std::vector<int> nvec;
                    std::stringstream ss(value);
                    std::string token;
                    for (int i = 0; std::getline(ss, token, ','); i++) {
                        try {
                            int intVal = std::stoi(token);
                            nvec.push_back(intVal);
                        }
                        catch (std::invalid_argument& ex) {
                            std::cout << "Error: invalid number: " << token << std::endl;
                        }
                    }
                    res->contents[key] = nvec;
                }
            }
        }
        else {
            // it's a number
            try {
                int intVal = std::stoi(value);
                res->contents[key] = intVal;
            }
            catch (std::invalid_argument& ex) {
                try {
                    double doubleVal = std::stod(value);
                    res->contents[key] = doubleVal;
                }
                catch (std::invalid_argument& ex) {
                    std::cout << "Error: invalid number: " << value << std::endl;
                }
            }
        }

        return res;
    }

    Result buildVector(const std::string& str, int startIdx, int endIdx) {
        std::vector<Object*> res;
        // we need to find the boundary of each Object, str is start with '[' and end with ']'
        int previdx = startIdx;
        int lvl = 0;
        for (int i = startIdx; i <= endIdx; i++) {
            if (str[i] == '{') {
                if (lvl == 0) {
                    previdx = i;
                }
                ++lvl;
            }
            else if (str[i] == '}') {
                --lvl;
                if (lvl == 0) {
                    Object* cur = std::get<Object*>(buildObject(str, previdx, i));   // I know it's always an Object
                    res.push_back(cur);
                }
            }
        }
        
        return res;
    }

    Result parse(const std::string& str) {
        std::string res;
        // filter out unnecessary spaces
        filter(str, res);

        // we need to check if it's a vector or a single object
        if (res[0] == '[') {
            return buildVector(res, 0, res.size() - 1);
        } else {
            return buildObject(res, 0, res.size() - 1);
        }
    }

    Result parse(const std::ifstream& in) {
        // we read in to a string, then we do the parse
        std::stringstream buffer;
        buffer << in.rdbuf();
        std::string res = buffer.str();
        // res need to be inserted to make first "s72-v1" => {"s72-v1" : 0}
        res.insert(1, "{");
        res.insert(10, ":0}");
        std::cout << res.substr(0, 20) << std::endl;
        return parse(res);
    }
    
    Result load(const std::string& filename) {
        std::ifstream in(filename, std::ios::binary);
        return parse(in);
    }
    
    void filter(const std::string& str, std::string& res) {
        bool in_string = false;

        for (char ch : str) {
            if (ch == '"' || ch == '\'') {
                in_string = !in_string;
            }

            if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
                if (in_string) {
                    res += ch;
                }
            }
            else {
                res += ch;
            }
        }
    }

    void Object::cleanup() {
        for (auto& pair : contents) {
            if (std::holds_alternative<Object*>(pair.second)) {
                std::get<Object*>(pair.second)->cleanup();
            }
            if (std::holds_alternative<std::vector<Object*>>(pair.second)) {
                for (auto& obj : std::get<std::vector<Object*>>(pair.second)) {
                    obj->cleanup();
                }
            }
        }
        contents.clear();
        delete this;
    }

    void printValue(std::ostream& os, const int& value) {
        os << value;
    }
    void printValue(std::ostream& os, const double& value) {
        os << value;
    }
    void printValue(std::ostream& os, const std::string& value) {
        os << value;
    }
    void printValue(std::ostream& os, const Object* value) {
        os << *value;
    }
    void printValue(std::ostream& os, std::monostate) {
        os << "null";
    }

    template <typename T>
    void printValue(std::ostream& os, const std::vector<T>& value) {
        os << "[";
        for (const auto& element : value) {
            os << element << ",";
        }
        os << "]";
    }

    std::ostream& operator<<(std::ostream& os, const Object& obj) {
        os << "{";
        for (const auto& pair : obj.contents) {
            os << pair.first << ": ";
            std::visit([&os](const auto& value) { printValue(os, value); }, pair.second);
            os << ", ";
        }
        os << "}";
        return os;
    }

} // namespace mcjp
