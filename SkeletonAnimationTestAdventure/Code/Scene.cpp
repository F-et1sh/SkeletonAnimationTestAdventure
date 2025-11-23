#include "Scene.hpp"

void Scene::loadModel(const std::filesystem::path& path) {
    Model& model = m_models.emplace_back();
    model.Initialize(path);

}