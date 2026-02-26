#ifndef FRACTAL_BOX_GRAPHICS_MESH_UTILS_HPP
#define FRACTAL_BOX_GRAPHICS_MESH_UTILS_HPP

#include "fractal_box/graphics/gl_mesh.hpp"
#include "fractal_box/math/math.hpp"

namespace fr {

inline constexpr auto attrib_pos = GlAttribLabel::make<glm::vec2>("pos");
inline constexpr auto attrib_tex_coords = GlAttribLabel::make<glm::vec2>("tex_coords");

/// Yes, technically using struct as an array is UB
struct UvVertex {
	glm::vec2 pos;
	glm::vec2 tex_coords;
};

inline constexpr auto make_uv_layout_interleaved = []{
	return GlBufferLayout {
		unknown_gl_vertex_count,
		{GlAttribBlock::interleaved({attrib_pos, attrib_tex_coords})},
	};
};

/// But we assure that the compiler doesn't add paddings or change alignment for no reason
static_assert(sizeof(UvVertex) == sizeof(float[4]));
static_assert(alignof(UvVertex) == alignof(float));

inline constexpr UvVertex mesh_line_data[] = {
	{{-0.5f, 0.f}, {0.f, 0.f}},
	{{ 0.5f, 0.f}, {1.f, 1.f}}
};

inline constexpr UvVertex mesh_square_solid_data[] = {
	{{-0.5f, -0.5f}, {0.0f, 0.0f}}, // bottom-left
	{{ 0.5f, -0.5f}, {1.0f, 0.0f}}, // bottom-right
	{{ 0.5f,  0.5f}, {1.0f, 1.0f}}, // top-right

	{{-0.5f, -0.5f}, {0.0f, 0.0f}}, // bottom-left
	{{ 0.5f,  0.5f}, {1.0f, 1.0f}}, // top-right
	{{-0.5f,  0.5f}, {0.0f, 1.0f}}  // top-left
};

inline constexpr UvVertex mesh_square_wire_data[] = {
	{{-0.5f, -0.5f}, {0.0f, 0.0f}}, // bottom-left
	{{ 0.5f, -0.5f}, {1.0f, 0.0f}}, // bottom-right
	{{ 0.5f,  0.5f}, {1.0f, 1.0f}}, // top-right
	{{-0.5f,  0.5f}, {0.0f, 1.0f}}  // top-left
};

inline
auto make_circle_solid_mesh_data(float radius, size_t numSegments) -> std::vector<UvVertex> {
	auto data = std::vector<UvVertex>(numSegments + 2);
	data[0] = {{0.f, 0.f}, {0.5f, 0.5f}};
	for (size_t i = 0; i < numSegments; ++i) {
		const float phi = static_cast<float>(i) / static_cast<float>(numSegments) * f_tau;
		const float p_cos = std::cos(phi);
		const float p_sin = std::sin(phi);
		data[i + 1] = {
			{p_cos * radius, p_sin * radius},
			{0.5f * p_cos + 0.5f, 0.5f * p_sin + 0.5f}
		};
	}
	data[numSegments + 1] = data[1];
	return data;
}

inline
auto make_circle_wire_mesh_data(float radius, size_t num_segments) -> std::vector<UvVertex> {
	auto data = std::vector<UvVertex>(num_segments);
	for (size_t i = 0; i < num_segments; ++i) {
		const float phi = static_cast<float>(i) / static_cast<float>(num_segments) * f_tau;
		const float p_cos = std::cos(phi);
		const float p_sin = std::sin(phi);
		data[i] = {
			{p_cos * radius, p_sin * radius},
			{0.5f * p_cos + 0.5f, 0.5f * p_sin + 0.5f}
		};
	}
	return data;
}

inline
void try_init_mesh(
	GlMesh& target_mesh,
	GlPrimitive primitive,
	std::span<const UvVertex> vertices,
	IDiagnosticSink& error_sink
) {
	auto buffer = GlVertexBuffer::make_from_raw_data(std::as_bytes(vertices),
		make_uv_layout_interleaved(), GlBufferUsage::StaticDraw, error_sink);
	if (!buffer)
		return;

	const auto buffer_id = buffer->native_id();
	auto mesh = GlMesh::make(
		primitive,
		{std::make_shared<GlVertexBuffer>(*std::move(buffer))},
		{
			{
				.label = attrib_pos,
				.array_buffer_id = buffer_id,
				.location = 0,
				.offset = offsetof(UvVertex, pos),
				.stride = sizeof(UvVertex)
			},
			{
				.label = attrib_tex_coords,
				.array_buffer_id = buffer_id,
				.location = 1,
				.offset = offsetof(UvVertex, tex_coords),
				.stride = sizeof(UvVertex)
			},
		},
		{},
		{.count = static_cast<GLsizei>(vertices.size())},
		error_sink
	);

	if (mesh)
		target_mesh = *std::move(mesh);
}

} // namespace fr
#endif
