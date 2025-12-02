#pragma once
#include "IRenderer.hpp"

class RendererOpenGL : public IRenderer {
public:
    RendererOpenGL()  = default;
    ~RendererOpenGL() = default;

    void Release() override;
    void Initialize(GLFWwindow* p_window) override;

    void onResize(uint32_t width, uint32_t height) override;

    void beginFrame() override;
    void submitCommand(const RenderCommand& cmd) override;
    void renderView(const RenderView& view) override;
    void endFrame() override;

    void waitIdle() override;

private:


};
