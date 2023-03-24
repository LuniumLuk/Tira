#include <OpenGL/buffer.h>
#include <OpenGL/shader.h>
#include <OpenGL/root.h>
#include <OpenGL/window.h>
#include <glad/glad.h>

namespace GL {

Buffer::Buffer() {
    glGenBuffers(1, &handle);
}

Buffer::~Buffer() {
    if (handle == 0) return;
    glDeleteBuffers(1, &handle);
}

auto Buffer::copyTo(Buffer const& other, size_t rOffset, size_t wOffset, size_t size) -> void {
    glBindBuffer(GL_COPY_READ_BUFFER, handle);
    glBindBuffer(GL_COPY_WRITE_BUFFER, other.handle);
    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, rOffset, wOffset, size);
}

inline auto DataTypeSize(DataType type) noexcept -> size_t {
    switch (type)
    {
    case DataType::None:	return 0;
    case DataType::Float:	return 4;
    case DataType::Float2:	return 4 * 2;
    case DataType::Float3:	return 4 * 3;
    case DataType::Float4:	return 4 * 4;
    case DataType::Int:		return 4;
    case DataType::Int2:	return 4 * 2;
    case DataType::Int3:	return 4 * 3;
    case DataType::Int4:	return 4 * 4;
    case DataType::Bool:	return 1;
    case DataType::Mat3:	return 4 * 3 * 3;
    case DataType::Mat4:	return 4 * 4 * 4;
    default:				return 0;
    }
}

VertexBufferLayout::VertexBufferLayout(std::initializer_list<BufferElementItem> const& list) {
    std::vector temp(list);
    for (auto& element : temp) {
        switch (element.type) {
        case DataType::Mat3:
            for (size_t i = 0; i < 3; i++) {
                elements.emplace_back(DataType::Float3, stride);
                stride += DataTypeSize(DataType::Float3);
            }
            break;
        case DataType::Mat4:
            for (size_t i = 0; i < 4; i++) {
                elements.emplace_back(DataType::Float4, stride);
                stride += DataTypeSize(DataType::Float4);
            }
            break;
        default:
            element.offset = stride;
            elements.push_back(element);
            stride += DataTypeSize(element.type);
            break;
        }
    }
}

VertexBuffer::VertexBuffer(void* data, size_t size, VertexBufferLayout const& layout, UsageType behavior)
    : layout(layout)
{
    glBindBuffer(GL_ARRAY_BUFFER, handle);
    switch (behavior) {
    case UsageType::Static:
        glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW); break;
    case UsageType::Dynamic:
        glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW); break;
    case UsageType::Stream:
        glBufferData(GL_ARRAY_BUFFER, size, data, GL_STREAM_DRAW); break;
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

