#include "OpenGLRenderer.hpp"

void OpenGLRenderer::Release() {
}

void OpenGLRenderer::Initialize(GLFWwindow* p_window) {
}

void OpenGLRenderer::onResize(uint32_t width, uint32_t height) {
}

void OpenGLRenderer::beginFrame() {
}

void OpenGLRenderer::submitCommand(const RenderCommand& cmd) {
    for (const auto& primitive : this->m_resourceManager.getPrimitives()) {
        primitive.vao.Bind();

        if (primitive.index_count > 0) {
            glDrawElements(primitive.mode, primitive.index_count, primitive.index_type, (void*)primitive.index_offset);
        }
        else {
            //glDrawArrays(GL_TRIANGLES, 0, primitive.vertices.size());
        }

        primitive.vao.Unbind();
    }
}

void OpenGLRenderer::renderView(const RenderView& view) {
}

void OpenGLRenderer::endFrame() {
}

void OpenGLRenderer::waitIdle() {
}
