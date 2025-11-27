#include "Camera.hpp"

#include <cmath>

void Camera::UpdateMatrix(float fov_deg, float near_plane, float far_plane) {
    auto view       = glm::mat4(1.0F);
    auto projection = glm::mat4(1.0F);

    view       = glm::lookAt(m_position, m_position + m_orientation, m_up);
    projection = glm::perspective(glm::radians(fov_deg), (float) m_width / m_height, near_plane, far_plane);

    m_cameraMatrix = projection * view;
}

void Camera::UploadUniforms(Shader& shader, const char* uniform) {
    shader.setUniformMat4(uniform, m_cameraMatrix);
}

void Camera::Inputs(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        m_position += m_speed * m_orientation;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        m_position += m_speed * -glm::normalize(glm::cross(m_orientation, m_up));
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        m_position += m_speed * -m_orientation;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        m_position += m_speed * glm::normalize(glm::cross(m_orientation, m_up));
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        m_position += m_speed * m_up;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        m_position += m_speed * -m_up;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        m_speed = 0.4F;
    }
    else if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE) {
        m_speed = 0.1F;
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

        if (m_firstClick) {
            glfwSetCursorPos(window, (m_width / 2), (m_height / 2));
            m_firstClick = false;
        }

        double mouse_x = 0;
        double mouse_y = 0;
        glfwGetCursorPos(window, &mouse_x, &mouse_y);

        float rot_x = m_sensitivity * (float) (mouse_y - (m_height / 2)) / m_height;
        float rot_y = m_sensitivity * (float) (mouse_x - (m_width / 2)) / m_width;

        glm::vec3 new_orientation = glm::rotate(m_orientation, glm::radians(-rot_x), glm::normalize(glm::cross(m_orientation, m_up)));

        if (abs(glm::angle(new_orientation, m_up) - glm::radians(90.0F)) <= glm::radians(85.0F)) {
            m_orientation = new_orientation;
        }

        m_orientation = glm::rotate(m_orientation, glm::radians(-rot_y), m_up);

        glfwSetCursorPos(window, (m_width / 2), (m_height / 2));
    }
    else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        m_firstClick = true;
    }
}
