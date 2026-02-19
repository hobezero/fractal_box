#include "fractal_box/graphics/gl_texture.hpp"

#include <fmt/format.h>

#include "fractal_box/core/enum_utils.hpp"
#include "fractal_box/core/preprocessor.hpp"
#include "fractal_box/core/scope.hpp"

namespace fr {

auto GlTexture2d::make(IDiagnosticSink& error_sink) noexcept -> std::optional<GlTexture2d> {
	GlObjectId oid = null_native_id;
	glGenTextures(1, &oid);
	if (const auto error_flags = get_all_gl_error_flags()) {
		error_sink.push(fmt::format("Can't make GlTexture2d: failed to create texture object "
			"(OpenGL error '{}')", error_flags));
		return std::nullopt;
	}
	return GlTexture2d{adopt, oid, GlTextureParams{}, {}};
}

auto GlTexture2d::make_from_raw_data(
	const GlTextureParams& params,
	std::span<const std::byte> src_data,
	glm::ivec2 src_dimensions,
	GlPixelFormat src_format,
	GlPixelDataType src_data_type,
	IDiagnosticSink& error_sink
) noexcept -> std::optional<GlTexture2d> {
	auto texture = make(error_sink);
	if (!texture)
		return texture;

	texture->bind();
	FR_DEFER [] { GlTexture2d::unbind(); };

	const auto type = to_underlying(type_target);

	glTexParameteri(type, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(type, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(params.num_mipmap_levels - 1));
	FR_GL_ASSERT_FAST();

	glTexImage2D(
		type,
		0, // mip level
		to_underlying(params.format),
		src_dimensions.x,
		src_dimensions.y,
		0, // border, reserved
		to_underlying(src_format),
		to_underlying(src_data_type),
		src_data.data()
	);
	if (const auto error_flags = get_all_gl_error_flags()) {
		error_sink.push(fmt::format("Can't make GlTexture2d: failed to send data "
			"(OpenGL error '{}')", error_flags));
		texture = std::nullopt; // Opt into NRVO
		return texture;
	}
	texture->_dimensions = src_dimensions;

	glTexParameteri(type, GL_TEXTURE_WRAP_S, to_underlying(params.wrapping_s));
	glTexParameteri(type, GL_TEXTURE_WRAP_T, to_underlying(params.wrapping_t));

	glTexParameteri(type, GL_TEXTURE_MIN_FILTER, to_underlying(params.min_filtering));
	glTexParameteri(type, GL_TEXTURE_MAG_FILTER, to_underlying(params.mag_filtering));

	FR_GL_ASSERT_FAST();

	if (params.autogen_mipmaps && params.num_mipmap_levels > 1) {
		glGenerateMipmap(type);
		if (const auto error_flags = get_all_gl_error_flags()) {
			error_sink.push(fmt::format("Can't make GlTexture2d: failed to generate mipmaps "
				"(OpenGL error '{}')", error_flags));
			texture = std::nullopt;
			return texture;
		}
	}

	texture->_params = params;

	return texture;
}

auto GlTexture2d::make_empty(
	const GlTextureParams& params,
	glm::ivec2 src_dimensions,
	GlPixelFormat src_format,
	GlPixelDataType src_data_type,
	IDiagnosticSink& error_sink
) noexcept -> std::optional<GlTexture2d> {
	return make_from_raw_data(
		params,
		std::span<const std::byte>{},
		src_dimensions,
		src_format,
		src_data_type,
		error_sink
	);
}

GlTexture2d::GlTexture2d(
	AdoptInit,
	GlObjectId object_id,
	const GlTextureParams& params,
	glm::ivec2 dimensions
) noexcept:
	_oid{object_id},
	_params{params},
	_dimensions{dimensions}
{ }

GlTexture2d& GlTexture2d::operator=(GlTexture2d&& other) noexcept {
	do_destroy();

	_oid = std::move(other._oid);
	_params = std::move(other._params);
	_dimensions = std::move(other._dimensions);

	return *this;
}

GlTexture2d::~GlTexture2d() {
	do_destroy();
}

void swap(GlTexture2d& lhs, GlTexture2d& rhs) noexcept {
	using std::swap;

	swap(lhs._oid, rhs._oid);
	swap(lhs._params, rhs._params);
	swap(lhs._dimensions, rhs._dimensions);
}

GlObjectId GlTexture2d::release() noexcept {
	_params = {};
	return _oid.release();
}

void GlTexture2d::destroy() noexcept {
	if (!_oid.is_default()) {
		do_destroy();
		FR_IGNORE(release());
	}
}

void GlTexture2d::do_destroy() noexcept {
	if (!_oid.is_default()) {
		glDeleteTextures(1, &_oid.value());
	}
}

void GlTexture2d::bind_by_id(GlObjectId object_id) noexcept {
	glBindTexture(to_underlying(type_target), object_id);
}

void GlTexture2d::bind() const noexcept {
	if (!_oid.is_default()) {
		bind_by_id(*_oid);
	}
}

void GlTexture2d::unbind() noexcept {
	glBindTexture(to_underlying(type_target), null_native_id);
}

} // namespace fr
