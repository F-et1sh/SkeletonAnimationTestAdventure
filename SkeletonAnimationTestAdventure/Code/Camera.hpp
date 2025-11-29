#pragma once
#include "Shader.hpp"
#include "GLFW/glfw3.h"

class Camera {
public:
    Camera(int width, int height, glm::vec3 position) : m_position(position), m_width(width), m_height(height) {}
    ~Camera() = default;

    void Inputs(GLFWwindow* window);
    void UpdateMatrix(float fov_deg, float near_plane, float far_plane);
    void UploadUniforms(Shader& shader, const char* uniform);

    inline const glm::vec3& getPosition() const noexcept { return m_position; }

private:
    glm::vec3 m_position{};
    glm::vec3 m_orientation  = glm::vec3(0.0F, 0.0F, -1.0F);
    glm::vec3 m_up           = glm::vec3(0.0F, 1.0F, 0.0F);
    glm::mat4 m_cameraMatrix = glm::mat4(1.0F);

    bool m_firstClick = true;

    int m_width  = 0;
    int m_height = 0;

    float m_speed       = 0.01F;
    float m_sensitivity = 100.0F;
};
