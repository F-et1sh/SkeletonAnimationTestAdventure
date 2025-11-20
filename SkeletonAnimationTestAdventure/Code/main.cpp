#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Texture.hpp"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

#include "Shader.hpp"
#include "VertexBuffers.hpp"
#include "Camera.hpp"

struct AnimationChannel {
    int         m_node = 0;
    std::string m_path; // "rotation", "translation", "scale"
    int         m_sampler = 0;

    AnimationChannel()  = default;
    ~AnimationChannel() = default;
};

struct AnimationSampler {
    std::vector<float>     m_times;
    std::vector<glm::vec4> m_values; // rotation as quat, translation/scale as vec3
    std::string            m_interpolation;

    AnimationSampler()  = default;
    ~AnimationSampler() = default;
};

struct Animation {
    std::vector<AnimationChannel> m_channels;
    std::vector<AnimationSampler> m_samplers;

    Animation()  = default;
    ~Animation() = default;
};

struct NodeTRS {
    glm::vec3 m_translation{ 0.0F };
    glm::quat m_rotation{ 1.0F, 0.0F, 0.0F, 0.0F };
    glm::vec3 m_scale{ 1.0F };

    NodeTRS()  = default;
    ~NodeTRS() = default;
};

struct MeshPart {
    int    material_index;
    size_t index_start;
    size_t index_count;

    MeshPart()  = default;
    ~MeshPart() = default;
};

struct MaterialTextures {
    Texture diffuse{};
    Texture specular{};
    Texture normal{};

    MaterialTextures()  = default;
    ~MaterialTextures() = default;
};

void readFloatAccessor(const tinygltf::Model& model, int accessor_index, std::vector<float>& out) {
    const tinygltf::Accessor&   accessor = model.accessors[accessor_index];
    const tinygltf::BufferView& view     = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer&     buffer   = model.buffers[view.buffer];

    const uint8_t* data_ptr = buffer.data.data() + view.byteOffset + accessor.byteOffset;

    out.resize(accessor.count);

    const float* f = reinterpret_cast<const float*>(data_ptr);

    for (size_t i = 0; i < accessor.count; i++) {
        out[i] = f[i];
    }
}

void readVec4Accessor(const tinygltf::Model& model, int accessor_index, std::vector<glm::vec4>& out) {
    const tinygltf::Accessor&   accessor = model.accessors[accessor_index];
    const tinygltf::BufferView& view     = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer&     buffer   = model.buffers[view.buffer];

    const uint8_t* data_ptr = buffer.data.data() + view.byteOffset + accessor.byteOffset;

    size_t stride = accessor.ByteStride(view);
    if (stride == 0) {
        stride = sizeof(float) * 4;
    }

    out.resize(accessor.count);

    for (size_t i = 0; i < accessor.count; i++) {
        const float* f = reinterpret_cast<const float*>(data_ptr + (stride * i));
        out[i]         = glm::vec4(f[0], f[1], f[2], f[3]);
    }
}

void readVec3toVec4Accessor(const tinygltf::Model& model, int accessor_index, std::vector<glm::vec4>& out) {
    const tinygltf::Accessor&   accessor = model.accessors[accessor_index];
    const tinygltf::BufferView& view     = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer&     buffer   = model.buffers[view.buffer];

    const uint8_t* data_ptr = buffer.data.data() + view.byteOffset + accessor.byteOffset;

    size_t stride = accessor.ByteStride(view);
    if (stride == 0) {
        stride = sizeof(float) * 3;
    }

    out.resize(accessor.count);

    for (size_t i = 0; i < accessor.count; i++) {
        const float* f = reinterpret_cast<const float*>(data_ptr + (stride * i));
        out[i]         = glm::vec4(f[0], f[1], f[2], 0.0F);
    }
}

