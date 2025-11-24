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
    bool               good     = false;

    if (path.extension() == ".gltf")
        good = loader.LoadASCIIFromFile(&model, &error, &warning, filename);
    else if (path.extension() == ".glb")
        good = loader.LoadBinaryFromFile(&model, &error, &warning, filename);

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
    this->loadMaterials(model);
    this->loadTextures(model);
    this->loadAnimations(model);
}

void Model::loadNodes(const tinygltf::Model& model) {
    m_nodes.resize(model.nodes.size());
    for (size_t i = 0; i < model.nodes.size(); i++) {
        const tinygltf::Node& node      = model.nodes[i];
        auto&                 this_node = m_nodes[i];

        this_node.camera   = node.camera;
        this_node.name     = node.name;
        this_node.skin     = node.skin;
        this_node.mesh     = node.mesh;
        this_node.light    = node.light;
        this_node.emitter  = node.emitter;
        this_node.children = node.children;
        this->readVector(this_node.rotation, node.rotation);
        this->readVector(this_node.scale, node.scale);
        this->readVector(this_node.translation, node.translation);
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
    m_skins.resize(model.skins.size());
    for (size_t i = 0; i < model.skins.size(); i++) {
        const tinygltf::Skin& skin      = model.skins[i];
        auto&                 this_skin = m_skins[i];

        this_skin.name                  = skin.name;
        this_skin.inverse_bind_matrices = skin.inverseBindMatrices;
        this_skin.skeleton              = skin.skeleton;
        this_skin.joints                = skin.joints;
    }
}

void Model::loadMeshes(const tinygltf::Model& model) {
    m_meshes.resize(model.meshes.size());
    for (size_t i = 0; i < model.meshes.size(); i++) {
        const tinygltf::Mesh& mesh      = model.meshes[i];
        auto&                 this_mesh = m_meshes[i];

        this_mesh.name = mesh.name;
        this->loadPrimitives(model, this_mesh.primitives, mesh.primitives);
        this_mesh.weights = mesh.weights;
    }
}

void Model::loadPrimitives(const tinygltf::Model& model, std::vector<Primitive>& this_primitives, const std::vector<tinygltf::Primitive>& primitives) {
    this_primitives.resize(primitives.size());
    for (size_t i = 0; i < primitives.size(); i++) {
        const tinygltf::Primitive& primitive      = primitives[i];
        auto&                      this_primitive = this_primitives[i];

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
            break;
    }
}

void Model::loadTextures(const tinygltf::Model& model) {
    m_textures.resize(model.textures.size());
    for (size_t i = 0; i < model.textures.size(); i++) {
        const tinygltf::Texture& texture = model.textures[i];
        const tinygltf::Image&   image   = model.images[texture.source];
        tinygltf::Sampler        sampler{};

        if (texture.sampler >= 0)
            sampler = model.samplers[texture.sampler];
        else { // default settings
            sampler.minFilter = TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR;
            sampler.magFilter = TINYGLTF_TEXTURE_FILTER_LINEAR;
            sampler.wrapS     = TINYGLTF_TEXTURE_WRAP_REPEAT;
            sampler.wrapT     = TINYGLTF_TEXTURE_WRAP_REPEAT;
        }

        int                        gltf_texture_index  = static_cast<int>(i);
        Texture::TextureColorSpace texture_color_space = Texture::TextureColorSpace::Linear;

        for (auto& material : m_materials) {
            if (material.pbr_metallic_roughness.base_color_texture.index == gltf_texture_index ||
                material.emissive_texture.index == gltf_texture_index)

                texture_color_space = Texture::TextureColorSpace::SRGB;
        }

        auto& this_texture = m_textures[i];
        this_texture.Create(image, sampler, texture_color_space);
    }
}

void Model::loadMaterials(const tinygltf::Model& model) {
    m_materials.resize(model.materials.size());
    for (size_t i = 0; i < model.materials.size(); i++) {
        const tinygltf::Material& material      = model.materials[i];
        auto&                     this_material = m_materials[i];

        this_material.name = material.name;
        this->readVector(this_material.emissive_factor, material.emissiveFactor);
        if (material.alphaMode == "OPAQUE")
            this_material.alpha_mode = Material::AlphaMode::OPAQUE;
        else if (material.alphaMode == "MASK")
            this_material.alpha_mode = Material::AlphaMode::MASK;
        else if (material.alphaMode == "BLEND")
            this_material.alpha_mode = Material::AlphaMode::BLEND;
        this_material.alpha_cutoff = material.alphaCutoff;
        this_material.double_sided = material.doubleSided;
        this_material.lods         = material.lods;

#define THIS_PBR this_material.pbr_metallic_roughness
#define PBR material.pbrMetallicRoughness

        this->readVector(THIS_PBR.base_color_factor, PBR.baseColorFactor);
        THIS_PBR.base_color_texture.index                 = PBR.baseColorTexture.index;
        THIS_PBR.base_color_texture.texture_coord         = PBR.baseColorTexture.texCoord;
        THIS_PBR.metallic_factor                          = PBR.metallicFactor;
        THIS_PBR.roughness_factor                         = PBR.roughnessFactor;
        THIS_PBR.metallic_roughness_texture.index         = PBR.metallicRoughnessTexture.index;
        THIS_PBR.metallic_roughness_texture.texture_coord = PBR.metallicRoughnessTexture.texCoord;

        this_material.normal_texture.index         = material.normalTexture.index;
        this_material.normal_texture.texture_coord = material.normalTexture.texCoord;
        this_material.normal_texture.scale         = material.normalTexture.scale;

        this_material.occlusion_texture.index         = material.occlusionTexture.index;
        this_material.occlusion_texture.texture_coord = material.occlusionTexture.texCoord;
        this_material.occlusion_texture.strength      = material.occlusionTexture.strength;

        this_material.emissive_texture.index         = material.emissiveTexture.index;
        this_material.emissive_texture.texture_coord = material.emissiveTexture.texCoord;
    }
}

void Model::loadAnimations(const tinygltf::Model& model) {
    m_animations.resize(model.animations.size());
    for (size_t i = 0; i < model.animations.size(); i++) {
        const tinygltf::Animation& animation      = model.animations[i];
        auto&                      this_animation = m_animations[i];

        this_animation.channels.resize(animation.channels.size());

        for (size_t j = 0; j < animation.channels.size(); j++) {
            const tinygltf::AnimationChannel& channel      = animation.channels[j];
            auto&                             this_channel = this_animation.channels[j];

            this_channel.sampler     = channel.sampler;
            this_channel.target_node = channel.target_node;
            this_channel.target_path = channel.target_path;
        }

        this_animation.samplers.resize(animation.samplers.size());

        for (size_t j = 0; j < animation.samplers.size(); j++) {
            const tinygltf::AnimationSampler& sampler      = animation.samplers[j];
            auto&                             this_sampler = this_animation.samplers[j];

            this->readAccessorFloat(model, sampler.input, this_sampler.times);
            this->readAccessorVec4(model, sampler.output, this_sampler.values);

            if (sampler.interpolation == "LINEAR")
                this_sampler.interpolation = AnimationSampler::InterpolationMode::LINEAR;
            else if (sampler.interpolation == "STEP")
                this_sampler.interpolation = AnimationSampler::InterpolationMode::STEP;
            else if (sampler.interpolation == "CUBICSPLINE")
                this_sampler.interpolation = AnimationSampler::InterpolationMode::CUBICSPLINE;
        }
    }
}

void Model::readVector(glm::vec2& dst, const std::vector<double>& src) {
    assert(src.size() == 2);
    dst = glm::vec2(static_cast<float>(src[0]), static_cast<float>(src[1]));
}

void Model::readVector(glm::vec3& dst, const std::vector<double>& src) {
    assert(src.size() == 3);
    dst = glm::vec3(static_cast<float>(src[0]), static_cast<float>(src[1]), static_cast<float>(src[2]));
}

void Model::readVector(glm::vec4& dst, const std::vector<double>& src) {
    assert(src.size() == 4);
    dst = glm::vec4(static_cast<float>(src[0]), static_cast<float>(src[1]), static_cast<float>(src[2]), static_cast<float>(src[3]));
}

void Model::readVector(glm::quat& dst, const std::vector<double>& src) {
    assert(src.size() == 4);
    dst = glm::quat(static_cast<float>(src[3]), static_cast<float>(src[0]), static_cast<float>(src[1]), static_cast<float>(src[2]));
}

float Model::readComponentAsFloat(const uint8_t* data, int component_type, bool normalized) {
    switch (component_type) {
        case TINYGLTF_COMPONENT_TYPE_FLOAT:
            return *reinterpret_cast<const float*>(data);

        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
            uint16_t v = *reinterpret_cast<const uint16_t*>(data);
            return normalized ? (float) v / 65535.0f : (float) v;
        }

        case TINYGLTF_COMPONENT_TYPE_SHORT: {
            int16_t v = *reinterpret_cast<const int16_t*>(data);
            return normalized ? glm::clamp((float) v / 32767.0f, -1.0f, 1.0f) : (float) v;
        }

        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
            uint8_t v = *reinterpret_cast<const uint8_t*>(data);
            return normalized ? (float) v / 255.0f : (float) v;
        }

        case TINYGLTF_COMPONENT_TYPE_BYTE: {
            int8_t v = *reinterpret_cast<const int8_t*>(data);
            return normalized ? glm::clamp((float) v / 127.0f, -1.0f, 1.0f) : (float) v;
        }

        default:
            std::cerr << "WARNING : Unsupported component type : " << component_type << "\n";
            return 0.0f;
            break;
    }
}

