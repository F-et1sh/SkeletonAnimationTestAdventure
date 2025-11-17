#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

#include "Shader.hpp"
#include "VertexBuffers.hpp"
#include "Camera.hpp"

template <typename T>
void readAttribute(const tinygltf::Model&     model,
                   const tinygltf::Primitive& primitive,
                   const std::string&         attribute_name,
                   std::vector<T>&            out) {

    auto it = primitive.attributes.find(attribute_name);
    if (it == primitive.attributes.end()) {
        return;
    }

    int                         accessor_index = it->second;
    const tinygltf::Accessor&   accessor       = model.accessors[accessor_index];
    const tinygltf::BufferView& view           = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer&     buffer         = model.buffers[view.buffer];

    const uint8_t* data_ptr = buffer.data.data() + view.byteOffset + accessor.byteOffset;

    size_t count = accessor.count;
    out.resize(count);

    const float* f = reinterpret_cast<const float*>(data_ptr);

    if constexpr (std::is_same_v<T, glm::vec2>) {
        for (size_t i = 0; i < count; i++) {
            out[i] = glm::vec2(f[(i * 2) + 0], f[(i * 2) + 1]);
        }
    }
    else if constexpr (std::is_same_v<T, glm::vec3>) {
        for (size_t i = 0; i < count; i++) {
            out[i] = glm::vec3(f[(i * 3) + 0], f[(i * 3) + 1], f[(i * 3) + 2]);
        }
    }
    else if constexpr (std::is_same_v<T, glm::vec4>) {
        for (size_t i = 0; i < count; i++) {
            out[i] = glm::vec4(f[(i * 4) + 0], f[(i * 4) + 1], f[(i * 4) + 2], f[(i * 4) + 3]);
        }
    }
}

void readJoints(const tinygltf::Model&     model,
                const tinygltf::Primitive& primitive,
                std::vector<glm::uvec4>&   out) {

    auto it = primitive.attributes.find("JOINTS_0");
    if (it == primitive.attributes.end()) {
        return;
    }

    int                         accessor_index = it->second;
    const tinygltf::Accessor&   accessor       = model.accessors[accessor_index];
    const tinygltf::BufferView& view           = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer&     buffer         = model.buffers[view.buffer];

    const uint8_t* data_ptr =
        buffer.data.data() + view.byteOffset + accessor.byteOffset;

    size_t count = accessor.count;
    out.resize(count);

    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
        const uint8_t* f = data_ptr;

        for (size_t i = 0; i < count; i++) {
            out[i] = glm::uvec4(f[(i * 4) + 0], f[(i * 4) + 1], f[(i * 4) + 2], f[(i * 4) + 3]);
        }
    }
    else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
        const uint16_t* f = reinterpret_cast<const uint16_t*>(data_ptr);

        for (size_t i = 0; i < count; i++) {
            out[i] = glm::uvec4(f[(i * 4) + 0], f[(i * 4) + 1], f[(i * 4) + 2], f[(i * 4) + 3]);
        }
    }
}

