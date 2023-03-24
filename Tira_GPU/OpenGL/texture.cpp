#include <OpenGL/texture.h>
#include <OpenGL/shader.h>
#include <glad/glad.h>
#include <stb_image.h>
#include <stb_image_write.h>
#include <iostream>

namespace GL {

Texture::Texture() {
    type = TextureType::None;
    glGenTextures(1, &handle);
}

Texture::~Texture() {
    if (handle == 0) return;
    glDeleteTextures(1, &handle);
}

auto Texture::bind(unsigned int binding) -> void {
    glActiveTexture(GL_TEXTURE0 + binding);
    switch (type) {
    case TextureType::Texture2D:
        glBindTexture(GL_TEXTURE_2D, handle); break;
    case TextureType::Cubemap:
        glBindTexture(GL_TEXTURE_CUBE_MAP, handle); break;
    }
}

auto Texture::bind(ShaderProgram* shader, std::string const& name, unsigned int binding) -> void {
    glActiveTexture(GL_TEXTURE0 + binding);
    switch (type) {
    case TextureType::Texture2D:
        glBindTexture(GL_TEXTURE_2D, handle); break;
    case TextureType::Cubemap:
        glBindTexture(GL_TEXTURE_CUBE_MAP, handle); break;
    }
    shader->setInt(name.c_str(), binding);
}

Texture2D::Texture2D(std::string const& filename, bool hdr, bool genMipmap, bool flipY) {
    type = TextureType::Texture2D;
    glBindTexture(GL_TEXTURE_2D, handle);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(flipY);
    if (hdr) {
        float* data = stbi_loadf(filename.c_str(), &width, &height, &nrChannels, 4);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "[Tira_GPU] " << "Texture2D failed to load at path: " << filename << std::endl;
            stbi_image_free(data);
        }
    }
    else {
        unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrChannels, 4);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "[Tira_GPU] " << "Texture2D failed to load at path: " << filename << std::endl;
            stbi_image_free(data);
        }
    }

    if (genMipmap)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, genMipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);
}

Texture2D::Texture2D(size_t width, size_t height, InternalFormat format, void* data) {
    type = TextureType::Texture2D;
    glBindTexture(GL_TEXTURE_2D, handle);

    switch (format) {
    case InternalFormat::RED:
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data); break;
    case InternalFormat::Default:
    case InternalFormat::RGB:
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data); break;
    case InternalFormat::RGBA:
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data); break;
    case InternalFormat::FloatRED:
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, data); break;
    case InternalFormat::FloatRGB:
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, data); break;
    case InternalFormat::FloatRGBA:
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, data); break;
    case InternalFormat::Depth:
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, data); break;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);
}

Cubemap::Cubemap(std::vector<std::string> const& filenames, bool genMipmap, bool flipY) {
    type = TextureType::Cubemap;
    glBindTexture(GL_TEXTURE_CUBE_MAP, handle);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(flipY);
    for (unsigned int i = 0; i < filenames.size(); i++)
    {
        unsigned char* data = stbi_load(filenames[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            if (nrChannels == 4)
            {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                    0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data
                );
            }
            else
            {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                    0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
                );
            }
            stbi_image_free(data);
        }
        else
        {
            std::cout << "[Tira_GPU] " << "Cubemap failed to load at path: " << filenames[i] << std::endl;
            stbi_image_free(data);
        }
    }
    if (genMipmap)
    {
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, genMipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_REPEAT);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

// https://lencerf.github.io/post/2019-09-21-save-the-opengl-rendering-to-image-file/
auto saveSnapshot(std::string const& path, int w, int h) -> void {
    GLsizei nrChannels = 3;
    GLsizei stride = nrChannels * w;
    stride += (stride % 4) ? (4 - stride % 4) : 0;
    GLsizei bufferSize = stride * h;
    std::vector<char> buffer(bufferSize);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, buffer.data());
    stbi_flip_vertically_on_write(true);
    stbi_write_png(path.c_str(), w, h, nrChannels, buffer.data(), stride);
}

}
