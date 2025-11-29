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

    if (!bytes) assert(1);

    GLenum internal_format{};
    GLenum data_format{};

    switch (component) {
        case 4:
            internal_format = GL_RGBA8;
            break;
        case 3:
            internal_format = GL_RGB8;
            break;
        case 2:
            internal_format = GL_RG8;
            break;
        case 1:
            internal_format = GL_R8;
            break;
        default:
            assert(1);
            break;
    }

    switch (component) {
        case 4:
            data_format = GL_RGBA;
            break;
        case 3:
            data_format = GL_RGB;
            break;
        case 2:
            data_format = GL_RG;
            break;
        case 1:
            data_format = GL_RED;
            break;
        default:
            assert(1);
            break;
    }

    this->createOpenGLTexture(width, height, component, bytes, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT, internal_format, data_format);

    stbi_image_free(bytes);
}

void Texture::Create(const tinygltf::Image& image, const tinygltf::Sampler& sampler, TextureColorSpace texture_color_space) {
    int                  width     = image.width;
    int                  height    = image.height;
    int                  component = image.component; // number of color channels
    const unsigned char* bytes     = image.image.data();

    int min_filter = sampler.minFilter;
    int mag_filter = sampler.magFilter;

    int wrap_s = sampler.wrapS;
    int wrap_t = sampler.wrapT;

    GLenum internal_format{};
    GLenum data_format{};

    if (texture_color_space == TextureColorSpace::SRGB) {
        if (component == 4) {
            internal_format = GL_SRGB8_ALPHA8;
        }
        else {
            internal_format = GL_SRGB8;
        }
    }
    else {
        switch (component) {
            case 4:
                internal_format = GL_RGBA8;
                break;
            case 3:
                internal_format = GL_RGB8;
                break;
            case 2:
                internal_format = GL_RG8;
                break;
            case 1:
                internal_format = GL_R8;
                break;
            default:
                assert(1);
                break;
        }
    }

    switch (component) {
        case 4:
            data_format = GL_RGBA;
            break;
        case 3:
            data_format = GL_RGB;
            break;
        case 2:
            data_format = GL_RG;
            break;
        case 1:
            data_format = GL_RED;
            break;
        default:
            assert(1);
            break;
    }

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

    this->createOpenGLTexture(width, height, component, bytes, min_filter, mag_filter, wrap_s, wrap_t, internal_format, data_format);
}

void Texture::Bind() const {
    assert(m_index != 0);
    glBindTexture(GL_TEXTURE_2D, m_index);
}

void Texture::Unbind() {
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::createOpenGLTexture(int width, int height, int component, const unsigned char* bytes, int min_filter, int mag_filter, int wrap_s, int wrap_t, GLenum internal_format, GLenum data_format) {
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
                internal_format,
                width,
                height,
                0,
                data_format,
                GL_UNSIGNED_BYTE,
                bytes);
            break;
        case 3:
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                internal_format,
                width,
                height,
                0,
                data_format,
                GL_UNSIGNED_BYTE,
                bytes);
            break;
        case 1:
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                internal_format,
                width,
                height,
                0,
                data_format,
                GL_UNSIGNED_BYTE,
                bytes);
            break;
        default:
            throw std::invalid_argument("ERROR : Wrong number of colors");
            break;
    }

    glGenerateMipmap(GL_TEXTURE_2D);

    Texture::Unbind();
}
