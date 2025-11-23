#include "Texture.hpp"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

Texture::~Texture() {
    glDeleteTextures(1, &m_index);
}

void Texture::Create(const std::filesystem::path& path) {
    int            width     = 0;
    int            height    = 0;
    int            component = 0;
    unsigned char* bytes     = stbi_load(path.string().c_str(), &width, &height, &component, 0);

    this->createOpenGLTexture(width, height, component, bytes, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT);

    stbi_image_free(bytes);
}

void Texture::Create(const tinygltf::Image& image, const tinygltf::Sampler& sampler) {
    int                  width     = image.width;
    int                  height    = image.height;
    int                  component = image.component; // number of color channels
    const unsigned char* bytes     = image.image.data();

    int min_filter = sampler.minFilter;
    int mag_filter = sampler.magFilter;

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
        default:
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
        default:
            min_filter = GL_LINEAR;
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

    this->createOpenGLTexture(width, height, component, bytes, min_filter, mag_filter, wrap_s, wrap_t);
}

void Texture::Bind() const {
    assert(m_index != 0);
    glBindTexture(GL_TEXTURE_2D, m_index);
}

void Texture::Unbind() {
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::createOpenGLTexture(int width, int height, int component, const unsigned char* bytes, int min_filter, int mag_filter, int wrap_s, int wrap_t) {
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
                GL_RGB,
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
                GL_RED,
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
