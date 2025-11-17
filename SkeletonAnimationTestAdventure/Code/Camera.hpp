#pragma once
#include "Shader.hpp"
#include "GLFW/glfw3.h"

class Camera {
public:
    Camera(int width, int height, glm::vec3 position) : m_Position(position), m_Width(width), m_Height(height) {}
    ~Camera() = default;

    void Inputs(GLFWwindow* window);
    void UpdateMatrix(float fov_deg, float near_plane, float far_plane);
    void UploadUniform(Shader& shader, const char* uniform);

private:
    glm::vec3 m_Position{};
    glm::vec3 m_Orientation  = glm::vec3(0.0F, 0.0F, -1.0F);
    glm::vec3 m_Up           = glm::vec3(0.0F, 1.0F, 0.0F);
    glm::mat4 m_CameraMatrix = glm::mat4(1.0F);

    bool m_FirstClick = true;

    int m_Width = 0;
    int m_Height = 0;

    float m_Speed       = 0.1F;
    float m_Sensitivity = 100.0F;
};
