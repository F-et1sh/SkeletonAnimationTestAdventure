#pragma once
#include <print>
#include <string>
#include <variant>

#include "tiny_gltf.h"

#include "Shader.hpp"
#include "VertexBuffers.hpp"
#include "Texture.hpp"
#include "Material.hpp"

struct Primitive {
    using Vertices = std::vector<Vertex>;
    using Indices  = std::variant<
         std::vector<uint8_t>,
         std::vector<uint16_t>,
         std::vector<uint32_t>>;

    Vertices vertices;
    Indices  indices;

    int material{ -1 };
    int mode{ GL_TRIANGLES }; // triangles by default

    VAO vao{};
    VBO vbo{};
    EBO ebo{};

    GLenum index_type{};
    size_t index_count{};
    size_t index_offset{};

    Primitive()  = default;
    ~Primitive() = default;
};

struct Mesh {
    std::string            name;
    std::vector<Primitive> primitives;
    std::vector<double>    weights; // weights to be applied to the Morph Targets

    Mesh()  = default;
    ~Mesh() = default;
};

struct AnimationChannel {
    int         sampler{ -1 };
    int         target_node{ -1 };
    std::string target_path; // "rotation", "translation", "scale"

    AnimationChannel()  = default;
    ~AnimationChannel() = default;
};

struct AnimationSampler {
    enum class InterpolationMode {
        LINEAR,
        STEP,
        CUBICSPLINE
    };

    std::vector<float>     times;
    std::vector<glm::vec4> values; // rotation as quat, translation/scale as vec3
    InterpolationMode      interpolation{ InterpolationMode::LINEAR };

    AnimationSampler() {}
    ~AnimationSampler() = default;
};

struct Animation {
    std::vector<AnimationChannel> channels;
    std::vector<AnimationSampler> samplers;

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
    std::vector<int> children;

    glm::quat rotation{ 1, 0, 0, 0 };
    glm::vec3 scale{ 1, 1, 1 };
    glm::vec3 translation{ 0, 0, 0 };
    glm::mat4 matrix{ 1.0F };

    std::vector<double> weights;

    Node() = default;
};

struct Skin {
    std::string      name;
    int              inverse_bind_matrices{ -1 }; // required here but not in the spec
    int              skeleton{ -1 };              // The index of the node used as a skeleton root
    std::vector<int> joints;                      // Indices of skeleton nodes

    Skin()  = default;
    ~Skin() = default;
};

class Model {
public:
    Model() = default;
    ~Model() { this->Release(); }

    void Release();
    void Initialize(const std::filesystem::path& path);
    void Draw(const Shader& shader, const glm::mat4& view, const glm::mat4& proj);

private:
    void        loadNodes(const tinygltf::Model& model);
    void        loadSceneRoots(const tinygltf::Model& model);
    void        loadSkins(const tinygltf::Model& model);
    void        loadMeshes(const tinygltf::Model& model);
    void        loadPrimitives(const tinygltf::Model& model, std::vector<Primitive>& this_primitives, const std::vector<tinygltf::Primitive>& primitives);
    void        loadVertices(const tinygltf::Model& model, Primitive::Vertices& this_vertices, const tinygltf::Primitive& primitive);
    static void loadIndices(const tinygltf::Model& model, Primitive& this_primitive, Primitive::Indices& this_indices, const tinygltf::Primitive& primitive);
    void        loadMaterials(const tinygltf::Model& model);
    void        loadTextures(const tinygltf::Model& model);
    void        loadAnimations(const tinygltf::Model& model);

    void updateNodeTransforms();
    void updateNodeRecursive(int index, const glm::mat4& parent);
    void updateSkinMatrices(const Shader& shader);
    void drawNode(const Node& node, const Shader& shader);
    void drawMesh(const Mesh& mesh, const Shader& shader, const glm::mat4& matrix);
    void drawPrimitive(const Primitive& primitive, const Shader& shader);
    void bindMaterial(const Material& material, const Shader& shader);
    void bindTexture(const Shader& shader, const std::string& uniform, int texture_index, int slot);

