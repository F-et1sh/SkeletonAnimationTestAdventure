#include "Shader.hpp"
#include "Camera.hpp"

// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include "tiny_gltf.h"

// I used this https://www.youtube.com/playlist?list=PLPaoO-vpZnumdcb4tZc4x5Q-v7CkrQ6M as a code base for OpenGL rendering
// So, honestly, I don't like this code, but it works, so I don't give a damn

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;

    glm::uvec4 joints;
    glm::vec4 weights;
};

struct Skin {
    std::vector<int> joints;
    std::vector<glm::mat4> inverseBindMatrices;
};

struct AnimSampler {
    std::vector<float> times;
    std::vector<glm::vec3> translations;
    std::vector<glm::quat> rotations;
    std::vector<glm::vec3> scales;
};

struct Node {
    int parent = -1;
    std::vector<int> children;

    glm::vec3 translation = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1, 0, 0, 0);
    glm::vec3 scale = glm::vec3(1.0f);

    glm::mat4 localMatrix = glm::mat4(1.0f);
    glm::mat4 globalMatrix = glm::mat4(1.0f);
};

void ComputeGlobal(Node& node, std::vector<Node>& nodes, const glm::mat4& parentMat) {
    node.globalMatrix = parentMat * node.localMatrix;

    for (int child : node.children) {
        ComputeGlobal(nodes[child], nodes, node.globalMatrix);
    }
}

std::vector<Node> LoadNodes(const tinygltf::Model& gltf) {
    std::vector<Node> nodes(gltf.nodes.size());

    for (size_t i = 0; i < gltf.nodes.size(); i++) {
        const auto& n = gltf.nodes[i];
        Node& node = nodes[i];

        if (n.translation.size() == 3)
            node.translation = glm::vec3(n.translation[0], n.translation[1], n.translation[2]);

        if (n.rotation.size() == 4)
            node.rotation = glm::quat(n.rotation[3], n.rotation[0], n.rotation[1], n.rotation[2]);

        if (n.scale.size() == 3)
            node.scale = glm::vec3(n.scale[0], n.scale[1], n.scale[2]);

        if (n.children.size() > 0)
            node.children = n.children;

        // localMatrix = T * R * S
        node.localMatrix =
            glm::translate(glm::mat4(1.0f), node.translation) *
            glm::mat4_cast(node.rotation) *
            glm::scale(glm::mat4(1.0f), node.scale);
    }

    for (int root : gltf.scenes[gltf.defaultScene].nodes) {
        ComputeGlobal(nodes[root], nodes, glm::mat4(1.0f));
    }

    return nodes;
}

Skin LoadSkin(const tinygltf::Model& gltf) {
    Skin result;

    if (gltf.skins.empty()) {
        std::cout << "No skin in model\n";
        return result;
    }

    const tinygltf::Skin& s = gltf.skins[0];

    result.joints = s.joints;

    // inverseBindMatrices
    const tinygltf::Accessor& acc = gltf.accessors[s.inverseBindMatrices];
    const tinygltf::BufferView& view = gltf.bufferViews[acc.bufferView];
    const tinygltf::Buffer& buf = gltf.buffers[view.buffer];

    const float* raw = reinterpret_cast<const float*>(&buf.data[view.byteOffset + acc.byteOffset]);

    size_t count = acc.count;
    result.inverseBindMatrices.resize(count);

    for (size_t i = 0; i < count; i++) {
        result.inverseBindMatrices[i] = glm::make_mat4(raw + i * 16);
    }

    return result;
}

std::vector<glm::mat4> boneMatrices;
std::vector<Node>      nodes;
Skin                   skin;
tinygltf::Model        gltf;

