#ifndef KEPELR_GRAPHICS_GL_SHADER_PROGRAM_HPP
#define KEPELR_GRAPHICS_GL_SHADER_PROGRAM_HPP

#include <span>
#include <string_view>

#include <glm/mat2x2.hpp>
#include <glm/mat2x3.hpp>
#include <glm/mat2x4.hpp>
#include <glm/mat3x2.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat3x4.hpp>
#include <glm/mat4x2.hpp>
#include <glm/mat4x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/default_utils.hpp"
#include "fractal_box/core/error_handling/diagnostic.hpp"
#include "fractal_box/core/error_handling/status.hpp"
#include "fractal_box/core/init_tags.hpp"
#include "fractal_box/core/ref.hpp"
#include "fractal_box/graphics/gl_common.hpp"
#include "fractal_box/graphics/gl_version.hpp"

namespace fr {

inline
auto str_or(std::string_view str, std::string_view fallback = "UNNAMED") -> std::string_view {
	return str.empty() ? fallback : str;
}

template<class T>
class GlUniform;

class GlMesh;

enum class GlShaderType : GLenum {
	Vertex = GL_VERTEX_SHADER,
	Fragment = GL_FRAGMENT_SHADER,
#if FR_TARGET_GL_DESKTOP || FR_TARGET_GLES3
	Compute = GL_COMPUTE_SHADER, // requires GL 4.3 or ARB_compute_shader
#endif
#if FR_TARGET_GL_DESKTOP
	TessControl = GL_TESS_CONTROL_SHADER, // requires GL 4.0 or ARB_tessellation_shader
	TessEvaluation = GL_TESS_EVALUATION_SHADER, // requires GL 4.0 or ARB_tessellation_shader
	Geometry = GL_GEOMETRY_SHADER,
#endif
};

inline constexpr
auto to_string_view(GlShaderType type) noexcept -> std::string_view {
	using enum GlShaderType;
	switch (type) {
		case Vertex: return "vertex";
		case Fragment: return "fragment";
#if FR_TARGET_GL_DESKTOP || FR_TARGET_GLES3
		case Compute: return "compute";
#endif
#if FR_TARGET_GL_DESKTOP
		case TessControl: return "tesselation control";
		case TessEvaluation: return "tesselation evaluation";
		case Geometry: return "geometry";
#endif
	}
	FR_UNREACHABLE();
}

class MakeGlShaderCreateError: public ErrorBase {
public:
	explicit
	MakeGlShaderCreateError(std::string name): _name(std::move(name)) { }

	friend
	auto to_string(const MakeGlShaderCreateError& self) -> std::string {
		return fmt::format("Can't make GlShader '{}': failed to create shader object", self._name);
	}

	auto name() const noexcept -> const std::string& { return _name; }

private:
	std::string _name;
};

class GlShaderCompilationError: public ErrorBase {
public:
	explicit
	GlShaderCompilationError(std::string message): _message(std::move(message)) { }

	friend
	auto to_string(const GlShaderCompilationError& self) -> std::string {
		return self._message;
	}

	friend
	auto to_string(GlShaderCompilationError&& self) -> std::string {
		return std::move(self)._message;
	}

	auto message() const noexcept -> const std::string& { return _message; }

private:
	std::string _message;
};

class GlShader: public GlObject {
public:
	struct Params {
		NonDefault<GlShaderType> type;
		NonDefault<GlVersion> version;
		NonDefault<std::vector<std::string>> sources;
		NonDefault<std::string> name = std::string {};
	};

	static constexpr GlObjectId null_native_id = 0;
	static constexpr GlShaderType null_type {};

	/// @brief Create a GlShader object
	static
	auto make(GlShaderType type, std::string name, DiagnosticSink& diag_sink)
		-> Status<GlShader>;

	explicit
	GlShader() noexcept = default;

	explicit
	GlShader(
		AdoptInit,
		GLuint oid,
		GlShaderType type,
		std::vector<std::string> sources = {},
		std::string name = {}
	) noexcept;

	GlShader(const GlShader& other) = delete;
	auto operator=(const GlShader& other) -> GlShader& = delete;

	GlShader(GlShader&& other) noexcept = default;
	auto operator=(GlShader&& other) noexcept -> GlShader&;

	~GlShader();

	friend
	void swap(GlShader& lhs, GlShader& rhs) noexcept;

	GlObjectId release() noexcept;

	void destroy() noexcept;

	/// @pre `!sources.empty()`
	void set_sources(GlVersion version, std::vector<std::string> sources);

