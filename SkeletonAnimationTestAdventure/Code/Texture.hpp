#pragma once
#include "Shader.hpp"
#include "stb_image.h"

class Texture {
public:
    Texture() = default;
    ~Texture();

    void Create(const char* image, GLuint slot);

    void        textureUnit(Shader& shader, const char* uniform) const;
    void        Bind() const;
    static void Unbind();

private:
    GLuint m_index;
    GLuint m_unit;
};