bool LoadGLTF(const std::string& filename, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices) {
    tinygltf::TinyGLTF loader;
    std::string        err, warn;

    bool ok = loader.LoadASCIIFromFile(&gltf, &err, &warn, filename);
    if (!ok) {
        std::cout << "Failed to load glTF: " << err << std::endl;
        return false;
    }

    if (!warn.empty()) std::cout << "WARN: " << warn << std::endl;

    if (gltf.meshes.empty()) {
        std::cout << "No meshes in glTF\n";
        return false;
    }

    const tinygltf::Mesh& mesh = gltf.meshes[0]; // берём первый

    for (const tinygltf::Primitive& primitive : mesh.primitives) {

        // ---- POSITIONS ----
        const tinygltf::Accessor&   posAcc    = gltf.accessors[primitive.attributes.at("POSITION")];
        const tinygltf::BufferView& posView   = gltf.bufferViews[posAcc.bufferView];
        const tinygltf::Buffer&     posBuffer = gltf.buffers[posView.buffer];
        const float*                positions = reinterpret_cast<const float*>(&posBuffer.data[posView.byteOffset + posAcc.byteOffset]);

        // ---- NORMALS (optional) ----
        const float* normals = nullptr;
        if (primitive.attributes.count("NORMAL")) {
            const tinygltf::Accessor&   nAcc    = gltf.accessors[primitive.attributes.at("NORMAL")];
            const tinygltf::BufferView& nView   = gltf.bufferViews[nAcc.bufferView];
            const tinygltf::Buffer&     nBuffer = gltf.buffers[nView.buffer];
            normals                             = reinterpret_cast<const float*>(&nBuffer.data[nView.byteOffset + nAcc.byteOffset]);
        }

        // ---- UV (optional) ----
        const float* uvs = nullptr;
        if (primitive.attributes.count("TEXCOORD_0")) {
            const tinygltf::Accessor&   uvAcc    = gltf.accessors[primitive.attributes.at("TEXCOORD_0")];
            const tinygltf::BufferView& uvView   = gltf.bufferViews[uvAcc.bufferView];
            const tinygltf::Buffer&     uvBuffer = gltf.buffers[uvView.buffer];
            uvs                                  = reinterpret_cast<const float*>(&uvBuffer.data[uvView.byteOffset + uvAcc.byteOffset]);
        }

        const uint16_t* jointsRaw = nullptr;
        if (primitive.attributes.count("JOINTS_0")) {
            const auto& acc  = gltf.accessors[primitive.attributes.at("JOINTS_0")];
            const auto& view = gltf.bufferViews[acc.bufferView];
            jointsRaw        = reinterpret_cast<const uint16_t*>(&gltf.buffers[view.buffer].data[view.byteOffset + acc.byteOffset]);
        }

        const float* weightsRaw = nullptr;
        if (primitive.attributes.count("WEIGHTS_0")) {
            const auto& acc  = gltf.accessors[primitive.attributes.at("WEIGHTS_0")];
            const auto& view = gltf.bufferViews[acc.bufferView];
            weightsRaw       = reinterpret_cast<const float*>(&gltf.buffers[view.buffer].data[view.byteOffset + acc.byteOffset]);
        }

        size_t              vertexCount = posAcc.count;
        std::vector<Vertex> localVerts;
        localVerts.resize(vertexCount);

        for (size_t i = 0; i < vertexCount; i++) {
            Vertex v{};
            v.pos = glm::vec3(
                positions[i * 3 + 0],
                positions[i * 3 + 1],
                positions[i * 3 + 2]);

            if (normals) {
                v.normal = glm::vec3(
                    normals[i * 3 + 0],
                    normals[i * 3 + 1],
                    normals[i * 3 + 2]);
            }

            if (uvs) {
                v.uv = glm::vec2(
                    uvs[i * 2 + 0],
                    uvs[i * 2 + 1]);
            }

            if (jointsRaw) {
                v.joints = glm::uvec4(
                    jointsRaw[i * 4 + 0],
                    jointsRaw[i * 4 + 1],
                    jointsRaw[i * 4 + 2],
                    jointsRaw[i * 4 + 3]);
            }

            if (weightsRaw) {
                v.weights = glm::vec4(
                    weightsRaw[i * 4 + 0],
                    weightsRaw[i * 4 + 1],
                    weightsRaw[i * 4 + 2],
                    weightsRaw[i * 4 + 3]);
            }

            outVertices.push_back(v);
        }

        // ---- INDICES ----
        const tinygltf::Accessor&   idxAcc    = gltf.accessors[primitive.indices];
        const tinygltf::BufferView& idxView   = gltf.bufferViews[idxAcc.bufferView];
        const tinygltf::Buffer&     idxBuffer = gltf.buffers[idxView.buffer];

        const uint8_t* idxData = &idxBuffer.data[idxView.byteOffset + idxAcc.byteOffset];

        for (size_t i = 0; i < idxAcc.count; i++) {
            uint32_t index = 0;

            switch (idxAcc.componentType) {
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                    index = reinterpret_cast<const uint16_t*>(idxData)[i];
                    break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                    index = reinterpret_cast<const uint32_t*>(idxData)[i];
                    break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                    index = reinterpret_cast<const uint8_t*>(idxData)[i];
                    break;
                default:
                    std::cout << "Unsupported index type\n";
                    return false;
            }

            outIndices.push_back(index);
        }

        nodes = LoadNodes(gltf);
        skin  = LoadSkin(gltf);
        boneMatrices.resize(skin.joints.size());
        
        for (size_t i = 0; i < skin.joints.size(); i++) {
            int jointIndex = skin.joints[i];
        
            glm::mat4 globalTransform = nodes[jointIndex].globalMatrix;
            glm::mat4 invBind         = skin.inverseBindMatrices[i];
        
            boneMatrices[i] = globalTransform * invBind;
        }
    }

    return true;
}

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1920, 1080, "Test", NULL, NULL);
    if (!window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        return -1;
    }

    glViewport(0, 0, 1920, 1080);

    Shader shaderProgram("F:\\Windows\\Desktop\\SkeletonAnimationTestAdventure\\Files\\Shaders\\default.vert",
                         "F:\\Windows\\Desktop\\SkeletonAnimationTestAdventure\\Files\\Shaders\\default.frag");

    // Take care of all the light related things
    glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec3 lightPos   = glm::vec3(0.5f, 0.5f, 0.5f);
    glm::mat4 lightModel = glm::mat4(1.0f);
    lightModel           = glm::translate(lightModel, lightPos);

    shaderProgram.Activate();
    glUniform4f(glGetUniformLocation(shaderProgram.ID, "lightColor"), lightColor.x, lightColor.y, lightColor.z, lightColor.w);
    glUniform3f(glGetUniformLocation(shaderProgram.ID, "lightPos"), lightPos.x, lightPos.y, lightPos.z);

    // Enables the Depth Buffer
    glEnable(GL_DEPTH_TEST);

    // Creates camera object
    Camera camera(1920, 1080, glm::vec3(0.0f, 0.0f, 2.0f));

    // Load in a model
    std::vector<Vertex>   vertices;
    std::vector<uint32_t> indices;

    if (!LoadGLTF("F:\\Windows\\Desktop\\SkeletonAnimationTestAdventure\\Files\\Models\\test_character\\scene.gltf", vertices, indices)) {
        return -1;
    }

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);
    
    GLsizei stride = sizeof(Vertex);
    
    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);
    
    // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    
    // uv
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(2);

    // JOINTS_0
    glVertexAttribIPointer(3, 4, GL_UNSIGNED_INT, stride, (void*) offsetof(Vertex, joints));
    glEnableVertexAttribArray(3);

    // WEIGHTS_0
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride, (void*) offsetof(Vertex, weights));
    glEnableVertexAttribArray(4);
    
    glBindVertexArray(0);

    while (!glfwWindowShouldClose(window)) {
        // Specify the color of the background
        glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
        // Clean the back buffer and depth buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Handles camera inputs
        camera.Inputs(window);
        // Updates and exports the camera matrix to the Vertex Shader
        camera.updateMatrix(45.0f, 0.01f, 1000.0f);

        shaderProgram.Activate();
        camera.updateMatrix(45.0f, 0.01f, 1000.0f);
        camera.Matrix(shaderProgram, "camMatrix");

        int loc = glGetUniformLocation(shaderProgram.ID, "bones");
        glUniformMatrix4fv(loc, boneMatrices.size(), GL_FALSE, glm::value_ptr(boneMatrices[0]));

        glm::mat4 modelMat = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram.ID, "model"), 1, GL_FALSE, glm::value_ptr(modelMat));

        float t = (float)glfwGetTime();

        // вращаем первую кость — просто тест
        nodes[ skin.joints[0] ].rotation = glm::angleAxis(t, glm::vec3(0, 1, 0));
        
        // пересчёт local + global
        for (size_t i = 0; i < nodes.size(); i++) {
            nodes[i].localMatrix =
                glm::translate(glm::mat4(1.0f), nodes[i].translation) *
                glm::mat4_cast(nodes[i].rotation) *
                glm::scale(glm::mat4(1.0f), nodes[i].scale);
        }
        
        for (int root : gltf.scenes[gltf.defaultScene].nodes) {
            ComputeGlobal(nodes[root], nodes, glm::mat4(1.0f));
        }
        
        for (size_t i = 0; i < skin.joints.size(); i++) {
            int joint = skin.joints[i];
            boneMatrices[i] = nodes[joint].globalMatrix * skin.inverseBindMatrices[i];
        }

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        // Swap the back buffer with the front buffer
        glfwSwapBuffers(window);
        // Take care of all GLFW events
        glfwPollEvents();
    }

    // Delete all the objects we've created
    shaderProgram.Delete();
    // Delete window before ending the program
    glfwDestroyWindow(window);
    // Terminate GLFW before ending the program
    glfwTerminate();
}
