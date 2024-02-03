#include "cglm.hpp"

int main() {
    cglm::Vec3f v0 = { 1.0f, 2.0f, 3.0f };
    cglm::Mat44f rot_z = cglm::rotate({ 0.0f, 1.0f, 0.0f }, cglm::to_radians(90.0f));
    cglm::Vec3f v1 = rot_z * v0;
    std::cout << v1 << std::endl;
    cglm::Mat44f db = rot_z * rot_z;
    cglm::Vec3f v2 = db * v0;
    std::cout << v2 << std::endl;
}