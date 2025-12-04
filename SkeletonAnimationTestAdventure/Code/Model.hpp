#pragma once
#include <print>
#include <string>

#include "Texture.hpp"
#include "Material.hpp"
#include "Shader.hpp"
#include "VertexBuffers.hpp"

inline static constexpr size_t JOINTS_COUNT = 128;

enum class RenderMode {
    POINTS,
    LINES,
    LINE_LOOP,
    LINE_STRIP,
    TRIANGLES,
    TRIANGLE_STRIP,
    TRIANGLE_FAN,
};

enum class RenderIndexType {
    UNSIGNED_BYTE,
    UNSIGNED_SHORT,
    UNSIGNED_INT,
};

struct Primitive {
    Vertices vertices;
    Indices  indices;

    int             material{ -1 };
    RenderMode      mode{ RenderMode::TRIANGLES }; // triangles by default
    RenderIndexType index_type{};
    size_t          index_count{};
    size_t          index_offset{};

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
    std::string target_path; // "rotation", "translation", "scale" // TODO : use enum class instead

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

    AnimationSampler()  = default;
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

    glm::quat rotation{ 1, 0, 0, 0 }; // order : xyzw
    glm::vec3 scale{ 1, 1, 1 };
    glm::vec3 translation{ 0, 0, 0 };

    glm::mat4 local_matrix{ 1.0F };
    glm::mat4 global_matrix{ 1.0F };

    std::vector<double> weights;

    Node()  = default;
    ~Node() = default;
};

struct Skin {
    std::string            name;
    std::vector<glm::mat4> inverse_bind_matrices;
    int                    skeleton{ -1 }; // the index of the node used as a skeleton root
    std::vector<int>       joints;         // indices of skeleton nodes

    std::vector<glm::mat4> bone_final_matrices;

    Skin()  = default;
    ~Skin() = default;
};

class Model {
public:
    Model() = default;
    ~Model() { this->Release(); }

    void Release();
    void Initialize(const std::filesystem::path& path);
    void Draw(const Shader& shader, float time);

    inline const std::vector<Mesh>& getMeshes() const noexcept { return m_meshes; }
    inline const std::vector<Texture>& getTextures() const noexcept { return m_textures; }

private:
    void        loadNodes(const tinygltf::Model& model);
    void        loadSceneRoots(const tinygltf::Model& model);
    void        loadSkins(const tinygltf::Model& model);
    void        loadMeshes(const tinygltf::Model& model);
    void        loadPrimitives(const tinygltf::Model& model, std::vector<Primitive>& this_primitives, const std::vector<tinygltf::Primitive>& primitives);
    void        loadVertices(const tinygltf::Model& model, Vertices& this_vertices, Indices& this_indices, const tinygltf::Primitive& primitive);
    static void loadIndices(const tinygltf::Model& model, Primitive& this_primitive, Indices& this_indices, const tinygltf::Primitive& primitive);
    void        loadMaterials(const tinygltf::Model& model);
    void        loadTextures(const tinygltf::Model& model);
    void        loadAnimations(const tinygltf::Model& model);

private:
    void applyAnimationToNodes(int index, float time);
    void updateNodeTransforms();
    void updateNodeRecursive(int index, const glm::mat4& parent);
    void updateSkinMatrices();
    void drawNode(const Node& node, const Shader& shader);
    void drawMesh(const Mesh& mesh, int skin_index, const Shader& shader, const glm::mat4& matrix);
    void drawPrimitive(const Primitive& primitive, const Shader& shader);
    void bindMaterial(const Material& material, const Shader& shader);
    void bindTexture(const Shader& shader, const std::string& uniform, int texture_index, int slot);

private:
    template <typename T>
        requires(std::is_same_v<T, glm::vec2> ||
                 std::is_same_v<T, glm::vec3> ||
                 std::is_same_v<T, glm::vec4> ||
                 std::is_same_v<T, glm::uvec4> ||
                 std::is_same_v<T, glm::u16vec4> ||
                 std::is_same_v<T, glm::mat4>)
    void readAttribute(const tinygltf::Model& model, size_t accessor_index, std::vector<T>& out) {
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
            else if constexpr (std::is_same_v<T, glm::u16vec4>) {
                const auto* ptr = reinterpret_cast<const uint16_t*>(p);
                out[i]          = glm::u16vec4(ptr[0], ptr[1], ptr[2], ptr[3]);
            }
            else if constexpr (std::is_same_v<T, glm::mat4>) {
                glm::mat4 m{};
                memcpy(glm::value_ptr(m), data_ptr + (i * stride), sizeof(glm::mat4));
                out[i] = m;
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
    static void  readAccessorVec4(const tinygltf::Model& model, int accessor_index, std::vector<glm::vec4>& out);
    static void  readAccessorFloat(const tinygltf::Model& model, int accessor_index, std::vector<float>& out);

private:
    std::vector<Node>      m_nodes;
    std::vector<int>       m_sceneRoots;
    std::vector<Skin>      m_skins;
    std::vector<Mesh>      m_meshes;
    std::vector<Material>  m_materials;
    std::vector<Texture>   m_textures;
    std::vector<Animation> m_animations;
};
