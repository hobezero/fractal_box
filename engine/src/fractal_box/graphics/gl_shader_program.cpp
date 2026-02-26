#include "fractal_box/graphics/gl_shader_program.hpp"

#include <algorithm>

#include <glm/gtc/type_ptr.hpp>

#include "fractal_box/core/meta/meta.hpp"
#include "fractal_box/graphics/gl_mesh.hpp"

namespace fr {

using namespace std::string_view_literals;

namespace {

std::string_view strOr(const auto& str, std::string_view fallback = "UNNAMED") {
	return str.empty() ? fallback : str;
}

} // namespace

auto GlShader::make(
	GlShaderType type, std::string name, IDiagnosticSink& error_sink
) -> std::optional<GlShader> {
	const auto oid = glCreateShader(to_underlying(type));
	if (oid == null_native_id) {
		error_sink.push(fmt::format("Can't make GlShader '{}': failed to create shader object",
			strOr(name)));
		return std::nullopt;
	}
	return GlShader{adopt, oid, type, {},
		(name.empty() ? fmt::format("#{}", oid) : std::move(name))};
}

GlShader::GlShader(AdoptInit,
	GLuint oid,
	GlShaderType type,
	std::vector<std::string> sources,
	std::string name
) noexcept
	: _oid{oid}
	, _type{type}
	, _sources(std::move(sources))
	, _name(std::move(name))
{ }

GlShader& GlShader::operator=(GlShader&& other) noexcept {
	do_destroy();

	_oid = std::move(other._oid);
	_type = std::move(other._type);
	_sources = std::move(other._sources);
	_name = std::move(other._name);

	return *this;
}

GlShader::~GlShader()  {
	do_destroy();
}

void swap(GlShader& lhs, GlShader& rhs) noexcept {
	using std::swap;

	swap(lhs._oid, rhs._oid);
	swap(lhs._type, rhs._type);
	swap(lhs._sources, rhs._sources);
	swap(lhs._name, rhs._name);
}

GlObjectId GlShader::release() noexcept {
	_type.release();
	_sources.clear();
	_name.clear();
	return _oid.release();
}

void GlShader::destroy() noexcept {
	if (!_oid.is_default()) {
		do_destroy();
		release();
	}
}

void GlShader::do_destroy() noexcept {
	if (!_oid.is_default()) {
		glDeleteShader(*_oid);
	}
}

void GlShader::setSources(GlVersion version, std::vector<std::string> sources) {
	FR_ASSERT(!sources.empty());
	if (version != GlVersion::Unspecified) {
		sources.emplace(sources.begin(), gl_shader_version_line(version));
	}
	// TODO: Fix line numbers
	_sources = std::move(sources);
}

bool GlShader::compile(
	std::span<Ref<GlShader>> shaders,
	IDiagnosticSink& error_sink,
	IDiagnosticSink& warning_sink
) {
#if FR_ASSERT_ENABLED
	FR_ASSERT_CHECK(!shaders.empty());
	for (Ref<GlShader> shader : shaders)
		FR_ASSERT_CHECK(!shader->_sources.empty());
#endif
	DiagnosticSinkSlice myErrors{error_sink};
	const auto addError = [&error_sink] (Ref<GlShader> shader, std::string_view message) {
		error_sink.push(fmt::format("Compilation of {} shader '{}' failed: {}",
			to_string_view(shader->type()), strOr(shader->name()), strOr(message, "unknown error")
		));
	};

	const auto addWarning = [&warning_sink] (Ref<GlShader> shader, std::string_view message) {
		warning_sink.push(fmt::format("Warning while compiling {} shader '{}': {}",
			to_string_view(shader->type()), strOr(shader->name()), strOr(message, "unknown warning")
		));
	};

	const size_t max_num_parts = std::ranges::max_element(shaders,
		[](Ref<GlShader> lhs, Ref<GlShader> rhs) {
			return lhs->_sources.size() < rhs->_sources.size();
		}
	)->get()._sources.size();

	std::vector<const GLchar*> pointers(max_num_parts);
	std::vector<GLint> sizes(max_num_parts);

	for (Ref<GlShader> shader : shaders) {
		const size_t num_parts = shader->_sources.size();
		FR_ASSERT(num_parts <= static_cast<size_t>(std::numeric_limits<GLsizei>::max()));
		for (size_t i = 0; i < num_parts; ++i) {
			pointers[i] = static_cast<const GLchar*>(shader->_sources[i].data());
			const size_t len = shader->_sources[i].size();
			FR_ASSERT(len <= static_cast<size_t>(std::numeric_limits<GLint>::max()));
			sizes[i] = static_cast<GLint>(len);
			// TODO: Convert Unicode characters in WebGL mode
		}
		glShaderSource(shader->native_id(), static_cast<GLsizei>(num_parts), pointers.data(),
			sizes.data());
	}

	for (Ref<GlShader> shader : shaders) {
		glCompileShader(shader->native_id());
	}

	for (Ref<GlShader> shader : shaders) {
		GLint success = GL_FALSE;
		GLint log_length = 0;
		glGetShaderiv(shader->native_id(), GL_COMPILE_STATUS, &success);
		glGetShaderiv(shader->native_id(), GL_INFO_LOG_LENGTH, &log_length);
		if (log_length < 0) {
			addWarning(shader, fmt::format("failed to retrieve info log: "
				"negative length {} reported", log_length));
			log_length = 0;
		}

		std::string message(static_cast<size_t>(log_length), '\0');
		if (log_length > 0) {
			glGetShaderInfoLog(shader->native_id(), log_length, &log_length, &message[0]);
			message.resize(static_cast<size_t>(log_length - 1));
		}

		if (success == GL_TRUE) {
			if (!message.empty())
				addWarning(shader, message);
		}
		else {
			addError(shader, message);
		}
	}

	return myErrors.empty();
}

auto GlShaderProgram::make(
	std::string name, IDiagnosticSink& error_sink
) -> std::optional<GlShaderProgram> {
	const auto oid = glCreateProgram();
	if (oid == null_native_id) {
		error_sink.push(fmt::format("Can't make GlShaderProgram '{}': failed to create "
			"shader program object", strOr(name)));
		return std::nullopt;
	}
	return GlShaderProgram{adopt, oid,
		(name.empty() ? fmt::format("#{}", oid) : std::move(name))};
}

auto GlShaderProgram::makeLinked(
	std::string name,
	std::span<GlShader::Params> params,
	IDiagnosticSink& error_sink,
	IDiagnosticSink& warning_sink
) -> std::optional<GlShaderProgram> {
	DiagnosticSinkSlice myErrors{error_sink, [&](const Diagnostic& diagnostic) {
		return fmt::format("Can't make GlShaderProgram '{}': {}", strOr(name), diagnostic);
	}};
	DiagnosticSinkSlice myWarnings{warning_sink, [&](const Diagnostic& diagnostic) {
		return fmt::format("Warning while making GlShaderProgram '{}': {}", strOr(name), diagnostic);
	}};

	const size_t num_shaders = params.size();
	std::vector<GlShader> shaders(num_shaders);
	for (size_t i = 0; i < num_shaders; ++i) {
		auto result = GlShader::make(*params[i].type, *std::move(params[i].name), myErrors);
		if (!result)
			continue;
		shaders[i] = *std::move(result);
		shaders[i].setSources(*params[i].version, *std::move(params[i].sources));
	}
	if (!myErrors.empty())
		return std::nullopt;

	// TODO: remove this allocation
	std::vector<Ref<GlShader>> shader_refs;
	shader_refs.reserve(num_shaders);
	for (size_t i = 0; i < num_shaders; ++i)
		shader_refs.push_back(Ref{shaders[i]});

	auto compRes = GlShader::compile(shader_refs, myErrors, myWarnings);
	if (!compRes)
		return std::nullopt;

	auto program = make(std::move(name), error_sink);
	if (!program)
		return std::nullopt;

	for (auto& shader : shaders)
		program->attachShader(shader);

	auto linkRes = program->link(myErrors, myWarnings);
	if (!linkRes)
		return std::nullopt;

	FR_ASSERT(program->isValid());
	return std::move(*program);
}

GlShaderProgram::GlShaderProgram(AdoptInit, GlObjectId oid, std::string name) noexcept
	: _oid{oid}
	, _name(std::move(name))
{ }

GlShaderProgram& GlShaderProgram::operator=(GlShaderProgram&& other) noexcept {
	destroy();

	_oid = std::move(other._oid);
	_name = std::move(other._name);

	FR_ASSERT(!other.isValid());
	return *this;
}

GlShaderProgram::~GlShaderProgram()  {
	destroy();
}

void swap(GlShaderProgram& lhs, GlShaderProgram& rhs) noexcept {
	using std::swap;

	swap(lhs._oid, rhs._oid);
	swap(lhs._name, rhs._name);
}

GlObjectId GlShaderProgram::release() noexcept {
	_name.clear();
	return _oid.release();
}

void GlShaderProgram::destroy() noexcept {
	if (!_oid.is_default()) {
		do_destroy();
		release();
	}
}

void GlShaderProgram::do_destroy() noexcept {
	if (!_oid.is_default()) {
		glDeleteProgram(*_oid);
	}
}

void GlShaderProgram::attachShader(GlShader& shader) {
	FR_ASSERT(!_oid.is_default());
	FR_ASSERT(shader.native_id() != GlShader::null_native_id);

	glAttachShader(*_oid, shader.native_id());
}

void GlShaderProgram::detachShader(GlShader& shader) {
	FR_ASSERT(!_oid.is_default());
	FR_ASSERT(shader.native_id() != GlShader::null_native_id);

	glDetachShader(*_oid, shader.native_id());
}

bool GlShaderProgram::link(
	std::span<Ref<GlShaderProgram>> programs,
	IDiagnosticSink& error_sink,
	IDiagnosticSink& warning_sink
) {
	FR_ASSERT(!programs.empty());

	DiagnosticSinkSlice myErrors{error_sink};
	const auto addError = [&error_sink] (Ref<GlShaderProgram> program, std::string_view message) {
		error_sink.push(fmt::format("Linking of GlShaderProgram '{}' failed: {}",
			strOr(program->name()), strOr(message, "unknown error")
		));
	};
	const auto addWarning = [&warning_sink] (Ref<GlShaderProgram> program, std::string_view message) {
		warning_sink.push(fmt::format("Warning while linking GlShaderProgram '{}': {}",
			strOr(program->name()), strOr(message, "unknown warning")
		));
	};

	for (Ref<GlShaderProgram> program : programs) {
		glLinkProgram(program->oid());
	}

	for (Ref<GlShaderProgram> program : programs) {
		GLint success = GL_FALSE;
		GLint logLength = 0;
		glGetProgramiv(program->oid(), GL_LINK_STATUS, &success);
		glGetProgramiv(program->oid(), GL_INFO_LOG_LENGTH, &logLength);

		if (logLength < 0) {
			addWarning(program, fmt::format("failed to retrieve info log: "
				"negative length {} reported", logLength));
			logLength = 0;
		}

		std::string message(static_cast<size_t>(logLength), '\0');
		if (logLength > 0) {
			glGetProgramInfoLog(program->oid(), logLength, &logLength, message.data());
			message.resize(static_cast<size_t>(logLength - 1));
		}

		if (success == GL_TRUE) {
			if (!message.empty())
				addWarning(program, message);
		}
		else {
			// Add an error even if the message is empty
			addError(program, message);
		}
	}

	return myErrors.empty();
}

auto GlShaderProgram::getUniformLocation(
	const char* uniform_name, IDiagnosticSink& error_sink
) const -> std::optional<GlUniformLocation> {
	FR_ASSERT(uniform_name);
	FR_ASSERT(!_oid.is_default());

	GlUniformLocation location = glGetUniformLocation(*_oid, uniform_name);
	if (location == -1) {
		error_sink.push(fmt::format("Failed to get location of uniform '{}' of GlShaderProgram '{}'",
			uniform_name, _name));
		return std::nullopt;
	}
	return location;
}

auto GlShaderProgram::getAttributeLocation(
	const char* attributeName, IDiagnosticSink& error_sink
) const -> std::optional<GlUniformLocation> {
	FR_ASSERT(attributeName);
	FR_ASSERT(!_oid.is_default());

	GlAttribLocation location = glGetAttribLocation(*_oid, attributeName);
	if (location == -1) {
		error_sink.push(fmt::format("Failed to get location of attribute '{}' of GlShaderProgram '{}'",
			attributeName, _name));
		return std::nullopt;
	}
	return location;
}

void GlShaderProgram::useById(GlObjectId programId) noexcept {
	glUseProgram(programId);
}

void GlShaderProgram::use() const noexcept {
	FR_ASSERT(!_oid.is_default());
	glUseProgram(*_oid);
}

void GlShaderProgram::unuse() noexcept {
	glUseProgram(null_native_id);
}

// set_uniform: singular scalar values

void GlShaderProgram::set_uniform(GlUniform<GLfloat> uniform, GLfloat value) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniform1f(uniform.location(), value);
}

