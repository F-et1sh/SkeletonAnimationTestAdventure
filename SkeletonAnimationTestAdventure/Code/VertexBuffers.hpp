#pragma once
#include <variant>
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

using Vertices = std::vector<Vertex>;
using Indices  = std::variant<
     std::vector<uint8_t>,
     std::vector<uint16_t>,
     std::vector<uint32_t>>;

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
    void Create(Indices& indices_variant);

    void        Bind() const;
    static void Unbind();

    [[nodiscard]] unsigned int index() const noexcept { return m_index; }

private:
    GLuint m_index;
};
