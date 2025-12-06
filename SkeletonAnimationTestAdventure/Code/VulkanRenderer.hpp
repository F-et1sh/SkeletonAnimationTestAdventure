#pragma once
#include "IRenderer.hpp"

#include <Volk/volk.h>
#include "vma/vk_mem_alloc.h"

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

    DeviceManager     m_DeviceManager;
    SwapchainManager  m_SwapchainManager;
    RenderPassManager m_RenderPassManager;
    PipelineManager   m_PipelineManager;

    RenderMesh m_RenderMesh;
};
