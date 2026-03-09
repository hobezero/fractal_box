#ifndef FRACTAL_BOX_CORE_ERROR_HANDLING_DIAGNOSTIC_HPP
#define FRACTAL_BOX_CORE_ERROR_HANDLING_DIAGNOSTIC_HPP

#include <functional>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include <fmt/format.h>

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/control_flow.hpp"
#include "fractal_box/core/functional.hpp"
#include "fractal_box/core/init_tags.hpp"
#include "fractal_box/core/platform.hpp"
#include "fractal_box/core/preprocessor.hpp"

namespace fr {

template<class T>
concept c_has_to_string = requires(const T obj) {
	{ to_string(obj) } -> std::convertible_to<std::string>;
};

enum class DiagnosticSeverity: uint8_t {
	Context,
	Warning,
	Error,
};

template<class T>
concept c_diagnosticable
	= c_nothrow_movable<T>
	&& (c_has_to_string<T> || fmt::formattable<T>)
	&& requires(const T obj) {
		{ obj.severity() } -> std::same_as<DiagnosticSeverity>;
	};

class Diagnostic;

namespace detail {

union DiagnosticStorage {
	explicit FR_FORCE_INLINE constexpr
	DiagnosticStorage(UninitializedInit) noexcept { }

	explicit FR_FORCE_INLINE constexpr
	DiagnosticStorage(ZeroInit) noexcept:
		ptr{nullptr}
	{ }

	[[nodiscard]] FR_FORCE_INLINE constexpr
	auto release_ptr() noexcept -> void* {
		auto* tmp = ptr;
		ptr = nullptr;
		return tmp;
	}

public:
	void* ptr;
	alignas(void*) unsigned char buffer[4 * sizeof(void*)];
};

template<class T>
concept c_diagnostic_sbo_suitable
	= c_diagnosticable<T>
	&& sizeof(T) <= sizeof(DiagnosticStorage)
	&& (alignof(DiagnosticStorage) % alignof(T) == 0)
;

struct DiagnosticVTable {
	using DestructiveMoveConstructFn = void (*)(Diagnostic& dest, Diagnostic& src) noexcept;
	using DestructiveMoveAssignFn = void (*)(Diagnostic& dest, Diagnostic& src) noexcept;
	using DestroyFn = void (*)(Diagnostic& dest) noexcept;

	using SwapFn = void (*)(Diagnostic& dest, Diagnostic& src) noexcept;
	using SwapWithHeapFn = void (*)(Diagnostic& dest, Diagnostic& src) noexcept;
	using SwapWithSboFn = void (*)(Diagnostic& dest, Diagnostic& src) noexcept;

	using SeverityFn = auto (*)(const Diagnostic& dest) -> DiagnosticSeverity;
	using FormatFn = auto (*)(const Diagnostic& dest) -> std::string;

public:
	DestructiveMoveConstructFn destructive_move_construct_fn;
	DestructiveMoveAssignFn destructive_move_assign_fn;
	DestroyFn destroy_fn;

	SwapFn swap_fn;
	SwapWithHeapFn swap_with_heap_fn;
	SwapWithSboFn swap_with_sbo_fn;

	SeverityFn severity_fn;
	FormatFn format_fn;
};

template<class T, class Derived>
struct DiagnosticActionsBase;

template<class T>
struct DiagnosticActions;

} // namespace detail

enum class DiagnosticTypeId: uintptr_t { };

class Diagnostic {
public:
	template<class T>
	static FR_FORCE_INLINE
	auto type_id_for() noexcept -> DiagnosticTypeId {
		return static_cast<DiagnosticTypeId>(reinterpret_cast<uintptr_t>(
			&detail::DiagnosticActions<T>::vtable));
	}

	Diagnostic() = default;

	template<class U>
	requires c_diagnosticable<std::remove_cvref_t<U>>
		&& (!std::same_as<std::remove_cvref_t<U>, Diagnostic>)
		&& (!c_in_place_as_init<std::remove_cvref_t<U>>)
	explicit(false)
	Diagnostic(U&& value, const Diagnostic* parent = nullptr):
		_storage{uninitialized},
		_vptr{&detail::DiagnosticActions<std::remove_cvref_t<U>>::vtable},
		_parent{parent}
	{
		using T = std::remove_cvref_t<U>;
		detail::DiagnosticActions<T>::create(_storage, std::forward<U>(value));
	}

	template<class U, class... Args>
	requires c_diagnosticable<U>
		&& std::constructible_from<U, Args...>
	explicit(false)
	Diagnostic(InPlaceAsInit<U>, Args&&... args):
		_storage{uninitialized},
		_vptr{&detail::DiagnosticActions<U>::vtable}
	{
		detail::DiagnosticActions<U>::create(_storage, std::forward<Args>(args)...);
	}

