#pragma once
#include "IRenderer.hpp"

#include "VulkanDeviceManager.hpp"
#include "VulkanSwapchainManager.hpp"

class VulkanRenderer : public IRenderer {
public:
    VulkanRenderer(GLFWwindow* p_glfwWindow) : p_glfwWindow{ p_glfwWindow },
                                               m_DeviceManager{ &m_SwapchainManager, p_glfwWindow, &m_RenderPassManager, &m_PipelineManager, &m_RenderMesh },
                                               m_SwapchainManager{ &m_DeviceManager, p_glfwWindow, &m_RenderPassManager },
                                               m_RenderPassManager{ &m_DeviceManager, &m_SwapchainManager },
                                               m_PipelineManager{ &m_DeviceManager, &m_RenderPassManager, &m_SwapchainManager, &m_RenderMesh } {}
    ~VulkanRenderer();

    void Initialize();
    void DrawFrame();

    GLFWwindow* getGLFWwindow() noexcept { return p_glfwWindow; }

private:
    GLFWwindow* p_glfwWindow = nullptr;

    VulkanDeviceManager           m_DeviceManager;
    VulkanSwapchainManagerManager m_SwapchainManager;
    VulkanRenderPassManager       m_RenderPassManager;
    VulkanPipelineManager         m_PipelineManager;

    VulkanRenderMesh m_RenderMesh;
};
