#pragma once
#include <array>
#include <filesystem>
#include "VulkanGameObject.hpp"
#include "Vertices.hpp"

#include <stb_image.h>

class VulkanDeviceManager; // forward declaration

class VulkanRenderMesh {
public:
    constexpr static int MAX_OBJECTS = 3;

public:
    VulkanRenderMesh() = default;
    ~VulkanRenderMesh() { this->Release(); }

    void Release();
    void Initialize(VulkanDeviceManager* device_manager);

    VkSampler   getTextureSampler() const noexcept { return m_TextureSampler; }
    VkImageView getTextureImageView() const noexcept { return m_TextureImageView; }

    VkBuffer getVertexBuffer() const noexcept { return m_VertexBuffer; }
    VkBuffer getIndexBuffer() const noexcept { return m_IndexBuffer; }

    std::vector<uint32_t> getIndices() const noexcept {
        return {}; // TODO
    }
    std::array<VulkanGameObject, MAX_OBJECTS>& getGameObjects() noexcept { return m_GameObjects; }

private:
    void createTextureImage();
    void createTextureImageView();
    void createTextureSampler();
    void loadModel();
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffers();

    void generateMipmaps(VkImage image, VkFormat image_format, int32_t texture_width, int32_t texture_height, uint32_t mip_levels);

private:
    VulkanDeviceManager* p_DeviceManager = nullptr;

    uint32_t       m_MipLevels = 0;
    VkImage        m_TextureImage{};
    VkDeviceMemory m_TextureImageMemory{};
    VkImageView    m_TextureImageView{};
    VkSampler      m_TextureSampler{};

    std::vector<Vertex>   m_Vertices;
    std::vector<uint32_t> m_Indices;

    VkBuffer       m_VertexBuffer{};
    VkDeviceMemory m_VertexBufferMemory{};

    VkBuffer       m_IndexBuffer{};
    VkDeviceMemory m_IndexBufferMemory{};

    std::array<VulkanGameObject, MAX_OBJECTS> m_GameObjects;

    std::filesystem::path m_ModelPath;
    std::filesystem::path m_TexturePath;
};
