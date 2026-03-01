#ifndef FRACTAL_BOX_CORE_ERROR_HANDLING_DIAGNOSTIC_HPP
#define FRACTAL_BOX_CORE_ERROR_HANDLING_DIAGNOSTIC_HPP

#include <span>
#include <string>
#include <memory>
#include <vector>
// #include <deque>

#include <fmt/format.h>

#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/functional.hpp"
#include "fractal_box/core/init_tags.hpp"
#include "fractal_box/core/platform.hpp"
#include "fractal_box/core/preprocessor.hpp"

namespace fr {

enum class DiagnosticSeverity: uint8_t {
	Context,
	Warning,
	Error,
};

template<class T>
concept c_diagnosticable = c_nothrow_movable<T> && fmt::formattable<T>;

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
	alignas(void*)  unsigned char buffer[3 * sizeof(void*)];
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

	using FormatFn = auto (*)(const Diagnostic& dest) -> std::string;

private:
public:
	DestructiveMoveConstructFn destructive_move_construct_fn;
	DestructiveMoveAssignFn destructive_move_assign_fn;
	DestroyFn destroy_fn;

	SwapFn swap_fn;
	SwapWithHeapFn swap_with_heap_fn;
	SwapWithSboFn swap_with_sbo_fn;

	FormatFn format_fn;
};

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
			&detail::DiagnosticActions<T>::template vtable<T>));
	}

	Diagnostic() = default;

	template<class U>
	requires c_diagnosticable<std::remove_cvref_t<U>>
		&& (!std::same_as<std::remove_cvref_t<U>, Diagnostic>)
		&& (!c_in_place_as_init<std::remove_cvref_t<U>>)
	Diagnostic(U&& value):
		_storage{uninitialized},
		_vptr{&detail::DiagnosticActions<std::remove_cvref_t<U>>
			::template vtable<std::remove_cvref_t<U>>}
	{
		using T = std::remove_cvref_t<U>;
		detail::DiagnosticActions<T>::create(_storage, std::forward<U>(value));
	}

	template<class U, class... Args>
	requires c_diagnosticable<U>
		&& std::constructible_from<U, Args...>
	Diagnostic(InPlaceAsInit<U>, Args&&... args):
		_storage{uninitialized},
		_vptr{&detail::DiagnosticActions<U>::template vtable<U>}
	{
		detail::DiagnosticActions<U>::create(_storage, std::forward<Args>(args)...);
	}

	Diagnostic(const Diagnostic&) = delete;
	auto operator=(const Diagnostic&) -> Diagnostic& = delete;

	Diagnostic(Diagnostic&& other) noexcept:
		_storage{uninitialized},
		_context{std::exchange(other._context, nullptr)}
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

		_context = std::exchange(other._context, nullptr);

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
		// Otherwise, both objects are empty, do nothing
	}

	void reset() noexcept {
		if (_vptr) {
			_vptr->destroy_fn(*this);
			_vptr = nullptr;
			_context = nullptr;
		}
	}

private:
	template<class T>
	friend struct detail::DiagnosticActions;

private:
	detail::DiagnosticStorage _storage {zero_init};
	detail::DiagnosticVTable* _vptr = nullptr;
	Diagnostic* _context = nullptr;
	// std::deque<Diagnostic> _children;
};

namespace detail {

template<class T>
struct DiagnosticActions {
private:
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

	static
	auto format_thunk(const Diagnostic& dest) -> std::string {
		return fmt::format("{}", get_object(dest._storage));
	}

public:
	static constexpr auto vtable = DiagnosticVTable{
		.destructive_move_construct_fn = &destructive_move_construct_thunk,
		.destructive_move_assign_fn = &destructive_move_assign_thunk,
		.destroy_fn = &destroy_thunk,
		.swap_fn = &swap_thunk,
		.swap_with_heap_fn = &swap_with_heap_thunk,
		.swap_with_sbo_fn = &swap_with_sbo_thunk,
		.format_fn = &format_thunk
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
struct DiagnosticActions<T> {
private:
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

	static
	auto format_thunk(const Diagnostic& dest) -> std::string {
		return fmt::format("{}", get_object(dest._storage));
	}

public:
	static constexpr auto vtable = DiagnosticVTable{
		.destructive_move_construct_fn = &destructive_move_construct_thunk,
		.destructive_move_assign_fn = &destructive_move_assign_thunk,
		.destroy_fn = &destroy_thunk,
		.swap_fn = &swap_thunk,
		.swap_with_heap_fn = &swap_with_heap_thunk,
		.swap_with_sbo_fn = &swap_with_sbo_thunk,
		.format_fn = &format_thunk
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
