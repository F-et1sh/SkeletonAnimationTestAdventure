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
                                               m_DeviceManager{ &m_SwapchainManager, p_glfwWindow, &m_RenderPassManager, &m_PipelineManager, &m_RenderMesh },
                                               m_SwapchainManager{ &m_DeviceManager, p_glfwWindow, &m_RenderPassManager },
                                               m_RenderPassManager{ &m_DeviceManager, &m_SwapchainManager },
                                               m_PipelineManager{ &m_DeviceManager, &m_RenderPassManager, &m_SwapchainManager, &m_RenderMesh } {}
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

    VulkanDeviceManager     m_DeviceManager;
    VulkanSwapchainManager  m_SwapchainManager;
    VulkanRenderPassManager m_RenderPassManager;
    VulkanPipelineManager   m_PipelineManager;

    VulkanRenderMesh m_RenderMesh;
};