auto VertexBuffer::bindAttributes() noexcept -> void {
    glBindBuffer(GL_ARRAY_BUFFER, handle);
    unsigned int location = 0;
    for (auto const& element : layout.elements) {
        glEnableVertexAttribArray(location);
        switch (element.type)
        {
        case DataType::Float:
            glVertexAttribPointer(location, 1, GL_FLOAT, GL_FALSE, layout.stride, (void*)element.offset); break;
        case DataType::Float2:
            glVertexAttribPointer(location, 2, GL_FLOAT, GL_FALSE, layout.stride, (void*)element.offset); break;
        case DataType::Float3:
            glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, layout.stride, (void*)element.offset); break;
        case DataType::Float4:
            glVertexAttribPointer(location, 4, GL_FLOAT, GL_FALSE, layout.stride, (void*)element.offset); break;
        case DataType::Int:
            glVertexAttribPointer(location, 1, GL_INT, GL_FALSE, layout.stride, (void*)element.offset); break;
        case DataType::Int2:
            glVertexAttribPointer(location, 2, GL_INT, GL_FALSE, layout.stride, (void*)element.offset); break;
        case DataType::Int3:
            glVertexAttribPointer(location, 3, GL_INT, GL_FALSE, layout.stride, (void*)element.offset); break;
        case DataType::Int4:
            glVertexAttribPointer(location, 4, GL_INT, GL_FALSE, layout.stride, (void*)element.offset); break;
        case DataType::Bool:
            glVertexAttribPointer(location, 1, GL_BOOL, GL_FALSE, layout.stride, (void*)element.offset); break;
        default:
            break;
        }
        location++;
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

auto VertexBuffer::bindInstanceAttributes(unsigned int start, unsigned int divisor) noexcept -> void {
    glBindBuffer(GL_ARRAY_BUFFER, handle);
    unsigned int location = start;
    for (auto const& element : layout.elements) {
        glEnableVertexAttribArray(location);
        switch (element.type)
        {
        case DataType::Float:
            glVertexAttribPointer(location, 1, GL_FLOAT, GL_FALSE, layout.stride, (void*)element.offset); break;
        case DataType::Float2:
            glVertexAttribPointer(location, 2, GL_FLOAT, GL_FALSE, layout.stride, (void*)element.offset); break;
        case DataType::Float3:
            glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, layout.stride, (void*)element.offset); break;
        case DataType::Float4:
            glVertexAttribPointer(location, 4, GL_FLOAT, GL_FALSE, layout.stride, (void*)element.offset); break;
        case DataType::Int:
            glVertexAttribPointer(location, 1, GL_INT, GL_FALSE, layout.stride, (void*)element.offset); break;
        case DataType::Int2:
            glVertexAttribPointer(location, 2, GL_INT, GL_FALSE, layout.stride, (void*)element.offset); break;
        case DataType::Int3:
            glVertexAttribPointer(location, 3, GL_INT, GL_FALSE, layout.stride, (void*)element.offset); break;
        case DataType::Int4:
            glVertexAttribPointer(location, 4, GL_INT, GL_FALSE, layout.stride, (void*)element.offset); break;
        case DataType::Bool:
            glVertexAttribPointer(location, 1, GL_BOOL, GL_FALSE, layout.stride, (void*)element.offset); break;
        default:
            break;
        }
        glVertexAttribDivisor(location, divisor);
        location++;
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

ElementBuffer::ElementBuffer(void* data, size_t size, UsageType behavior) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle);
    switch (behavior) {
    case UsageType::Static:
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW); break;
    case UsageType::Dynamic:
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW); break;
    case UsageType::Stream:
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STREAM_DRAW); break;
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

UniformBuffer::UniformBuffer(void* data, size_t _size, UsageType behavior)
    : size(_size)
{
    glBindBuffer(GL_UNIFORM_BUFFER, handle);
    switch (behavior) {
    case UsageType::Static:
        glBufferData(GL_UNIFORM_BUFFER, size, data, GL_STATIC_DRAW); break;
    case UsageType::Dynamic:
        glBufferData(GL_UNIFORM_BUFFER, size, data, GL_DYNAMIC_DRAW); break;
    case UsageType::Stream:
        glBufferData(GL_UNIFORM_BUFFER, size, data, GL_STREAM_DRAW); break;
    }
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, handle, 0, size);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

auto UniformBuffer::bind(unsigned int binding) -> void {
    glBindBuffer(GL_UNIFORM_BUFFER, handle);
    glBindBufferRange(GL_UNIFORM_BUFFER, binding, handle, 0, size);
    glBindBufferBase(GL_UNIFORM_BUFFER, binding, handle);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}
auto UniformBuffer::bind(ShaderProgram* shader, std::string const& name, unsigned int binding) -> void {
    unsigned int ub_index = glGetUniformBlockIndex(shader->handle, name.c_str());
    glUniformBlockBinding(shader->handle, ub_index, binding);
}