void GlShaderProgram::set_uniform(GlUniform<GLint> uniform, GLint value) noexcept {
	glUniform1i(uniform.location(), value);
}

void GlShaderProgram::set_uniform(GlUniform<GLuint> uniform, GLuint value) noexcept {
	glUniform1ui(uniform.location(), value);
}

#if FR_TARGET_GL_DESKTOP
void GlShaderProgram::set_uniform(GlUniform<GLdouble> uniform, GLdouble value) noexcept {
	glUniform1d(uniform.location(), value);
}
#endif

// set_uniform: float vectors

void GlShaderProgram::set_uniform(
	GlUniform<GLfloat> uniform, std::span<const GLfloat> values
) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniform1fv(uniform.location(), static_cast<GLsizei>(values.size()), values.data());
}

void GlShaderProgram::set_uniform(
	GlUniform<glm::vec2> uniform, std::span<const glm::vec2> values
) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniform2fv(uniform.location(), static_cast<GLsizei>(values.size()),
		glm::value_ptr(values.front()));
}

void GlShaderProgram::set_uniform(
	GlUniform<glm::vec3> uniform, std::span<const glm::vec3> values
) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniform3fv(uniform.location(), static_cast<GLsizei>(values.size()),
		glm::value_ptr(values.front()));
}