	Diagnostic(const Diagnostic&) = delete;
	auto operator=(const Diagnostic&) -> Diagnostic& = delete;

	Diagnostic(Diagnostic&& other) noexcept:
		_storage{uninitialized},
		_parent{std::exchange(other._parent, nullptr)}
	{
		if (other._vptr) {
			other._vptr->destructive_move_construct_fn(*this, other);
			_vptr = std::exchange(other._vptr, nullptr);
		}
		else {
			_vptr = nullptr;
		}
	}

	auto operator=(Diagnostic&& other) noexcept -> Diagnostic& {
		if (&other == this)
			return *this;

		if (_vptr && other._vptr) {
			if (_vptr == other._vptr) {
				_vptr->destructive_move_assign_fn(*this, other);
				other._vptr = nullptr;
			}
			else {
				_vptr->destroy_fn(*this);
				other._vptr->destructive_move_construct_fn(*this, other);
				_vptr = std::exchange(other._vptr, nullptr);
			}
		}
		else if (_vptr) {
			_vptr->destroy_fn(*this);
			_vptr = nullptr;
		}
		else if (other._vptr) {
			other._vptr->destructive_move_construct_fn(*this, other);
			_vptr = std::exchange(other._vptr, nullptr);
		}

		_parent = std::exchange(other._parent, nullptr);

		return *this;
	}

	~Diagnostic() {
		if (_vptr) {
			_vptr->destroy_fn(*this);
		}
	}

	friend
	void swap(Diagnostic& lhs, Diagnostic& rhs) noexcept {
		if (&lhs == &rhs)
			return;

		if (lhs._vptr && rhs._vptr) {
			lhs._vptr->swap_fn(lhs, rhs);
			using std::swap;
			swap(lhs._vptr, rhs._vptr);
		}
		else if (lhs._vptr) {
			lhs._vptr->destructive_move_construct_fn(rhs, lhs);
			rhs._vptr = std::exchange(lhs._vptr, nullptr);
		}
		else if (rhs._vptr) {
			rhs._vptr->destructive_move_construct_fn(lhs, rhs);
			lhs._vptr = std::exchange(rhs._vptr, nullptr);
		}

		using std::swap;
		swap(lhs._parent, rhs._parent);
	}

	void reset() noexcept {
		if (_vptr) {
			_vptr->destroy_fn(*this);
			_vptr = nullptr;
			_parent = nullptr;
		}
	}

	auto severity() const noexcept -> DiagnosticSeverity {
		FR_ASSERT(_vptr);
		return _vptr->severity_fn(*this);
	}

	auto type_id() const noexcept -> DiagnosticTypeId {
		return static_cast<DiagnosticTypeId>(reinterpret_cast<uintptr_t>(_vptr));
	}

	template<c_diagnosticable T>
	auto is() const noexcept -> bool {
		return type_id() == type_id_for<T>();
	}

	auto parent() const noexcept -> const Diagnostic* { return _parent; }
	auto set_parent(const Diagnostic* parent) noexcept { _parent = parent; }

private:
	template<class T, class Derived>
	friend struct detail::DiagnosticActionsBase;

	template<class T>
	friend struct detail::DiagnosticActions;

private:
	detail::DiagnosticStorage _storage {zero_init};
	const detail::DiagnosticVTable* _vptr = nullptr;
	const Diagnostic* _parent = nullptr;
};

namespace detail {

template<class T, class Derived>
struct DiagnosticActionsBase {
	static
	auto severity_thunk(const Diagnostic& dest) -> DiagnosticSeverity {
		return Derived::get_object(dest._storage)->severity();
	}

	static
	auto format_thunk(const Diagnostic& dest) -> std::string {
		if constexpr (c_has_to_string<T>) {
			return to_string(*Derived::get_object(dest._storage));
		}
		else {
			return fmt::format("{}", *Derived::get_object(dest._storage));
		}
	}
};

template<class T>
struct DiagnosticActions: private DiagnosticActionsBase<T, DiagnosticActions<T>> {
private:
	using Base = DiagnosticActionsBase<T, DiagnosticActions<T>>;

	static
	void destructive_move_construct_thunk(Diagnostic& dest, Diagnostic& src) noexcept {
		dest._storage.ptr = src._storage.release_ptr();
	}

	static
	void destructive_move_assign_thunk(Diagnostic& dest, Diagnostic& src) noexcept {
		FR_ASSERT_AUDIT(dest._vptr == src._vptr && dest._vptr == &vtable);
		destroy(dest._storage);
		dest._storage.ptr = src._storage.release_ptr();
	}

