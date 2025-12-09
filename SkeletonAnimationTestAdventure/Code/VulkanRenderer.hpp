#pragma once
#include "IRenderer.hpp"

#include "VulkanDeviceManager.hpp"
#include "VulkanSwapchainManager.hpp"
#include "VulkanRenderPassManager.hpp"
#include "VulkanPipelineManager.hpp"
#include "VulkanRenderMesh.hpp"

class VulkanRenderer : public IRenderer {
public:
    VulkanRenderer(GLFWwindow* p_glfwWindow) : p_glfwWindow{ p_glfwWindow },
                                               m_deviceManager{ &m_swapchainManager, p_glfwWindow, &m_renderPassManager, &m_pipelineManager, &m_renderMesh },
                                               m_swapchainManager{ &m_deviceManager, p_glfwWindow, &m_renderPassManager },
                                               m_renderPassManager{ &m_deviceManager, &m_swapchainManager },
                                               m_pipelineManager{ &m_deviceManager, &m_renderPassManager, &m_swapchainManager, &m_renderMesh } {}
    ~VulkanRenderer();

    void Release() override;
    void Initialize(GLFWwindow* p_window) override;

    void loadModel(const Model& model) override;

    void onResize(uint32_t width, uint32_t height) override;

    void beginFrame() override;
    void submitCommand(const RenderCommand& cmd) override;
    void renderView(const RenderView& view) override;
    void endFrame() override;

    void waitIdle() override;

    void DrawFrame();

    GLFWwindow* getGLFWwindow() noexcept { return p_glfwWindow; }

private:
    GLFWwindow* p_glfwWindow = nullptr;

    VulkanDeviceManager     m_deviceManager;
    VulkanSwapchainManager  m_swapchainManager;
    VulkanRenderPassManager m_renderPassManager;
    VulkanPipelineManager   m_pipelineManager;

    VulkanRenderMesh m_renderMesh;
};
