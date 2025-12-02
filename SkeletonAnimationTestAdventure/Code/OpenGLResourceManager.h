#pragma once
#include "Model.hpp"

struct OpenGLTexture {
    // ...
};

struct OpenGLPrimitive {
    int material{ -1 };

    RenderMode enum_mode{};
    int mode{ GL_TRIANGLES }; // triangles by default

    VAO vao{};
    VBO vbo{};
    EBO ebo{};

    RenderIndexType enum_index_type{};
    GLenum index_type{};
    
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
    std::vector<OpenGLTexture> m_textures;
    std::vector<OpenGLPrimitive> m_primitives;
};
