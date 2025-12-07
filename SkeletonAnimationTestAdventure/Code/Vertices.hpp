#pragma once

#include <Volk/volk.h>

#pragma pack(push, 1)
struct Vertex {
    glm::vec3    position;
    glm::vec3    normal;
    glm::vec2    texture_coord;
    glm::u16vec4 joints;
    glm::vec4    weights;
    glm::vec4    tangent;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription binding_description{};
        binding_description.binding   = 0;
        binding_description.stride    = sizeof(Vertex);
        binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return binding_description;
    }

    static std::array<VkVertexInputAttributeDescription, 6> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 6> attribute_descriptions{};

        attribute_descriptions[0].binding  = 0;
        attribute_descriptions[0].location = 0;
        attribute_descriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_descriptions[0].offset   = offsetof(Vertex, position);

        attribute_descriptions[1].binding  = 0;
        attribute_descriptions[1].location = 1;
        attribute_descriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_descriptions[1].offset   = offsetof(Vertex, normal);

        attribute_descriptions[2].binding  = 0;
        attribute_descriptions[2].location = 2;
        attribute_descriptions[2].format   = VK_FORMAT_R32G32_SFLOAT;
        attribute_descriptions[2].offset   = offsetof(Vertex, texture_coord);

        attribute_descriptions[3].binding = 0;
        attribute_descriptions[3].location = 3;
        attribute_descriptions[3].format   = VK_FORMAT_R16G16B16A16_UINT;
        attribute_descriptions[3].offset   = offsetof(Vertex, joints);

        attribute_descriptions[4].binding = 0;
        attribute_descriptions[4].location = 4;
        attribute_descriptions[4].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
        attribute_descriptions[4].offset   = offsetof(Vertex, weights);

        attribute_descriptions[5].binding = 0;
        attribute_descriptions[5].location = 5;
        attribute_descriptions[5].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
        attribute_descriptions[5].offset   = offsetof(Vertex, tangent);

        return attribute_descriptions;
    }

    Vertex()  = default;
    ~Vertex() = default;
};
#pragma pack(pop)