auto UniformBuffer::update(void* data) -> void {
    glBindBuffer(GL_UNIFORM_BUFFER, handle);
    void* buffer_ptr = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
    memcpy(buffer_ptr, data, size);
    glUnmapBuffer(GL_UNIFORM_BUFFER);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

auto UniformBuffer::update(void* data, size_t offset, size_t _size) -> void {
    glBindBuffer(GL_UNIFORM_BUFFER, handle);
    void* buffer_ptr = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
    memcpy((char*)buffer_ptr + offset, data, _size);
    glUnmapBuffer(GL_UNIFORM_BUFFER);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

StorageBuffer::StorageBuffer(void* data, size_t _size, UsageType behavior)
    : size(_size)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, handle);
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, handle, 0, size);
    switch (behavior) {
    case UsageType::Static:
        //glBufferStorage(GL_SHADER_STORAGE_BUFFER, size, data, GL_DYNAMIC_STORAGE_BIT); break;
        glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_STATIC_COPY); break;
    case UsageType::Dynamic:
        glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_DYNAMIC_DRAW); break;
    case UsageType::Stream:
        glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_STREAM_DRAW); break;
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

auto StorageBuffer::bind(unsigned int binding) -> void {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, handle);
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, binding, handle, 0, size);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, handle);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

auto StorageBuffer::bind(ShaderProgram* shader, std::string const& name, unsigned int binding) -> void {
    unsigned int ub_index = glGetUniformBlockIndex(shader->handle, name.c_str());
    glShaderStorageBlockBinding(shader->handle, ub_index, binding);
}

auto StorageBuffer::update(void* data) -> void {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, handle);
    GLvoid* buffer_ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    memcpy(buffer_ptr, data, size);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

auto StorageBuffer::update(void* data, size_t offset, size_t _size) -> void {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, handle);
    GLvoid* buffer_ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    memcpy(((char*)buffer_ptr + offset), data, _size);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

FrameBufferLayout::FrameBufferLayout(std::initializer_list<AttachmentElement> const& list)
    : elements(list) {}

