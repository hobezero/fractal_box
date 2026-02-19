#include "fractal_box/resources/image_io.hpp"

#include <optional>
#include <span>

#include <fmt/format.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "fractal_box/core/functional.hpp"
#include "fractal_box/resources/resource_utils.hpp"

namespace fr {
namespace {

using RawImageHandle = std::unique_ptr<std::byte, FuncLifter<&stbi_image_free>>;

class RawImageData {
public:
	/// @pre Both `dimensions` and `channel_count` values are positive
	RawImageData(RawImageHandle handle, glm::ivec2 dimensions, int channel_couont):
		_handle{std::move(handle)},
		_dimensions{dimensions},
		_channel_count{channel_couont}
	{
		// stbi should never return bogus negative values, assert just to be sure
		FR_PANIC_CHECK(_dimensions.x > 0 && _dimensions.y > 0);
		FR_PANIC_CHECK(_channel_count > 0);
	}

	auto as_bytes() const noexcept -> std::span<const std::byte> {
		static constexpr size_t bytes_per_channel = 1;
		const size_t length
			= static_cast<size_t>(_dimensions.x) * static_cast<size_t>(_dimensions.y)
				* static_cast<size_t>(_channel_count) * bytes_per_channel; // size in bytes
		return {_handle.get(), length};
	}

	auto pixel_format() const noexcept -> std::optional<GlPixelFormat> {
		using enum GlPixelFormat;
		switch (_channel_count) {
			case 1: return Red;
			case 2: return RG;
			case 3: return RGB;
			case 4: return RGBA;
		}
		return std::nullopt;
	}

	auto pixel_data_type() const noexcept -> GlPixelDataType {
		return GlPixelDataType::UByte;
	}

	auto dimensions() const noexcept -> glm::ivec2 { return _dimensions; }
	auto channel_count() const noexcept -> int { return _channel_count; }

private:
	RawImageHandle _handle;
	/// @brief Width and height in pixels
	glm::ivec2 _dimensions;
	/// @brief Number of channels (R, G, B, A)
	int _channel_count{};
};

auto read_raw_image_data(
	std::span<const std::byte> buffer,
	IDiagnosticSink& error_sink
) -> std::optional<RawImageData> {
	glm::ivec2 size;
	int channel_count {};
	// Always zero. Any non-zero value will force the number of channels in the output buffer
	constexpr int desired_channels = 0;
	RawImageHandle handle {reinterpret_cast<std::byte*>(stbi_load_from_memory(
		reinterpret_cast<const stbi_uc*>(buffer.data()), static_cast<int>(buffer.size()),
		&size.x, &size.y,
		&channel_count, desired_channels
	))};
	if (!handle) {
		error_sink.push(stbi_failure_reason());
		return std::nullopt;
	}
	return RawImageData{std::move(handle), size, channel_count};
}

} // namespace

auto make_texture2d_from_resources(
	cmrc::embedded_filesystem assets_fs,
	const std::string& file_name,
	const GlTextureParams& texture_params,
	IDiagnosticSink& error_sink
) -> std::optional<GlTexture2d> {
	DiagnosticSinkSlice loading_errors {error_sink, [&] (const Diagnostic& error) {
		return fmt::format("Failed to load texture asset '{}': {}", file_name, error);
	}};
	const auto file_data = try_get_resource_data(assets_fs, file_name);
	if (!file_data) {
		loading_errors.push("file not found");
		return std::nullopt;
	}

	DiagnosticSinkSlice decoding_errors {error_sink, [&] (const Diagnostic& error) {
		return fmt::format("Failed to load texture asset '{}': can't decode image: {}",
			file_name, error);
	}};
	auto raw_img_data = read_raw_image_data(std::as_bytes(*file_data), decoding_errors);
	if (!raw_img_data)
		return std::nullopt;
	const auto pixel_format = raw_img_data->pixel_format();
	if (!pixel_format) {
		decoding_errors.push(fmt::format("unsupported number of channels ({})",
		raw_img_data->channel_count()));
		return std::nullopt;
	}

	return GlTexture2d::make_from_raw_data(
		texture_params,
		raw_img_data->as_bytes(),
		raw_img_data->dimensions(),
		*pixel_format,
		raw_img_data->pixel_data_type(),
		loading_errors
	);
}

} // namespace fr