Animation loadAnimation(const tinygltf::Model& model, const tinygltf::Animation& animation) {
    Animation result;
    result.m_samplers.reserve(animation.samplers.size());
    result.m_channels.reserve(animation.channels.size());

    for (const auto& s : animation.samplers) {
        AnimationSampler sampler{};

        sampler.m_interpolation = s.interpolation; // "LINEAR", "STEP", "CUBICSPLINE"

        // read time keys
        readFloatAccessor(model, s.input, sampler.m_times);

        const tinygltf::Accessor& accessor = model.accessors[s.output];

        if (accessor.type == TINYGLTF_TYPE_VEC4) {
            // rotations
            readVec4Accessor(model, s.output, sampler.m_values);
        }
        else if (accessor.type == TINYGLTF_TYPE_VEC3) {
            // translation / scale
            readVec3toVec4Accessor(model, s.output, sampler.m_values);
        }
        else if (accessor.type == TINYGLTF_TYPE_SCALAR) {
            sampler.m_values.clear();
        }
        else {
            std::cerr << "Unsupported animation output type : " << accessor.type << "\n";
        }

        result.m_samplers.emplace_back(std::move(sampler));
    }

    for (const auto& ch : animation.channels) {
        AnimationChannel channel{};
        channel.m_node    = ch.target_node;
        channel.m_path    = ch.target_path; // "translation", "rotation", "scale"
        channel.m_sampler = ch.sampler;

        result.m_channels.emplace_back(channel);
    }

    return result;
}

void applyAnimationToNodes(const Animation&      animation,
                           float                 time,
                           std::vector<NodeTRS>& node_trs) {

    for (const auto& ch : animation.m_channels) {

        const AnimationSampler& s = animation.m_samplers[ch.m_sampler];

        if (s.m_times.empty() || s.m_values.empty()) {
            continue; // skip unsupported sampler
        }

        int k1 = 0;
        while (k1 < s.m_times.size() - 1 && time > s.m_times[k1 + 1]) {
            k1++;
        }
        int k2 = k1 + 1;

        glm::vec4 v1 = s.m_values[k1];
        glm::vec4 v2 = s.m_values[k2];

        float t = (time - s.m_times[k1]) / (s.m_times[k2] - s.m_times[k1]);
        t       = glm::clamp(t, 0.0F, 1.0F);

        NodeTRS& trs = node_trs[ch.m_node];

        if (ch.m_path == "rotation") {
            glm::quat q1(v1.w, v1.x, v1.y, v1.z);
            glm::quat q2(v2.w, v2.x, v2.y, v2.z);
            trs.m_rotation = glm::slerp(q1, q2, t);
        }
        else if (ch.m_path == "translation") {
            trs.m_translation = glm::mix(glm::vec3(v1), glm::vec3(v2), t);
        }
        else if (ch.m_path == "scale") {
            trs.m_scale = glm::mix(glm::vec3(v1), glm::vec3(v2), t);
        }
    }
}

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

    int component_size = tinygltf::GetComponentSizeInBytes(accessor.componentType);
    int num_components = tinygltf::GetNumComponentsInType(accessor.type);

    size_t element_size = component_size * num_components;

    size_t stride = view.byteStride != 0 ? view.byteStride : element_size;

    out.resize(accessor.count);

    for (size_t i = 0; i < accessor.count; i++) {
        const uint8_t* p = data_ptr + (i * stride);

        if constexpr (std::is_same_v<T, glm::vec2>) {
            const float* f = reinterpret_cast<const float*>(p);
            out[i]         = glm::vec2(f[0], f[1]);
        }
        else if constexpr (std::is_same_v<T, glm::vec3>) {
            const float* f = reinterpret_cast<const float*>(p);
            out[i]         = glm::vec3(f[0], f[1], f[2]);
        }
        else if constexpr (std::is_same_v<T, glm::vec4>) {
            const float* f = reinterpret_cast<const float*>(p);
            out[i]         = glm::vec4(f[0], f[1], f[2], f[3]);
        }
    }
}