void GlShaderProgram::set_uniform(
	GlUniform<glm::vec4> uniform, std::span<const glm::vec4> values
) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniform4fv(uniform.location(), static_cast<GLsizei>(values.size()),
		glm::value_ptr(values.front()));
}

// set_uniform: int vectors

void GlShaderProgram::set_uniform(
	GlUniform<GLint> uniform, std::span<const GLint> values
) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniform1iv(uniform.location(), static_cast<GLsizei>(values.size()), values.data());
}

void GlShaderProgram::set_uniform(
	GlUniform<glm::ivec2> uniform, std::span<const glm::ivec2> values
) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniform2iv(uniform.location(), static_cast<GLsizei>(values.size()),
		glm::value_ptr(values.front()));
}

void GlShaderProgram::set_uniform(
	GlUniform<glm::ivec3> uniform, std::span<const glm::ivec3> values
) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniform3iv(uniform.location(), static_cast<GLsizei>(values.size()),
		glm::value_ptr(values.front()));
}

void GlShaderProgram::set_uniform(
	GlUniform<glm::ivec4> uniform, std::span<const glm::ivec4> values
) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniform4iv(uniform.location(), static_cast<GLsizei>(values.size()),
		glm::value_ptr(values.front()));
}

// set_uniform: unsigned int vectors

