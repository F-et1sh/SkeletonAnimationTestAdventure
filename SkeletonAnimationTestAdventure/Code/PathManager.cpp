#include "PathManager.hpp"

void PathManager::init(const char* argv0, bool is_editor) {
    if (argv0 == nullptr) {
        throw std::runtime_error("Failed to Initialize PathManager\nargv0 was nullptr");
    }

    try {
        m_ExecutablePath = std::filesystem::canonical(argv0).parent_path();

        std::filesystem::path solution_path =
            m_ExecutablePath.parent_path(). // exit from the platform folder
            parent_path();                  // exit from the configuration folder

        static std::filesystem::path assets_folder_name = L"Files";

        std::filesystem::path editor_assets_folder_path      = solution_path / assets_folder_name;
        std::filesystem::path application_assets_folder_path = m_ExecutablePath / assets_folder_name;

        if (is_editor) {
            m_AssetsPath = editor_assets_folder_path;

            if (!std::filesystem::exists(m_AssetsPath)) {
                m_AssetsPath = application_assets_folder_path;
            }
        }
        else {
            m_AssetsPath = application_assets_folder_path;
        }

        if (std::filesystem::exists(editor_assets_folder_path)) {

            std::filesystem::remove_all(application_assets_folder_path);
            this->copy_if_new(editor_assets_folder_path, application_assets_folder_path);
        }
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Failed to Initialize PathManager\n" + std::string(e.what()));
    }
}

void PathManager::copy_if_new(const std::filesystem::path& from, const std::filesystem::path& to) {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(from)) {
        auto relative_path = std::filesystem::relative(entry.path(), from);
        auto target_path   = to / relative_path;

        if (entry.is_directory()) {
            std::filesystem::create_directories(target_path);
        }
        else if (!std::filesystem::exists(target_path) ||
                 std::filesystem::last_write_time(entry) > std::filesystem::last_write_time(target_path)) {
            std::filesystem::copy_file(entry, target_path, std::filesystem::copy_options::overwrite_existing);
        }
    }
}
