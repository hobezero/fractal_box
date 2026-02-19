#include "fractal_box/graphics/gl_framebuffer.hpp"

#include "fractal_box/core/preprocessor.hpp"

namespace fr {

// GlRenderbuffer
// --------------

auto GlRenderbuffer::make() noexcept -> ErrorOr<GlRenderbuffer> {
	auto oid = null_native_id;
	glGenRenderbuffers(1, &oid);

	return ErrorOr<GlRenderbuffer>{
		std::in_place,
		adopt,
		oid
	};
}

auto GlRenderbuffer::make_with_storage(
	GlRenderbufferFormat format,
	glm::ivec2 dimensions
) noexcept -> ErrorOr<GlRenderbuffer> {
	auto renderbuffer = make();
	if (!renderbuffer)
		return renderbuffer;

	renderbuffer->bind();
	glRenderbufferStorage(
		GL_RENDERBUFFER,
		std::to_underlying(format),
		dimensions.x,
		dimensions.y
	);

	if (const auto error_flags = get_all_gl_error_flags()) {
		renderbuffer = make_error_fmt(Errc::OpenGlError, "Can't make GlRenderbuffer: failed to "
			"allocate storage (OpenGL error '{}')", error_flags);
		return renderbuffer;
	}
	return renderbuffer;
}

GlRenderbuffer::GlRenderbuffer(AdoptInit, GlObjectId oid) noexcept:
	_oid{oid}
{ }

auto GlRenderbuffer::operator=(GlRenderbuffer&& other) noexcept -> GlRenderbuffer& {
	do_destroy();

	_oid = std::move(other._oid);

	return *this;
}

GlRenderbuffer::~GlRenderbuffer() {
	do_destroy();
}

void GlRenderbuffer::swap(GlRenderbuffer& other) noexcept {
	using std::swap;

	swap(_oid, other._oid);
}

[[nodiscard]]
auto GlRenderbuffer::release() noexcept -> GlObjectId {
	return _oid.release();
}

void GlRenderbuffer::destroy() {
	if (!_oid.is_default()) {
		do_destroy();
		FR_IGNORE(release());
	}
}

void GlRenderbuffer::do_destroy() noexcept {
	if (!_oid.is_default())
		glDeleteRenderbuffers(1, &_oid.value());
}

void GlRenderbuffer::bind() const noexcept {
	if (!_oid.is_default())
		glBindRenderbuffer(GL_RENDERBUFFER, _oid.value());
}

void GlRenderbuffer::unbind() noexcept {
	glBindRenderbuffer(GL_RENDERBUFFER, null_native_id);
}

// GlFramebuffer
// -------------

auto GlFramebuffer::make() noexcept -> ErrorOr<GlFramebuffer> {
	auto oid = null_native_id;
	glGenFramebuffers(1, &oid);
	glBindFramebuffer(GL_FRAMEBUFFER, oid);

	return ErrorOr<GlFramebuffer>{
		std::in_place,
		adopt,
		oid
	};
}

GlFramebuffer::GlFramebuffer(AdoptInit, GlObjectId oid) noexcept:
	_oid{oid}
{ }

auto GlFramebuffer::operator=(GlFramebuffer&& other) noexcept -> GlFramebuffer& {
	do_destroy();

	_oid = std::move(other._oid);

	return *this;
}

GlFramebuffer::~GlFramebuffer() {
	do_destroy();
}

void GlFramebuffer::swap(GlFramebuffer& other) noexcept {
	using std::swap;

	swap(_oid, other._oid);
}

[[nodiscard]]
auto GlFramebuffer::release() noexcept -> GlObjectId {
	return _oid.release();
}

void GlFramebuffer::destroy() {
	if (!_oid.is_default()) {
		do_destroy();
		FR_IGNORE(release());
	}
}

void GlFramebuffer::do_destroy() noexcept {
	if (!_oid.is_default())
		glDeleteFramebuffers(1, &_oid.value());
}

auto GlFramebuffer::attach(
	GlAttachmentPoint point, const GlRenderbuffer& renderbuffer
) -> ErrorOr<> {
	const auto result = _attachments.insert(Attachment{
		.point = point,
		.kind = GlAttachmentKind::Renderbuffer,
		.object_id = renderbuffer.native_id(),
	});
	if (!result)
		return make_error_fmt(Errc::OpenGlError, "GlFramebuffer: Attachment point {} already "
			"occupied by a {} with id = {}", point, result.where->kind, result.where->object_id);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, point.value(), GL_RENDERBUFFER,
		renderbuffer.native_id());
	if (const auto error_flags = get_all_gl_error_flags()) {
		_attachments.erase(result.where);
		return make_error_fmt(Errc::OpenGlError, "GlFramebuffer: failed to attach renderbuffer "
			"(OpenGL error {})", error_flags);
	}

	return {};
}

auto GlFramebuffer::attach(GlAttachmentPoint point, const GlTexture2d& texture) -> ErrorOr<> {
	const auto result = _attachments.insert(Attachment{
		.point = point,
		.kind = GlAttachmentKind::Texture,
		.object_id = texture.native_id(),
	});
	if (!result)
		return make_error_fmt(Errc::OpenGlError, "GlFramebuffer: Attachment point {} already "
			"occupied by a {} with id = {}", point, result.where->kind, result.where->object_id);

	constexpr auto mip_map_level = GLint{0};
	glFramebufferTexture(GL_FRAMEBUFFER, point.value(), texture.native_id(), mip_map_level);
	if (const auto error_flags = get_all_gl_error_flags()) {
		_attachments.erase(result.where);
		return make_error_fmt(Errc::OpenGlError, "GlFramebuffer: failed to attach renderbuffer "
			"(OpenGL error {})", error_flags);
	}

	return {};
}

auto GlFramebuffer::complete() -> ErrorOr<> {
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return make_error(Errc::OpenGlError, "GlFramebuffer failed to complete");
	return {};
}

void GlFramebuffer::bind_default() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GlFramebuffer::bind() const noexcept {
	if (!_oid.is_default())
		glBindFramebuffer(GL_FRAMEBUFFER, _oid.value());
}

void GlFramebuffer::bind_read() const noexcept {
	if (!_oid.is_default())
		glBindFramebuffer(GL_READ_FRAMEBUFFER, _oid.value());
}

void GlFramebuffer::bind_draw() const noexcept {
	if (!_oid.is_default())
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _oid.value());
}

void GlFramebuffer::unbind() noexcept {
	glBindFramebuffer(GL_FRAMEBUFFER, null_native_id);
}

} // namespace fr