void GlShaderProgram::set_uniform(
	GlUniform<GLuint> uniform, std::span<const GLuint> values
) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniform1uiv(uniform.location(), static_cast<GLsizei>(values.size()), values.data());
}

void GlShaderProgram::set_uniform(
	GlUniform<glm::uvec2> uniform, std::span<const glm::uvec2> values
) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniform2uiv(uniform.location(), static_cast<GLsizei>(values.size()),
		glm::value_ptr(values.front()));
}

void GlShaderProgram::set_uniform(
	GlUniform<glm::uvec3> uniform, std::span<const glm::uvec3> values
) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniform3uiv(uniform.location(), static_cast<GLsizei>(values.size()),
		glm::value_ptr(values.front()));
}

void GlShaderProgram::set_uniform(
	GlUniform<glm::uvec4> uniform, std::span<const glm::uvec4> values
) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniform4uiv(uniform.location(), static_cast<GLsizei>(values.size()),
		glm::value_ptr(values.front()));
}

#if FR_TARGET_GL_DESKTOP
// set_uniform: double vectors

void GlShaderProgram::set_uniform(GlUniform<GLdouble> uniform, std::span<const GLdouble> values
) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniform1dv(uniform.location(), static_cast<GLsizei>(values.size()), values.data());
}

void GlShaderProgram::set_uniform(
	GlUniform<glm::dvec2> uniform, std::span<const glm::dvec2> values
) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniform2dv(uniform.location(), static_cast<GLsizei>(values.size()),
		glm::value_ptr(values.front()));
}