	static
	void destroy_thunk(Diagnostic& dest) noexcept {
		destroy(dest._storage);
	}

	static
	void swap_thunk(Diagnostic& dest, Diagnostic& src) noexcept {
		FR_ASSERT_AUDIT(dest._vptr == &vtable);
		if (src._vptr == &vtable) { // Same type
			using std::swap;
			swap(dest._storage.ptr, src._storage.ptr);
		}
		else {
			// Double dispatch to account for all possibilities
			src._vptr->swap_with_heap_fn(src, dest);
		}
	}

	static
	void swap_with_heap_thunk(Diagnostic& dest, Diagnostic& src) noexcept {
		// Different types, but since both objects are on the heap, we can just swap ptrs
		using std::swap;
		swap(dest._storage.ptr, src._storage.ptr);
	}

	static
	void swap_with_sbo_thunk(Diagnostic& dest, Diagnostic& src) noexcept {
		// Before: dest contains Heap<T>, src contains SBO<Unknown>
		// After: dest contains SBO<Unknown>, src contains Heap<T>
		auto tmp = std::unique_ptr<T>{static_cast<T*>(dest._storage.release_ptr())};
		src._vptr->destructive_move_construct_fn(dest, src);
		src._storage.ptr = tmp.release();
	}

public:
	static constexpr auto vtable = DiagnosticVTable{
		.destructive_move_construct_fn = &destructive_move_construct_thunk,
		.destructive_move_assign_fn = &destructive_move_assign_thunk,
		.destroy_fn = &destroy_thunk,
		.swap_fn = &swap_thunk,
		.swap_with_heap_fn = &swap_with_heap_thunk,
		.swap_with_sbo_fn = &swap_with_sbo_thunk,
		.severity_fn = &Base::severity_thunk,
		.format_fn = &Base::format_thunk
	};

	static FR_FORCE_INLINE
	auto get_object(DiagnosticStorage& storage) noexcept -> T* {
		return static_cast<T*>(storage.ptr);
	}

	static FR_FORCE_INLINE
	auto get_object(const DiagnosticStorage& storage) noexcept -> const T* {
		return static_cast<const T*>(storage.ptr);
	}

	template<class... Args>
	static FR_FORCE_INLINE
	void create(DiagnosticStorage& storage, Args&&... args) {
		auto holder = std::make_unique<T>(std::forward<Args>(args)...);
		storage.ptr = holder.release();
	}

	template<class U>
	static FR_FORCE_INLINE
	void assign(DiagnosticStorage& storage, U&& other) {
		FR_ASSERT_AUDIT(storage.ptr);
		*static_cast<T*>(storage.ptr) = std::forward<U>(other);
	}

	template<class... Args>
	static FR_FORCE_INLINE constexpr
	void emplace(DiagnosticStorage& storage, Args&&... args) {
		FR_ASSERT_AUDIT(storage.ptr);
		std::destroy_at(static_cast<T*>(storage.ptr));
		std::construct_at(static_cast<T*>(storage.ptr), std::forward<Args>(args)...);
	}

	static FR_FORCE_INLINE constexpr
	void destroy(DiagnosticStorage& storage) {
		delete static_cast<T*>(storage.release_ptr());
	}
};

template<c_diagnostic_sbo_suitable T>
struct DiagnosticActions<T>: private DiagnosticActionsBase<T, DiagnosticActions<T>> {
private:
	using Base = DiagnosticActionsBase<T, DiagnosticActions<T>>;

	static
	void destructive_move_construct_thunk(Diagnostic& dest, Diagnostic& src) noexcept {
		create(dest._storage, std::move(*get_object(src._storage)));
		destroy(src._storage);
	}

	static
	void destructive_move_assign_thunk(Diagnostic& dest, Diagnostic& src) noexcept {
		FR_ASSERT_AUDIT(dest._vptr == src._vptr && dest._vptr == &vtable);
		assign(dest._storage, std::move(*get_object(src._storage)));
		destroy(src._storage);
	}

	static
	void destroy_thunk(Diagnostic& dest) noexcept {
		destroy(dest._storage);
	}

	static
	void swap_thunk(Diagnostic& dest, Diagnostic& src) noexcept {
		FR_ASSERT_AUDIT(dest._vptr == &vtable);
		if (src._vptr == &vtable) {
			using std::swap;
			swap(*get_object(dest._storage), *get_object(src._storage));
		}
		else {
			src._vptr->swap_with_sbo_fn(src, dest);
		}
	}

