#pragma once
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3  m_Position;
    glm::vec3  m_Normal;
    glm::vec3  m_Color;
    glm::uvec4 m_Joints;
    glm::vec4  m_Weights;

    Vertex() = default;
    ~Vertex() = default;
};

class VBO {
public:
    VBO() = default;
    ~VBO();

    void Create(std::vector<Vertex>& vertices);

    void        Bind() const;
    static void Unbind();

private:
    GLuint m_Index;
};

class VAO {
public:
    VAO() = default;
    ~VAO();

    void Create();

    static void LinkAttrib(VBO& vbo, GLuint layout, GLuint num_components, GLenum type, GLsizeiptr stride, void* offset);

    void        Bind() const;
    static void Unbind();

private:
    GLuint m_Index;
};

class EBO {
public:
    EBO() = default;
    ~EBO();

    void Create(std::vector<GLuint>& indices);

    void        Bind() const;
    static void Unbind();

private:
    GLuint m_Index;
};