void readJoints(const tinygltf::Model&     model,
                const tinygltf::Primitive& primitive,
                std::vector<glm::uvec4>&   out) {

    auto it = primitive.attributes.find("JOINTS_0");
    if (it == primitive.attributes.end()) return;

    int                         accessor_index = it->second;
    const tinygltf::Accessor&   accessor       = model.accessors[accessor_index];
    const tinygltf::BufferView& view           = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer&     buffer         = model.buffers[view.buffer];

    const uint8_t* base = buffer.data.data() + view.byteOffset + accessor.byteOffset;

    size_t compSize    = tinygltf::GetComponentSizeInBytes(accessor.componentType);
    size_t numComp     = tinygltf::GetNumComponentsInType(accessor.type);
    size_t elementSize = compSize * numComp;
    size_t stride      = view.byteStride != 0 ? view.byteStride : elementSize;

    out.resize(accessor.count);

    for (size_t i = 0; i < accessor.count; i++) {
        const uint8_t* p = base + i * stride;
        if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
            const uint8_t* v = reinterpret_cast<const uint8_t*>(p);
            out[i]           = glm::u16vec4(v[0], v[1], v[2], v[3]);
        }
        else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
            const uint16_t* v = reinterpret_cast<const uint16_t*>(p);
            out[i]            = glm::u16vec4(v[0], v[1], v[2], v[3]);
        }
        else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
            const uint32_t* v = reinterpret_cast<const uint32_t*>(p);
            out[i]            = glm::u16vec4(v[0], v[1], v[2], v[3]);
        }
        else {
            std::cerr << "Unsupported JOINTS componentType\n";
            out[i] = glm::u16vec4(0);
        }
    }
}

void loadInverseBindMatrices(const tinygltf::Model&  model,
                             std::vector<glm::mat4>& inverse_bind_matrices) {

    const tinygltf::Skin& skin = model.skins[0];

    inverse_bind_matrices.resize(skin.joints.size(), glm::mat4(1.0F));

    if (skin.inverseBindMatrices < 0) {
        return;
    }

    const tinygltf::Accessor&   accessor = model.accessors[skin.inverseBindMatrices];
    const tinygltf::BufferView& view     = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer&     buffer   = model.buffers[view.buffer];

    inverse_bind_matrices.resize(accessor.count);

    const uint8_t* data = buffer.data.data() + view.byteOffset + accessor.byteOffset;

    for (size_t i = 0; i < accessor.count; i++) {
        glm::mat4 m{};
        memcpy(glm::value_ptr(m), data + (i * sizeof(glm::mat4)), sizeof(glm::mat4));
        inverse_bind_matrices[i] = m;
    }
}

glm::mat4 getLocalNodeMatrix(const tinygltf::Node& node) {
    glm::mat4 m{ 1.0F };

    // ----- MATRIX -----
    if (!node.matrix.empty()) {
        glm::mat4 result{ 1.0F };

        for (int i = 0; i < 16; i++) {
            reinterpret_cast<float*>(glm::value_ptr(result))[i] = float(node.matrix[i]);
        }

        return result;
    }

    // ----- TRS -----
    glm::vec3 translate{ 0.0F };
    glm::quat rotate{ 1.0F, 0.0F, 0.0F, 0.0F };
    glm::vec3 scale{ 1.0F };

    if (!node.translation.empty()) {
        translate = glm::vec3(
            float(node.translation[0]),
            float(node.translation[1]),
            float(node.translation[2]));
    }

    if (!node.rotation.empty()) {
        rotate = glm::quat(
            float(node.rotation[3]),  // w
            float(node.rotation[0]),  // x
            float(node.rotation[1]),  // y
            float(node.rotation[2])); // z
    }

    if (!node.scale.empty()) {
        scale = glm::vec3(
            float(node.scale[0]),
            float(node.scale[1]),
            float(node.scale[2]));
    }

    return glm::translate(glm::mat4(1.0F), translate) * glm::mat4_cast(rotate) * glm::scale(glm::mat4(1.0F), scale);
}

void computeGlobalNodeMatrix(const tinygltf::Model&  model,
                             int                     node_index,
                             const glm::mat4&        parent,
                             std::vector<glm::mat4>& node_local_matrices,
                             std::vector<glm::mat4>& node_global_matrices) {

    const tinygltf::Node& node  = model.nodes[node_index];
    glm::mat4             local = node_local_matrices[node_index];
    glm::mat4             world = parent * local;

    node_global_matrices[node_index] = world;

    for (int child : node.children) {
        computeGlobalNodeMatrix(model, child, world, node_local_matrices, node_global_matrices);
    }
}

std::vector<glm::mat4> getBoneFinalMatrices(const tinygltf::Model&        model,
                                            const std::vector<glm::mat4>& node_global_matrices,
                                            const std::vector<glm::mat4>& inverse_bind_matrices) {

    const tinygltf::Skin& skin = model.skins[0];

    std::vector<glm::mat4> bones(skin.joints.size(), glm::mat4{ 1.0F });

    for (int i = 0; i < skin.joints.size(); i++) {
        int node_index = skin.joints[i];
        bones[i]       = node_global_matrices[node_index] * inverse_bind_matrices[i];
    }

    return bones;
}