	static
	void swap_with_heap_thunk(Diagnostic& dest, Diagnostic& src) noexcept {
		// Before: dest contains SBO<T>, src contains Heap<Unknown>
		// After: dest contains Heap<Unknown>, src contains SBO<T>
		T tmp = std::move(*get_object(dest._storage));
		destroy(dest._storage);
		// TODO: Try to avoid triple dispatch to improve performance
		src._vptr->destructive_move_construct_fn(dest, src);
		create(src._storage, std::move(tmp));
	}

	static
	void swap_with_sbo_thunk(Diagnostic& dest, Diagnostic& src) noexcept {
		// Before: dest contains SBO<T>, src contains SBO<Unknown>
		// After: dest contains SBO<Unknown>, src contains SBO<T>
		T tmp = std::move(*get_object(dest._storage));
		destroy(dest._storage);
		// TODO: Try to avoid triple dispatch to improve performance
		src._vptr->destructive_move_construct_fn(dest, src);
		create(src._storage, std::move(tmp));
	}

public:
	static constexpr auto vtable = DiagnosticVTable{
		.destructive_move_construct_fn = &destructive_move_construct_thunk,
		.destructive_move_assign_fn = &destructive_move_assign_thunk,
		.destroy_fn = &destroy_thunk,
		.swap_fn = &swap_thunk,
		.swap_with_heap_fn = &swap_with_heap_thunk,
		.swap_with_sbo_fn = &swap_with_sbo_thunk,
		.severity_fn = &Base::severity_thunk,
		.format_fn = &Base::format_thunk
	};

	static FR_FORCE_INLINE
	auto get_object(DiagnosticStorage& storage) noexcept -> T* {
		return std::launder(reinterpret_cast<T*>(&storage.buffer));
	}

	static FR_FORCE_INLINE
	auto get_object(const DiagnosticStorage& storage) noexcept -> const T* {
		return std::launder(reinterpret_cast<const T*>(&storage.buffer));
	}

	template<class... Args>
	static FR_FORCE_INLINE
	void create(DiagnosticStorage& storage, Args&&... args) {
		auto* p = static_cast<T*>(static_cast<void*>(&storage.buffer));
		std::construct_at(p, std::forward<Args>(args)...);
	}

	template<class U>
	static FR_FORCE_INLINE constexpr
	auto assign(DiagnosticStorage& storage, U&& other) {
		auto* p = std::launder(static_cast<T*>(static_cast<void*>(&storage.buffer)));
		*p = std::forward<U>(other);
	}

	template<class... Args>
	static FR_FORCE_INLINE constexpr
	void emplace(DiagnosticStorage& storage, Args&&... args) {
		std::destroy_at(get_object(storage));
		auto* p = static_cast<T*>(static_cast<void*>(&storage.buffer));
		std::construct_at(p, std::forward<Args>(args)...);
	}

	static FR_FORCE_INLINE constexpr
	void destroy(DiagnosticStorage& storage) {
		std::destroy_at(get_object(storage));
		FR_IGNORE(storage.release_ptr());
	}
};

} // namespace detail

class DiagnosticSink;

class DiagnosticFrame {
public:
	explicit
	DiagnosticFrame(DiagnosticSink& sink) noexcept;

	DiagnosticFrame(const DiagnosticFrame&) = delete;
	auto operator=(const DiagnosticFrame&) -> DiagnosticFrame& = delete;

	DiagnosticFrame(DiagnosticFrame&&) = delete;
	auto operator=(DiagnosticFrame&&) -> DiagnosticFrame& = delete;

	~DiagnosticFrame();

	auto warning_count() const noexcept -> size_t;
	auto error_count() const noexcept -> size_t;

	FR_FORCE_INLINE
	auto has_warnings() const noexcept -> bool { return warning_count() == 0zu; }

	FR_FORCE_INLINE
	auto has_errors() const noexcept -> bool { return error_count() == 0zu; }

private:
	DiagnosticSink* _sink;
	size_t _init_warning_count;
	size_t _init_error_count;
};

class DiagnosticObserver {
public:
	explicit
	DiagnosticObserver(const DiagnosticSink& sink) noexcept;

	auto warning_count() const noexcept -> size_t;
	auto error_count() const noexcept -> size_t;

	FR_FORCE_INLINE
	auto has_warnings() const noexcept -> bool { return warning_count() == 0zu; }

	FR_FORCE_INLINE
	auto has_errors() const noexcept -> bool { return error_count() == 0zu; }

private:
	const DiagnosticSink* _sink;
	size_t _init_warning_count;
	size_t _init_error_count;
};

class DiagnosticSink {
public:
	using Handler = std::function<ControlFlow (Diagnostic)>;

