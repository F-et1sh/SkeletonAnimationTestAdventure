#pragma once
#include <print>
#include <string>
#include <variant>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

#include "VertexBuffers.hpp"
#include "Texture.hpp"
#include "Material.hpp"

struct Primitive {
    using Vertex = Vertex;
    using Index  = std::variant<uint8_t, uint16_t, uint32_t>;

    std::vector<Vertex> vertices{};
    std::vector<Index>  indices{};

    int material{ -1 };
    int mode{ 4 };

    Primitive()  = default;
    ~Primitive() = default;
};

struct Mesh {
    std::string            name;
    std::vector<Primitive> primitives{};
    std::vector<double>    weights{}; // weights to be applied to the Morph Targets

    Mesh()  = default;
    ~Mesh() = default;
};

struct AnimationChannel {
    int         node{ -1 };
    std::string path; // "rotation", "translation", "scale"
    int         sampler{ -1 };

    AnimationChannel()  = default;
    ~AnimationChannel() = default;
};

struct AnimationSampler {
    std::vector<float>     times{};
    std::vector<glm::vec4> values{}; // rotation as quat, translation/scale as vec3
    std::string            interpolation;

    AnimationSampler() : interpolation("LINEAR") {}
    ~AnimationSampler() = default;
};

struct Animation {
    std::vector<AnimationChannel> channels{};
    std::vector<AnimationSampler> samplers{};

    Animation()  = default;
    ~Animation() = default;
};

struct Node {
    int camera  = -1;
    int skin    = -1;
    int mesh    = -1;
    int light   = -1;
    int emitter = -1;

    std::string      name;
    std::vector<int> children{};

    glm::quat rotation{ 1, 0, 0, 0 };
    glm::vec3 scale{ 1, 1, 1 };
    glm::vec3 translation{ 0, 0, 0 };
    glm::mat4 matrix{ 1.0f };

    std::vector<double> weights{};

    Node() = default;
};

struct Skin {
    std::string      name;
    int              inverse_bind_matrices{ -1 }; // required here but not in the spec
    int              skeleton{ -1 };              // The index of the node used as a skeleton root
    std::vector<int> joints{};                    // Indices of skeleton nodes

    Skin()  = default;
    ~Skin() = default;
};

class Model {
public:
    Model() = default;
    ~Model() { this->Release(); }

    void Release();
    void Initialize(const std::filesystem::path& path);

private:
    void loadNodes(const tinygltf::Model& model);
    void loadSceneRoots(const tinygltf::Model& model);
    void loadSkins(const tinygltf::Model& model);
    void loadMeshes(const tinygltf::Model& model);
    void loadPrimitives(const tinygltf::Model& model, std::vector<Primitive>& this_primitives, const std::vector<tinygltf::Primitive>& primitives);
    void loadVertices(const tinygltf::Model& model, std::vector<Primitive::Vertex>& this_vertices, const tinygltf::Primitive& primitive);
    void loadIndices(const tinygltf::Model& model, std::vector<Primitive::Index>& this_indices, const tinygltf::Primitive& primitive);
    void loadTextures(const tinygltf::Model& model);
    void loadMaterials(const tinygltf::Model& model);
    void loadAnimations(const tinygltf::Model& model);

private:
    template <typename T>
    void readAttribute(const tinygltf::Model&     model,
                       const tinygltf::Primitive& primitive,
                       const std::string&         attribute_name,
                       std::vector<T>&            out) {

        auto it = primitive.attributes.find(attribute_name);
        if (it == primitive.attributes.end()) assert(1);

        int                         accessor_index = it->second;
        const tinygltf::Accessor&   accessor       = model.accessors[accessor_index];
        const tinygltf::BufferView& buffer_view    = model.bufferViews[accessor.bufferView];
        const tinygltf::Buffer&     buffer         = model.buffers[buffer_view.buffer];

        const uint8_t* data_ptr       = buffer.data.data() + buffer_view.byteOffset + accessor.byteOffset;
        int            component_size = tinygltf::GetComponentSizeInBytes(accessor.componentType);
        int            num_components = tinygltf::GetNumComponentsInType(accessor.type);

        size_t element_size = static_cast<size_t>(component_size * num_components);
        size_t stride       = buffer_view.byteStride != 0 ? buffer_view.byteStride : element_size;

        out.reserve(accessor.count);

        if constexpr (std::is_same_v<T, glm::vec2> ||
                      std::is_same_v<T, glm::vec3> ||
                      std::is_same_v<T, glm::vec4>) {
            if (fast_copy<T>(accessor, buffer_view, buffer, out)) return;

            for (size_t i = 0; i < accessor.count; i++) {
                const uint8_t* p = data_ptr + (i * stride);

                if constexpr (std::is_same_v<T, glm::vec2>) {
                    const auto* f = reinterpret_cast<const float*>(p);
                    out[i]        = glm::vec2(f[0], f[1]);
                }
                else if constexpr (std::is_same_v<T, glm::vec3>) {
                    const auto* f = reinterpret_cast<const float*>(p);
                    out[i]        = glm::vec3(f[0], f[1], f[2]);
                }
                else if constexpr (std::is_same_v<T, glm::vec4>) {
                    const auto* f = reinterpret_cast<const float*>(p);
                    out[i]        = glm::vec4(f[0], f[1], f[2], f[3]);
                }
            }

            return;
        }

        if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
            const auto* _buffer = reinterpret_cast<const uint8_t*>(data_ptr);
            for (size_t i = 0; i < accessor.count; i++) out[i] = _buffer[i];
        }
        else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
            const auto* _buffer = reinterpret_cast<const uint16_t*>(data_ptr);
            for (size_t i = 0; i < accessor.count; i++) out[i] = _buffer[i];
        }
        else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
            const auto* _buffer = reinterpret_cast<const uint32_t*>(data_ptr);
            for (size_t i = 0; i < accessor.count; i++) out[i] = _buffer[i];
        }
    }

    template <typename T>
    [[nodiscard]] bool fast_copy(const tinygltf::Accessor& accessor, const tinygltf::BufferView& buffer_view, const tinygltf::Buffer& buffer, std::vector<T>& out) {
        if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT && accessor.ByteStride(buffer_view) == sizeof(T)) {
            const float* src = reinterpret_cast<const float*>(buffer.data.data() + buffer_view.byteOffset + accessor.byteOffset);
            out.resize(accessor.count);
            memcpy(out.data(), src, accessor.count * sizeof(T));

            return true;
        }
        else
            return false;
    }

private:
    std::vector<Node>      m_nodes{};
    std::vector<int>       m_scene_roots{};
    std::vector<Skin>      m_skins{};
    std::vector<Mesh>      m_meshes{};
    std::vector<Texture>   m_textures{};
    std::vector<Material>  m_materials{};
    std::vector<Animation> m_animations{};
};