    template <typename T>
    void readAttribute(const tinygltf::Model&     model,
                       const tinygltf::Primitive& primitive,
                       const std::string&         attribute_name,
                       std::vector<T>&            out,
                       bool                       is_indices = false) {

        if constexpr (!(
                          std::is_same_v<T, glm::vec2> ||
                          std::is_same_v<T, glm::vec3> ||
                          std::is_same_v<T, glm::vec4> ||
                          std::is_same_v<T, glm::u8vec4> ||
                          std::is_same_v<T, glm::u16vec4>) ) {
            assert("Unsupported attribute type");
        }

        int accessor_index = -1;

        if (!is_indices) {
            auto it = primitive.attributes.find(attribute_name);
            if (it == primitive.attributes.end()) {
                out.clear();
                return;
            }
            accessor_index = it->second;
        }
        else {
            accessor_index = primitive.indices;
        }

        const tinygltf::Accessor&   accessor    = model.accessors[accessor_index];
        const tinygltf::BufferView& buffer_view = model.bufferViews[accessor.bufferView];
        const tinygltf::Buffer&     buffer      = model.buffers[buffer_view.buffer];

        const uint8_t* data_ptr       = buffer.data.data() + buffer_view.byteOffset + accessor.byteOffset;
        int            component_size = tinygltf::GetComponentSizeInBytes(accessor.componentType);
        int            num_components = tinygltf::GetNumComponentsInType(accessor.type);

        auto   element_size = static_cast<size_t>(component_size * num_components);
        size_t stride       = buffer_view.byteStride != 0 ? buffer_view.byteStride : element_size;

        out.resize(accessor.count);

        if (fastCopy<T>(accessor, buffer_view, buffer, out)) {
            return;
        }

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
            else if constexpr (std::is_same_v<T, glm::u8vec4>) {
                const auto* ptr = p;
                out[i]          = glm::u8vec4(ptr[0], ptr[1], ptr[2], ptr[3]);
            }
            else if constexpr (std::is_same_v<T, glm::u16vec4>) {
                const auto* ptr = reinterpret_cast<const uint16_t*>(p);
                out[i]          = glm::u16vec4(ptr[0], ptr[1], ptr[2], ptr[3]);
            }
        }
    }

    template <typename T>
    [[nodiscard]] bool fastCopy(const tinygltf::Accessor& accessor, const tinygltf::BufferView& buffer_view, const tinygltf::Buffer& buffer, std::vector<T>& out) {
        if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
            size_t element_size = sizeof(T);
            size_t stride       = buffer_view.byteStride ? buffer_view.byteStride : element_size;
            if (stride == element_size) {
                const uint8_t* src = buffer.data.data() + buffer_view.byteOffset + accessor.byteOffset;
                out.resize(accessor.count);
                memcpy(out.data(), src, accessor.count * element_size);
                return true;
            }
        }
        return false;
    }

    static void readVector(glm::vec2& dst, const std::vector<double>& src);
    static void readVector(glm::vec3& dst, const std::vector<double>& src);
    static void readVector(glm::vec4& dst, const std::vector<double>& src);
    static void readVector(glm::quat& dst, const std::vector<double>& src);

    static float readComponentAsFloat(const uint8_t* data, int component_type, bool normalized);
    void         readAccessorVec4(const tinygltf::Model& model, int accessor_index, std::vector<glm::vec4>& out);
    void         readAccessorFloat(const tinygltf::Model& model, int accessor_index, std::vector<float>& out);

    std::vector<Node>      m_nodes;
    std::vector<int>       m_sceneRoots;
    std::vector<Skin>      m_skins;
    std::vector<Mesh>      m_meshes;
    std::vector<Material>  m_materials;
    std::vector<Texture>   m_textures;
    std::vector<Animation> m_animations;
};