FrameBuffer::FrameBuffer(FrameBufferLayout const& layout, size_t w, size_t h)
    : width(w)
    , height(h)
    , depthAttachment { AttachmentType::Depth, InternalFormat::Default, 1, 0 }
{
    if (width == 0) width = Root::get()->window->width;
    if (height == 0) height = Root::get()->window->height;

    glGenFramebuffers(1, &handle);
    bool hasColorAttachment = false;
    size_t binding_idx = 0;
    for (auto element : layout.elements) {
        switch (element.type) {
        case AttachmentType::Color:
        case AttachmentType::ColorCubemap:
        case AttachmentType::ColorMSAA:
        case AttachmentType::ColorRenderBuffer:
        case AttachmentType::ColorMSAARenderBuffer:
            attach(element.type, element.format, binding_idx, element.samples);
            binding_idx++;
            hasColorAttachment = true;
            break;
        default:
            attach(element.type, element.format, 0, element.samples);
            break;
        }
    }
    // explicitly tell OpenGL we're not going to render any color data
    if (!hasColorAttachment) {
        glBindFramebuffer(GL_FRAMEBUFFER, handle);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

FrameBuffer::~FrameBuffer() {
    if (handle == 0) return;
    glDeleteFramebuffers(1, &handle);
}

auto FrameBuffer::bind() -> void {
    glBindFramebuffer(GL_FRAMEBUFFER, handle);
}

auto FrameBuffer::EnableDepthTest(bool flag) -> void {
    if (flag) glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);
}

auto FrameBuffer::BindDefault() -> void {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

auto FrameBuffer::SetWindowViewport() -> void {
    glViewport(0, 0, Root::get()->window->width, Root::get()->window->height);
}

auto FrameBuffer::attach(AttachmentType type, InternalFormat format, size_t binding, unsigned int samples) -> void {
    unsigned int attachment;
    size_t w = width;
    size_t h = height;
    float depthBorderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glBindFramebuffer(GL_FRAMEBUFFER, handle);
    GLint internalFormat;
    GLint colorFormat;
    GLenum dataType;
    GLint clamp = GL_CLAMP_TO_EDGE; // default clamp to edge for color attachments
    switch (format) {
    case InternalFormat::RED:
        internalFormat = GL_R8; colorFormat = GL_RED; dataType = GL_UNSIGNED_BYTE; break;
    case InternalFormat::Default:
    case InternalFormat::RGB:
        internalFormat = GL_RGB8; colorFormat = GL_RGB; dataType = GL_UNSIGNED_BYTE; break;
    case InternalFormat::RGBA:
        internalFormat = GL_RGBA8; colorFormat = GL_RGBA; dataType = GL_UNSIGNED_BYTE; break;
    case InternalFormat::FloatRED:
        internalFormat = GL_R32F; colorFormat = GL_RED; dataType = GL_FLOAT; break;
    case InternalFormat::FloatRGB:
        internalFormat = GL_RGB32F; colorFormat = GL_RGB; dataType = GL_FLOAT; break;
    case InternalFormat::FloatRGBA:
        internalFormat = GL_RGBA32F; colorFormat = GL_RGBA; dataType = GL_FLOAT; break;
    }

    if (type <= AttachmentType::DepthStencilMSAA) {
        glGenTextures(1, &attachment);
        if (type == AttachmentType::DepthCubemap || type == AttachmentType::ColorCubemap)
            glBindTexture(GL_TEXTURE_CUBE_MAP, attachment);
        else if (type == AttachmentType::DepthArray)
            glBindTexture(GL_TEXTURE_2D_ARRAY, attachment);
        else if (type == AttachmentType::ColorMSAA || type == AttachmentType::DepthStencilMSAA)
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, attachment);
        else
            glBindTexture(GL_TEXTURE_2D, attachment);
    }
    else {
        glGenRenderbuffers(1, &attachment);
        glBindRenderbuffer(GL_RENDERBUFFER, attachment);
    }

    switch (type) {
    case AttachmentType::Color:
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, colorFormat, dataType, NULL);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + binding, GL_TEXTURE_2D, attachment, 0);
        colorAttachments.push_back(AttachmentElement{ type, format, samples, attachment });
        break;
    case AttachmentType::ColorCubemap: // must use with a cubemap depth buffer
        for (unsigned int i = 0; i < 6; i++) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, internalFormat, w, h, 0, colorFormat, dataType, NULL);
        }
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + binding, attachment, 0);
        colorAttachments.push_back(AttachmentElement{ type, format, samples, attachment });
        break;
    case AttachmentType::ColorMSAA:
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB, w, h, GL_TRUE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + binding, GL_TEXTURE_2D_MULTISAMPLE, attachment, 0);
        colorAttachments.push_back(AttachmentElement{ type, format, samples, attachment });
        break;
    case AttachmentType::Depth:
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        // set depth border and beyond to 1.0
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, depthBorderColor);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, attachment, 0);
        depthAttachment = AttachmentElement{ type, format, samples, attachment };
        clamp = GL_CLAMP_TO_BORDER;
        break;
    case AttachmentType::DepthArray:
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT, w, h, samples, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, depthBorderColor);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, attachment, 0);
        depthAttachment = AttachmentElement{ type, format, samples, attachment };
        clamp = GL_CLAMP_TO_BORDER;
        break;
    case AttachmentType::DepthCubemap:
        for (unsigned int i = 0; i < 6; i++) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        }
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, attachment, 0);
        depthAttachment = AttachmentElement{ type, format, samples, attachment };
        clamp = GL_CLAMP_TO_BORDER;
        break;
    case AttachmentType::Stencil:
        glTexImage2D(GL_TEXTURE_2D, 0, GL_STENCIL_INDEX8, w, h, 0, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, NULL);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, attachment, 0);
        break;
    case AttachmentType::DepthStencil:
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, w, h, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, attachment, 0);
        break;
    case AttachmentType::DepthStencilMSAA:
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_DEPTH_STENCIL, w, h, GL_TRUE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, attachment, 0);
        break;
    case AttachmentType::ColorRenderBuffer:
        glRenderbufferStorage(GL_RENDERBUFFER, internalFormat, w, h); break;
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + binding, GL_RENDERBUFFER, attachment);
        break;
    case AttachmentType::DepthRenderBuffer:
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, attachment);
        break;
    case AttachmentType::StencilRenderBuffer:
        glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, w, h);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, attachment);
        break;
    case AttachmentType::DepthStencilRenderBuffer:
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, attachment);
        break;
    case AttachmentType::DepthStencilMSAARenderBuffer:
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, w, h);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, attachment);
        break;
    }

    if (type == AttachmentType::DepthCubemap || type == AttachmentType::ColorCubemap) {
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, samples > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, clamp);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, clamp);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, clamp);
        if (samples > 1) glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }
    else if (type == AttachmentType::DepthArray) {
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, samples > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, clamp);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, clamp);
        if (samples > 1) glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    }
    else if (type <= AttachmentType::DepthStencilMSAA) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamp);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamp);
        if (type == AttachmentType::ColorMSAA || type == AttachmentType::DepthStencilMSAA)
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
        else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, samples > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
            if (samples > 1) glGenerateMipmap(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
    else {
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "[Tira_GPU] " << "Framebuffer is not complete" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

auto FrameBuffer::bindColorAttachmentAsTexture(unsigned int attachment, unsigned int binding) -> void {
    if (attachment >= colorAttachments.size()) {
        std::cout << "[Tira_GPU] " << "attachment index greater than current color attachments" << std::endl;
        return;
    }
    glActiveTexture(GL_TEXTURE0 + binding);
    if (colorAttachments[attachment].type == AttachmentType::ColorMSAA) {
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, colorAttachments[attachment].handle);
    }
    else if (colorAttachments[attachment].type == AttachmentType::ColorCubemap) {
        glBindTexture(GL_TEXTURE_CUBE_MAP, colorAttachments[attachment].handle);
    }
    else {
        glBindTexture(GL_TEXTURE_2D, colorAttachments[attachment].handle);
    }
}

auto FrameBuffer::setViewport(unsigned int level) -> void {
    if (level == 0) {
        glViewport(0, 0, width, height);
    }
    else {
        unsigned int w = static_cast<unsigned int>(width * std::pow(0.5, level));
        unsigned int h = static_cast<unsigned int>(height * std::pow(0.5, level));
        glViewport(0, 0, w, h);
    }
}

auto FrameBuffer::ClearColor(float r, float g, float b, float a) -> void {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
}

auto FrameBuffer::ClearColorAndDepth(float r, float g, float b, float a) -> void {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

auto FrameBuffer::ClearDepth() -> void {
    glClear(GL_DEPTH_BUFFER_BIT);
}

auto FrameBuffer::bindDepthAttachmentAsTexture(unsigned int binding) -> void {
    if (depthAttachment.handle == 0) {
        std::cout << "[Tira_GPU] " << "no depth attachment can be bind as texture" << std::endl;
        return;
    }
    glActiveTexture(GL_TEXTURE0 + binding);
    if (depthAttachment.type == AttachmentType::Depth)
        glBindTexture(GL_TEXTURE_2D, depthAttachment.handle);
    else if (depthAttachment.type == AttachmentType::DepthArray)
        glBindTexture(GL_TEXTURE_2D_ARRAY, depthAttachment.handle);
    else if (depthAttachment.type == AttachmentType::DepthCubemap)
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthAttachment.handle);
}

auto FrameBuffer::bindColorMipmapLevel(unsigned int attachment, unsigned int level) -> void {
    glBindFramebuffer(GL_FRAMEBUFFER, handle);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachment, colorAttachments[attachment].handle, level);
    if (depthAttachment.handle > 0) {
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthAttachment.handle, level);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


auto FrameBuffer::BlitFrameBuffer(FrameBuffer* tar, FrameBuffer* src) -> void {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, src->handle);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tar->handle);
    glBlitFramebuffer(0, 0, src->width, src->height, 0, 0, tar->width, tar->height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

}
