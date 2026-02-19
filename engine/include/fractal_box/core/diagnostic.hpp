#ifndef FRACTAL_BOX_CORE_DIAGNOSTIC_HPP
#define FRACTAL_BOX_CORE_DIAGNOSTIC_HPP

#include <span>
#include <string>
#include <vector>

#include "fractal_box/core/functional.hpp"
#include "fractal_box/core/logging.hpp"
#include "fractal_box/core/platform.hpp"

namespace fr {

/// @todo
///   TODO: Support the ability to declare that only one error is possible
///   TODO: Typed diagnostic sink: restrict the type of errors
///   TODO: SinkSlice: a span-like thin pointer into Sink to allow locally inspect the errors
///         which only occured within the scope of SinkSlice. Also remove `clear`?
///   TODO: Generic Diagnostic class: errc, pointer to error domain (category), user data.
///         Possible aligmnet issues
///   TODO: Evaluate the usage ergonomics. What about interoperability with `expected`? What are the
///         guidelines regarding `Sink` vs `expected`
///   TODO: Move at least one virtual method definition out-of-line to some .cpp file. The idea is
///         to reduce object file bloat and improve link times since the compiler won't generate the
///         vtable for every TU.

using Diagnostic = std::string;

class IDiagnosticSink {
public:
	virtual ~IDiagnosticSink() = default;

	virtual void push(Diagnostic diagnostic) = 0;
	virtual size_t size() const noexcept = 0;
	[[nodiscard]] virtual bool empty() const noexcept = 0;
	virtual std::span<const Diagnostic> get_all() const noexcept = 0;

protected:
	IDiagnosticSink() = default;

	IDiagnosticSink(const IDiagnosticSink& other) = default;
	IDiagnosticSink& operator=(const IDiagnosticSink& other) = default;

	IDiagnosticSink(IDiagnosticSink&& other) = default;
	IDiagnosticSink& operator=(IDiagnosticSink&& other) = default;
};

/// @note Inheriting from the interface enables passing a `DiagnosticSinkSlice` object to functions
/// that expect a `IDiagnosticSink` which allows us to inspect errors that occured during only
/// one call
template<class DiagnosticProj = Identity>
class DiagnosticSinkSlice final: public IDiagnosticSink {
public:
	explicit DiagnosticSinkSlice(IDiagnosticSink& sink, DiagnosticProj proj = {}) noexcept
		: _sink{&sink}
		, _initialCount{sink.size()}
		, _proj{std::move(proj)}
	{ }

	void push(Diagnostic diagnostic) override {
		_sink->push(_proj(std::move(diagnostic)));
	}

	size_t size() const noexcept override {
		return _sink->size() - _initialCount;
	}

	[[nodiscard]]
	bool empty() const noexcept override {
		return _sink->size() == _initialCount;
	}

	std::span<const Diagnostic> get_all() const noexcept override {
		return _sink->get_all().subspan(_initialCount);
	}

private:
	IDiagnosticSink* _sink;
	size_t _initialCount;
	FR_NO_UNIQUE_ADDRESS DiagnosticProj _proj;
};

template<class DiagnosticProj>
DiagnosticSinkSlice(IDiagnosticSink&, DiagnosticProj) -> DiagnosticSinkSlice<DiagnosticProj>;

class DiagnosticStore: public IDiagnosticSink {
public:
	void push(Diagnostic diagnostic) override {
		_diagnostics.push_back(std::move(diagnostic));
	}

	size_t size() const noexcept override {
		return _diagnostics.size();
	}

	[[nodiscard]]
	bool empty() const noexcept override {
		return _diagnostics.empty();
	}

	std::span<const Diagnostic> get_all() const noexcept override {
		return _diagnostics;
	}

private:
	std::vector<Diagnostic> _diagnostics;
};


/// @brief Sink every result (an `std::expected`-like object) that is an error
template<typename... T>
requires (sizeof...(T) > 1)
inline  bool sinkErrors(IDiagnosticSink& sink, T&&... results) {
	bool ok = true;
	// Expands into:
	// (ok = ok && result1, result1) || (sink.add(std::move(result1.error()), false));
	// ...
	// (ok = ok && resultN, resultN) || (sink.add(std::move(resultN.error()), false));
	(static_cast<void>(
		((ok = ok && results) || results) || (sink.push(std::move(results).error()), false)),
	...);
	return ok;
}

} // namespace fr
#endif // include guard