void Model::readAccessorVec4(const tinygltf::Model& model, int accessor_index, std::vector<glm::vec4>& out) {
    const auto& accessor    = model.accessors[accessor_index];
    const auto& buffer_view = model.bufferViews[accessor.bufferView];
    const auto& buffer      = model.buffers[buffer_view.buffer];

    const uint8_t* data = buffer.data.data() + buffer_view.byteOffset + accessor.byteOffset;

    int num_components{};

    switch (accessor.type) {
        case TINYGLTF_TYPE_SCALAR:
            num_components = 1;
            break;
        case TINYGLTF_TYPE_VEC2:
            num_components = 2;
            break;
        case TINYGLTF_TYPE_VEC3:
            num_components = 3;
            break;
        case TINYGLTF_TYPE_VEC4:
            num_components = 4;
            break;
        default:
            num_components = 0;
            std::cerr << "ERROR : Unsupported accessor type!\n";
            return;
            break;
    }

    size_t component_size{};

    switch (accessor.componentType) {
        case TINYGLTF_COMPONENT_TYPE_FLOAT:
            component_size = 4;
            break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            component_size = 2;
            break;
        case TINYGLTF_COMPONENT_TYPE_SHORT:
            component_size = 2;
            break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            component_size = 1;
            break;
        case TINYGLTF_COMPONENT_TYPE_BYTE:
            component_size = 1;
            break;
        default:
            component_size = 0;
            std::cerr << "ERROR : Unsupported component size!\n";
            return;
            break;
    }

    size_t stride = buffer_view.byteStride ? buffer_view.byteStride : num_components * component_size;

    out.resize(accessor.count);

    for (size_t i = 0; i < accessor.count; i++) {

        glm::vec4 v(0.0f);

        const uint8_t* element = data + i * stride;

        for (int component = 0; component < num_components; component++) {
            v[component] = readComponentAsFloat(
                element + component * component_size,
                accessor.componentType,
                accessor.normalized);
        }

        out[i] = v;
    }
}

void Model::readAccessorFloat(const tinygltf::Model& model, int accessor_index, std::vector<float>& out) {
    std::vector<glm::vec4> tmp;
    readAccessorVec4(model, accessor_index, tmp);

    out.resize(tmp.size());
    for (size_t i = 0; i < tmp.size(); i++)
        out[i] = tmp[i].x;
}