tinygltf::Model loadModel(std::vector<Vertex>&         vertices,
                          std::vector<GLuint>&         indices,
                          std::vector<glm::mat4>&      bone_final_matrices,
                          Animation&                   animation,
                          std::vector<MeshPart>&       mesh_parts,
                          const std::filesystem::path& path) {

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

                size_t base_vertex = vertices.size();
                vertices.resize(base_vertex + vertex_count);

                for (int i = 0; i < vertex_count; i++) {
                    vertices[base_vertex + i].m_position = positions[i];
                }

                if (primitive.attributes.contains("NORMAL")) {
                    std::vector<glm::vec3> normals;
                    readAttribute(model, primitive, "NORMAL", normals);
                    for (int i = 0; i < vertex_count; i++) {
                        vertices[base_vertex + i].m_normal = normals[i];
                    }
                }

                if (primitive.attributes.contains("TANGENT")) {
                    std::vector<glm::vec4> tangent;
                    readAttribute(model, primitive, "TANGENT", tangent);
                    for (int i = 0; i < vertex_count; i++) {
                        vertices[base_vertex + i].m_tangent = tangent[i];
                    }
                }

                if (primitive.attributes.contains("TEXCOORD_0")) {
                    std::vector<glm::vec2> tex;
                    readAttribute(model, primitive, "TEXCOORD_0", tex);
                    for (int i = 0; i < vertex_count; i++)
                        vertices[base_vertex + i].m_texCoord = tex[i];
                }

                if (primitive.attributes.contains("JOINTS_0")) {
                    std::vector<glm::uvec4> joints;
                    readJoints(model, primitive, joints);
                    for (int i = 0; i < vertex_count; i++) {
                        vertices[base_vertex + i].m_joints = joints[i];
                    }
                }

                if (primitive.attributes.contains("WEIGHTS_0")) {
                    std::vector<glm::vec4> weights;
                    readAttribute(model, primitive, "WEIGHTS_0", weights);
                    for (int i = 0; i < vertex_count; i++) {
                        vertices[base_vertex + i].m_weights = weights[i];
                    }
                }

                MeshPart part{};
                part.material_index = primitive.material;
                part.index_start    = indices.size();

                const tinygltf::Accessor&   index_accessor    = model.accessors[primitive.indices];
                const tinygltf::BufferView& index_buffer_view = model.bufferViews[index_accessor.bufferView];
                const tinygltf::Buffer&     index_buffer      = model.buffers[index_buffer_view.buffer];

                const uint8_t* data_ptr = index_buffer.data.data() + index_buffer_view.byteOffset + index_accessor.byteOffset;

                if (index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                    const uint16_t* buffer = reinterpret_cast<const uint16_t*>(data_ptr);
                    for (size_t i = 0; i < index_accessor.count; i++) {
                        indices.emplace_back(base_vertex + buffer[i]);
                    }
                }
                else if (index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                    const uint32_t* buffer = reinterpret_cast<const uint32_t*>(data_ptr);
                    for (size_t i = 0; i < index_accessor.count; i++) {
                        indices.emplace_back(base_vertex + buffer[i]);
                    }
                }

                part.index_count = indices.size() - part.index_start;
                mesh_parts.push_back(part);
            }
        }
    }

    return model;
}

