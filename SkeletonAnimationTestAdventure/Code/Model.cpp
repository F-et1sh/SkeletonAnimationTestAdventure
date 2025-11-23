#include "Model.hpp"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

void Model::Initialize(const std::filesystem::path& path) {
    tinygltf::Model    model{};
    tinygltf::TinyGLTF loader{};
    std::string        error{};
    std::string        warning{};
    std::string        filename = path.string();

    bool good = loader.LoadBinaryFromFile(&model, &error, &warning, filename);

    if (!warning.empty()) {
        std::println("WARNING : {}", warning);
    }
    if (!error.empty()) {
        std::println("ERROR : {}", error);
        assert(1);
    }
    if (!good) {
        std::println("ERROR : Failed to parse glTF : {}", filename);
        assert(1);
    }

    this->loadNodes(model);
    this->loadSceneRoots(model);
    this->loadSkins(model);
    this->loadMeshes(model);
    this->loadTextures(model);
    this->loadMaterials(model);
    this->loadAnimations(model);
}

void Model::loadNodes(const tinygltf::Model& model) {
    m_nodes.reserve(model.nodes.size());
    for (const tinygltf::Node& node : model.nodes) {
        auto& this_node = this->m_nodes.emplace_back();

        this_node.camera   = node.camera;
        this_node.name     = node.name;
        this_node.skin     = node.skin;
        this_node.mesh     = node.mesh;
        this_node.light    = node.light;
        this_node.emitter  = node.emitter;
        this_node.children = node.children;
        if (node.rotation.size() == 4) {
            this_node.rotation = glm::quat(
                static_cast<float>(node.rotation[3]),
                static_cast<float>(node.rotation[0]),
                static_cast<float>(node.rotation[1]),
                static_cast<float>(node.rotation[2]));
        }
        if (node.scale.size() == 3) {
            this_node.scale = glm::vec3(
                static_cast<float>(node.scale[0]),
                static_cast<float>(node.scale[1]),
                static_cast<float>(node.scale[2]));
        }
        if (node.translation.size() == 3) {
            this_node.translation = glm::vec3(
                static_cast<float>(node.translation[0]),
                static_cast<float>(node.translation[1]),
                static_cast<float>(node.translation[2]));
        }
        if (!node.matrix.empty()) {
            glm::mat4 m(1.0f);
            for (int i = 0; i < 16; i++) {
                m[i / 4][i % 4] = node.matrix[i];
            }
            this_node.matrix = m;
        }
        else {
            this_node.matrix = glm::translate(glm::mat4(1.0F), this_node.translation) * glm::mat4_cast(this_node.rotation) * glm::scale(glm::mat4(1.0F), this_node.scale);
        }
        this_node.weights = node.weights;
    }
}

void Model::loadSceneRoots(const tinygltf::Model& model) {
    int scene_index = model.defaultScene;

    if (scene_index < 0 && !model.scenes.empty()) {
        scene_index = 0;
    }
    if (scene_index >= 0) {
        m_sceneRoots = model.scenes[scene_index].nodes;
    }
}

void Model::loadSkins(const tinygltf::Model& model) {
    m_skins.reserve(model.skins.size());
    for (const tinygltf::Skin& skin : model.skins) {
        auto& this_skin = this->m_skins.emplace_back();

        this_skin.name                  = skin.name;
        this_skin.inverse_bind_matrices = skin.inverseBindMatrices;
        this_skin.skeleton              = skin.skeleton;
        this_skin.joints                = skin.joints;
    }
}

void Model::loadMeshes(const tinygltf::Model& model) {
    m_meshes.reserve(model.meshes.size());
    for (const tinygltf::Mesh& mesh : model.meshes) {
        auto& this_mesh = this->m_meshes.emplace_back();

        this_mesh.name = mesh.name;
        this->loadPrimitives(model, this_mesh.primitives, mesh.primitives);
        this_mesh.weights = mesh.weights;
    }
}

void Model::loadPrimitives(const tinygltf::Model& model, std::vector<Primitive>& this_primitives, const std::vector<tinygltf::Primitive>& primitives) {
    this_primitives.reserve(primitives.size());
    for (const tinygltf::Primitive& primitive : primitives) {
        auto& this_primitive = this_primitives.emplace_back();

        loadVertices(model, this_primitive.vertices, primitive);
        loadIndices(model, this_primitive.indices, primitive);
        this_primitive.material = primitive.material;
        this_primitive.mode     = primitive.mode;
    }
}

