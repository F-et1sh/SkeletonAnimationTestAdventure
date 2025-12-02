#pragma once
#include "IRenderer.hpp"
#include "OpenGLResourceManager.h"

class OpenGLRenderer : public IRenderer {
public:
    OpenGLRenderer()  = default;
    ~OpenGLRenderer() = default;

    void Release() override;
    void Initialize(GLFWwindow* p_window) override;

    inline void loadModel(const Model& model) override { this->m_resourceManager.loadModel(model); }

    void onResize(uint32_t width, uint32_t height) override;

    void beginFrame() override;
    void submitCommand(const RenderCommand& cmd) override;
    void renderView(const RenderView& view) override;
    void endFrame() override;

    void waitIdle() override;

private:
    OpenGLResourceManager m_resourceManager{};
};
