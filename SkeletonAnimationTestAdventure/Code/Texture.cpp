#include "Texture.hpp"

#define STB_IMAGE_IMPLEMENTATION

Texture::~Texture() {
    glDeleteTextures(1, &m_index);
}

void Texture::Create(const char* image, GLuint slot) {
    int width_img  = 0;
    int height_img = 0;
    int num_col_ch = 0;
    stbi_set_flip_vertically_on_load(0);
    unsigned char* bytes = stbi_load(image, &width_img, &height_img, &num_col_ch, 0);

    glGenTextures(1, &m_index);
    glActiveTexture(GL_TEXTURE0 + slot);
    m_unit = slot;
    glBindTexture(GL_TEXTURE_2D, m_index);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    if (num_col_ch == 4) {
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA,
            width_img,
            height_img,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            bytes);
    }
    else if (num_col_ch == 3) {
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA,
            width_img,
            height_img,
            0,
            GL_RGB,
            GL_UNSIGNED_BYTE,
            bytes);
    }
    else if (num_col_ch == 1) {
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA,
            width_img,
            height_img,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            bytes);
    }
    else {
        throw std::invalid_argument("Automatic Texture type recognition failed");
    }

    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(bytes);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::textureUnit(Shader& shader, const char* uniform) const {
    GLuint tex_uni = glGetUniformLocation(shader.reference(), uniform);
    shader.Bind();
    glUniform1i(tex_uni, m_unit);
}

void Texture::Bind() const {
    glActiveTexture(GL_TEXTURE0 + m_unit);
    glBindTexture(GL_TEXTURE_2D, m_index);
}

void Texture::Unbind() {
    glBindTexture(GL_TEXTURE_2D, 0);
}
