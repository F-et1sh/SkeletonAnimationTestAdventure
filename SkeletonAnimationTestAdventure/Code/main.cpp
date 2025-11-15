#include <iostream>
#include <filesystem>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

int main() {
    if (!glfwInit()) return -1;

    GLFWwindow* window = glfwCreateWindow(800, 600, "Test", NULL, NULL);
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        return -1;
    }

    tinygltf::Model       model;
    tinygltf::TinyGLTF    gltf_ctx;
    std::string           err;
    std::string           warn;
    std::filesystem::path input_filename = L"F:\\Windows\\Desktop\\SkeletonAnimationTestAdventure\\Files\\Cube\\Cube.gltf";
    std::string           ext            = input_filename.extension().string();

    gltf_ctx.SetStoreOriginalJSONForExtrasAndExtensions(false);

    bool ret = false;
    if (ext.compare("glb") == 0) {
        std::cout << "Reading binary glTF" << std::endl;
        // assume binary glTF.
        ret = gltf_ctx.LoadBinaryFromFile(&model, &err, &warn, input_filename.string().c_str());
    }
    else {
        std::cout << "Reading ASCII glTF" << std::endl;
        // assume ascii glTF.
        ret = gltf_ctx.LoadASCIIFromFile(&model, &err, &warn, input_filename.string().c_str());
    }

    if (!warn.empty()) {
        printf("Warn: %s\n", warn.c_str());
    }

    if (!err.empty()) {
        printf("Err: %s\n", err.c_str());
    }

    if (!ret) {
        printf("Failed to parse glTF\n");
        return -1;
    }

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
}
