#include "Model.hpp"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

void Model::Release() {
}

void Model::Initialize(const std::filesystem::path& path) {
    tinygltf::Model    model{};
    tinygltf::TinyGLTF loader{};
    std::string        error{};
    std::string        warning{};
    std::string        filename = path.string();
    bool               good     = false;

    if (path.extension() == ".gltf") {
        good = loader.LoadASCIIFromFile(&model, &error, &warning, filename);
    }
    else if (path.extension() == ".glb") {
        good = loader.LoadBinaryFromFile(&model, &error, &warning, filename);
    }

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

void Model::Draw(const Shader& shader, float time) {
    shader.Bind();

    if (!m_animations.empty()) {
        size_t animation_index = 1;

        float duration = m_animations[animation_index].samplers[0].times.back();

        for (const auto& s : m_animations[animation_index].samplers)
            for (const auto& t : s.times)
                if (duration < t) duration = t;

        time = fmod(time, duration);

        this->applyAnimationToNodes(animation_index, time); // play the first animation
    }

    this->updateNodeTransforms();

    if (!m_animations.empty())
        this->updateSkinMatrices();

    for (int i : m_sceneRoots) {
        drawNode(m_nodes[i], shader);
    }
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
        Model::readVector(this_node.rotation, node.rotation);
        Model::readVector(this_node.scale, node.scale);
        Model::readVector(this_node.translation, node.translation);
        if (!node.matrix.empty()) {
            glm::mat4 m(1.0F);
            for (int i = 0; i < 16; i++) {
                m[i / 4][i % 4] = node.matrix[i];
            }
            this_node.local_matrix = m;
        }
        else {
            this_node.local_matrix = glm::translate(glm::mat4(1.0F), this_node.translation) * glm::mat4_cast(this_node.rotation) * glm::scale(glm::mat4(1.0F), this_node.scale);
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

        this_skin.name = skin.name;

        size_t accessor_index = skin.inverseBindMatrices;
        this->readAttribute(model, accessor_index, this_skin.inverse_bind_matrices);

        /*std::vector<glm::mat4> ibm_bone(skin.joints.size());

        for (int bone = 0; bone < skin.joints.size(); bone++) {
            ibm_bone[bone] = this_skin.inverse_bind_matrices[bone];
        }

        this_skin.inverse_bind_matrices = ibm_bone;*/

        this_skin.skeleton = skin.skeleton;
        this_skin.joints   = skin.joints;
        this_skin.bone_final_matrices.resize(JOINTS_COUNT);
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
        loadIndices(model, this_primitive, this_primitive.indices, primitive);
        this_primitive.material = primitive.material;
        this_primitive.mode     = primitive.mode;

        this_primitive.vao.Create();
        this_primitive.vao.Bind();

        this_primitive.vbo.Create(this_primitive.vertices);
        this_primitive.vbo.Bind();

        constexpr GLsizei stride = sizeof(Vertex);

        VAO::LinkAttrib(this_primitive.vbo, 0, 3, GL_FLOAT, stride, (void*) offsetof(Vertex, position));
        VAO::LinkAttrib(this_primitive.vbo, 1, 3, GL_FLOAT, stride, (void*) offsetof(Vertex, normal));
        VAO::LinkAttrib(this_primitive.vbo, 2, 2, GL_FLOAT, stride, (void*) offsetof(Vertex, texture_coord));

        this_primitive.vbo.Bind();
        glVertexAttribIPointer(3, 4, GL_UNSIGNED_SHORT, stride, (void*) offsetof(Vertex, joints));
        glEnableVertexAttribArray(3);

        VAO::LinkAttrib(this_primitive.vbo, 4, 4, GL_FLOAT, stride, (void*) offsetof(Vertex, weights));
        VAO::LinkAttrib(this_primitive.vbo, 5, 4, GL_FLOAT, stride, (void*) offsetof(Vertex, tangent));

        this_primitive.ebo.Create(this_primitive.indices);
        this_primitive.ebo.Bind();

        VBO::Unbind();
        VAO::Unbind();
        EBO::Unbind();
    }
}

void Model::loadVertices(const tinygltf::Model& model, Vertices& this_vertices, const tinygltf::Primitive& primitive) {

    auto read_attribute = [&](const std::string& attribute_name, auto& data) {
        auto it = primitive.attributes.find(attribute_name);
        if (it == primitive.attributes.end()) {
            data.clear();
            return;
        }
        this->readAttribute(model, it->second, data);
    };

    std::vector<glm::vec3> positions{};
    read_attribute("POSITION", positions);

    std::vector<glm::vec3> normals{};
    read_attribute("NORMAL", normals);

    std::vector<glm::vec4> tangents{};
    read_attribute("TANGENT", tangents);

    std::vector<glm::vec2> texture_coords{};
    read_attribute("TEXCOORD_0", texture_coords);

    std::vector<glm::u16vec4> joints{};
    read_attribute("JOINTS_0", joints);

    std::vector<glm::uvec4> weights{};
    read_attribute("WEIGHTS_0", weights);

    size_t count = positions.size();
    this_vertices.resize(count);

    for (size_t i = 0; i < count; i++) {
        auto& v    = this_vertices[i];
        v.position = positions[i];
        if (i < normals.size()) {
            v.normal = normals[i];
        }
        if (i < tangents.size()) {
            v.tangent = tangents[i];
        }
        if (i < texture_coords.size()) {
            v.texture_coord = texture_coords[i];
        }
        if (i < joints.size()) {
            v.joints = joints[i];
        }
        if (i < weights.size()) {
            v.weights = weights[i];
        }
    }
}

void Model::loadIndices(const tinygltf::Model& model, Primitive& this_primitive, Indices& this_indices, const tinygltf::Primitive& primitive) {
    if (primitive.indices < 0) {
        throw std::runtime_error("Primitive has no indices");
    }

    const tinygltf::Accessor&   accessor    = model.accessors[primitive.indices];
    const tinygltf::BufferView& buffer_view = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer&     buffer      = model.buffers[buffer_view.buffer];
    const uint8_t*              data_ptr    = buffer.data.data() + buffer_view.byteOffset + accessor.byteOffset;

    switch (accessor.componentType) {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
            this_indices = std::vector<uint8_t>{};
            auto& vec    = std::get<std::vector<uint8_t>>(this_indices);
            vec.resize(accessor.count);
            memcpy(vec.data(), data_ptr, accessor.count * sizeof(uint8_t));

            this_primitive.index_type   = GL_UNSIGNED_BYTE;
            this_primitive.index_count  = accessor.count;
            this_primitive.index_offset = 0;

            break;
        }
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
            this_indices = std::vector<uint16_t>{};
            auto& vec    = std::get<std::vector<uint16_t>>(this_indices);
            vec.resize(accessor.count);
            if (buffer_view.byteStride == 0 || buffer_view.byteStride == sizeof(uint16_t)) { // tightly packed
                memcpy(vec.data(), data_ptr, accessor.count * sizeof(uint16_t));
            }
            else {
                for (size_t i = 0; i < accessor.count; i++) {
                    vec[i] = *reinterpret_cast<const uint16_t*>(data_ptr + (i * buffer_view.byteStride));
                }
            }

            this_primitive.index_type   = GL_UNSIGNED_SHORT;
            this_primitive.index_count  = accessor.count;
            this_primitive.index_offset = 0;

            break;
        }
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
            this_indices = std::vector<uint32_t>{};
            auto& vec    = std::get<std::vector<uint32_t>>(this_indices);
            vec.resize(accessor.count);
            if (buffer_view.byteStride == 0 || buffer_view.byteStride == sizeof(uint32_t)) { // tightly packed
                memcpy(vec.data(), data_ptr, accessor.count * sizeof(uint32_t));
            }
            else {
                for (size_t i = 0; i < accessor.count; i++) {
                    vec[i] = *reinterpret_cast<const uint32_t*>(data_ptr + (i * buffer_view.byteStride));
                }
            }

            this_primitive.index_type   = GL_UNSIGNED_INT;
            this_primitive.index_count  = accessor.count;
            this_primitive.index_offset = 0;

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

        if (texture.sampler >= 0) {
            sampler = model.samplers[texture.sampler];
        }
        else { // default settings
            sampler.minFilter = TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR;
            sampler.magFilter = TINYGLTF_TEXTURE_FILTER_LINEAR;
            sampler.wrapS     = TINYGLTF_TEXTURE_WRAP_REPEAT;
            sampler.wrapT     = TINYGLTF_TEXTURE_WRAP_REPEAT;
        }

        int                        gltf_texture_index  = static_cast<int>(i);
        Texture::TextureColorSpace texture_color_space = Texture::TextureColorSpace::LINEAR;

        for (auto& material : m_materials) {
            if (material.pbr_metallic_roughness.base_color_texture.index == gltf_texture_index ||
                material.emissive_texture.index == gltf_texture_index) {

                texture_color_space = Texture::TextureColorSpace::SRGB;
            }
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
        Model::readVector(this_material.emissive_factor, material.emissiveFactor);
        if (material.alphaMode == "OPAQUE") {
            this_material.alpha_mode = Material::AlphaMode::OPAQUE;
        }
        else if (material.alphaMode == "MASK") {
            this_material.alpha_mode = Material::AlphaMode::MASK;
        }
        else if (material.alphaMode == "BLEND") {
            this_material.alpha_mode = Material::AlphaMode::BLEND;
        }
        this_material.alpha_cutoff = material.alphaCutoff;
        this_material.double_sided = material.doubleSided;
        this_material.lods         = material.lods;

#define THIS_PBR this_material.pbr_metallic_roughness
#define PBR material.pbrMetallicRoughness

        Model::readVector(THIS_PBR.base_color_factor, PBR.baseColorFactor);
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
            Model::readAccessorVec4(model, sampler.output, this_sampler.values);

            if (sampler.interpolation == "LINEAR") {
                this_sampler.interpolation = AnimationSampler::InterpolationMode::LINEAR;
            }
            else if (sampler.interpolation == "STEP") {
                this_sampler.interpolation = AnimationSampler::InterpolationMode::STEP;
            }
            else if (sampler.interpolation == "CUBICSPLINE") {
                this_sampler.interpolation = AnimationSampler::InterpolationMode::CUBICSPLINE;
            }
        }
    }
}

