#ifndef FRACTAL_BOX_RESOURCES_IMAGE_IO_HPP
#define FRACTAL_BOX_RESOURCES_IMAGE_IO_HPP

#include <string>
#include <string_view>

#include <fmt/format.h>
#include <cmrc/cmrc.hpp>

#include "fractal_box/core/error_handling/diagnostic.hpp"
#include "fractal_box/core/error_handling/status.hpp"
#include "fractal_box/graphics/gl_texture.hpp"

namespace fr {

class LoadingTextureAsset: public ContextBase {
public:
	explicit
	LoadingTextureAsset(std::string_view file_name) noexcept: _file_name{file_name} { }

	friend
	auto to_string(LoadingTextureAsset self) -> std::string {
		return fmt::format("While loading texture asset '{}':", self._file_name);
	}

	auto file_name() const noexcept -> std::string_view { return _file_name; }

private:
	std::string_view _file_name;
};

class StbFailure: public ErrorBase {
public:
	explicit
	StbFailure(const char* reason) noexcept: _reason{reason} { }

	friend
	auto to_string(StbFailure self) -> std::string {
		return fmt::format("STB failure: {}", self._reason);
	}

private:
	const char* _reason;
};

class FileNotFoundError: public ErrorBase {
public:
	explicit
	FileNotFoundError(std::string_view file_name) noexcept: _file_name{file_name} { }

	friend
	auto to_string(FileNotFoundError self) -> std::string {
		return fmt::format("File '{}' not found", self._file_name);
	}

	auto file_name() const noexcept -> std::string_view { return _file_name; }

private:
	std::string_view _file_name;
};

class UnsupportedImageChannelsError: public ErrorBase {
public:
	explicit
	UnsupportedImageChannelsError(int channel_count) noexcept: _channel_count{channel_count} { }

	friend
	auto to_string(UnsupportedImageChannelsError self) -> std::string {
		return fmt::format("Unsupported number of channels ({})", self._channel_count);
	}

private:
	int _channel_count;
};

auto make_texture2d_from_resources(
	cmrc::embedded_filesystem assets_fs,
	const std::string& file_name,
	const GlTextureParams& texture_params,
	DiagnosticSink& diag_sink
) -> Status<GlTexture2d>;

inline
void try_init_texture(
	GlTexture2d& target_texture,
	cmrc::embedded_filesystem fs,
	const std::string& file_name,
	const GlTextureParams& params,
	DiagnosticSink& diag_sink
) {
	auto texture = make_texture2d_from_resources(fs, file_name,
		params, diag_sink);
	if (texture)
		target_texture = std::move(*texture);
}

} // namespace fr
#endif // include guard
