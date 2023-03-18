#include <OpenGL/shader.h>
#include <glad/glad.h>
#include <iostream>

namespace GL {

Shader::Shader(char const* const code, ShaderType type) {
    handle = [&]() -> unsigned int {
        switch (type)
        {
        case ShaderType::Geometry:
            return glCreateShader(GL_GEOMETRY_SHADER);
        case ShaderType::Vertex:
            return glCreateShader(GL_VERTEX_SHADER);
        case ShaderType::Fragment:
            return glCreateShader(GL_FRAGMENT_SHADER);
        case ShaderType::Compute:
            return glCreateShader(GL_COMPUTE_SHADER);
        default:
            return 0;
        }
    }();
    glShaderSource(handle, 1, &code, nullptr);
    glCompileShader(handle);
    int success;
    char infoLog[512];
    glGetShaderiv(handle, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(handle, 512, nullptr, infoLog);
        std::cout << "Shader::Shader compile failed" << infoLog << std::endl;
    }
}

Shader::~Shader() {
    if (handle == 0) return;
    glDeleteShader(handle);
}

ShaderProgram::ShaderProgram(std::initializer_list<Shader*> shaders) {
    handle = glCreateProgram();
    for (auto iter = shaders.begin(); iter != shaders.end(); iter++) {
        glAttachShader(handle, (*iter)->handle);
    }
    glLinkProgram(handle);
    int success;
    char infoLog[512];
    glGetProgramiv(handle, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(handle, 512, NULL, infoLog);
        std::cout << "Shader::ShaderProgram link failed" << infoLog << std::endl;
    }
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept {
    handle = other.handle;
    other.handle = 0;
}

ShaderProgram::~ShaderProgram() {
    if (handle == 0) return;
    glDeleteProgram(handle);
}

auto ShaderProgram::use() const noexcept -> void {
    glUseProgram(handle);
}

auto ShaderProgram::bind(StorageBuffer const& buffer, size_t index, std::string const& name) -> void {
    uint32_t blockID = glGetProgramResourceIndex(buffer.handle, GL_SHADER_STORAGE_BLOCK, name.c_str());
    glShaderStorageBlockBinding(handle, blockID, index);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, buffer.handle);
}

auto ShaderProgram::bind(UniformBuffer const& buffer, size_t index, std::string const& name) -> void {
    uint32_t blockID = glGetUniformBlockIndex(handle, name.c_str());
    glUniformBlockBinding(handle, blockID, index);
    glBindBufferBase(GL_UNIFORM_BUFFER, index, buffer.handle);
}

auto ShaderProgram::setBool(const std::string& name, bool value) const noexcept -> void {
    glUniform1i(glGetUniformLocation(handle, name.c_str()), (int)value);
}

auto ShaderProgram::setInt(const std::string& name, int value) const noexcept -> void {
    glUniform1i(glGetUniformLocation(handle, name.c_str()), value);
}

auto ShaderProgram::setFloat(const std::string& name, float value) const noexcept -> void {
    glUniform1f(glGetUniformLocation(handle, name.c_str()), value);
}

auto ShaderProgram::setVec2(const std::string& name, const glm::vec2& value) const noexcept -> void {
    glUniform2fv(glGetUniformLocation(handle, name.c_str()), 1, &value[0]);
}

auto ShaderProgram::setVec3(const std::string& name, const glm::vec3& value) const noexcept -> void {
    glUniform3fv(glGetUniformLocation(handle, name.c_str()), 1, &value[0]);
}

auto ShaderProgram::setVec4(const std::string& name, const glm::vec4& value) const noexcept -> void {
    glUniform4fv(glGetUniformLocation(handle, name.c_str()), 1, &value[0]);
}

auto ShaderProgram::setMat3(const std::string& name, const glm::mat3& mat) const noexcept -> void {
    glUniformMatrix3fv(glGetUniformLocation(handle, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

auto ShaderProgram::setMat4(const std::string& name, const glm::mat4& mat) const noexcept -> void {
    glUniformMatrix4fv(glGetUniformLocation(handle, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

}
