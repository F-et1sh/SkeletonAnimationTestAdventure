#pragma once
#include <filesystem>

#include "stb_image.h"
#include "tiny_gltf.h"
#include <glad/glad.h>

class Texture {
public:
    enum class TextureColorSpace {
        LINEAR,
        SRGB
    };

    enum class TextureInternalFormat {
        RGBA8,
        RGB8,
        RG8,
        R8,
        SRGB8_ALPHA8,
        SRGB8
    };

    enum class TextureDataFormat {
        RGBA,
        RGB,
        RG,
        RED
    };

    enum class TextureFilter {
        NEAREST,
        LINEAR,
        NEAREST_MIPMAP_NEAREST,
        LINEAR_MIPMAP_NEAREST,
        NEAREST_MIPMAP_LINEAR,
        LINEAR_MIPMAP_LINEAR
    };

    enum class TextureWrap {
        CLAMP_TO_EDGE,
        MIRRORED_REPEAT,
        REPEAT
    };

public:
    Texture() = default;
    ~Texture();

    void Create(const std::filesystem::path& path);
    void Create(const tinygltf::Image& image, const tinygltf::Sampler& sampler, TextureColorSpace texture_color_space);

private:
    unsigned int                   m_width{ 0 };
    unsigned int                   m_height{ 0 };
    unsigned int                   m_components{ 0 };
    std::unique_ptr<unsigned char> m_bytes;

    TextureFilter m_minFilter{ TextureFilter::LINEAR_MIPMAP_LINEAR };
    TextureFilter m_magFilter{ TextureFilter::LINEAR };
    TextureWrap   m_wrapS{ TextureWrap::REPEAT };
    TextureWrap   m_wrapT{ TextureWrap::REPEAT };

    TextureInternalFormat m_internalFormat{};
    TextureDataFormat     m_dataFormat{};
};
