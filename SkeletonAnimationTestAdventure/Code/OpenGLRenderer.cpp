#include "OpenGLRenderer.hpp"

void OpenGLRenderer::Release() {
    m_commandBuffer.clear();
}

void OpenGLRenderer::Initialize(GLFWwindow* p_window) {
    this->p_window = p_window;

    if (gladLoadGLLoader((GLADloadproc) glfwGetProcAddress) == 0) {
        assert(false);
    }

    glViewport(0, 0, 2560, 1440);
    glEnable(GL_DEPTH_TEST);
}

void OpenGLRenderer::onResize(uint32_t width, uint32_t height) {
    glViewport(0, 0, width, height);
}

void OpenGLRenderer::beginFrame() {
    m_commandBuffer.clear();

    glClearColor(0.07F, 0.13F, 0.17F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLRenderer::submitCommand(const RenderCommand& cmd) {
    m_commandBuffer.emplace_back(cmd);
}

void OpenGLRenderer::renderView(const RenderView& view) {
    m_renderView = view;
    //m_renderView.camera->UploadUniforms(shader, "u_cameraMatrix");
}

void OpenGLRenderer::endFrame() {
    for (const RenderCommand& cmd : m_commandBuffer) {
        //auto* mesh = cmd.mesh;
        //auto* material = cmd.material;

        //material->bind();               // glUseProgram + текстуры
        //material->setMat4("u_View", m_currentView.view);
        //material->setMat4("u_Proj", m_currentView.projection);
        //material->setMat4("u_Model", cmd.transform);

        //mesh->bind();                  // VAO + VBO + IBO
        //glDrawElements(GL_TRIANGLES, mesh->indexCount(), GL_UNSIGNED_INT, 0);
    }

    glfwSwapBuffers(p_window);
}

void OpenGLRenderer::waitIdle() {
    glFinish();
}
