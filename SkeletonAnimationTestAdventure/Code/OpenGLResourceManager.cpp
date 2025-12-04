#include "OpenGLResourceManager.hpp"

void OpenGLResourceManager::loadModel(const Model& model) {
    const auto& meshes = model.getMeshes();
    for (const auto& mesh : meshes) {
        for (const auto& primitive : mesh.primitives) {
            this->createPrimitive(primitive);
        }
    }

    const auto& textures = model.getTextures();
    for (const auto& texture : textures) {
        this->createTexture(texture);
    }
}

void OpenGLResourceManager::createPrimitive(const Primitive& primitive) {
    auto& new_primitive = m_primitives.emplace_back();

    new_primitive.material = primitive.material;

    new_primitive.enum_mode = primitive.mode;
    switch (new_primitive.enum_mode) {
        case RenderMode::POINTS:
            new_primitive.mode = GL_POINTS;
            break;
        case RenderMode::LINES:
            new_primitive.mode = GL_LINES;
            break;
        case RenderMode::LINE_LOOP:
            new_primitive.mode = GL_LINE_LOOP;
            break;
        case RenderMode::LINE_STRIP:
            new_primitive.mode = GL_LINE_STRIP;
            break;
        case RenderMode::TRIANGLES:
            new_primitive.mode = GL_TRIANGLES;
            break;
        case RenderMode::TRIANGLE_STRIP:
            new_primitive.mode = GL_TRIANGLE_STRIP;
            break;
        case RenderMode::TRIANGLE_FAN:
            new_primitive.mode = GL_TRIANGLE_FAN;
            break;
        default:
            assert(false);
            break;
    }

    this->createBuffers(new_primitive, primitive);

    new_primitive.enum_index_type = primitive.index_type;
    switch (new_primitive.enum_index_type) {
        case RenderIndexType::UNSIGNED_BYTE:
            new_primitive.index_type = GL_UNSIGNED_BYTE;
            break;
        case RenderIndexType::UNSIGNED_SHORT:
            new_primitive.index_type = GL_UNSIGNED_SHORT;
            break;
        case RenderIndexType::UNSIGNED_INT:
            new_primitive.index_type = GL_UNSIGNED_INT;
            break;
        default:
            assert(false);
            break;
    }

    new_primitive.index_count  = primitive.index_count;
    new_primitive.index_offset = primitive.index_offset;
}

void OpenGLResourceManager::createBuffers(OpenGLPrimitive& new_primitive, const Primitive& primitive) {
    new_primitive.vao.Create();
    new_primitive.vao.Bind();

    new_primitive.vbo.Create(primitive.vertices);
    new_primitive.vbo.Bind();

    constexpr GLsizei stride = sizeof(Vertex);

    VAO::LinkAttrib(new_primitive.vbo, 0, 3, GL_FLOAT, stride, (void*) offsetof(Vertex, position));
    VAO::LinkAttrib(new_primitive.vbo, 1, 3, GL_FLOAT, stride, (void*) offsetof(Vertex, normal));
    VAO::LinkAttrib(new_primitive.vbo, 2, 2, GL_FLOAT, stride, (void*) offsetof(Vertex, texture_coord));

    new_primitive.vbo.Bind();
    glVertexAttribIPointer(3, 4, GL_UNSIGNED_SHORT, stride, (void*) offsetof(Vertex, joints));
    glEnableVertexAttribArray(3);

    VAO::LinkAttrib(new_primitive.vbo, 4, 4, GL_FLOAT, stride, (void*) offsetof(Vertex, weights));
    VAO::LinkAttrib(new_primitive.vbo, 5, 4, GL_FLOAT, stride, (void*) offsetof(Vertex, tangent));

    new_primitive.ebo.Create(primitive.indices);
    new_primitive.ebo.Bind();

    VBO::Unbind();
    VAO::Unbind();
    EBO::Unbind();
}

