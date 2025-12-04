#pragma once
#include "Model.hpp"
#include <GLFW/glfw3.h>
#include "Camera.hpp"

struct RenderCommand {
    Model* model = nullptr;

    RenderCommand()  = default;
    ~RenderCommand() = default;
};

struct RenderView {
    Camera* camera = nullptr;

    RenderView()  = default;
    ~RenderView() = default;
};

class IRenderer {
public:
    virtual ~IRenderer() = default;

    virtual void Release()                        = 0;
    virtual void Initialize(GLFWwindow* p_window) = 0;

    virtual void loadModel(const Model& model) = 0;

    virtual void onResize(uint32_t width, uint32_t height) = 0;

    virtual void beginFrame()                            = 0;
    virtual void submitCommand(const RenderCommand& cmd) = 0;
    virtual void renderView(const RenderView& view)      = 0;
    virtual void endFrame()                              = 0;

    virtual void waitIdle() = 0;
};