void loadModel(std::vector<Vertex>& vertices, std::vector<GLuint>& indices, const std::filesystem::path& path) {
    tinygltf::Model    model;
    tinygltf::TinyGLTF loader;
    std::string        err;
    std::string        warn;
    std::string        filename = path.string();

    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, filename);

    if (!warn.empty()) {
        printf("Warn : %s\n", warn.c_str());
    }

    if (!err.empty()) {
        printf("Err : %s\n", err.c_str());
    }

    if (!ret) {
        printf("Failed to parse glTF : %s\n", filename.c_str());
    }

    for (const auto& node : model.nodes) {
        if (node.mesh >= 0) {
            const tinygltf::Mesh& mesh = model.meshes[node.mesh];

            for (const auto& primitive : mesh.primitives) {

                std::vector<glm::vec3> positions;
                readAttribute(model, primitive, "POSITION", positions);

                int vertex_count = positions.size();
                vertices.resize(vertex_count);

                for (int i = 0; i < vertex_count; i++) {
                    vertices[i].m_Position = positions[i];
                }

                if (primitive.attributes.contains("NORMAL")) {
                    std::vector<glm::vec3> normals;
                    readAttribute(model, primitive, "NORMAL", normals);
                    for (int i = 0; i < vertex_count; i++) {
                        vertices[i].m_Normal = normals[i];
                    }
                }

                if (primitive.attributes.contains("TEXCOORD_0")) {
                    /*std::vector<glm::vec2> tex;
                    readAttribute("TEXCOORD_0", tex);
                    for (int i = 0; i < vertexCount; i++)
                        vertices[i].m_UV = tex[i];*/
                }

                if (primitive.attributes.contains("JOINTS_0")) {
                    std::vector<glm::uvec4> joints;
                    readJoints(model, primitive, joints);
                    for (int i = 0; i < vertex_count; i++) {
                        vertices[i].m_Joints = joints[i];
                    }
                }

                if (primitive.attributes.contains("WEIGHTS_0")) {
                    std::vector<glm::vec4> weights;
                    readAttribute(model, primitive, "WEIGHTS_0", weights);
                    for (int i = 0; i < vertex_count; i++) {
                        vertices[i].m_Weights = weights[i];
                    }
                }

                const tinygltf::Accessor&   index_accessor    = model.accessors[primitive.indices];
                const tinygltf::BufferView& index_buffer_view = model.bufferViews[index_accessor.bufferView];
                const tinygltf::Buffer&     index_buffer      = model.buffers[index_buffer_view.buffer];

                const uint8_t* data_ptr = index_buffer.data.data() + index_buffer_view.byteOffset + index_accessor.byteOffset;

                if (index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                    const uint16_t* buf = reinterpret_cast<const uint16_t*>(data_ptr);
                    for (size_t i = 0; i < index_accessor.count; i++) {
                        indices.emplace_back(buf[i]);
                    }
                }
                else if (index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                    const uint32_t* buf = reinterpret_cast<const uint32_t*>(data_ptr);
                    for (size_t i = 0; i < index_accessor.count; i++) {
                        indices.emplace_back(buf[i]);
                    }
                }
            }
        }
    }
}

int main() {
    if (glfwInit() == 0) {
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1920, 1080, "Test", NULL, NULL);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (gladLoadGLLoader((GLADloadproc) glfwGetProcAddress) == 0) {
        return -1;
    }

    glViewport(0, 0, 1920, 1080);

    Camera camera(1920, 1080, glm::vec3{});

    Shader shader;
    shader.Initialize(L"F:\\Windows\\Desktop\\SkeletonAnimationTestAdventure\\Files\\Shaders\\default");
    shader.Bind();

    std::vector<Vertex> vertices{};
    std::vector<GLuint> indices{};

    loadModel(vertices, indices, L"F:\\Windows\\Desktop\\SkeletonAnimationTestAdventure\\Files\\Models\\test_character\\scene.gltf");

    VBO vbo{};
    vbo.Create(vertices);
    vbo.Bind();

    VAO vao{};
    vao.Create();
    vbo.Bind();

    constexpr GLsizei stride = sizeof(Vertex);

    VAO::LinkAttrib(vbo, 0, 2, GL_FLOAT, stride, (void*) offsetof(Vertex, m_Position));
    VAO::LinkAttrib(vbo, 1, 3, GL_FLOAT, stride, (void*) offsetof(Vertex, m_Normal));
    VAO::LinkAttrib(vbo, 2, 3, GL_FLOAT, stride, (void*) offsetof(Vertex, m_Color));
    VAO::LinkAttrib(vbo, 3, 4, GL_UNSIGNED_INT, stride, (void*) offsetof(Vertex, m_Joints));
    VAO::LinkAttrib(vbo, 4, 4, GL_FLOAT, stride, (void*) offsetof(Vertex, m_Weights));

    EBO ebo{};
    ebo.Create(indices);
    ebo.Bind();

    vertices.clear();
    indices.clear();

    while (glfwWindowShouldClose(window) == 0) {
        glClearColor(0.07F, 0.13F, 0.17F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        camera.Inputs(window);
        camera.UpdateMatrix(70.0f, 0.01f, 1000.0f);
        camera.UploadUniform(shader, "u_CameraMatrix");

        glUniformMatrix4fv(glGetUniformLocation(shader.reference(), "u_Model"), 1, GL_FALSE, glm::value_ptr(glm::mat4{ 1.0f }));

        glDrawElements(GL_TRIANGLE_STRIP, indices.size(), GL_UNSIGNED_INT, nullptr);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}
