#include "Model.hpp"

void Model::Initialize(const std::filesystem::path& path) {
    tinygltf::Model    model{};
    tinygltf::TinyGLTF loader{};
    std::string        error{};
    std::string        warning{};
    std::string        filename = path.string();

    bool good = loader.LoadASCIIFromFile(&model, &error, &warning, filename);

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
        m_scene_roots = model.scenes[scene_index].nodes;
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

void Model::loadVertices(const tinygltf::Model& model, std::vector<Primitive::Vertex>& this_vertices, const tinygltf::Primitive& primitive) {
    std::vector<glm::vec3> positions{};
    this->readAttribute(model, primitive, "POSITION", positions);

    std::vector<glm::vec3> normals{};
    this->readAttribute(model, primitive, "NORMAL", normals);

    std::vector<glm::vec4> tangent{};
    this->readAttribute(model, primitive, "TANGENT", tangent);

    std::vector<glm::vec2> texture_coords{};
    this->readAttribute(model, primitive, "TEXCOORD_0", texture_coords);

    std::vector<glm::uvec4> joints{};
    this->readAttribute(model, primitive, "JOINTS_0", joints);

    std::vector<glm::vec4> weights{};
    this->readAttribute(model, primitive, "WEIGHTS_0", weights);

    this_vertices.resize(positions.size()); // positions, normals, tangent, etc are they have the same size ?

    for (size_t i = 0; i < this_vertices.size(); i++) {
        Vertex& vertex       = this_vertices[i];
        vertex.position      = positions[i];
        vertex.normal        = normals[i];
        vertex.tangent       = tangent[i];
        vertex.texture_coord = texture_coords[i];
        vertex.joints        = joints[i];
        vertex.weights       = weights[i];
    }
}

void Model::loadIndices(const tinygltf::Model& model, std::vector<Primitive::Index>& this_indices, const tinygltf::Primitive& primitive) {
    constexpr inline static bool IS_INDICES = true;
    this->readAttribute(model, primitive, "", this_indices, IS_INDICES);
}

void Model::loadTextures(const tinygltf::Model& model) {
}

void Model::loadMaterials(const tinygltf::Model& model) {
}

void Model::loadAnimations(const tinygltf::Model& model) {
}
