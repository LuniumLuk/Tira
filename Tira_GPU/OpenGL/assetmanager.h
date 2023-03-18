#pragma once

#include <OpenGL/io.h>
#include <OpenGL/buffer.h>
#include <OpenGL/gltexture.h>
#include <OpenGL/shader.h>
#include <OpenGL/mesh.h>
#include <string>
#include <unordered_map>

namespace GL {

enum struct AssetType {
    VertexBuffer,
    ElementBuffer,
    UniformBuffer,
    StorageBuffer,
    Texture2D,
    Cubemap,
    FrameBuffer,
};

template <class T>
struct AssetLib {
    AssetLib() = default;

    auto emplaceAsset(std::string const& name, T&& asset) {
        assets.emplace(name, std::move(asset));
    }
    auto getAsset(std::string const& name) -> T* {
        auto pair = assets.find(name);
        if (pair == assets.end())
            throw std::runtime_error(std::string("Acquiring none exist asset: ") + name);
        else
            return &(pair->second);
    }
private:
    std::unordered_map<std::string, T> assets;
};

struct AssetManager {
public:
    AssetManager();

    static auto GetTexture(std::string const& name) noexcept -> GLTexture* { return singleton->textureLib.getAsset(name); }
    static auto LoadTexture2D(std::string const& name, Filepath const& filename, bool relativePath = true) -> void;
    static auto LoadTexture2D(std::string const& name, size_t width, size_t height, InternalFormat format) -> void;
    static auto LoadCubemap(std::string const& name, std::vector<Filepath> const& paths) -> void;

    static auto GetShader(std::string const& name) noexcept -> ShaderProgram* { return singleton->shaderLib.getAsset(name); }
    static auto LoadShaderProgramVF(std::string const& name, Filepath const& vert, Filepath const& frag, std::string const& macros = "") -> void;
    static auto LoadShaderProgramVGF(std::string const& name, Filepath const& vert, Filepath const& geom, Filepath const& frag, std::string const& macros = "") -> void;
    static auto LoadShaderProgramCompute(std::string const& name, Filepath const& path, std::string const& macros) -> void;

    static auto GetMesh(std::string const& name) noexcept -> Mesh* { return singleton->meshLib.getAsset(name); }
    static auto LoadMeshData(std::string const& name, float* vdata, size_t vsize, VertexBufferLayout const& layout, uint32_t* idata, size_t isize, PrimitiveType type = PrimitiveType::Triangles) -> void;

    static auto GetUniformBuffer(std::string const& name) noexcept -> UniformBuffer* { return singleton->uniformBufferLib.getAsset(name); }
    static auto GetStorageBuffer(std::string const& name) noexcept -> StorageBuffer* { return singleton->storageBufferLib.getAsset(name); }
    static auto LoadUniformBuffer(std::string const& name, void* data, size_t size, UsageType usage) -> void;
    static auto LoadStorageBuffer(std::string const& name, void* data, size_t size, UsageType usage) -> void;

    static auto GetFrameBuffer(std::string const& name) noexcept -> FrameBuffer* { return singleton->frameBufferLib.getAsset(name); }
    static auto LoadFrameBuffer(std::string const& name, FrameBufferLayout const& layout, size_t w = 0, size_t h = 0) -> void;

private:
    static AssetManager* singleton;
    AssetLib<GLTexture> textureLib;
    AssetLib<ShaderProgram> shaderLib;
    AssetLib<Mesh> meshLib;
    AssetLib<UniformBuffer> uniformBufferLib;
    AssetLib<StorageBuffer> storageBufferLib;
    AssetLib<FrameBuffer> frameBufferLib;
};

}
