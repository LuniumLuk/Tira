#pragma once

#include <OpenGL/buffer.h>

namespace GL {

enum struct PrimitiveType {
    Points,
    LineStrip,
    Lines,
    LineStripAdjacency,
    LinesAdjacency,
    TriangleStrip,
    TriangleFan,
    Triangles,
    TriangleStripAdjacency,
    TrianglesAdjacency,
};

struct Mesh {
    Mesh(float* vdata, size_t vsize, VertexBufferLayout const& layout, uint32_t* idata, size_t isize, PrimitiveType type = PrimitiveType::Triangles);
    Mesh(Mesh const&) = delete;
    Mesh(Mesh&& other) noexcept
        : elementCount(other.elementCount)
        , vertexBuffer(std::move(other.vertexBuffer))
        , elementBuffer(std::move(other.elementBuffer))
        , type(other.type)
    {
        handle = other.handle;
        other.handle = 0;
    }

    auto operator=(Mesh const&) -> Mesh& = delete;
    auto operator=(Mesh&& other) noexcept -> Mesh& {
        elementCount = other.elementCount;
        vertexBuffer = std::move(other.vertexBuffer);
        elementBuffer = std::move(other.elementBuffer);
        type = other.type;
        handle = other.handle;
        other.handle = 0;
        return *this;
    }

    auto bind() -> void;
    auto draw() -> void;
    auto drawInstanced(size_t num, VertexBuffer* instanceBuffer = nullptr, unsigned int start = 0, unsigned int divisor = 0) -> void;

    unsigned int handle;
    size_t elementCount;
    VertexBuffer vertexBuffer;
    ElementBuffer elementBuffer;
    PrimitiveType type;
};

struct VertexArray {
    VertexArray(void* vdata, size_t vsize, VertexBufferLayout const& layout);
    VertexArray(VertexArray const&) = delete;
    VertexArray(VertexArray&& other) noexcept
        : vertexCount(other.vertexCount)
        , vertexBuffer(std::move(other.vertexBuffer))
    {
        handle = other.handle;
        other.handle = 0;
    }

    auto operator=(VertexArray const&)->VertexArray & = delete;
    auto operator=(VertexArray&& other) noexcept -> VertexArray& {
        vertexCount = other.vertexCount;
        vertexBuffer = std::move(other.vertexBuffer);
        handle = other.handle;
        other.handle = 0;
        return *this;
    }

    auto bind() -> void;
    auto draw(PrimitiveType type) -> void;
    auto drawInstanced(PrimitiveType type, size_t num, VertexBuffer* instanceBuffer = nullptr, unsigned int start = 0, unsigned int divisor = 0) -> void;

    unsigned int handle;
    size_t vertexCount;
    VertexBuffer vertexBuffer;
};

}