	[[nodiscard]]
	auto compile(DiagnosticSink& diag_sink) -> Status<> {
		return compile(to_span<Ref<GlShader>>({*this}), diag_sink);
	}

	/// @brief Compile a set of shaders. The compilation is potentionally parallel and is more
	/// efficient than compiling each shader separately
	/// @pre `!shaders.empty()`
	[[nodiscard]] static
	auto compile(std::span<Ref<GlShader>> shaders, DiagnosticSink& diag_sink) -> Status<>;

	bool is_valid() const noexcept { return !_oid.is_default(); }

	GlObjectId native_id() const noexcept { return *_oid; }
	GlShaderType type() const noexcept { return *_type; }
	const auto& name() const noexcept { return _name; }

private:
	void do_destroy() noexcept;

private:
	WithDefaultValue<null_native_id> _oid;
	WithDefaultValue<null_type> _type;
	std::vector<std::string> _sources;
	std::string _name;
};

class WhileCompilingShader: public ContextBase {
public:
	explicit
	WhileCompilingShader(const GlShader& shader):
		_shader{&shader}
	{ }

	friend
	auto to_string(WhileCompilingShader self) -> std::string {
		return fmt::format("While compiling {} shader '{}':", to_string_view(self._shader->type()),
			str_or(self._shader->name()));
	}

	auto shader() const noexcept -> const GlShader& { return *_shader; }

private:
	const GlShader* _shader;
};

class MakeGlShaderProgramCreateError: public ErrorBase {
public:
	explicit
	MakeGlShaderProgramCreateError(std::string name): _name(std::move(name)) { }

	friend
	auto to_string(const MakeGlShaderProgramCreateError& self) -> std::string {
		return fmt::format("Can't make GlShaderProgram '{}': failed to create shader prrogram "
			"object", str_or(self._name));
	}

	auto name() const noexcept -> const std::string& { return _name; }

private:
	std::string _name;
};

class GlShaderProgramLinkingError: public ErrorBase {
public:
	explicit
	GlShaderProgramLinkingError(std::string message): _message(std::move(message)) { }

	friend
	auto to_string(const GlShaderProgramLinkingError& self) -> std::string {
		return self._message;
	}

	friend
	auto to_string(GlShaderProgramLinkingError&& self) -> std::string {
		return std::move(self)._message;
	}

	auto message() const noexcept -> const std::string& { return _message; }

private:
	std::string _message;
};

class GlUniformLocationError: public ErrorBase {
public:
	explicit
	GlUniformLocationError(std::string uniform_name, std::string program_name):
		_uniform_name(std::move(uniform_name)),
		_program_name(std::move(program_name))
	{ }

	friend
	auto to_string(const GlUniformLocationError& self) -> std::string {
		return fmt::format("Failed to get location of uniform '{}' of GlShaderProgram '{}'",
			self._uniform_name, self._program_name);
	}

	auto uniform_name() const noexcept -> const std::string& { return _uniform_name; }
	auto program_name() const noexcept -> const std::string& { return _program_name; }

private:
	std::string _uniform_name;
	std::string _program_name;
};

class GlAttribLocationError: public ErrorBase {
public:
	explicit
	GlAttribLocationError(std::string attrib_name, std::string program_name):
		_attrib_name(std::move(attrib_name)),
		_program_name(std::move(program_name))
	{ }

	friend
	auto to_string(const GlAttribLocationError& self) -> std::string {
		return fmt::format("Failed to get location of attribute '{}' of GlShaderProgram '{}'",
			self._attrib_name, self._program_name);
	}

	auto attrib_name() const noexcept -> const std::string& { return _attrib_name; }
	auto program_name() const noexcept -> const std::string& { return _program_name; }

private:
	std::string _attrib_name;
	std::string _program_name;
};

class GlShaderProgram: public GlObject {
public:
	static constexpr GlObjectId null_native_id = 0;

	/// @brief Create a GlShaderProgram object
	static
	auto make(std::string name, DiagnosticSink& diag_sink) -> Status<GlShaderProgram>;

	/// @brief Create, compile, and link a GlShaderProgram object using the provided shader
	/// parameters
	static
	auto make_linked(
		std::string name,
		std::span<GlShader::Params> params,
		DiagnosticSink& diag_sink
	) -> Status<GlShaderProgram>;

	explicit
	GlShaderProgram() noexcept = default;

	explicit
	GlShaderProgram(AdoptInit, GlObjectId oid, std::string name = {}) noexcept;

	GlShaderProgram(const GlShaderProgram& other) = delete;
	GlShaderProgram& operator=(const GlShaderProgram& other) = delete;