std::string getTextureUri(const tinygltf::Model& model, int texture_index) {
    if (texture_index < 0 || texture_index >= model.textures.size()) return "";

    const tinygltf::Texture& texture     = model.textures[texture_index];
    int                      image_index = texture.source;

    if (image_index < 0 || image_index >= model.images.size()) return "";

    const tinygltf::Image& image = model.images[image_index];

    if (!image.uri.empty()) return image.uri;

    return "[Embedded Data]";
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
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (gladLoadGLLoader((GLADloadproc) glfwGetProcAddress) == 0) {
        return -1;
    }

    glViewport(0, 0, 1920, 1080);
    glEnable(GL_DEPTH_TEST);

    Camera camera(1920, 1080, glm::vec3{});

    Shader shader;
    shader.Initialize(L"F:\\Windows\\Desktop\\SkeletonAnimationTestAdventure\\Files\\Shaders\\default");
    shader.Bind();

    glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec3 lightPos   = glm::vec3(0.5f, 0.5f, 0.5f);

    glUniform4f(glGetUniformLocation(shader.reference(), "u_LightColor"), lightColor.x, lightColor.y, lightColor.z, lightColor.w);
    glUniform3f(glGetUniformLocation(shader.reference(), "u_LightPosition"), lightPos.x, lightPos.y, lightPos.z);

    std::vector<MeshPart> mesh_parts{};

    std::vector<Vertex> vertices{};
    std::vector<GLuint> indices{};

    std::vector<glm::mat4> bone_final_matrices(128, glm::mat4{ 1.0F });

    Animation animation{};

    glm::mat4 model_matrix{ 1.0F };
    model_matrix *= glm::rotate(glm::radians(-90.0F), glm::vec3(1, 0, 0));

    std::filesystem::path model_path = L"F:\\Windows\\Desktop\\SkeletonAnimationTestAdventure\\Files\\Models\\test_character\\scene.gltf";
    tinygltf::Model       model;
    model = loadModel(vertices, indices, bone_final_matrices, animation, mesh_parts, model_path);

    std::vector<MaterialTextures> material_textures{};

    material_textures.reserve(model.materials.size());

    for (const auto& material : model.materials) {
        material_textures.emplace_back();
        auto& textures = material_textures.back();

        const auto& pbr = material.pbrMetallicRoughness;

        auto base_path = model_path.parent_path();

        if (pbr.baseColorTexture.index > -1) {
            std::string uri = getTextureUri(model, pbr.baseColorTexture.index);
            textures.diffuse.Create((base_path / uri).string().c_str(), 0);
        }

        if (pbr.metallicRoughnessTexture.index > -1) {
            std::string uri = getTextureUri(model, pbr.metallicRoughnessTexture.index);
            textures.specular.Create((base_path / uri).string().c_str(), 1);
        }

        if (material.normalTexture.index > -1) {
            std::string uri = getTextureUri(model, material.normalTexture.index);
            textures.normal.Create((base_path / uri).string().c_str(), 2);
        }

        if (material.occlusionTexture.index > -1) {
            std::string uri = getTextureUri(model, material.occlusionTexture.index);
            //std::cout << "  Type: Occlusion Map, URI : " << uri << std::endl;
        }

        if (material.emissiveTexture.index > -1) {
            std::string uri = getTextureUri(model, material.emissiveTexture.index);
            //std::cout << "  Type: Emissive Map, URI : " << uri << std::endl;
        }
    }

    const tinygltf::Skin& skin = model.skins[0];

    std::vector<glm::mat4> inverse_bind_matrices(skin.joints.size(), glm::mat4{ 1.0F });

    loadInverseBindMatrices(model, inverse_bind_matrices);

    std::vector<glm::mat4> ibm_bone(skin.joints.size());

    for (int bone = 0; bone < skin.joints.size(); bone++) {
        ibm_bone[bone] = inverse_bind_matrices[bone];
    }

    inverse_bind_matrices = ibm_bone;

    std::vector<glm::mat4> node_local_matrices(model.nodes.size(), glm::mat4{ 1.0F });
    std::vector<glm::mat4> node_global_matrices(model.nodes.size(), glm::mat4{ 1.0F });

    for (size_t i = 0; i < model.nodes.size(); i++) {
        node_local_matrices[i] = getLocalNodeMatrix(model.nodes[i]);
    }

    std::vector<NodeTRS> base_trs(model.nodes.size());
    for (size_t i = 0; i < model.nodes.size(); i++) {
        const auto& n = model.nodes[i];

        if (!n.translation.empty()) base_trs[i].m_translation = glm::vec3(n.translation[0], n.translation[1], n.translation[2]);
        if (!n.rotation.empty()) base_trs[i].m_rotation = glm::quat(n.rotation[3], n.rotation[0], n.rotation[1], n.rotation[2]);
        if (!n.scale.empty()) base_trs[i].m_scale = glm::vec3(n.scale[0], n.scale[1], n.scale[2]);
    }

    animation = loadAnimation(model, model.animations[0]); // load the first animation

    std::vector<NodeTRS> node_trs = base_trs;
    applyAnimationToNodes(animation, 0, node_trs); // 0 - time

    for (size_t i = 0; i < model.nodes.size(); i++) {
        const NodeTRS& trs = node_trs[i];
        node_local_matrices[i] =
            glm::translate(glm::mat4(1.0f), trs.m_translation) *
            glm::mat4_cast(trs.m_rotation) *
            glm::scale(glm::mat4(1.0f), trs.m_scale);
    }

    const tinygltf::Scene& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];

    for (int root : scene.nodes) {
        computeGlobalNodeMatrix(model, root, glm::mat4{ 1.0F }, node_local_matrices, node_global_matrices);
    }

    bone_final_matrices = getBoneFinalMatrices(model, node_global_matrices, inverse_bind_matrices);

    VAO vao{};
    vao.Create();
    vao.Bind();

    VBO vbo{};
    vbo.Create(vertices);
    vbo.Bind();

    constexpr GLsizei stride = sizeof(Vertex);

    VAO::LinkAttrib(vbo, 0, 3, GL_FLOAT, stride, (void*) offsetof(Vertex, m_position));
    VAO::LinkAttrib(vbo, 1, 3, GL_FLOAT, stride, (void*) offsetof(Vertex, m_normal));
    VAO::LinkAttrib(vbo, 2, 2, GL_FLOAT, stride, (void*) offsetof(Vertex, m_texCoord));

    vbo.Bind();
    glVertexAttribIPointer(3, 4, GL_UNSIGNED_SHORT, stride, (void*) offsetof(Vertex, m_joints));
    glEnableVertexAttribArray(3);

    VAO::LinkAttrib(vbo, 4, 4, GL_FLOAT, stride, (void*) offsetof(Vertex, m_weights));
    VAO::LinkAttrib(vbo, 5, 4, GL_FLOAT, stride, (void*) offsetof(Vertex, m_tangent));

    EBO ebo{};
    ebo.Create(indices);
    ebo.Bind();

    while (glfwWindowShouldClose(window) == 0) {
        glClearColor(0.07F, 0.13F, 0.17F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        camera.Inputs(window);
        camera.UpdateMatrix(70.0F, 0.01F, 1000.0F);
        camera.UploadUniform(shader, "u_CameraMatrix");

        node_trs = base_trs;

        float time     = glfwGetTime();
        float duration = animation.m_samplers[0].m_times.back();
        time           = fmod(time, duration);

        applyAnimationToNodes(animation, time, node_trs);

        for (size_t i = 0; i < model.nodes.size(); i++) {
            const NodeTRS& trs = node_trs[i];
            node_local_matrices[i] =
                glm::translate(glm::mat4(1.0f), trs.m_translation) *
                glm::mat4_cast(trs.m_rotation) *
                glm::scale(glm::mat4(1.0f), trs.m_scale);
        }

        for (int root : scene.nodes) {
            computeGlobalNodeMatrix(model, root, glm::mat4(1.0f), node_local_matrices, node_global_matrices);
        }

        bone_final_matrices = getBoneFinalMatrices(model, node_global_matrices, inverse_bind_matrices);

        glUniformMatrix4fv(glGetUniformLocation(shader.reference(), "u_Model"), 1, GL_FALSE, glm::value_ptr(model_matrix));

        for (int i = 0; i < bone_final_matrices.size(); i++) {
            std::string uniform = "u_Bones[" + std::to_string(i) + "]";
            glUniformMatrix4fv(
                glGetUniformLocation(shader.reference(), uniform.c_str()),
                1,
                GL_FALSE,
                glm::value_ptr(bone_final_matrices[i]));
        }

        for (auto& part : mesh_parts) {

            const auto& material = model.materials[part.material_index];
            const auto& pbr      = material.pbrMetallicRoughness;

            auto& textures = material_textures[part.material_index];

            textures.diffuse.textureUnit(shader, "u_Diffuse0");
            textures.diffuse.Bind();

            textures.specular.textureUnit(shader, "u_Specular0");
            textures.specular.Bind();

            textures.normal.textureUnit(shader, "u_Normal0");
            textures.normal.Bind();

            glDrawElements(GL_TRIANGLES, part.index_count, GL_UNSIGNED_INT, (void*) (part.index_start * sizeof(GLuint)));
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}
