#include "Texture.hpp"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

Texture::~Texture() {
    //stbi_image_free(m_bytes.get());
}

void Texture::Create(const std::filesystem::path& path) {
    int            width      = 0;
    int            height     = 0;
    int            components = 0;
    unsigned char* bytes      = stbi_load(path.string().c_str(), &width, &height, &components, 0);

    if (bytes == nullptr) {
        assert(false);
    }

    TextureInternalFormat internal_format{};
    TextureDataFormat     data_format{};

    switch (components) {
        case 4:
            internal_format = TextureInternalFormat::RGBA8;
            break;
        case 3:
            internal_format = TextureInternalFormat::RGB8;
            break;
        case 2:
            internal_format = TextureInternalFormat::RG8;
            break;
        case 1:
            internal_format = TextureInternalFormat::R8;
            break;
        default:
            assert(false);
            break;
    }

    switch (components) {
        case 4:
            data_format = TextureDataFormat::RGBA;
            break;
        case 3:
            data_format = TextureDataFormat::RGB;
            break;
        case 2:
            data_format = TextureDataFormat::RG;
            break;
        case 1:
            data_format = TextureDataFormat::RED;
            break;
        default:
            assert(false);
            break;
    }

    m_width          = width;
    m_height         = height;
    m_components     = components;
    m_internalFormat = internal_format;
    m_dataFormat     = data_format;

    size_t buffer_size = width * height * components;
    m_bytes            = std::make_unique<unsigned char[]>(buffer_size);

    // bytes are already checked that it is not nullptr
    std::copy(bytes, bytes + buffer_size, m_bytes.get());
}

void Texture::Create(const tinygltf::Image& image, const tinygltf::Sampler& sampler, TextureColorSpace texture_color_space) {
    int                  width      = image.width;
    int                  height     = image.height;
    int                  components = image.component; // number of color channels
    const unsigned char* bytes      = image.image.data();

    int min_filter = sampler.minFilter;
    int mag_filter = sampler.magFilter;

    int wrap_s = sampler.wrapS;
    int wrap_t = sampler.wrapT;

    TextureInternalFormat internal_format{};
    TextureDataFormat     data_format{};

    if (texture_color_space == TextureColorSpace::SRGB) {
        if (components == 4) {
            internal_format = TextureInternalFormat::SRGB8_ALPHA8;
        }
        else {
            internal_format = TextureInternalFormat::SRGB8;
        }
    }
    else {
        switch (components) {
            case 4:
                internal_format = TextureInternalFormat::RGBA8;
                break;
            case 3:
                internal_format = TextureInternalFormat::RGB8;
                break;
            case 2:
                internal_format = TextureInternalFormat::RG8;
                break;
            case 1:
                internal_format = TextureInternalFormat::R8;
                break;
            default:
                assert(false);
                break;
        }
    }

    switch (components) {
        case 4:
            data_format = TextureDataFormat::RGBA;
            break;
        case 3:
            data_format = TextureDataFormat::RGB;
            break;
        case 2:
            data_format = TextureDataFormat::RG;
            break;
        case 1:
            data_format = TextureDataFormat::RED;
            break;
        default:
            assert(false);
            break;
    }

    switch (min_filter) {
        case TINYGLTF_TEXTURE_FILTER_NEAREST:
            m_minFilter = TextureMinFilter::NEAREST;
            break;
        case TINYGLTF_TEXTURE_FILTER_LINEAR:
            m_minFilter = TextureMinFilter::LINEAR;
            break;
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
            m_minFilter = TextureMinFilter::NEAREST_MIPMAP_NEAREST;
            break;
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
            m_minFilter = TextureMinFilter::LINEAR_MIPMAP_NEAREST;
            break;
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
            m_minFilter = TextureMinFilter::NEAREST_MIPMAP_LINEAR;
            break;
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
            m_minFilter = TextureMinFilter::LINEAR_MIPMAP_LINEAR;
            break;
        default:
            m_minFilter = TextureMinFilter::LINEAR_MIPMAP_LINEAR;
            break;
    }

    switch (mag_filter) {
        case TINYGLTF_TEXTURE_FILTER_NEAREST:
            m_magFilter = TextureMagFilter::NEAREST;
            break;
        case TINYGLTF_TEXTURE_FILTER_LINEAR:
            m_magFilter = TextureMagFilter::LINEAR;
            break;
        default:
            m_magFilter = TextureMagFilter::LINEAR;
            break;
    }

    switch (wrap_s) {
        case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
            m_wrapS = TextureWrap::CLAMP_TO_EDGE;
            break;
        case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
            m_wrapS = TextureWrap::MIRRORED_REPEAT;
            break;
        case TINYGLTF_TEXTURE_WRAP_REPEAT:
            m_wrapS = TextureWrap::REPEAT;
            break;
    }

    switch (wrap_t) {
        case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
            m_wrapT = TextureWrap::CLAMP_TO_EDGE;
            break;
        case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
            m_wrapT = TextureWrap::MIRRORED_REPEAT;
            break;
        case TINYGLTF_TEXTURE_WRAP_REPEAT:
            m_wrapT = TextureWrap::REPEAT;
            break;
    }

    m_width          = width;
    m_height         = height;
    m_components     = components;
    m_internalFormat = internal_format;
    m_dataFormat     = data_format;

    size_t buffer_size = image.width * image.height * image.component;
    m_bytes            = std::make_unique<unsigned char[]>(buffer_size);

    if (!image.image.empty() && buffer_size == image.image.size()) {
        std::copy(image.image.begin(), image.image.end(), m_bytes.get());
    }
    else {
        assert(false);
    }
}
