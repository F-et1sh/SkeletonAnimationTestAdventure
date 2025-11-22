#pragma once
#include <string>
#include <variant>
#include "VertexBuffers.hpp"

enum class AlphaMode {
    OPAQUE,
    MASK,
    BLEND
};

struct TextureRef {
    int   index    = -1;
    float scale    = 1.0F;
    float strength = 1.0F;
};

struct Material {
    std::string name;

    glm::vec4 base_color_factor  = glm::vec4(1.0F);
    int       base_color_texture = -1;

    float metallic_factor            = 1.0F;
    float roughness_factor           = 1.0F;
    int   metallic_roughness_texture = -1;

    TextureRef normal_texture{};
    TextureRef occlusion_texture{};
    int        emissive_texture = -1;
    glm::vec3  emissive_factor  = glm::vec3(0.0F);

    AlphaMode alpha_mode   = AlphaMode::OPAQUE;
    float     alpha_cutoff = 0.5F;
    bool      double_sided = false;
};

struct Primitive {
    using Vertices = std::vector<Vertex>;
    using Indices  = std::variant<
         std::vector<uint8_t>,
         std::vector<uint16_t>,
         std::vector<uint32_t>>;

    Vertices vertex;
    Indices  indices;

    Primitive()  = default;
    ~Primitive() = default;
};

class Model {
public:
    Model()  = default;
    ~Model() = default;

private:
};
