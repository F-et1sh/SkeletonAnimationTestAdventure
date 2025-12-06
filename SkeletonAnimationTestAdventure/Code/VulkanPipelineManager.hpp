#pragma once
#include <iostream>
#include <optional>
#include <array>
#include <vector>
#include <set>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanRenderMesh.hpp"

/* forward declarations */
class VulkanDeviceManager;
class VulkanRenderPassManager;
class VulkanSwapchainManager;
class VulkanRenderMesh;

class VulkanPipelineManager {
public:
    VulkanPipelineManager(VulkanDeviceManager* device_manager, VulkanRenderPassManager* render_pass_manager, VulkanSwapchainManager* swapchain_manager, VulkanRenderMesh* render_mesh)
        : p_DeviceManager{ device_manager }, p_RenderPassManager{ render_pass_manager }, p_SwapchainManager{ swapchain_manager }, p_RenderMesh{ render_mesh } {}
    ~VulkanPipelineManager() { this->Release(); }

    void Release();

    void CreateDescriptorSetLayout();
    void CreateGraphicsPipeline();
    void CreateDescriptorPool();
    void CreateDescriptorSets();

    VkPipelineLayout getPipelineLayout() const noexcept { return m_PipelineLayout; }
    VkPipeline       getGraphicsPipeline() const noexcept { return m_GraphicsPipeline; }

private:
    static std::vector<char> readFile(const std::filesystem::path& path);
    VkShaderModule           createShaderModule(const std::vector<char>& code) const;

private:
    VulkanDeviceManager*     p_DeviceManager     = nullptr;
    VulkanRenderPassManager* p_RenderPassManager = nullptr;
    VulkanSwapchainManager*  p_SwapchainManager  = nullptr;
    VulkanRenderMesh*        p_RenderMesh        = nullptr;

    VkDescriptorSetLayout m_DescriptorSetLayout{};
    VkDescriptorPool      m_DescriptorPool{};

    VkPipelineLayout m_PipelineLayout{};
    VkPipeline       m_GraphicsPipeline{};
};
