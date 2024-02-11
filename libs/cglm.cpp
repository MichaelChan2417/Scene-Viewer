#include "cglm.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

// g++ -o tectc cglm.cpp -ID:\STUDY\Libs\glm -std=c++17

int main() {
    // randomly generate a Mat44f that can inverse
    cglm::Mat44f mat44 = {
        {2.0f, 1.0f, 3.0f, 4.0f},
        {-1.0f, 0.0f, 2.0f, -3.0f},
        {0.0f, 1.0f, 2.0f, 1.0f},
        {3.0f, -2.0f, 1.0f, 5.0f}
    };
    // generate inverse
    cglm::Mat44f inv = cglm::inverse(mat44);
    std::cout << "CGLM:" << std::endl;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            std::cout << inv(i, j) << " ";
        }
        std::cout << std::endl;
    }
    cglm::Mat44f db = mat44 * inv;
    std::cout << "CGLM:" << std::endl;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            std::cout << db(i, j) << " ";
        }
        std::cout << std::endl;
    }

    // randomly generate a Mat44f
    glm::mat4 mat44_glm = {
        {2.0f, 1.0f, 3.0f, 4.0f},
        {-1.0f, 0.0f, 2.0f, -3.0f},
        {0.0f, 1.0f, 2.0f, 1.0f},
        {3.0f, -2.0f, 1.0f, 5.0f}
    };
    // generate inverse
    glm::mat4 inv_glm = glm::inverse(mat44_glm);
    std::cout << "GLM:" << std::endl;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            std::cout << inv_glm[i][j] << " ";
        }
        std::cout << std::endl;
    }



    // cglm::Vec3f v0 = { 1.0f, 2.0f, 3.0f };
    // cglm::Mat44f rot_y = cglm::rotate({ 0.0f, 1.0f, 0.0f }, cglm::to_radians(90.0f));
    // cglm::Vec3f v1 = rot_y * v0;
    // std::cout << v1 << std::endl;
    // cglm::Mat44f db = rot_y * rot_y;
    // cglm::Vec3f v2 = db * v0;
    // std::cout << v2 << std::endl;

    // simple rotate y for 90 degree
    // glm::vec3 v3 = { 1.0f, 2.0f, 3.0f };
    // glm::mat4 rot_y_glm = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // std::cout << "GLM:" << std::endl;
    // for (int i = 0; i < 4; i++) {
    //     for (int j = 0; j < 4; j++) {
    //         std::cout << rot_y_glm[i][j] << " ";
    //     }
    //     std::cout << std::endl;
    // }

    // // Now print my solution
    // std::cout << "CGLM:" << std::endl;
    // for (int i = 0; i < 4; i++) {
    //     for (int j = 0; j < 4; j++) {
    //         std::cout << rot_y(i, j) << " ";
    //     }
    //     std::cout << std::endl;
    // }

    // // Now test lookAt Matrix
    // cglm::Vec3f camera_pos = { 1.0f, 2.0f, 3.0f };
    // cglm::Vec3f view_point = { 0.0f, 0.0f, 0.0f };
    // cglm::Vec3f up = { 0.0f, 0.0f, 1.0f };
    // cglm::Mat44f view_matrix = cglm::lookAt(camera_pos, view_point, up);
    // std::cout << "CGLM:" << std::endl;
    // for (int i = 0; i < 4; i++) {
    //     for (int j = 0; j < 4; j++) {
    //         std::cout << view_matrix(i, j) << " ";
    //     }
    //     std::cout << std::endl;
    // }

    // // Now test lookAt Matrix
    // glm::vec3 camera_pos_glm = { 1.0f, 2.0f, 3.0f };
    // glm::vec3 view_point_glm = { 0.0f, 0.0f, 0.0f };
    // glm::vec3 up_glm = { 0.0f, 0.0f, 1.0f };
    // glm::mat4 view_matrix_glm = glm::lookAt(camera_pos_glm, view_point_glm, up_glm);
    // std::cout << "GLM:" << std::endl;
    // for (int i = 0; i < 4; i++) {
    //     for (int j = 0; j < 4; j++) {
    //         std::cout << view_matrix_glm[i][j] << " ";
    //     }
    //     std::cout << std::endl;
    // }


    // Now test perspective Matrix
    // cglm::Mat44f proj_matrix = cglm::perspective(cglm::to_radians(45.0f), 800.0f / 600.0f, 0.1f, 10.0f);
    // std::cout << "CGLM:" << std::endl;
    // for (int i = 0; i < 4; i++) {
    //     for (int j = 0; j < 4; j++) {
    //         std::cout << proj_matrix(i, j) << " ";
    //     }
    //     std::cout << std::endl;
    // }

    // // Now test perspective Matrix
    // glm::mat4 proj_matrix_glm = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 10.0f);
    // std::cout << "GLM:" << std::endl;
    // for (int i = 0; i < 4; i++) {
    //     for (int j = 0; j < 4; j++) {
    //         std::cout << proj_matrix_glm[i][j] << " ";
    //     }
    //     std::cout << std::endl;
    // }

    // // generate mat44 and print
    // cglm::Mat44f mat44 = {
    //     {1.0f, 2.0f, 3.0f, 4.0f},
    //     {5.0f, 6.0f, 7.0f, 8.0f},
    //     {9.0f, 10.0f, 11.0f, 12.0f},
    //     {13.0f, 14.0f, 15.0f, 16.0f}
    // };
    // std::cout << "CGLM:" << std::endl;
    // for (int i = 0; i < 4; i++) {
    //     for (int j = 0; j < 4; j++) {
    //         std::cout << mat44(i, j) << " ";
    //     }
    //     std::cout << std::endl;
    // }

    // // generate mat44 and print
    // glm::mat4 mat44_glm = {
    //     {1.0f, 2.0f, 3.0f, 4.0f},
    //     {5.0f, 6.0f, 7.0f, 8.0f},
    //     {9.0f, 10.0f, 11.0f, 12.0f},
    //     {13.0f, 14.0f, 15.0f, 16.0f}
    // };

    // std::cout << "GLM:" << std::endl;
    // for (int i = 0; i < 4; i++) {
    //     for (int j = 0; j < 4; j++) {
    //         std::cout << mat44_glm[i][j] << " ";
    //     }
    //     std::cout << std::endl;
    // }

    // glm::vec4 v = { 1.0f, 1.0f, 1.0f, 1.0f };
    // glm::vec4 v1 = mat44_glm * v;
    // std::cout << "GLM: " << v1.x << " " << v1.y << " " << v1.z << " " << v1.w << std::endl;

    // cglm::Vec4f v_cglm = { 1.0f, 1.0f, 1.0f, 1.0f };
    // cglm::Vec4f v1_cglm = mat44 * v_cglm;
    // std::cout << "CGLM: " << v1_cglm[0] << " " << v1_cglm[1] << " " << v1_cglm[2] << " " << v1_cglm[3] << std::endl;


    // Test translation
    // cglm::Vec3f v = { 1.0f, 2.0f, 3.0f };
    // cglm::Mat44f translation = cglm::translation(v);
    // std::cout << "CGLM:" << std::endl;
    // for (int i = 0; i < 4; i++) {
    //     for (int j = 0; j < 4; j++) {
    //         std::cout << translation(i, j) << " ";
    //     }
    //     std::cout << std::endl;
    // }

    // Test translation
    // glm::vec3 v_glm = { 1.0f, 2.0f, 3.0f };
    // glm::mat4 translation_glm = glm::translate(glm::mat4(1.0f), v_glm);
    // std::cout << "GLM:" << std::endl;
    // for (int i = 0; i < 4; i++) {
    //     for (int j = 0; j < 4; j++) {
    //         std::cout << translation_glm[i][j] << " ";
    //     }
    //     std::cout << std::endl;
    // }


    // Test rotation
    // cglm::Vec4f v = { 0.667428,-0.233539,-0.233539,0.667428 };
    // cglm::Mat44f rot = cglm::rotation(v);
    // std::cout << "CGLM:" << std::endl;
    // for (int i = 0; i < 4; i++) {
    //     for (int j = 0; j < 4; j++) {
    //         std::cout << rot(i, j) << " ";
    //     }
    //     std::cout << std::endl;
    // }

    // // Test rotation
    // glm::vec4 v_glm = { 0.667428,-0.233539,-0.233539,0.667428 };
    // glm::quat rotationQuat = glm::quat(v_glm[3], v_glm[0], v_glm[1], v_glm[2]);
    // glm::mat4 res = glm::mat4_cast(rotationQuat);
    // std::cout << "GLM:" << std::endl;
    // for (int i = 0; i < 4; i++) {
    //     for (int j = 0; j < 4; j++) {
    //         std::cout << res[i][j] << " ";
    //     }
    //     std::cout << std::endl;
    // }
}