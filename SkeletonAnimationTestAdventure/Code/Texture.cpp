#include "Texture.hpp"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

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

void Texture::Create(const tinygltf::Image& image, const tinygltf::Sampler& sampler) {
    int                  width     = image.width;
    int                  height    = image.height;
    int                  component = image.component; // number of color channels
    const unsigned char* bytes     = image.image.data();

    int min_filter = sampler.minFilter == -1 ? GL_LINEAR_MIPMAP_LINEAR : sampler.minFilter;
    int mag_filter = sampler.magFilter == -1 ? GL_LINEAR : sampler.magFilter;

    int wrap_s = sampler.wrapS;
    int wrap_t = sampler.wrapT;

    switch (min_filter) {
        case TINYGLTF_TEXTURE_FILTER_NEAREST:
            min_filter = GL_NEAREST;
            break;
        case TINYGLTF_TEXTURE_FILTER_LINEAR:
            min_filter = GL_LINEAR;
            break;
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
            min_filter = GL_NEAREST_MIPMAP_NEAREST;
            break;
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
            min_filter = GL_LINEAR_MIPMAP_NEAREST;
            break;
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
            min_filter = GL_NEAREST_MIPMAP_LINEAR;
            break;
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
            min_filter = GL_LINEAR_MIPMAP_LINEAR;
            break;
    }

    switch (mag_filter) {
        case TINYGLTF_TEXTURE_FILTER_NEAREST:
            mag_filter = GL_NEAREST;
            break;
        case TINYGLTF_TEXTURE_FILTER_LINEAR:
            mag_filter = GL_LINEAR;
            break;
    }

    switch (wrap_s) {
        case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
            wrap_s = GL_CLAMP_TO_EDGE;
            break;
        case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
            wrap_s = GL_MIRRORED_REPEAT;
            break;
        case TINYGLTF_TEXTURE_WRAP_REPEAT:
            wrap_s = GL_REPEAT;
            break;
    }

    switch (wrap_t) {
        case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
            wrap_t = GL_CLAMP_TO_EDGE;
            break;
        case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
            wrap_t = GL_MIRRORED_REPEAT;
            break;
        case TINYGLTF_TEXTURE_WRAP_REPEAT:
            wrap_t = GL_REPEAT;
            break;
    }

    glCreateTextures(GL_TEXTURE_2D, 1, &m_index);
    glBindTexture(GL_TEXTURE_2D, m_index);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);

    switch (component) {
        case 4:
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RGBA,
                width,
                height,
                0,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                bytes);
            break;
        case 3:
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RGBA,
                width,
                height,
                0,
                GL_RGB,
                GL_UNSIGNED_BYTE,
                bytes);
            break;
        case 1:
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RGBA,
                width,
                height,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                bytes);
            break;
        default:
            throw std::invalid_argument("ERROR : Wrong number of colors");
            break;
    }

    glGenerateMipmap(GL_TEXTURE_2D);

    this->Unbind();
}

void Texture::textureUnit(Shader& shader, const char* uniform, GLuint unit) {
    if (m_unit == ~0) m_unit = unit;

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