	GlShaderProgram(GlShaderProgram&& other) noexcept = default;
	GlShaderProgram& operator=(GlShaderProgram&& other) noexcept;

	virtual
	~GlShaderProgram();

	friend
	void swap(GlShaderProgram& lhs, GlShaderProgram& rhs) noexcept;

	GlObjectId release() noexcept;
	void destroy() noexcept;

	void attach_shader(GlShader& shader);
	void detach_shader(GlShader& shader);

	[[nodiscard]]
	auto link(DiagnosticSink& diag_sink) -> Status<> {
		return link(to_span<Ref<GlShaderProgram>>({*this}), diag_sink);
	}

	/// @brief Link a set of programs. The linking is potentionally parallel and is more
	/// efficient than linking each program separately
	/// @pre `!program.empty()`
	[[nodiscard]] static
	auto link(
		std::span<Ref<GlShaderProgram>> programs,
		DiagnosticSink& diag_sink
	) -> Status<>;

	auto get_uniform_location(const char* uniform_name, DiagnosticSink& diag_sink) const
		-> Status<GlUniformLocation>;
	auto get_attribute_location(const char* attribute_name, DiagnosticSink& diag_sink) const
		-> Status<GlAttribLocation>;

	static
	void use_by_id(GlObjectId program_id) noexcept;

	void use() const noexcept;

	static
	void unuse() noexcept;

	// scalars
	void set_uniform(GlUniform<GLfloat> uniform, GLfloat value) noexcept;
	void set_uniform(GlUniform<GLint> uniform, GLint value) noexcept;
	void set_uniform(GlUniform<GLuint> uniform, GLuint value) noexcept;
#if FR_TARGET_GL_DESKTOP
	void set_uniform(GlUniform<GLdouble> uniform, GLdouble value) noexcept;
#endif

	// single vector
	template<size_t Dimensions, class T>
	void set_uniform(
		GlUniform<glm::vec<Dimensions, T>> uniform, glm::vec<Dimensions, T> value
	) noexcept {
		set_uniform(uniform, std::span<const glm::vec<Dimensions, T>>(&value, 1));
	}

	// single matrix
	// TODO: refactor
	template<class MatT>
	void set_uniform(GlUniform<MatT> uniform, const MatT& value) noexcept {
		set_uniform(uniform, std::span<const MatT>(&value, 1));
	}

	// float vectors
	void set_uniform(GlUniform<GLfloat> uniform, std::span<const GLfloat> values) noexcept;
	void set_uniform(GlUniform<glm::vec2> uniform, std::span<const glm::vec2> values) noexcept;
	void set_uniform(GlUniform<glm::vec3> uniform, std::span<const glm::vec3> values) noexcept;
	void set_uniform(GlUniform<glm::vec4> uniform, std::span<const glm::vec4> values) noexcept;

	// int vectors
	void set_uniform(GlUniform<GLint> uniform, std::span<const GLint> values) noexcept;
	void set_uniform(GlUniform<glm::ivec2> uniform, std::span<const glm::ivec2> values) noexcept;
	void set_uniform(GlUniform<glm::ivec3> uniform, std::span<const glm::ivec3> values) noexcept;
	void set_uniform(GlUniform<glm::ivec4> uniform, std::span<const glm::ivec4> values) noexcept;

	// unsigned int vectors
	void set_uniform(GlUniform<GLuint> uniform, std::span<const GLuint> values) noexcept;
	void set_uniform(GlUniform<glm::uvec2> uniform, std::span<const glm::uvec2> values) noexcept;
	void set_uniform(GlUniform<glm::uvec3> uniform, std::span<const glm::uvec3> values) noexcept;
	void set_uniform(GlUniform<glm::uvec4> uniform, std::span<const glm::uvec4> values) noexcept;

#if FR_TARGET_GL_DESKTOP
	// double vectors
	void set_uniform(GlUniform<GLdouble> uniform, std::span<const GLdouble> values) noexcept;
	void set_uniform(GlUniform<glm::dvec2> uniform, std::span<const glm::dvec2> values) noexcept;
	void set_uniform(GlUniform<glm::dvec3> uniform, std::span<const glm::dvec3> values) noexcept;
	void set_uniform(GlUniform<glm::dvec4> uniform, std::span<const glm::dvec4> values) noexcept;
#endif

	// TODO: Pass transform parameter for matrix uniforms
	// square float matrices
	void set_uniform(GlUniform<glm::mat2> uniform, std::span<const glm::mat2> values) noexcept;
	void set_uniform(GlUniform<glm::mat3> uniform, std::span<const glm::mat3> values) noexcept;
	void set_uniform(GlUniform<glm::mat4> uniform, std::span<const glm::mat4> values) noexcept;

