#pragma once

#include <initializer_list>
#include <string>
#include <glm/glm.hpp>
#include <OpenGL/buffer.h>

namespace GL {

enum struct ShaderType
{
    Geometry,
    Vertex,
    Fragment,
    Compute,
};

struct Shader {
    Shader(char const* const code, ShaderType type);
    Shader(Shader const&) = delete;
    Shader(Shader&&) = delete;
    ~Shader();

    unsigned int handle;
};

struct ShaderProgram {
    ShaderProgram(std::initializer_list<Shader*> shaders);
    ShaderProgram(ShaderProgram const&) = delete;
    ShaderProgram(ShaderProgram&& other) noexcept;
    ~ShaderProgram();

    auto use() const noexcept -> void;
    auto bind(StorageBuffer const& buffer, size_t index, std::string const& name) -> void;
    auto bind(UniformBuffer const& buffer, size_t index, std::string const& name) -> void;

    auto setBool(std::string const& name, bool value) const noexcept -> void;
    auto setInt(std::string const& name, int value) const noexcept -> void;
    auto setFloat(std::string const& name, float value) const noexcept -> void;
    auto setVec2(std::string const& name, glm::vec2 const& value) const noexcept -> void;
    auto setVec3(std::string const& name, glm::vec3 const& value) const noexcept -> void;
    auto setVec4(std::string const& name, glm::vec4 const& value) const noexcept -> void;
    auto setMat3(std::string const& name, glm::mat3 const& value) const noexcept -> void;
    auto setMat4(std::string const& name, glm::mat4 const& value) const noexcept -> void;

    unsigned int handle;
};

}
