#include "OpenGLResourceManager.h"

void OpenGLResourceManager::loadModel(const Model& model) {
    const auto& meshes = model.getMeshes();
    for (const auto& mesh : meshes) {
        for (const auto& primitive : mesh.primitives) {
            this->loadPrimitive(primitive);
        }
    }

    const auto& textures = model.getTextures();
    
}

void OpenGLResourceManager::loadPrimitive(const Primitive& primitive) {
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

void OpenGLResourceManager::createTexture(OpenGLTexture& texture, int width, int height, int component, const unsigned char* bytes, int min_filter, int mag_filter, int wrap_s, int wrap_t, GLenum internal_format, GLenum data_format) {
    glCreateTextures(GL_TEXTURE_2D, 1, &texture.index);
    glBindTexture(GL_TEXTURE_2D, texture.index);

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
    glBindTexture(GL_TEXTURE_2D, 0);
}
