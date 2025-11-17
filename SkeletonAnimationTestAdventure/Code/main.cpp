#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "VertexBuffers.hpp"
#include "Shader.hpp"

int main() {
    if (glfwInit() == 0) {
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1920, 1080, "Test", NULL, NULL);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (gladLoadGLLoader((GLADloadproc) glfwGetProcAddress) == 0) {
        return -1;
    }

    glViewport(0, 0, 1920, 1080);

    std::vector<Vertex> vertices{};
    std::vector<GLuint> indices{};

    VBO vbo{};
    vbo.Create(vertices);
    vbo.Bind();

    VAO vao{};
    vao.Create();
    vbo.Bind();

    constexpr GLsizei stride = sizeof(Vertex);

    VAO::LinkAttrib(vbo, 0, 2, GL_FLOAT, stride, (void*) offsetof(Vertex, m_Position));
    VAO::LinkAttrib(vbo, 1, 3, GL_FLOAT, stride, (void*) offsetof(Vertex, m_Normal));
    VAO::LinkAttrib(vbo, 2, 3, GL_FLOAT, stride, (void*) offsetof(Vertex, m_Color));
    VAO::LinkAttrib(vbo, 3, 4, GL_UNSIGNED_INT, stride, (void*) offsetof(Vertex, m_Joints));
    VAO::LinkAttrib(vbo, 4, 4, GL_FLOAT, stride, (void*) offsetof(Vertex, m_Weights));

    EBO ebo;
    ebo.Create(indices);
    ebo.Bind();

    while (glfwWindowShouldClose(window) == 0) {
        glClearColor(0.07F, 0.13F, 0.17F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDrawElements(GL_TRIANGLE_STRIP, indices.size(), GL_UNSIGNED_INT, nullptr);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}
