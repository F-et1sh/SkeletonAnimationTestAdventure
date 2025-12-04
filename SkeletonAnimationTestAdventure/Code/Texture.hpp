#pragma once
#include <filesystem>
#include <memory>

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

    enum class TextureMinFilter {
        NEAREST,
        LINEAR,
        NEAREST_MIPMAP_NEAREST,
        LINEAR_MIPMAP_NEAREST,
        NEAREST_MIPMAP_LINEAR,
        LINEAR_MIPMAP_LINEAR
    };

    enum class TextureMagFilter {
        NEAREST,
        LINEAR,
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

    inline unsigned int          getWidth() const noexcept { return m_width; }
    inline unsigned int          getHeight() const noexcept { return m_height; }
    inline unsigned int          getComponents() const noexcept { return m_components; }
    inline const unsigned char*  getBytes() const noexcept { return m_bytes.get(); }
    inline TextureMinFilter      getMinFilter() const noexcept { return m_minFilter; }
    inline TextureMagFilter      getMagFilter() const noexcept { return m_magFilter; }
    inline TextureWrap           getWrapS() const noexcept { return m_wrapS; }
    inline TextureWrap           getWrapT() const noexcept { return m_wrapT; }
    inline TextureInternalFormat getInternalFormat() const noexcept { return m_internalFormat; }
    inline TextureDataFormat     getDataFormat() const noexcept { return m_dataFormat; }

    Texture(const Texture&)            = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&&) noexcept            = default;
    Texture& operator=(Texture&&) noexcept = default;

private:
    unsigned int                     m_width{ 0 };
    unsigned int                     m_height{ 0 };
    unsigned int                     m_components{ 0 };
    std::unique_ptr<unsigned char[]> m_bytes{};

    TextureMinFilter m_minFilter{ TextureMinFilter::LINEAR_MIPMAP_LINEAR };
    TextureMagFilter m_magFilter{ TextureMagFilter::LINEAR };
    TextureWrap      m_wrapS{ TextureWrap::REPEAT };
    TextureWrap      m_wrapT{ TextureWrap::REPEAT };

    TextureInternalFormat m_internalFormat{};
    TextureDataFormat     m_dataFormat{};
};
