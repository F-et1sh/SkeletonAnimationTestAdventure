#pragma once
#include "Shader.hpp"

#include "tiny_gltf.h"

class Texture {
public:
    Texture() = default;
    ~Texture();

    void Create(const char* image, GLuint slot);
    void Create(const tinygltf::Image& image, const tinygltf::Sampler& sampler);

    void        textureUnit(Shader& shader, const char* uniform) const;
    void        Bind() const;
    static void Unbind();

private:
    GLuint m_index;
    GLuint m_unit;
};