void Model::applyAnimationToNodes(int index, float time) {
    const Animation& animation = m_animations[index];

    for (const AnimationChannel& channel : animation.channels) {
        const AnimationSampler& sampler = animation.samplers[channel.sampler];

        if (sampler.times.size() < 2 || sampler.values.size() < 2)
            continue;

        int k1 = 0;
        while (k1 < sampler.times.size() - 1 && time > sampler.times[k1 + 1]) {
            k1++;
        }

        int k2 = std::min<int>(k1 + 1, sampler.times.size() - 1);

        glm::vec4 v1 = sampler.values[k1];
        glm::vec4 v2 = sampler.values[k2];

        float t = (time - sampler.times[k1]) / (sampler.times[k2] - sampler.times[k1]);
        t       = glm::clamp(t, 0.0f, 1.0f);

        Node& node = m_nodes[channel.target_node];

        if (channel.target_path == "rotation") {
            glm::quat q1(v1.w, v1.x, v1.y, v1.z);
            glm::quat q2(v2.w, v2.x, v2.y, v2.z);
            node.rotation = glm::normalize(glm::slerp(q1, q2, t));
        }
        else if (channel.target_path == "translation") {
            node.translation = glm::mix(glm::vec3(v1), glm::vec3(v2), t);
        }
        else if (channel.target_path == "scale") {
            node.scale = glm::mix(glm::vec3(v1), glm::vec3(v2), t);
        }
    }
}

