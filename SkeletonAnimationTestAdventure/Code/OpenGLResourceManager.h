#pragma once
#include "Model.hpp"

class OpenGLResourceManager {
public:
    struct GPUPrimitive {
        int  material{ -1 };
        int  mode{ GL_TRIANGLES }; // triangles by default

        VAO vao{};
        VBO vbo{};
        EBO ebo{};

        GLenum index_type{};
        size_t index_count{};
        size_t index_offset{};

        GPUPrimitive()  = default;
        ~GPUPrimitive() = default;
    };

public:
    OpenGLResourceManager()  = default;
    ~OpenGLResourceManager() = default;

    void loadModel(const Model& model);

private:
};