	// non-square float matrices
	void set_uniform(GlUniform<glm::mat2x3> uniform, std::span<const glm::mat2x3> values) noexcept;
	void set_uniform(GlUniform<glm::mat3x2> uniform, std::span<const glm::mat3x2> values) noexcept;
	void set_uniform(GlUniform<glm::mat2x4> uniform, std::span<const glm::mat2x4> values) noexcept;
	void set_uniform(GlUniform<glm::mat4x2> uniform, std::span<const glm::mat4x2> values) noexcept;
	void set_uniform(GlUniform<glm::mat3x4> uniform, std::span<const glm::mat3x4> values) noexcept;
	void set_uniform(GlUniform<glm::mat4x3> uniform, std::span<const glm::mat4x3> values) noexcept;

#if FR_TARGET_GL_DESKTOP
	// ARB_gpu_shader_fp64 extension is required in OpenGL 3.3 mode

	// square double matrices
	void set_uniform(GlUniform<glm::dmat2> uniform, std::span<const glm::dmat2> values) noexcept;
	void set_uniform(GlUniform<glm::dmat3> uniform, std::span<const glm::dmat3> values) noexcept;
	void set_uniform(GlUniform<glm::dmat4> uniform, std::span<const glm::dmat4> values) noexcept;
	// non-square double matrices
	void set_uniform(GlUniform<glm::dmat2x3> uniform, std::span<const glm::dmat2x3> values) noexcept;
	void set_uniform(GlUniform<glm::dmat3x2> uniform, std::span<const glm::dmat3x2> values) noexcept;
	void set_uniform(GlUniform<glm::dmat2x4> uniform, std::span<const glm::dmat2x4> values) noexcept;
	void set_uniform(GlUniform<glm::dmat4x2> uniform, std::span<const glm::dmat4x2> values) noexcept;
	void set_uniform(GlUniform<glm::dmat3x4> uniform, std::span<const glm::dmat3x4> values) noexcept;
	void set_uniform(GlUniform<glm::dmat4x3> uniform, std::span<const glm::dmat4x3> values) noexcept;
#endif

	void draw(GlMesh& mesh);

	[[nodiscard]]
	auto is_valid() const noexcept -> bool { return !_oid.is_default(); }

	auto oid() const noexcept -> GlObjectId { return *_oid; }
	auto name() const noexcept -> const std::string& { return _name; }

private:
	void do_destroy() noexcept;

private:
	WithDefaultValue<null_native_id> _oid;
	std::string _name;
};

namespace detail {

class GlLocationUnwrapper;

} // namespace detail

class GlCheckedUniformLocation {
public:
	GlCheckedUniformLocation() = delete;

	auto value() const noexcept -> GlUniformLocation { return _value; }

private:
	friend detail::GlLocationUnwrapper;

	explicit
	GlCheckedUniformLocation(GlUniformLocation location): _value{location} { }

private:
	GlUniformLocation _value;
};

namespace detail {

class GlLocationUnwrapper {
public:
	constexpr
	GlLocationUnwrapper(GlShaderProgram& program, DiagnosticSink& error_sink) noexcept:
		_program{&program},
		_error_sink{&error_sink}
	{ }

	auto operator()(const char* name) -> GlCheckedUniformLocation {
		auto loc = _program->get_uniform_location(name, *_error_sink);
		if (!loc)
			return GlCheckedUniformLocation{-1};
		return GlCheckedUniformLocation{*loc};
	}

private:
	GlShaderProgram* _program;
	DiagnosticSink* _error_sink;
};

} // namespace detail

inline
auto make_gl_uniforms_object(
	GlShaderProgram& program, auto factory, DiagnosticSink& error_sink
) -> Status<decltype(factory(std::declval<detail::GlLocationUnwrapper>()))> {
	const auto obs = error_sink.make_observer();
	auto object = factory(detail::GlLocationUnwrapper{program, error_sink});
	if (obs.has_errors())
		return from_error;
	return object;
}

template<class T>
class GlUniform {
public:
	using Type = T;

	GlUniform() = delete;

	explicit(false) constexpr
	GlUniform(GlCheckedUniformLocation loc) noexcept: _location{loc.value()} { }

	explicit constexpr
	GlUniform(AdoptInit, GlUniformLocation location) noexcept:
		_location{location}
	{ }

	explicit(false) constexpr
	GlUniform(UninitializedInit) noexcept: _location{0} { }

	auto location() const noexcept -> GlUniformLocation { return _location; }

private:
	GlUniformLocation _location;
};

} // namespace fr
#endif // include guard