void OpenGLResourceManager::createTexture(const Texture& texture) {
    auto& new_texture = m_textures.emplace_back();

    int min_filter{};
    int mag_filter{};

    int wrap_s{};
    int wrap_t{};

    GLenum internal_format{};
    GLenum data_format{};

    switch (texture.getMinFilter()) { // Texture::TextureMinFilter
        case Texture::TextureMinFilter::NEAREST:
            min_filter = GL_NEAREST;
            break;
        case Texture::TextureMinFilter::LINEAR:
            min_filter = GL_LINEAR;
            break;
        case Texture::TextureMinFilter::NEAREST_MIPMAP_NEAREST:
            min_filter = GL_NEAREST_MIPMAP_NEAREST;
            break;
        case Texture::TextureMinFilter::LINEAR_MIPMAP_NEAREST:
            min_filter = GL_LINEAR_MIPMAP_NEAREST;
            break;
        case Texture::TextureMinFilter::NEAREST_MIPMAP_LINEAR:
            min_filter = GL_NEAREST_MIPMAP_LINEAR;
            break;
        case Texture::TextureMinFilter::LINEAR_MIPMAP_LINEAR:
            min_filter = GL_LINEAR_MIPMAP_LINEAR;
            break;
        default:
            assert(false);
            break;
    }

    switch (texture.getMagFilter()) { // Texture::TextureMagFilter
        case Texture::TextureMagFilter::NEAREST:
            mag_filter = GL_NEAREST;
            break;
        case Texture::TextureMagFilter::LINEAR:
            mag_filter = GL_LINEAR;
            break;
        default:
            assert(false);
            break;
    }

    switch (texture.getWrapS()) { // Texture::TextureWrap
        case Texture::TextureWrap::CLAMP_TO_EDGE:
            wrap_s = GL_CLAMP_TO_EDGE;
            break;
        case Texture::TextureWrap::MIRRORED_REPEAT:
            wrap_s = GL_MIRRORED_REPEAT;
            break;
        case Texture::TextureWrap::REPEAT:
            wrap_s = GL_REPEAT;
            break;
        default:
            assert(false);
            break;
    }

    switch (texture.getWrapT()) { // Texture::TextureWrap
        case Texture::TextureWrap::CLAMP_TO_EDGE:
            wrap_t = GL_CLAMP_TO_EDGE;
            break;
        case Texture::TextureWrap::MIRRORED_REPEAT:
            wrap_t = GL_MIRRORED_REPEAT;
            break;
        case Texture::TextureWrap::REPEAT:
            wrap_t = GL_REPEAT;
            break;
        default:
            assert(false);
            break;
    }

    switch (texture.getInternalFormat()) { // Texture::TextureInternalFormat
        case Texture::TextureInternalFormat::RGBA8:
            internal_format = GL_RGBA8;
            break;
        case Texture::TextureInternalFormat::RGB8:
            internal_format = GL_RGB8;
            break;
        case Texture::TextureInternalFormat::RG8:
            internal_format = GL_RG8;
            break;
        case Texture::TextureInternalFormat::R8:
            internal_format = GL_R8;
            break;
        case Texture::TextureInternalFormat::SRGB8_ALPHA8:
            internal_format = GL_SRGB8_ALPHA8;
            break;
        case Texture::TextureInternalFormat::SRGB8:
            internal_format = GL_SRGB8;
            break;
        default:
            assert(false);
            break;
    }

    switch (texture.getDataFormat()) { // Texture::TextureDataFormat
        case Texture::TextureDataFormat::RGBA:
            data_format = GL_RGBA;
            break;
        case Texture::TextureDataFormat::RGB:
            data_format = GL_RGB;
            break;
        case Texture::TextureDataFormat::RG:
            data_format = GL_RG;
            break;
        case Texture::TextureDataFormat::RED:
            data_format = GL_RED;
            break;
        default:
            assert(false);
            break;
    }

    glCreateTextures(GL_TEXTURE_2D, 1, &new_texture.index);
    glBindTexture(GL_TEXTURE_2D, new_texture.index);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);

    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, texture.getWidth(), texture.getHeight(), 0, data_format, GL_UNSIGNED_BYTE, texture.getBytes());

    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}
