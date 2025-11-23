#pragma once
#include "Shader.hpp"

#include "stb_image.h"
#include "tiny_gltf.h"

class Texture {
public:
    Texture() = default;
    ~Texture();

    void Create(const char* image, GLuint slot);
    void Create(const tinygltf::Image& image, const tinygltf::Sampler& sampler);

    void        textureUnit(Shader& shader, const char* uniform, GLuint unit = 0);
    void        Bind() const;
    static void Unbind();

private:
    GLuint m_index = ~0;
    GLuint m_unit  = ~0;
};
