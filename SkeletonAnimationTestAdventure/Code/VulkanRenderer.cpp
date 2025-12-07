#include "VulkanRenderer.hpp"

VulkanRenderer::~VulkanRenderer() {
    this->Release();
}

void VulkanRenderer::Release() {
    vkDeviceWaitIdle(m_DeviceManager.getDevice());
}

void VulkanRenderer::Initialize(GLFWwindow* p_window) {

    m_DeviceManager.CreateInstance();
    m_DeviceManager.SetupDebugMessenger();

    m_SwapchainManager.CreateSurface();

    m_DeviceManager.PickPhysicalDevice();

    m_DeviceManager.CreateLogicalDevice();
    m_SwapchainManager.CreateSwapchain();
    m_SwapchainManager.CreateImageViews();

    m_RenderPassManager.CreateRenderPass();

    m_PipelineManager.CreateDescriptorSetLayout();
    m_PipelineManager.CreateGraphicsPipeline();

    m_DeviceManager.CreateCommandPool();

    m_SwapchainManager.CreateColorResources();
    m_SwapchainManager.CreateDepthResources();

    m_SwapchainManager.CreateFramebuffers();

    m_RenderMesh.Initialize(&m_DeviceManager);

    m_PipelineManager.CreateDescriptorPool();
    m_PipelineManager.CreateDescriptorSets();

    m_DeviceManager.CreateCommandBuffers();
    m_DeviceManager.CreateSyncObjects();
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
    m_DeviceManager.DrawFrame();
}
