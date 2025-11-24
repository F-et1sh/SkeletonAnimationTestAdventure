#include "Shader.hpp"

Shader::~Shader() {
    glDeleteProgram(m_program);
}

void Shader::Initialize(const std::filesystem::path& file_path) {

    this->createProgram();

    const std::filesystem::path& path = file_path;

    const std::filesystem::path vertex_path   = path.wstring() + L".vert";
    const std::filesystem::path fragment_path = path.wstring() + L".frag";

    const bool vert_init = this->loadFromFile(vertex_path, Shader::VERTEX);
    const bool frag_init = this->loadFromFile(fragment_path, Shader::FRAGMENT);

    if (vert_init && frag_init) {
        this->linkShader();
        return;
    }

    // if user will use path with an extension, program anyway can work
    // Example : SomeShader.shader ==> SomeShader.shader.vert & SomeShader.shader.frag
    // Else -> throw an error

    if (path.has_extension()) {
        std::runtime_error(std::string("\nFailed to Initialize a shader") +
                           std::string("\nYou tried to use one path for both shaders (vertex and fragment)") +
                           std::string("\nDelete the extension of the path") +
                           std::string("\nSource Path : ") + path.string() +
                           std::string("\nVertex Path : ") + vertex_path.string() +
                           std::string("\nFragment Path : ") + fragment_path.string());
    }
}

void Shader::Initialize(const std::filesystem::path& vert_path, const std::filesystem::path& frag_path) {

    this->createProgram();

    const std::filesystem::path& vertex_path   = vert_path;
    const std::filesystem::path& fragment_path = frag_path;

    bool vert_init = this->loadFromFile(vertex_path, Shader::VERTEX);
    bool frag_init = this->loadFromFile(fragment_path, Shader::FRAGMENT);

    if (vert_init && frag_init) {
        this->linkShader();
        return;
    }

    std::runtime_error(std::string("Failed to Initialize a shader") +
                       std::string("\nVertex Path : ") + vertex_path.string() +
                       std::string("\nFrament Path : ") + fragment_path.string());
}

void Shader::createProgram() noexcept {
    m_program = glCreateProgram();
}

bool Shader::loadFromSource(const char* source, const unsigned int& type) const {
    // Vertex or Fragment Shader
    unsigned int shader = glCreateShader(type);

    // Load Source to Shader
    glShaderSource(shader, 1, &source, nullptr);
    // Compile the Shader
    glCompileShader(shader);
    // Bind the Shader to the Program
    glAttachShader(m_program, shader);
    // Delete the Shader
    glDeleteShader(shader);

    // Check Shader Errors
    int result = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        int length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        char* message = (char*) _malloca(length * sizeof(char));
        glGetShaderInfoLog(shader, length, &length, message);

        std::wcerr << "ERROR : Failed to Compile Shader\n"
                   << message << '\n'
                   << '\n';

        glDeleteShader(shader);

        return false;
    }

    return true;
}

bool Shader::loadFromFile(const std::filesystem::path& path, const unsigned int& type) {
    std::ifstream file(path);
    if (!file.good()) {
        std::wcerr << L"ERROR: Failed to open shader file\nPath : " << path.c_str() << '\n'
                   << '\n';
        return false;
    }

    std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    file.close();

    if (source.empty()) {
        std::wcerr << L"ERROR: Shader file is empty\nPath: " << path.c_str() << '\n'
                   << '\n';
        return false;
    }

    return loadFromSource(source.c_str(), type);
}

void Shader::linkShader() const {
    glLinkProgram(m_program);
    glValidateProgram(m_program);
}

void Shader::Bind() const noexcept {
    glUseProgram(m_program);
}
void Shader::Unbind() noexcept {
    glUseProgram(0);
}

#pragma region Legacy Uniform Setting

// Methods with Uniform Setting
#pragma region Set Uniforms

void Shader::setUniformInt(const char* uniform_name, int load_uniform) const noexcept {
    glUniform1i(glGetUniformLocation(this->m_program, uniform_name), load_uniform);
}
void Shader::setUniformIntArray(const char* uniform_name, int* load_uniform, unsigned int count) const noexcept {
    glUniform1iv(glGetUniformLocation(this->m_program, uniform_name), count, load_uniform);
}

void Shader::setUniformFloat(const char* uniform_name, float load_uniform) const noexcept {
    glUniform1f(glGetUniformLocation(this->m_program, uniform_name), load_uniform);
}
void Shader::setUniformFloatArray(const char* uniform_name, float* load_uniform, unsigned int count) const noexcept {
    glUniform1fv(glGetUniformLocation(this->m_program, uniform_name), count, load_uniform);
}

void Shader::setUniformVec2(const char* uniform_name, const glm::vec2& load_uniform) const noexcept {
    glUniform2f(glGetUniformLocation(this->m_program, uniform_name), load_uniform.x, load_uniform.y);
}
void Shader::setUniformVec2Array(const char* uniform_name, glm::vec2* load_uniform, unsigned int count) const noexcept {
    glUniform2fv(glGetUniformLocation(this->m_program, uniform_name), count, &load_uniform[0].x);
}

void Shader::setUniformVec3(const char* uniform_name, const glm::vec3& load_uniform) const noexcept {
    glUniform3f(glGetUniformLocation(this->m_program, uniform_name), load_uniform.x, load_uniform.y, load_uniform.z);
}
void Shader::setUniformVec3Array(const char* uniform_name, glm::vec3* load_uniform, unsigned int count) const noexcept {
    glUniform3fv(glGetUniformLocation(this->m_program, uniform_name), count, &load_uniform[0].x);
}

