#include "Camera.hpp"

#include <math.h>

void Camera::UpdateMatrix(float fov_deg, float near_plane, float far_plane) {
    glm::mat4 view       = glm::mat4(1.0F);
    glm::mat4 projection = glm::mat4(1.0F);

    view       = glm::lookAt(m_Position, m_Position + m_Orientation, m_Up);
    projection = glm::perspective(glm::radians(fov_deg), (float) m_Width / m_Height, near_plane, far_plane);

    m_CameraMatrix = projection * view;
}

void Camera::UploadUniform(Shader& shader, const char* uniform) {
    glUniformMatrix4fv(glGetUniformLocation(shader.reference(), uniform), 1, GL_FALSE, glm::value_ptr(m_CameraMatrix));
}

void Camera::Inputs(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        m_Position += m_Speed * m_Orientation;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        m_Position += m_Speed * -glm::normalize(glm::cross(m_Orientation, m_Up));
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        m_Position += m_Speed * -m_Orientation;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        m_Position += m_Speed * glm::normalize(glm::cross(m_Orientation, m_Up));
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        m_Position += m_Speed * m_Up;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        m_Position += m_Speed * -m_Up;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        m_Speed = 0.4F;
    }
    else if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE) {
        m_Speed = 0.1F;
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

        if (m_FirstClick) {
            glfwSetCursorPos(window, (m_Width / 2), (m_Height / 2));
            m_FirstClick = false;
        }

        double mouse_x = 0;
        double mouse_y = 0;
        glfwGetCursorPos(window, &mouse_x, &mouse_y);

        float rot_x = m_Sensitivity * (float) (mouse_y - (m_Height / 2)) / m_Height;
        float rot_y = m_Sensitivity * (float) (mouse_x - (m_Width / 2)) / m_Width;

        glm::vec3 new_orientation = glm::rotate(m_Orientation, glm::radians(-rot_x), glm::normalize(glm::cross(m_Orientation, m_Up)));

        if (abs(glm::angle(new_orientation, m_Up) - glm::radians(90.0F)) <= glm::radians(85.0F)) {
            m_Orientation = new_orientation;
        }

        m_Orientation = glm::rotate(m_Orientation, glm::radians(-rot_y), m_Up);

        glfwSetCursorPos(window, (m_Width / 2), (m_Height / 2));
    }
    else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        m_FirstClick = true;
    }
}