	DiagnosticSink(Handler handler):
		_handler{std::move(handler)}
	{ }

	void set_handler(Handler handler) {
		_handler = std::move(handler);
	}

	template<class T>
	requires c_diagnosticable<std::remove_cvref_t<T>>
	auto push(T&& diagnostic) -> ControlFlow {
		switch (diagnostic.severity()) {
			case DiagnosticSeverity::Context:
				FR_PANIC();
			case DiagnosticSeverity::Warning:
				++_warning_count;
				break;
			case DiagnosticSeverity::Error:
				++_error_count;
				break;
		}

		FR_PANIC_CHECK(_handler);
		return _handler(Diagnostic{std::forward<T>(diagnostic), top_frame()});
	}

	template<class T>
	requires c_diagnosticable<std::remove_cvref_t<T>>
	[[nodiscard]]
	auto make_frame(T&& context) -> DiagnosticFrame {
		FR_PANIC_CHECK(context.severity() == DiagnosticSeverity::Context);
		_frame_stack.emplace_back(context, top_frame());
		return DiagnosticFrame{*this};
	}

	auto make_observer() const noexcept -> DiagnosticObserver {
		return DiagnosticObserver{*this};
	}

	void pop_frame() noexcept {
		_frame_stack.pop_back();
	}

	auto top_frame() const noexcept -> const Diagnostic* {
		return _frame_stack.empty() ? nullptr : &_frame_stack.back();
	}

	FR_FORCE_INLINE
	auto warning_count() const noexcept -> size_t { return _warning_count; }

	FR_FORCE_INLINE
	auto error_count() const noexcept -> size_t { return _error_count; }

private:
	std::vector<Diagnostic> _frame_stack;
	Handler _handler;
	size_t _warning_count = 0zu;
	size_t _error_count = 0zu;
};

inline
DiagnosticFrame::DiagnosticFrame(DiagnosticSink& sink) noexcept:
	_sink{&sink},
	_init_warning_count{sink.warning_count()},
	_init_error_count{sink.error_count()}
{ }

inline
auto DiagnosticFrame::warning_count() const noexcept -> size_t {
	return _sink->warning_count() - _init_warning_count;
}

inline
auto DiagnosticFrame::error_count() const noexcept -> size_t {
	return _sink->error_count() - _init_error_count;
}

inline
DiagnosticFrame::~DiagnosticFrame() {
	_sink->pop_frame();
}

inline
DiagnosticObserver::DiagnosticObserver(const DiagnosticSink& sink) noexcept:
	_sink{&sink},
	_init_warning_count{sink.warning_count()},
	_init_error_count{sink.error_count()}
{ }

inline
auto DiagnosticObserver::warning_count() const noexcept -> size_t {
	return _sink->warning_count() - _init_warning_count;
}

inline
auto DiagnosticObserver::error_count() const noexcept -> size_t {
	return _sink->error_count() - _init_error_count;
}

template<class Func>
class StringContext {
public:
	explicit constexpr
	StringContext(Func&& func): _func{std::move(func)} { }

	static constexpr
	auto severity() noexcept -> DiagnosticSeverity { return DiagnosticSeverity::Context; }

	friend
	auto to_string(const StringContext& context) -> std::string { return context._func(); }

private:
	Func _func;
};

template<class F>
StringContext(F&&) -> StringContext<std::remove_cvref_t<F>>;

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

using OldDiagnostic = std::string;

class IDiagnosticSink {
public:
	virtual ~IDiagnosticSink() = default;

	virtual void push(OldDiagnostic diagnostic) = 0;
	virtual size_t size() const noexcept = 0;
	[[nodiscard]] virtual bool empty() const noexcept = 0;
	virtual std::span<const OldDiagnostic> get_all() const noexcept = 0;

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

	void push(OldDiagnostic diagnostic) override {
		_sink->push(_proj(std::move(diagnostic)));
	}

	size_t size() const noexcept override {
		return _sink->size() - _initialCount;
	}

	[[nodiscard]]
	bool empty() const noexcept override {
		return _sink->size() == _initialCount;
	}

	std::span<const OldDiagnostic> get_all() const noexcept override {
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
	void push(OldDiagnostic diagnostic) override {
		_diagnostics.push_back(std::move(diagnostic));
	}

	size_t size() const noexcept override {
		return _diagnostics.size();
	}

	[[nodiscard]]
	bool empty() const noexcept override {
		return _diagnostics.empty();
	}

	std::span<const OldDiagnostic> get_all() const noexcept override {
		return _diagnostics;
	}

private:
	std::vector<OldDiagnostic> _diagnostics;
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