void GlShaderProgram::set_uniform(
	GlUniform<glm::dvec3> uniform, std::span<const glm::dvec3> values
) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniform3dv(uniform.location(), static_cast<GLsizei>(values.size()),
		glm::value_ptr(values.front()));
}

void GlShaderProgram::set_uniform(
	GlUniform<glm::dvec4> uniform, std::span<const glm::dvec4> values
) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniform4dv(uniform.location(), static_cast<GLsizei>(values.size()),
		glm::value_ptr(values.front()));
}
#endif

// square float matrices
void GlShaderProgram::set_uniform(
	GlUniform<glm::mat2> uniform, std::span<const glm::mat2> values
) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniformMatrix2fv(uniform.location(), static_cast<GLsizei>(values.size()), GL_FALSE,
		glm::value_ptr(values.front()));
}

void GlShaderProgram::set_uniform(
	GlUniform<glm::mat3> uniform, std::span<const glm::mat3> values
) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniformMatrix3fv(uniform.location(), static_cast<GLsizei>(values.size()), GL_FALSE,
		glm::value_ptr(values.front()));
}

void GlShaderProgram::set_uniform(
	GlUniform<glm::mat4> uniform, std::span<const glm::mat4> values
) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniformMatrix4fv(uniform.location(), static_cast<GLsizei>(values.size()), GL_FALSE,
		glm::value_ptr(values.front()));
}

// non-square float matrices
void GlShaderProgram::set_uniform(
	GlUniform<glm::mat2x3> uniform, std::span<const glm::mat2x3> values
) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniformMatrix2x3fv(uniform.location(), static_cast<GLsizei>(values.size()), GL_FALSE,
		glm::value_ptr(values.front()));
}

void GlShaderProgram::set_uniform(
	GlUniform<glm::mat3x2> uniform, std::span<const glm::mat3x2> values
) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniformMatrix3x2fv(uniform.location(), static_cast<GLsizei>(values.size()), GL_FALSE,
		glm::value_ptr(values.front()));
}

void GlShaderProgram::set_uniform(
	GlUniform<glm::mat2x4> uniform, std::span<const glm::mat2x4> values
) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniformMatrix2x4fv(uniform.location(), static_cast<GLsizei>(values.size()), GL_FALSE,
		glm::value_ptr(values.front()));
}