void Model::updateNodeTransforms() {
    for (int root : m_sceneRoots) {
        this->updateNodeRecursive(root, glm::mat4(1.0F));
    }
}

void Model::updateNodeRecursive(int index, const glm::mat4& parent) {
    Node& node = m_nodes[index];

    node.local_matrix =
        glm::translate(glm::mat4(1.0f), node.translation) *
        glm::mat4_cast(node.rotation) *
        glm::scale(glm::mat4(1.0f), node.scale);

    node.global_matrix = parent * node.local_matrix;

    for (int child : node.children) {
        this->updateNodeRecursive(child, node.global_matrix);
    }
}

void Model::updateSkinMatrices() {
    for (Skin& skin : m_skins) {
        for (size_t i = 0; i < skin.joints.size(); i++) {
            int         joint_node_index = skin.joints[i];
            const Node& joint_node       = m_nodes[joint_node_index];

            skin.bone_final_matrices[i] = joint_node.global_matrix * skin.inverse_bind_matrices[i];
        }
    }
}

void Model::drawNode(const Node& node, const Shader& shader) {
    if (node.mesh >= 0) {
        this->drawMesh(m_meshes[node.mesh], node.skin, shader, node.global_matrix);
    }

    for (int child : node.children) {
        this->drawNode(m_nodes[child], shader);
    }
}

void Model::drawMesh(const Mesh& mesh, int skin_index, const Shader& shader, const glm::mat4& matrix) {
    if (skin_index >= 0) {
        const Skin& skin = m_skins[skin_index];
        shader.setUniformMat4Array("u_bones", skin.bone_final_matrices.data(), JOINTS_COUNT);
        shader.setUniformInt("u_isAnimated", true);
    }
    else
        shader.setUniformInt("u_isAnimated", false);

    shader.setUniformMat4("u_model", matrix);

    for (const Primitive& primitive : mesh.primitives) {
        this->drawPrimitive(primitive, shader);
    }
}

