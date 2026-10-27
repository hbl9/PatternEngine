#pragma once

#include <cstdint>
#include <span>

struct VertexAttribute
{
	unsigned	index = 0;
	int			size = 0;
	unsigned	type = 0;
	bool		normalized = false;
	std::size_t	stride = 0;
	std::size_t	offset = 0;
};

class Mesh
{
public:
	Mesh();
	~Mesh();

	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;

	Mesh(Mesh&& other)			  noexcept;
	Mesh& operator=(Mesh&& other) noexcept;

	void Upload(std::span<const float> vertices,
		std::span<const VertexAttribute> layout, std::span<const std::uint32_t> indices);

	void Draw() const;

	void Bind()   const;
	void Unbind() const;

private:
	unsigned _vao = 0, _vbo = 0, _ebo = 0;
	std::size_t  _indexCount = 0;

	void Release();
};