#pragma once
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 m_Position;
    glm::vec3 m_Normal;
    glm::vec3 m_Color;
    glm::vec2 m_TextureUV;
};

class VBO {
public:
    VBO()  = default;
    ~VBO() = default;

    void Delete();
    void Create(std::vector<Vertex>& vertices);

    void        Bind() const;
    static void Unbind();

private:
    GLuint m_Index;
};

class VAO {
public:
    VAO()  = default;
    ~VAO() = default;

    void Delete();
    void Create();

    static void LinkAttrib(VBO& vbo, GLuint layout, GLuint num_components, GLenum type, GLsizeiptr stride, void* offset);

    void        Bind() const;
    static void Unbind();

private:
    GLuint m_Index;
};

class EBO {
public:
    void Delete();
    void Create(std::vector<GLuint>& indices);

    void        Bind() const;
    static void Unbind();

private:
    GLuint m_Index;
};
