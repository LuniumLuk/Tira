#include <OpenGL/assetmanager.h>

namespace GL {

AssetManager* AssetManager::singleton = nullptr;

AssetManager::AssetManager() {
    singleton = this;
}

auto AssetManager::LoadTexture2D(std::string const& name, Filepath const& path, bool relativePath) -> void {
    if (relativePath)
        singleton->textureLib.emplaceAsset(name, GLTexture2D(getAssetPath(path)));
    else
        singleton->textureLib.emplaceAsset(name, GLTexture2D(path));
}

auto AssetManager::LoadTexture2D(std::string const& name, size_t width, size_t height, InternalFormat format) -> void {
    singleton->textureLib.emplaceAsset(name, GLTexture2D(width, height, format));
}

auto AssetManager::LoadCubemap(std::string const& name, std::vector<Filepath> const& paths) -> void {
    std::vector<std::string> filenames;
    for (auto path : paths) {
        filenames.emplace_back(std::move(getAssetPath(path)));
    }
    singleton->textureLib.emplaceAsset(name, GLCubemap(filenames));
}

auto AssetManager::LoadShaderProgramVF(std::string const& name, Filepath const& vert, Filepath const& frag, std::string const& macros) -> void {
    std::string vertCode = readFile(getShaderPath(vert));
    std::string fragCode = readFile(getShaderPath(frag));

    {
        size_t macroPos = vertCode.find("core");
        if (macroPos != std::string::npos) vertCode.insert(macroPos + 6, macros);
    }
    {
        size_t macroPos = fragCode.find("core");
        if (macroPos != std::string::npos) fragCode.insert(macroPos + 6, macros);
    }

    Shader vertShader(vertCode.c_str(), ShaderType::Vertex);
    Shader fragShader(fragCode.c_str(), ShaderType::Fragment);

    singleton->shaderLib.emplaceAsset(name, ShaderProgram({&vertShader, &fragShader}));
}

auto AssetManager::LoadShaderProgramVGF(std::string const& name, Filepath const& vert, Filepath const& geom, Filepath const& frag, std::string const& macros) -> void {
    std::string vertCode = readFile(getShaderPath(vert));
    std::string geomCode = readFile(getShaderPath(geom));
    std::string fragCode = readFile(getShaderPath(frag));

    {
        size_t macroPos = vertCode.find("core");
        if (macroPos != std::string::npos) vertCode.insert(macroPos + 6, macros);
    }
    {
        size_t macroPos = geomCode.find("core");
        if (macroPos != std::string::npos) geomCode.insert(macroPos + 6, macros);
    }
    {
        size_t macroPos = fragCode.find("core");
        if (macroPos != std::string::npos) fragCode.insert(macroPos + 6, macros);
    }

    auto vertShader = Shader(vertCode.c_str(), ShaderType::Vertex);
    auto geomShader = Shader(geomCode.c_str(), ShaderType::Geometry);
    auto fragShader = Shader(fragCode.c_str(), ShaderType::Fragment);

    singleton->shaderLib.emplaceAsset(name, ShaderProgram({ &vertShader, &geomShader, &fragShader }));
}

auto AssetManager::LoadShaderProgramCompute(std::string const& name, Filepath const& path, std::string const& macros) -> void {
    std::string code = readFile(getShaderPath(path));
    {
        size_t macroPos = code.find("core");
        if (macroPos != std::string::npos) code.insert(macroPos + 6, macros);
    }
    Shader shader(code.c_str(), ShaderType::Compute);
    singleton->shaderLib.emplaceAsset(name, ShaderProgram({ &shader }));
}

auto AssetManager::LoadMeshData(
    std::string const& name, 
    float* vdata, size_t vsize, 
    VertexBufferLayout const& layout, 
    uint32_t* idata, size_t isize, 
    PrimitiveType type) -> void {
    singleton->meshLib.emplaceAsset(name, Mesh{ vdata, vsize, layout, idata, isize, type });
}

auto AssetManager::LoadUniformBuffer(std::string const& name, void* data, size_t size, UsageType usage) -> void {
    singleton->uniformBufferLib.emplaceAsset(name, UniformBuffer{data, size, usage});
}

auto AssetManager::LoadStorageBuffer(std::string const& name, void* data, size_t size, UsageType usage) -> void {
    singleton->storageBufferLib.emplaceAsset(name, StorageBuffer{ data, size, usage });
}

auto AssetManager::LoadFrameBuffer(std::string const& name, FrameBufferLayout const& layout, size_t w, size_t h) -> void {
    singleton->frameBufferLib.emplaceAsset(name, FrameBuffer{layout, w, h});
}

}
