#pragma once
#include <string>
#include <glm/glm.hpp>
#include "tiny_gltf.h"

#undef OPAQUE

class Material {
public:
    enum class AlphaMode {
        OPAQUE,
        MASK,
        BLEND
    };

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
        glm::f64vec4 base_color_factor{ 1.0, 1.0, 1.0, 1.0 }; // default [1,1,1,1]
        TextureInfo  base_color_texture;
        double       metallic_factor{ 1.0 };  // default 1
        double       roughness_factor{ 1.0 }; // default 1
        TextureInfo  metallic_roughness_texture;
    };

public:
    Material()  = default;
    ~Material() { this->Release(); }

    void Release();
    void Initialize(const tinygltf::Model& model);

private:
    std::string m_name;

    glm::f64vec3     m_emissiveFactor{ 0.0, 0.0, 0.0 }; // default [0, 0, 0]
    AlphaMode        m_alphaMode{ AlphaMode::OPAQUE };  // default - OPAQUE
    double           m_alphaCutoff{ 0.5 };              // default 0.5
    bool             m_doubleSided{ false };            // default false
    std::vector<int> m_lods;                             // level of detail materials (MSFT_lod)

    PbrMetallicRoughness m_pbrMetallicRoughness;

    NormalTextureInfo    m_normalTexture;
    OcclusionTextureInfo m_occlusionTexture;
    TextureInfo          m_emissiveTexture;
};
