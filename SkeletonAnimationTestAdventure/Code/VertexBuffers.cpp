#include "VertexBuffers.hpp"

void VBO::Create(std::vector<Vertex>& vertices) {
    glGenBuffers(1, &m_Index);
    glBindBuffer(GL_ARRAY_BUFFER, m_Index);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
}

void VBO::Bind() const {
    glBindBuffer(GL_ARRAY_BUFFER, m_Index);
}

void VBO::Unbind() {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VBO::Delete() {
    glDeleteBuffers(1, &m_Index);
}


void VAO::Create() {
    glGenVertexArrays(1, &m_Index);
}

void VAO::LinkAttrib(VBO& vbo, GLuint layout, GLuint num_components, GLenum type, GLsizeiptr stride, void* offset) {
    vbo.Bind();
    glVertexAttribPointer(layout, num_components, type, GL_FALSE, stride, offset);
    glEnableVertexAttribArray(layout);
    VBO::Unbind();
}

void VAO::Bind() const {
    glBindVertexArray(m_Index);
}

void VAO::Unbind() {
    glBindVertexArray(0);
}

void VAO::Delete() {
    glDeleteVertexArrays(1, &m_Index);
}

void EBO::Create(std::vector<GLuint>& indices) {
    glGenBuffers(1, &m_Index);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Index);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
}

void EBO::Bind() const {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Index);
}

void EBO::Unbind() {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void EBO::Delete() {
    glDeleteBuffers(1, &m_Index);
}
