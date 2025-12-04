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

    if (gladLoadGLLoader((GLADloadproc) glfwGetProcAddress) == 0) {
        return -1;
    }

    glViewport(0, 0, 2560, 1440);
    glEnable(GL_DEPTH_TEST);

    Camera camera(2560, 1440, glm::vec3{});

    Shader shader;
    shader.Initialize(L"F:\\Windows\\Desktop\\SkeletonAnimationTestAdventure\\Files\\Shaders\\default");
    shader.Bind();

    Model model{};
    model.Initialize(L"F:\\Windows\\Desktop\\SkeletonAnimationTestAdventure\\Files\\Models\\rifle-awp-weapon-model-cs2-original\\source\\AWP.glb");

    std::unique_ptr<IRenderer> renderer = std::make_unique<OpenGLRenderer>();
    renderer->Initialize(window);
    renderer->loadModel(model);

    RenderCommand render_command{};
    render_command.model = &model;

    RenderView render_view{};
    render_view.camera = &camera;

    while (glfwWindowShouldClose(window) == 0) {
        glClearColor(0.07F, 0.13F, 0.17F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        camera.Inputs(window);
        camera.UpdateMatrix(70.0F, 0.01F, 1000.0F);
        camera.UploadUniforms(shader, "u_cameraMatrix");

        /*shader.setUniformVec3("u_lightDirection", glm::vec3(1, 1, 1));
        shader.setUniformVec3("u_lightColor", glm::vec3(30, 30, 30));
        shader.setUniformVec3("u_cameraPosition", camera.getPosition());

        model.Draw(shader, glfwGetTime());*/

        renderer->beginFrame();
        renderer->submitCommand(render_command);
        renderer->renderView(render_view);
        renderer->endFrame();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}
