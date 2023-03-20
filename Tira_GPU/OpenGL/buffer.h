#pragma once

#include <OpenGL/texture.h>
#include <iostream>
#include <vector>

namespace GL {

struct ShaderProgram;

struct Buffer {
    Buffer();
    ~Buffer();
    Buffer(Buffer const&) = delete;
    Buffer(Buffer&& other) noexcept
        : handle(other.handle)
    {
        other.handle = 0;
    }
    auto operator=(Buffer const&)->Buffer & = delete;
    auto operator=(Buffer&& other) noexcept -> Buffer& {
        handle = other.handle;
        other.handle = 0;
        return *this;
    }
    unsigned int handle;

    auto copyTo(Buffer const& other, size_t rOffset, size_t wOffset, size_t size) -> void;
};

enum struct DataType {
    None,
    Float,
    Float2,
    Float3,
    Float4,
    Int,
    Int2,
    Int3,
    Int4,
    Bool,
    Mat3,
    Mat4,
};
inline auto DataTypeSize(DataType type) noexcept -> size_t;

enum struct UsageType {
    Static,
    Dynamic,
    Stream,
};

struct BufferElementItem {
    DataType type;
    size_t offset;

    BufferElementItem() = delete;
    BufferElementItem(DataType _type)
        : type(_type), offset(0)
    {}
    BufferElementItem(DataType _type, size_t _offset)
        : type(_type), offset(_offset)
    {}
};

struct VertexBufferLayout {
    VertexBufferLayout(std::initializer_list<BufferElementItem> const& list);
    std::vector<BufferElementItem> elements;
    size_t stride = 0;
};

struct VertexBuffer : public Buffer {
    VertexBuffer(void* data, size_t size, VertexBufferLayout const& layout, UsageType behavior = UsageType::Static);

    VertexBuffer(VertexBuffer const&) = delete;
    VertexBuffer(VertexBuffer&& other) noexcept
        : layout(other.layout)
    {
        handle = other.handle;
        other.handle = 0;
    }
    auto operator=(VertexBuffer const& other)->VertexBuffer & = delete;
    auto operator=(VertexBuffer&& other) noexcept -> VertexBuffer& {
        layout = other.layout;
        handle = other.handle;
        other.handle = 0;
        return *this;
    }

    auto bindAttributes() noexcept -> void;
    auto bindInstanceAttributes(unsigned int start, unsigned int divisor) noexcept -> void;

    VertexBufferLayout layout;
};

struct ElementBuffer : public Buffer {
    ElementBuffer(void* data, size_t size, UsageType behavior = UsageType::Static);

    ElementBuffer(ElementBuffer const&) = delete;
    ElementBuffer(ElementBuffer&& other) noexcept {
        handle = other.handle;
        other.handle = 0;
    }
    auto operator=(ElementBuffer const& other)->ElementBuffer & = delete;
    auto operator=(ElementBuffer&& other) noexcept -> ElementBuffer& {
        handle = other.handle;
        other.handle = 0;
        return *this;
    }
};

struct UniformBuffer : public Buffer {
    UniformBuffer(void* data, size_t _size, UsageType behavior = UsageType::Static);

    UniformBuffer(UniformBuffer const&) = delete;
    UniformBuffer(UniformBuffer&& other) noexcept {
        size = other.size;
        handle = other.handle;
        other.handle = 0;
    }
    auto operator=(UniformBuffer const& other)->UniformBuffer & = delete;
    auto operator=(UniformBuffer&& other) noexcept -> UniformBuffer& {
        size = other.size;
        handle = other.handle;
        other.handle = 0;
        return *this;
    }

    auto bind(unsigned int binding) -> void;
    auto bind(ShaderProgram* shader, std::string const& name, unsigned int binding) -> void;
    auto update(void* data) -> void;
    // deprecate
    auto update(void* data, size_t offset, size_t _size) -> void;

    size_t size;
};

struct StorageBuffer : public Buffer {
    StorageBuffer(void* data, size_t _size, UsageType behavior = UsageType::Static);

    StorageBuffer(StorageBuffer const&) = delete;
    StorageBuffer(StorageBuffer&& other) noexcept {
        size = other.size;
        handle = other.handle;
        other.handle = 0;
    }
    auto operator=(StorageBuffer const& other)->StorageBuffer & = delete;
    auto operator=(StorageBuffer&& other) noexcept -> StorageBuffer& {
        size = other.size;
        handle = other.handle;
        other.handle = 0;
        return *this;
    }

    auto bind(unsigned int binding) -> void;
    auto bind(ShaderProgram* shader, std::string const& name, unsigned int binding) -> void;
    auto update(void* data) -> void;
    // deprecate
    auto update(void* data, size_t offset, size_t _size) -> void;

    size_t size;
};

enum struct AttachmentType {
    Color,
    ColorCubemap,
    ColorMSAA,
    Depth,
    DepthArray,
    DepthCubemap,
    Stencil,
    DepthStencil,
    DepthStencilMSAA,
    ColorRenderBuffer,
    ColorMSAARenderBuffer,
    DepthRenderBuffer,
    StencilRenderBuffer,
    DepthStencilRenderBuffer,
    DepthStencilMSAARenderBuffer,
};

struct AttachmentElement {
    AttachmentType type;
    InternalFormat format = InternalFormat::Default;
    union {
        unsigned int levels = 1;
        unsigned int samples;
    };
    unsigned int handle = 0;
};

struct FrameBufferLayout {
    FrameBufferLayout(std::initializer_list<AttachmentElement> const& list);
    std::vector<AttachmentElement> elements;
};

struct FrameBuffer {
    FrameBuffer(FrameBufferLayout const& layout, size_t w = 0, size_t h = 0);
    ~FrameBuffer();

    FrameBuffer(FrameBuffer const&) = delete;
    FrameBuffer(FrameBuffer&& other) noexcept {
        handle = other.handle;
        width = other.width;
        height = other.height;
        colorAttachments = std::move(other.colorAttachments);
        depthAttachment = other.depthAttachment;

        other.handle = 0;
    }
    auto operator=(FrameBuffer const& other)->FrameBuffer & = delete;
    auto operator=(FrameBuffer&& other) noexcept -> FrameBuffer& {
        handle = other.handle;
        width = other.width;
        height = other.height;
        other.handle = 0;
        return *this;
    }

    auto bind() -> void;
    auto attach(AttachmentType type, InternalFormat format = InternalFormat::Default, size_t binding = 0, unsigned int samples = 1) -> void;
    auto bindColorAttachmentAsTexture(unsigned int attachment, unsigned int binding) -> void;
    auto bindDepthAttachmentAsTexture(unsigned int binding) -> void;
    auto bindColorMipmapLevel(unsigned int attachment, unsigned int level) -> void;
    auto setViewport(unsigned int level = 0) -> void;

    static auto EnableDepthTest(bool flag) -> void;
    static auto ClearColor(float r, float g, float b, float a) -> void;
    static auto ClearColorAndDepth(float r, float g, float b, float a) -> void;
    static auto ClearDepth() -> void;
    static auto BindDefault() -> void;
    static auto SetWindowViewport() -> void;
    static auto BlitFrameBuffer(FrameBuffer* tar, FrameBuffer* src) -> void;

    unsigned int handle;
    size_t width;
    size_t height;
    std::vector<AttachmentElement> colorAttachments;
    AttachmentElement depthAttachment;
};

}
