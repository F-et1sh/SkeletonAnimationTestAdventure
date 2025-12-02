#include "VertexBuffers.hpp"

VBO::~VBO() {
    glDeleteBuffers(1, &m_index);
}

void VBO::Create(const std::vector<Vertex>& vertices) {
    glGenBuffers(1, &m_index);
    glBindBuffer(GL_ARRAY_BUFFER, m_index);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
}

void VBO::Bind() const {
    glBindBuffer(GL_ARRAY_BUFFER, m_index);
}

void VBO::Unbind() {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VAO::Create() {
    glGenVertexArrays(1, &m_index);
}

void VAO::LinkAttrib(VBO& vbo, GLuint layout, GLuint num_components, GLenum type, GLsizeiptr stride, void* offset) {
    vbo.Bind();
    glVertexAttribPointer(layout, num_components, type, GL_FALSE, stride, offset);
    glEnableVertexAttribArray(layout);
    VBO::Unbind();
}

void VAO::Bind() const {
    glBindVertexArray(m_index);
}

void VAO::Unbind() {
    glBindVertexArray(0);
}

VAO::~VAO() {
    glDeleteVertexArrays(1, &m_index);
}

void EBO::Create(const std::vector<GLuint>& indices) {
    glGenBuffers(1, &m_index);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
}

void EBO::Create(const Indices& indices_variant) {
    glGenBuffers(1, &m_index);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index);

    std::visit([&](const auto& indices) {
        using T = std::ranges::range_value_t<std::decay_t<decltype(indices)>>;
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(T), indices.data(), GL_STATIC_DRAW);
    },
               indices_variant);
}

void EBO::Bind() const {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index);
}

void EBO::Unbind() {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

EBO::~EBO() {
    glDeleteBuffers(1, &m_index);
}
