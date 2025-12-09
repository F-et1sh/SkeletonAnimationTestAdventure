#include "VulkanRenderer.hpp"

VulkanRenderer::~VulkanRenderer() {
    this->Release();
}

void VulkanRenderer::Release() {
    vkDeviceWaitIdle(m_deviceManager.getDevice());
}

void VulkanRenderer::Initialize(GLFWwindow* p_window) {

    m_deviceManager.CreateInstance();
    m_deviceManager.SetupDebugMessenger();

    m_swapchainManager.CreateSurface();

    m_deviceManager.PickPhysicalDevice();

    m_deviceManager.CreateLogicalDevice();
    m_swapchainManager.CreateSwapchain();
    m_swapchainManager.CreateImageViews();

    m_renderPassManager.CreateRenderPass();

    m_pipelineManager.CreateDescriptorSetLayout();
    m_pipelineManager.CreateGraphicsPipeline();

    m_deviceManager.CreateCommandPool();

    m_swapchainManager.CreateColorResources();
    m_swapchainManager.CreateDepthResources();

    m_swapchainManager.CreateFramebuffers();

    m_renderMesh.Initialize(&m_deviceManager);

    m_pipelineManager.CreateDescriptorPool();
    m_pipelineManager.CreateDescriptorSets();

    m_deviceManager.CreateCommandBuffers();
    m_deviceManager.CreateSyncObjects();
}

void VulkanRenderer::loadModel(const Model& model) {
}

void VulkanRenderer::onResize(uint32_t width, uint32_t height) {
}

void VulkanRenderer::beginFrame() {
}

void VulkanRenderer::submitCommand(const RenderCommand& cmd) {
}

void VulkanRenderer::renderView(const RenderView& view) {
}

void VulkanRenderer::endFrame() {
}

void VulkanRenderer::waitIdle() {
}

void VulkanRenderer::DrawFrame() {
    m_deviceManager.DrawFrame();
}
