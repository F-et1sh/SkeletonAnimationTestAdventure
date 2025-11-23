#pragma once
#include <string>
#include <glm/glm.hpp>

#undef OPAQUE

struct TextureInfo {
    int index{ -1 };
    int texture_coord{ 0 };
};

struct NormalTextureInfo {
    int index{ -1 };
    int texture_coord{ 0 };

    double scale{ 1.0 };
};

struct OcclusionTextureInfo {
    int index{ -1 };
    int texture_coord{ 0 };

    double strength{ 1.0 };
};

struct PbrMetallicRoughness {
    glm::f64vec4 base_color_factor{ 1.0, 1.0, 1.0, 1.0 }; // len = 4. default [1,1,1,1]
    TextureInfo  base_color_texture;
    double       metallic_factor{ 1.0 };  // default 1
    double       roughness_factor{ 1.0 }; // default 1
    TextureInfo  metallic_roughness_texture;
};

struct Material {
    enum class AlphaMode {
        OPAQUE,
        MASK,
        BLEND
    };

    std::string name;

    glm::f64vec3     emissive_factor{ 0.0, 0.0, 0.0 }; // length 3. default [0, 0, 0]
    AlphaMode        alpha_mode{ AlphaMode::OPAQUE };  // default - OPAQUE
    double           alpha_cutoff{ 0.5 };              // default 0.5
    bool             double_sided{ false };            // default false
    std::vector<int> lods;                             // level of detail materials (MSFT_lod)

    PbrMetallicRoughness pbr_metallic_roughness;

    NormalTextureInfo    normal_texture;
    OcclusionTextureInfo occlusion_texture;
    TextureInfo          emissive_texture;
};
