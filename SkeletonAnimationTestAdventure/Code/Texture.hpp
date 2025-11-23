#pragma once
#include <filesystem>

#include "stb_image.h"
#include "tiny_gltf.h"
#include <glad/glad.h>

class Texture {
public:
    Texture() = default;
    ~Texture();

    void Create(const std::filesystem::path& path);
    void Create(const tinygltf::Image& image, const tinygltf::Sampler& sampler);

    void        Bind() const;
    static void Unbind();

private:
    void createOpenGLTexture(int                  width,
                             int                  height,
                             int                  component,
                             const unsigned char* bytes,
                             int                  min_filter,
                             int                  mag_filter,
                             int                  wrap_s,
                             int                  wrap_t);

private:
    GLuint m_index{ 0 };
};
