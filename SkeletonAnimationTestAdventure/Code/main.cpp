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

    VBO vbo;
    vbo.Create(vertices);

    VAO vao;
    vao.Create();

    GLsizei stride = sizeof(Vertex);

    vao.LinkAttrib(vbo, 0, 2, GL_FLOAT, stride, (void*) offsetof(Vertex, m_Position));

    while (glfwWindowShouldClose(window) == 0) {
        glClearColor(0.07F, 0.13F, 0.17F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}
