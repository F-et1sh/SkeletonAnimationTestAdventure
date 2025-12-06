#include "VulkanGameObject.hpp"

VulkanGameObject::~VulkanGameObject() {
    for (auto* buffer : uniform_buffers) {
        vkDestroyBuffer(p_device, buffer, nullptr);
    }
    for (auto* memory : uniform_buffers_memory) {
        vkFreeMemory(p_device, memory, nullptr);
    }
}