void Shader::setUniformVec4(const char* uniform_name, const glm::vec4& load_uniform) const noexcept {
    glUniform4f(glGetUniformLocation(this->m_program, uniform_name), load_uniform.x, load_uniform.y, load_uniform.z, load_uniform.w);
}
void Shader::setUniformVec4Array(const char* uniform_name, glm::vec4* load_uniform, unsigned int count) const noexcept {
    glUniform4fv(glGetUniformLocation(this->m_program, uniform_name), count, &load_uniform[0].x);
}

void Shader::setUniformMat3(const char* uniform_name, const glm::mat3& load_uniform) const noexcept {
    glUniformMatrix3fv(glGetUniformLocation(this->m_program, uniform_name), 1, GL_FALSE, glm::value_ptr(load_uniform));
}
void Shader::setUniformMat3Array(const char* uniform_name, glm::mat3* load_uniform, unsigned int count) const noexcept {
    glUniformMatrix3fv(glGetUniformLocation(this->m_program, uniform_name), count, GL_FALSE, glm::value_ptr(load_uniform[0]));
}

void Shader::setUniformMat4(const char* uniform_name, const glm::mat4& load_uniform) const noexcept {
    glUniformMatrix4fv(glGetUniformLocation(this->m_program, uniform_name), 1, GL_FALSE, glm::value_ptr(load_uniform));
}
void Shader::setUniformMat4Array(const char* uniform_name, glm::mat4* load_uniform, unsigned int count) const noexcept {
    glUniformMatrix4fv(glGetUniformLocation(this->m_program, uniform_name), count, GL_FALSE, glm::value_ptr(load_uniform[0]));
}

#pragma endregion

// Static Methods with Uniform Setting
#pragma region Set Uniforms

void Shader::setUniformInt(unsigned int shader_ref, const char* uniform_name, int load_uniform) noexcept {
    glUniform1i(glGetUniformLocation(shader_ref, uniform_name), load_uniform);
}
void Shader::setUniformIntArray(unsigned int shader_ref, const char* uniform_name, int* load_uniform, unsigned int count) noexcept {
    glUniform1iv(glGetUniformLocation(shader_ref, uniform_name), count, load_uniform);
}

void Shader::setUniformFloat(unsigned int shader_ref, const char* uniform_name, float load_uniform) noexcept {
    glUniform1f(glGetUniformLocation(shader_ref, uniform_name), load_uniform);
}
void Shader::setUniformFloatArray(unsigned int shader_ref, const char* uniform_name, float* load_uniform, unsigned int count) noexcept {
    glUniform1fv(glGetUniformLocation(shader_ref, uniform_name), count, load_uniform);
}

void Shader::setUniformVec2(unsigned int shader_ref, const char* uniform_name, const glm::vec2& load_uniform) noexcept {
    glUniform2f(glGetUniformLocation(shader_ref, uniform_name), load_uniform.x, load_uniform.y);
}
void Shader::setUniformVec2Array(unsigned int shader_ref, const char* uniform_name, glm::vec2* load_uniform, unsigned int count) noexcept {
    glUniform2fv(glGetUniformLocation(shader_ref, uniform_name), count, &load_uniform[0].x);
}

void Shader::setUniformVec3(unsigned int shader_ref, const char* uniform_name, const glm::vec3& load_uniform) noexcept {
    glUniform3f(glGetUniformLocation(shader_ref, uniform_name), load_uniform.x, load_uniform.y, load_uniform.z);
}
void Shader::setUniformVec3Array(unsigned int shader_ref, const char* uniform_name, glm::vec3* load_uniform, unsigned int count) noexcept {
    glUniform3fv(glGetUniformLocation(shader_ref, uniform_name), count, &load_uniform[0].x);
}

void Shader::setUniformVec4(unsigned int shader_ref, const char* uniform_name, const glm::vec4& load_uniform) noexcept {
    glUniform4f(glGetUniformLocation(shader_ref, uniform_name), load_uniform.x, load_uniform.y, load_uniform.z, load_uniform.w);
}
void Shader::setUniformVec4Array(unsigned int shader_ref, const char* uniform_name, glm::vec4* load_uniform, unsigned int count) noexcept {
    glUniform4fv(glGetUniformLocation(shader_ref, uniform_name), count, &load_uniform[0].x);
}

void Shader::setUniformMat3(unsigned int shader_ref, const char* uniform_name, const glm::mat3& load_uniform) noexcept {
    glUniformMatrix3fv(glGetUniformLocation(shader_ref, uniform_name), 1, GL_FALSE, glm::value_ptr(load_uniform));
}
void Shader::setUniformMat3Array(unsigned int shader_ref, const char* uniform_name, glm::mat3* load_uniform, unsigned int count) noexcept {
    glUniformMatrix3fv(glGetUniformLocation(shader_ref, uniform_name), count, GL_FALSE, glm::value_ptr(load_uniform[0]));
}

void Shader::setUniformMat4(unsigned int shader_ref, const char* uniform_name, const glm::mat4& load_uniform) noexcept {
    glUniformMatrix4fv(glGetUniformLocation(shader_ref, uniform_name), 1, GL_FALSE, glm::value_ptr(load_uniform));
}
void Shader::setUniformMat4Array(unsigned int shader_ref, const char* uniform_name, glm::mat4* load_uniform, unsigned int count) noexcept {
    glUniformMatrix4fv(glGetUniformLocation(shader_ref, uniform_name), count, GL_FALSE, glm::value_ptr(load_uniform[0]));
}

#pragma endregion

#pragma endregion
