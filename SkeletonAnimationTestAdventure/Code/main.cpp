#include "OpenGLRenderer.hpp"
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

int main() {
    if (glfwInit() == 0) {
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(2560, 1440, "Test", nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "Failed to create GLFW window" << '\n';
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    std::unique_ptr<IRenderer> renderer = std::make_unique<OpenGLRenderer>();
    renderer->Initialize(window);

    Camera camera(2560, 1440, glm::vec3{});

    Shader shader;
    shader.Initialize(L"F:\\Windows\\Desktop\\SkeletonAnimationTestAdventure\\Files\\Shaders\\default");
    shader.Bind();

    Model model{};
    model.Initialize(L"F:\\Windows\\Desktop\\SkeletonAnimationTestAdventure\\Files\\Models\\rifle-awp-weapon-model-cs2-original\\source\\AWP.glb");

    renderer->loadModel(model);

    RenderCommand render_command{};
    render_command.model = &model;

    RenderView render_view{};
    render_view.camera = &camera;

    while (glfwWindowShouldClose(window) == 0) {
        camera.Inputs(window);
        camera.UpdateMatrix(70.0F, 0.01F, 1000.0F);

        /*shader.setUniformVec3("u_lightDirection", glm::vec3(1, 1, 1));
        shader.setUniformVec3("u_lightColor", glm::vec3(30, 30, 30));
        shader.setUniformVec3("u_cameraPosition", camera.getPosition());

        model.Draw(shader, glfwGetTime());*/

        shader.Bind();

        renderer->beginFrame();
        renderer->submitCommand(render_command);
        renderer->renderView(render_view);
        renderer->endFrame();

        shader.Unbind();

        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}
