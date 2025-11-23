#pragma once
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

#pragma pack(push, 1)
struct Vertex {
    glm::vec3    position;
    glm::vec3    normal;
    glm::vec2    texture_coord;
    glm::u16vec4 joints;
    glm::vec4    weights;
    glm::vec4    tangent;

    Vertex()  = default;
    ~Vertex() = default;
};
#pragma pack(pop)

class VBO {
public:
    VBO() = default;
    ~VBO();

    void Create(std::vector<Vertex>& vertices);

    void        Bind() const;
    static void Unbind();

private:
    GLuint m_index;
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
    GLuint m_index;
};

class EBO {
public:
    EBO() = default;
    ~EBO();

    void Create(std::vector<GLuint>& indices);

    void        Bind() const;
    static void Unbind();

private:
    GLuint m_index;
};
