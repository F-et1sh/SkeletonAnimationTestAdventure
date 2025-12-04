#pragma once
#include "Model.hpp"

struct OpenGLTexture {
    GLuint index{ 0 };

    OpenGLTexture() = default;
    ~OpenGLTexture() {
        glDeleteTextures(1, &index);
    }
};

struct OpenGLPrimitive {
    int material{ -1 };

    RenderMode enum_mode{};
    int        mode{ GL_TRIANGLES }; // triangles by default

    VAO vao{};
    VBO vbo{};
    EBO ebo{};

    RenderIndexType enum_index_type{};
    GLenum          index_type{};

    size_t index_count{};
    size_t index_offset{};

    OpenGLPrimitive()  = default;
    ~OpenGLPrimitive() = default;
};

class OpenGLResourceManager {
public:
    OpenGLResourceManager()  = default;
    ~OpenGLResourceManager() = default;

    void loadModel(const Model& model);

private:
    void loadPrimitive(const Primitive& primitive);
    void createBuffers(OpenGLPrimitive& new_primitive, const Primitive& primitive);
    void createTexture(OpenGLTexture& texture, int width, int height, int component, const unsigned char* bytes, int min_filter, int mag_filter, int wrap_s, int wrap_t, GLenum internal_format, GLenum data_format);

private:
    std::vector<OpenGLTexture>   m_textures;
    std::vector<OpenGLPrimitive> m_primitives;
};
