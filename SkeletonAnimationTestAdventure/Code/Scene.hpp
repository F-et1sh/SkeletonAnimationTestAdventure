#pragma once
#include <vector>
#include "Model.hpp"
#include "Material.h"

class Scene {
public:
    Scene()  = default;
    ~Scene() { this->Release(); }

    void Release();
    void Initialize();

    void loadModel(const std::filesystem::path& path);

private:
    std::vector<Model>    m_models{};
    std::vector<Material> m_materials{};
    std::vector<Texture>  m_textures{};
};