void Model::drawPrimitive(const Primitive& primitive, const Shader& shader) {
    const Material& material = m_materials[primitive.material];

    this->bindMaterial(material, shader);

    primitive.vao.Bind();

    if (primitive.index_count > 0) {
        glDrawElements(GL_TRIANGLES, primitive.index_count, primitive.index_type, (void*) primitive.index_offset);
    }
    else {
        glDrawArrays(GL_TRIANGLES, 0, primitive.vertices.size());
    }
}

void Model::bindMaterial(const Material& material, const Shader& shader) {
    shader.setUniformVec4("u_baseColorFactor", material.pbr_metallic_roughness.base_color_factor);
    shader.setUniformFloat("u_metallicFactor", material.pbr_metallic_roughness.metallic_factor);
    shader.setUniformFloat("u_roughnessFactor", material.pbr_metallic_roughness.roughness_factor);

    int slot = 0;

    bindTexture(shader, "u_baseColorTex", material.pbr_metallic_roughness.base_color_texture.index, slot++);                 // 0
    bindTexture(shader, "u_metallicRoughnessTex", material.pbr_metallic_roughness.metallic_roughness_texture.index, slot++); // 1
    bindTexture(shader, "u_normalTex", material.normal_texture.index, slot++);                                               // 2
    bindTexture(shader, "u_occlusionTex", material.occlusion_texture.index, slot++);                                         // 3
    bindTexture(shader, "u_emissiveTex", material.emissive_texture.index, slot++);                                           // 4
}

void Model::bindTexture(const Shader& shader, const std::string& uniform, int texture_index, int slot) {
    shader.setUniformInt(uniform.c_str(), slot);

    if (texture_index < 0) {
        return;
    }

    glActiveTexture(GL_TEXTURE0 + slot);
    m_textures[texture_index].Bind();
}

void Model::readVector(glm::vec2& dst, const std::vector<double>& src) {
    if (src.size() != 2) return;
    dst = glm::vec2(static_cast<float>(src[0]), static_cast<float>(src[1]));
}

void Model::readVector(glm::vec3& dst, const std::vector<double>& src) {
    if (src.size() != 3) return;
    dst = glm::vec3(static_cast<float>(src[0]), static_cast<float>(src[1]), static_cast<float>(src[2]));
}

void Model::readVector(glm::vec4& dst, const std::vector<double>& src) {
    if (src.size() != 4) return;
    dst = glm::vec4(static_cast<float>(src[0]), static_cast<float>(src[1]), static_cast<float>(src[2]), static_cast<float>(src[3]));
}

void Model::readVector(glm::quat& dst, const std::vector<double>& src) {
    if (src.size() != 4) return;
    dst = glm::quat(static_cast<float>(src[0]), static_cast<float>(src[1]), static_cast<float>(src[2]), static_cast<float>(src[3]));
}

float Model::readComponentAsFloat(const uint8_t* data, int component_type, bool normalized) {
    switch (component_type) {
        case TINYGLTF_COMPONENT_TYPE_FLOAT:
            return *reinterpret_cast<const float*>(data);

        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
            uint16_t v = *reinterpret_cast<const uint16_t*>(data);
            return normalized ? (float) v / 65535.0F : (float) v;
        }

        case TINYGLTF_COMPONENT_TYPE_SHORT: {
            int16_t v = *reinterpret_cast<const int16_t*>(data);
            return normalized ? glm::clamp((float) v / 32767.0F, -1.0F, 1.0F) : (float) v;
        }

        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
            uint8_t v = *data;
            return normalized ? (float) v / 255.0F : (float) v;
        }

        case TINYGLTF_COMPONENT_TYPE_BYTE: {
            int8_t v = *reinterpret_cast<const int8_t*>(data);
            return normalized ? glm::clamp((float) v / 127.0F, -1.0F, 1.0F) : (float) v;
        }

        default:
            std::cerr << "WARNING : Unsupported component type : " << component_type << "\n";
            return 0.0F;
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

    size_t stride = (buffer_view.byteStride != 0U) ? buffer_view.byteStride : num_components * component_size;

    out.resize(accessor.count);

    for (size_t i = 0; i < accessor.count; i++) {

        glm::vec4 v(0.0F);

        const uint8_t* element = data + (i * stride);

        for (int component = 0; component < num_components; component++) {
            v[component] = readComponentAsFloat(
                element + (component * component_size),
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
    for (size_t i = 0; i < tmp.size(); i++) {
        out[i] = tmp[i].x;
    }
}
