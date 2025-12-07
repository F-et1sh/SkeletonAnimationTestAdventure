#pragma once
#include <iostream>
#include <array>

#include <Volk/volk.h>

/* forward declaration */
class VulkanDeviceManager;
class VulkanSwapchainManager;

class VulkanRenderPassManager {
public:
    VulkanRenderPassManager(VulkanDeviceManager* device_manager, VulkanSwapchainManager* swapchain_manager) : p_DeviceManager{ device_manager }, p_SwapchainManager{ swapchain_manager } {}
    ~VulkanRenderPassManager() { this->Release(); }

    void Release();
    void CreateRenderPass();

    VkRenderPass getRenderPass() const noexcept { return m_RenderPass; }

private:
    VulkanDeviceManager*    p_DeviceManager    = nullptr;
    VulkanSwapchainManager* p_SwapchainManager = nullptr;

    VkRenderPass m_RenderPass{};
};
