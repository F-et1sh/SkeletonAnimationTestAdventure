#pragma once
#include <filesystem>
#include <iostream>
#include <fstream>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <glad/glad.h>

class Shader {
public:
    Shader() = default;
    ~Shader();

    enum {
        VERTEX   = 0x8B31,
        FRAGMENT = 0x8B30
    };

    // Initialize the shader program with one path for both shaders ( vertex and fragment )
    // don't use an extension here
    void Initialize(const std::filesystem::path& file_path);
    void Initialize(const std::filesystem::path& vert_path, const std::filesystem::path& frag_path);

    inline void Bind() const noexcept;
    inline void Unbind() const noexcept;

    // Get Shader Program Refecence
    const unsigned int& reference() const noexcept { return m_Program; }

private:
    void createProgram() noexcept;

    bool loadFromSource(const char* source, const unsigned int& type);
    bool loadFromFile(const std::filesystem::path& path, const unsigned int& type);

    void linkShader();

    // Reference of this Shader Program
    GLuint m_Program = 0;

public:
    // Legacy Methods with Uniform Setting
#pragma region Set Uniforms

    // Set int [ ACTIVATE SHADER FIRST ]
    inline void setUniformInt(const char* uniform_name, int load_uniform) const noexcept;
    // Set int Array [ ACTIVATE SHADER FIRST ]
    inline void setUniformIntArray(const char* uniform_name, int* load_uniform, unsigned int count) const noexcept;

    // Set float [ ACTIVATE SHADER FIRST ]
    inline void setUniformFloat(const char* uniform_name, float load_uniform) const noexcept;
    // Set float Array [ ACTIVATE SHADER FIRST ]
    inline void setUniformFloatArray(const char* uniform_name, float* load_uniform, unsigned int count) const noexcept;

    // Set vec2 [ ACTIVATE SHADER FIRST ]
    inline void setUniformVec2(const char* uniform_name, glm::vec2 load_uniform) const noexcept;
    // Set vec2 Array [ ACTIVATE SHADER FIRST ]
    inline void setUniformVec2Array(const char* uniform_name, glm::vec2* load_uniform, unsigned int count) const noexcept;
    // Set vec3 [ ACTIVATE SHADER FIRST ]
    inline void setUniformVec3(const char* uniform_name, glm::vec3 load_uniform) const noexcept;
    // Set vec3 [ ACTIVATE SHADER FIRST ]
    inline void setUniformVec3Array(const char* uniform_name, glm::vec3* load_uniform, unsigned int count) const noexcept;
    // Set vec4 [ ACTIVATE SHADER FIRST ]
    inline void setUniformVec4(const char* uniform_name, glm::vec4 load_uniform) const noexcept;
    // Set vec4 Array [ ACTIVATE SHADER FIRST ]
    inline void setUniformVec4Array(const char* uniform_name, glm::vec4* load_uniform, unsigned int count) const noexcept;

    // Set mat3 [ ACTIVATE SHADER FIRST ]
    inline void setUniformMat3(const char* uniform_name, glm::mat3 load_uniform) const noexcept;
    // Set mat3 Array [ ACTIVATE SHADER FIRST ]
    inline void setUniformMat3Array(const char* uniform_name, glm::mat3* load_uniform, unsigned int count) const noexcept;

    // Set mat4 [ ACTIVATE SHADER FIRST ]
    inline void setUniformMat4(const char* uniform_name, glm::mat4 load_uniform) const noexcept;
    // Set mat4 Array [ ACTIVATE SHADER FIRST ]
    inline void setUniformMat4Array(const char* uniform_name, glm::mat4* load_uniform, unsigned int count) const noexcept;

#pragma endregion

    // Static Legacy Methods with Uniform Setting
#pragma region Set Uniforms without Existing Shader

    static inline void setUniformInt(unsigned int shader_ref, const char* uniform_name, int load_uniform) noexcept;
    static inline void setUniformIntArray(unsigned int shader_ref, const char* uniform_name, int* load_uniform, unsigned int count) noexcept;

    static inline void setUniformFloat(unsigned int shader_ref, const char* uniform_name, float load_uniform) noexcept;
    static inline void setUniformFloatArray(unsigned int shader_ref, const char* uniform_name, float* load_uniform, unsigned int count) noexcept;

    static inline void setUniformVec2(unsigned int shader_ref, const char* uniform_name, const glm::vec2& load_uniform) noexcept;
    static inline void setUniformVec2Array(unsigned int shader_ref, const char* uniform_name, glm::vec2* load_uniform, unsigned int count) noexcept;

    static inline void setUniformVec3(unsigned int shader_ref, const char* uniform_name, const glm::vec3& load_uniform) noexcept;
    static inline void setUniformVec3Array(unsigned int shader_ref, const char* uniform_name, glm::vec3* load_uniform, unsigned int count) noexcept;

    static inline void setUniformVec4(unsigned int shader_ref, const char* uniform_name, const glm::vec4& load_uniform) noexcept;
    static inline void setUniformVec4Array(unsigned int shader_ref, const char* uniform_name, glm::vec4* load_uniform, unsigned int count) noexcept;

    static inline void setUniformMat3(unsigned int shader_ref, const char* uniform_name, const glm::mat3& load_uniform) noexcept;
    static inline void setUniformMat3Array(unsigned int shader_ref, const char* uniform_name, glm::mat3* load_uniform, unsigned int count) noexcept;

    static inline void setUniformMat4(unsigned int shader_ref, const char* uniform_name, const glm::mat4& load_uniform) noexcept;
    static inline void setUniformMat4Array(unsigned int shader_ref, const char* uniform_name, glm::mat4* load_uniform, unsigned int count) noexcept;

#pragma endregion
};