void Model::loadVertices(const tinygltf::Model& model, Primitive::Vertices& this_vertices, const tinygltf::Primitive& primitive) {
    std::vector<glm::vec3> positions{};
    this->readAttribute(model, primitive, "POSITION", positions);

    std::vector<glm::vec3> normals{};
    this->readAttribute(model, primitive, "NORMAL", normals);

    std::vector<glm::vec4> tangents{};
    this->readAttribute(model, primitive, "TANGENT", tangents);

    std::vector<glm::vec2> texture_coords{};
    this->readAttribute(model, primitive, "TEXCOORD_0", texture_coords);

    std::vector<glm::uvec4> joints{};
    this->readAttribute(model, primitive, "JOINTS_0", joints);

    std::vector<glm::vec4> weights{};
    this->readAttribute(model, primitive, "WEIGHTS_0", weights);

    size_t count = positions.size();
    this_vertices.resize(count);

    for (size_t i = 0; i < count; i++) {
        auto& v    = this_vertices[i];
        v.position = positions[i];
        if (i < normals.size()) v.normal = normals[i];
        if (i < tangents.size()) v.tangent = tangents[i];
        if (i < texture_coords.size()) v.texture_coord = texture_coords[i];
        if (i < joints.size()) v.joints = joints[i];
        if (i < weights.size()) v.weights = weights[i];
    }
}

void Model::loadIndices(const tinygltf::Model& model, Primitive::Indices& this_indices, const tinygltf::Primitive& primitive) {
    if (primitive.indices < 0) assert(1);

    const tinygltf::Accessor&   accessor   = model.accessors[primitive.indices];
    const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer&     buffer     = model.buffers[bufferView.buffer];
    const uint8_t*              data_ptr   = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;

    switch (accessor.componentType) {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
            this_indices = std::vector<uint8_t>{};
            auto& vec    = std::get<std::vector<uint8_t>>(this_indices);
            vec.resize(accessor.count);
            memcpy(vec.data(), data_ptr, accessor.count * sizeof(uint8_t));
            break;
        }
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
            this_indices = std::vector<uint16_t>{};
            auto& vec    = std::get<std::vector<uint16_t>>(this_indices);
            vec.resize(accessor.count);
            if (bufferView.byteStride == 0 || bufferView.byteStride == sizeof(uint16_t)) { // tightly packed
                memcpy(vec.data(), data_ptr, accessor.count * sizeof(uint16_t));
            }
            else {
                for (size_t i = 0; i < accessor.count; i++) {
                    vec[i] = *reinterpret_cast<const uint16_t*>(data_ptr + i * bufferView.byteStride);
                }
            }
            break;
        }
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
            this_indices = std::vector<uint32_t>{};
            auto& vec    = std::get<std::vector<uint32_t>>(this_indices);
            vec.resize(accessor.count);
            if (bufferView.byteStride == 0 || bufferView.byteStride == sizeof(uint32_t)) { // tightly packed
                memcpy(vec.data(), data_ptr, accessor.count * sizeof(uint16_t));
            }
            else {
                for (size_t i = 0; i < accessor.count; i++) {
                    vec[i] = *reinterpret_cast<const uint32_t*>(data_ptr + i * bufferView.byteStride);
                }
            }
            break;
        }
        default:
            throw std::runtime_error("Unsupported index component type");
    }
}

void Model::loadTextures(const tinygltf::Model& model) {
    m_textures.reserve(model.textures.size());
    for (const tinygltf::Texture& texture : model.textures) {
        const tinygltf::Image& image = model.images[texture.source];
        const tinygltf::Sampler& sampler = model.samplers[texture.sampler];

        auto& this_texture = m_textures.emplace_back();
        this_texture.Create(image, sampler);
    }
}

void Model::loadMaterials(const tinygltf::Model& model) {
    m_materials.reserve(model.materials.size());
    for (const tinygltf::Material& material : model.materials) {
        auto& this_material = m_materials.emplace_back();
        
        /*
        
        std::string m_name;

        glm::f64vec3     m_emissiveFactor{ 0.0, 0.0, 0.0 }; // default [0, 0, 0]
        AlphaMode        m_alphaMode{ AlphaMode::OPAQUE };  // default - OPAQUE
        double           m_alphaCutoff{ 0.5 };              // default 0.5
        bool             m_doubleSided{ false };            // default false
        std::vector<int> m_lods;                             // level of detail materials (MSFT_lod)

        PbrMetallicRoughness m_pbrMetallicRoughness;

        NormalTextureInfo    m_normalTexture;
        OcclusionTextureInfo m_occlusionTexture;
        TextureInfo          m_emissiveTexture;

        */

        //this_material
    }
}

void Model::loadAnimations(const tinygltf::Model& model) {
}