void GlShaderProgram::set_uniform(
	GlUniform<glm::mat4x2> uniform, std::span<const glm::mat4x2> values
) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniformMatrix4x2fv(uniform.location(), static_cast<GLsizei>(values.size()), GL_FALSE,
		glm::value_ptr(values.front()));
}

void GlShaderProgram::set_uniform(
	GlUniform<glm::mat3x4> uniform, std::span<const glm::mat3x4> values
) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniformMatrix3x4fv(uniform.location(), static_cast<GLsizei>(values.size()), GL_FALSE,
		glm::value_ptr(values.front()));
}

void GlShaderProgram::set_uniform(
	GlUniform<glm::mat4x3> uniform, std::span<const glm::mat4x3> values
) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniformMatrix4x3fv(uniform.location(), static_cast<GLsizei>(values.size()), GL_FALSE,
		glm::value_ptr(values.front()));
}

#if FR_TARGET_GL_DESKTOP
// square double matrices

void GlShaderProgram::set_uniform(
	GlUniform<glm::dmat2> uniform, std::span<const glm::dmat2> values
) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniformMatrix2dv(uniform.location(), static_cast<GLsizei>(values.size()), GL_FALSE,
		glm::value_ptr(values.front()));
}

void GlShaderProgram::set_uniform(
	GlUniform<glm::dmat3> uniform, std::span<const glm::dmat3> values
) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniformMatrix3dv(uniform.location(), static_cast<GLsizei>(values.size()), GL_FALSE,
		glm::value_ptr(values.front()));
}

void GlShaderProgram::set_uniform(
	GlUniform<glm::dmat4> uniform, std::span<const glm::dmat4> values
) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniformMatrix4dv(uniform.location(), static_cast<GLsizei>(values.size()), GL_FALSE,
		glm::value_ptr(values.front()));
}

// non-square double matrices
void GlShaderProgram::set_uniform(
	GlUniform<glm::dmat2x3> uniform, std::span<const glm::dmat2x3> values
) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniformMatrix2x3dv(uniform.location(), static_cast<GLsizei>(values.size()), GL_FALSE,
		glm::value_ptr(values.front()));
}

void GlShaderProgram::set_uniform(
	GlUniform<glm::dmat3x2> uniform, std::span<const glm::dmat3x2> values
) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniformMatrix3x2dv(uniform.location(), static_cast<GLsizei>(values.size()), GL_FALSE,
		glm::value_ptr(values.front()));
}

void GlShaderProgram::set_uniform(
	GlUniform<glm::dmat2x4> uniform, std::span<const glm::dmat2x4> values
) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniformMatrix2x4dv(uniform.location(), static_cast<GLsizei>(values.size()), GL_FALSE,
		glm::value_ptr(values.front()));
}

void GlShaderProgram::set_uniform(
	GlUniform<glm::dmat4x2> uniform, std::span<const glm::dmat4x2> values
) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniformMatrix4x2dv(uniform.location(), static_cast<GLsizei>(values.size()), GL_FALSE,
		glm::value_ptr(values.front()));
}

void GlShaderProgram::set_uniform(
	GlUniform<glm::dmat3x4> uniform, std::span<const glm::dmat3x4> values
) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniformMatrix3x4dv(uniform.location(), static_cast<GLsizei>(values.size()), GL_FALSE,
		glm::value_ptr(values.front()));
}

void GlShaderProgram::set_uniform(
	GlUniform<glm::dmat4x3> uniform, std::span<const glm::dmat4x3> values
) noexcept {
	FR_ASSERT(!_oid.is_default());
	glUniformMatrix4x3dv(uniform.location(), static_cast<GLsizei>(values.size()), GL_FALSE,
		glm::value_ptr(values.front()));
}

void GlShaderProgram::draw(GlMesh& mesh) {
	FR_ASSERT(!_oid.is_default());

	use();
	mesh.drawWithBoundShader();
}

#endif
} // namespace fr
