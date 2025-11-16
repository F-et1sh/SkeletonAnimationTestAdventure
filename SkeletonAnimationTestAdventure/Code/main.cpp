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
};

bool LoadGLTF(const std::string& filename, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices) {
    tinygltf::TinyGLTF loader;
    tinygltf::Model    gltf;
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

    if (!LoadGLTF("F:\\Windows\\Desktop\\SkeletonAnimationTestAdventure\\Files\\Models\\map\\scene.gltf", vertices, indices)) {
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
        
        glm::mat4 modelMat = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram.ID, "model"), 1, GL_FALSE, glm::value_ptr(modelMat));
        
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
