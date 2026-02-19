#ifndef FRACTAL_BOX_RUNTIME_RUNTIME_HPP
#define FRACTAL_BOX_RUNTIME_RUNTIME_HPP

#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "fractal_box/core/assert_fmt.hpp"
#include "fractal_box/core/containers/sparse_set.hpp"
#include "fractal_box/core/containers/type_object_map.hpp"
#include "fractal_box/core/error.hpp"
#include "fractal_box/core/functional.hpp"
#include "fractal_box/core/init_tags.hpp"
#include "fractal_box/core/logging.hpp"
#include "fractal_box/core/meta/reflection.hpp"
#include "fractal_box/core/timeline.hpp"
#include "fractal_box/core/type_index.hpp"
#include "fractal_box/runtime/message_manager.hpp"
#include "fractal_box/runtime/tracer_profiler.hpp"

namespace fr {

class Runtime;
class AnySystem;
class AnyPhase;

/// @todo Rename to piece (?)
template<class T>
concept c_part = c_user_object<T> && c_maybe_with_inline_name<T>;

template<class T>
concept c_ephemeral_part
	= c_part<T>
	&& T::is_ephemeral_part
	&& (std::constructible_from<T, Runtime&, AnySystem&> || std::constructible_from<T, Runtime&>)
;

template<class T>
concept c_system_param_unqualified = c_user_object<T>;

template<class T>
concept c_system_param_mut
	= c_system_param_unqualified<RemoveConstRef<T>>
	&& (c_lvalue_ref<T> || c_fast_by_value<RemoveConstRef<T>>)
;

template<class T>
concept c_system_param_const
	= c_system_param_unqualified<RemoveConstRef<T>>
	&& c_immutable_param<T> && (c_lvalue_ref<T> || c_fast_by_value<RemoveConstRef<T>>)
	&& !c_ephemeral_part<T>
;

template<class T>
concept c_system_run_result = std::same_as<T, void> || std::same_as<T, ErrorOr<>>;

namespace detail {

template<class T>
inline constexpr auto is_system_param_list = false;

template<c_system_param_mut... Args>
inline constexpr auto is_system_param_list<MpList<Args...>> = true;

template<class T>
inline constexpr auto is_system_param_list_const = false;

template<c_system_param_const... Args>
inline constexpr auto is_system_param_list_const<MpList<Args...>> = true;

} // namespace detail

struct EphemeralPartBase {
	static constexpr auto is_ephemeral_part = true;
};

/// @brief System callback is a function or a function object that takes `c_system_param`s and
/// returns `c_system_run_result`
/// @note Non-static member functions and mutable function objects are not allowed
template<class T>
concept c_system_run_callback
	= c_static_callable<T>
	&& detail::is_system_param_list<typename FuncTraits<T>::Arguments>
	&& c_system_run_result<typename FuncTraits<T>::Result>;

template<class T>
concept c_system_condition_callback
	= c_static_callable<T>
	&& detail::is_system_param_list_const<typename FuncTraits<T>::Arguments>
	&& std::same_as<typename FuncTraits<T>::Result, bool>;

template<class T>
concept c_system
	= c_class<T> && std::is_empty_v<T>
	&& c_maybe_with_inline_name<T>
	&& c_system_run_callback<decltype(T::run)>
	&& (!requires { T::condition; } || c_system_condition_callback<decltype(T::condition)>);

struct SystemTypeIdxDomain: CustomTypeIndexDomainBase<> {
	static constexpr auto null_value = std::numeric_limits<ValueType>::max();
};

using SystemTypeIdx = TypeIndex<SystemTypeIdxDomain>;

using SystemRoutine = auto (*)(Runtime&, AnySystem&) -> ErrorOr<>;
using PhaseRoutine = auto (*)(Runtime&, AnyPhase&) -> ErrorOr<>;
using SystemPredicate = auto (*)(const Runtime&, const AnySystem&) -> bool;

class SystemToken {
public:
	explicit
	SystemToken(AdoptInit, SystemTypeIdx type_idx, const TypeInfo& type_info) noexcept:
		_type_idx{type_idx},
		_type_info{&type_info}
	{ }

	template<c_system System>
	explicit
	SystemToken(InPlaceAsInit<System>) noexcept:
		_type_idx{SystemTypeIdx::of<System>},
		_type_info{&::fr::type_info<System>}
	{ }

	template<c_system System>
	static
	auto make() noexcept -> SystemToken { return SystemToken{in_place_as<System>}; }

	auto type_idx() const noexcept -> SystemTypeIdx { return _type_idx; }
	auto type_info() const noexcept -> const TypeInfo& { return *_type_info; }

private:
	SystemTypeIdx _type_idx;
	const TypeInfo* _type_info;
};

static_assert(c_fast_by_value<SystemToken>);

using RealTickId = Timeline::TickIdType;

struct AppEpochTag;

struct AppClock: Stopwatch<AppClock, AppEpochTag> { };
using AppTickId = AppClock::TickIdType;

class AnySystem {
public:
	enum class HookId: uint32_t { };

	template<c_system System>
	static
	auto make() noexcept -> AnySystem { return AnySystem{in_place_as<System>}; }

	template<c_system System>
	explicit
	AnySystem(InPlaceAsInit<System>) noexcept;

	AnySystem(const AnySystem&) = delete;
	auto operator=(const AnySystem&) -> AnySystem& = delete;

	AnySystem(AnySystem&&) noexcept = default;
	auto operator=(AnySystem&&) noexcept -> AnySystem& = default;

	~AnySystem() = default;

	constexpr
	auto has_condition() const noexcept -> bool { return _condition_fn; }

	auto should_run(Runtime& runtime) const noexcept -> bool {
		return !_condition_fn || _condition_fn(runtime, *this);
	}

	auto invoke_run(Runtime& runtime) -> ErrorOr<> {
		FR_ASSERT(_run_fn);

		for (auto i = 0uz; i < _pre_run_hooks.size(); ++i) {
			if (auto hook_result = _pre_run_hooks.values()[i](runtime, *this); !hook_result) {
				FR_LOG_ERROR("System '{}': pre-run hook #{} failed. {}", _name.str(),
					std::to_underlying(_pre_run_hooks.keys()[i]), hook_result.error());
				// Ignore hook error
			}
		}

		const auto result =  _run_fn(runtime, *this);

		for (auto i = 0uz; i < _post_run_hooks.size(); ++i) {
			if (auto hook_result = _post_run_hooks.values()[i](runtime, *this); !hook_result) {
				FR_LOG_ERROR("System '{}': post-run hook #{} failed. {}", _name.str(),
					std::to_underlying(_post_run_hooks.keys()[i]),  hook_result.error());
				// Ignore hook error
			}
		}
		return result;
	}

	auto type_idx() const noexcept -> SystemTypeIdx { return _type_idx; }
	auto type_info() const noexcept -> const TypeInfo& { return *_type_info; }

	auto make_token() const noexcept -> SystemToken {
		return SystemToken{adopt, _type_idx, *_type_info};
	}

	auto name() const noexcept -> std::string_view { return _name.str(); }
	auto hashed_name() const noexcept -> const HashedStr& { return _name; }

	template<c_mutable Self>
	auto set_name(this Self&& self, std::string name) noexcept -> Self&& {
		self._name = std::move(name);
		return std::forward<Self>(self);
	}

	template<const auto& Condition, c_mutable Self>
	requires c_system_condition_callback<decltype(Condition)>
	auto set_condition(this Self&& self) noexcept -> Self&&;

	auto is_enabled() const noexcept -> bool { return _enabled; }

	template<c_mutable Self>
	auto set_enabled(this Self&& self, bool enabled) noexcept -> Self&& {
		self._enabled = enabled;
		return std::forward<Self>(self);
	}

	auto pre_run_hooks() const noexcept -> const SparseMap<HookId, SystemRoutine>& {
		return _pre_run_hooks;
	}

	auto pre_run_hooks() noexcept -> SparseMap<HookId, SystemRoutine>& {
		return _pre_run_hooks;
	}

	auto post_run_hooks() const noexcept -> const SparseMap<HookId, SystemRoutine>& {
		return _post_run_hooks;
	}

	auto post_run_hooks() noexcept -> SparseMap<HookId, SystemRoutine>& {
		return _post_run_hooks;
	}

	void set_last_invoke_info(Timeline::TickIdType tick_id, SteadyTimePoint tpoint) noexcept {
		_last_real_tick_id = tick_id;
		_last_real_tpoint = tpoint;
	}

	auto last_real_tick_id() const noexcept -> Timeline::TickIdType { return _last_real_tick_id; }
	auto last_real_tpoint() const noexcept -> Timeline::TimePoint { return _last_real_tpoint; }

private:
	SystemTypeIdx _type_idx;
	bool _enabled = true;
	const TypeInfo* _type_info;
	HashedStr _name;
	SystemRoutine _run_fn = nullptr;
	SystemPredicate _condition_fn = nullptr;
	SparseMap<HookId, SystemRoutine> _pre_run_hooks;
	SparseMap<HookId, SystemRoutine> _post_run_hooks;
	/// ID of the last `Timeline` tick at which the system was ran
	Timeline::TickIdType _last_real_tick_id;
	SteadyTimePoint _last_real_tpoint;
};

struct PartTypeIdxDomain: CustomTypeIndexDomainBase<> { };

using PartTypeIdx = TypeIndex<PartTypeIdxDomain>;

struct PhaseTypeIdxDomain: CustomTypeIndexDomainBase<> {
	static constexpr auto null_value = std::numeric_limits<ValueType>::max();
};

using PhaseTypeIdx = TypeIndex<PhaseTypeIdxDomain>;

template<class T>
concept c_phase
	= c_class<T> && std::is_empty_v<T> && !c_mp_list<T>
	&& c_maybe_with_inline_name<T>;

class PhaseToken {
public:
	explicit
	PhaseToken(AdoptInit, PhaseTypeIdx type_idx, const TypeInfo& type_info) noexcept:
		_type_idx{type_idx},
		_type_info{&type_info}
	{ }

	template<c_phase Phase>
	explicit
	PhaseToken(InPlaceAsInit<Phase>) noexcept:
		_type_idx{PhaseTypeIdx::of<Phase>},
		_type_info{&::fr::type_info<Phase>}
	{ }

	template<c_phase Phase>
	static
	auto make() noexcept -> PhaseToken { return PhaseToken{in_place_as<Phase>}; }

	auto type_idx() const noexcept -> PhaseTypeIdx { return _type_idx; }
	auto type_info() const noexcept -> const TypeInfo& { return *_type_info; }

private:
	PhaseTypeIdx _type_idx;
	const TypeInfo* _type_info;
};

static_assert(c_fast_by_value<PhaseToken>);

class AnyPhase {
public:
	enum class HookId: uint32_t { };

	template<c_phase Phase>
	explicit
	AnyPhase(InPlaceAsInit<Phase>) noexcept:
		_type_idx{PhaseTypeIdx::of<Phase>},
		_type_info{&::fr::type_info<Phase>},
		_name{std::string(refl_custom_name<Phase>)}
	{
		if constexpr (requires { typename Phase::IsOneShot; })
			_is_one_shot = bool{typename Phase::IsOneShot{}};
		else
			_is_one_shot = false;
	}

	AnyPhase(const AnyPhase&) = delete;
	auto operator=(const AnyPhase&) -> AnyPhase& = delete;

	AnyPhase(AnyPhase&&) noexcept = default;
	auto operator=(AnyPhase&&) noexcept -> AnyPhase& = default;

	~AnyPhase() = default;

	void append_system(SystemToken system_token) {
		_systems.push_back(system_token);
	}

	void invoke_pre_run_hooks(Runtime& runtime) {
		for (auto i = 0uz; i < _pre_run_hooks.size(); ++i) {
			if (auto hook_result = _pre_run_hooks.values()[i](runtime, *this); !hook_result) {
				FR_LOG_ERROR("Phase '{}': pre-run hook #{} failed. {}", _name.str(),
					std::to_underlying(_pre_run_hooks.keys()[i]), hook_result.error());
				// Ignore hook error
			}
		}
	}

	void invoke_post_run_hooks(Runtime& runtime) {
		for (auto i = 0uz; i < _post_run_hooks.size(); ++i) {
			if (auto hook_result = _post_run_hooks.values()[i](runtime, *this); !hook_result) {
				FR_LOG_ERROR("Phase '{}': post-run hook #{} failed. {}", _name.str(),
					std::to_underlying(_post_run_hooks.keys()[i]),  hook_result.error());
				// Ignore hook error
			}
		}
	}

	[[nodiscard]] constexpr
	auto empty() const noexcept { return _systems.empty(); }

	auto type_idx() const noexcept -> PhaseTypeIdx { return _type_idx; }

	auto type_info() const noexcept -> const TypeInfo& { return *_type_info; }

	auto make_token() const noexcept -> PhaseToken {
		return PhaseToken{adopt, _type_idx, *_type_info};
	}

	auto name() const noexcept -> std::string_view { return _name.str(); }

	auto hashed_name() const noexcept -> const HashedStr& { return _name; }

	template<class Self>
	auto set_name(this Self&& self, HashedStr name) noexcept -> Self&& {
		self._name = std::move(name);
		return std::forward<Self>(self);
	}

	auto systems() const noexcept -> const std::vector<SystemToken>& { return _systems; }

	auto pre_run_hooks() const noexcept -> const SparseMap<HookId, PhaseRoutine>& {
		return _pre_run_hooks;
	}

	auto pre_run_hooks() noexcept -> SparseMap<HookId, PhaseRoutine>& {
		return _pre_run_hooks;
	}

	auto post_run_hooks() const noexcept -> const SparseMap<HookId, PhaseRoutine>& {
		return _post_run_hooks;
	}

	auto post_run_hooks() noexcept -> SparseMap<HookId, PhaseRoutine>& {
		return _post_run_hooks;
	}

	auto is_one_shot() const noexcept -> bool { return _is_one_shot; }

	template<class Self>
	auto set_is_one_shot(this Self&& self, bool value) noexcept -> Self&& {
		self._is_one_shot = value;
		return std::forward<Self>(self);
	}

private:
	PhaseTypeIdx _type_idx;
	const TypeInfo* _type_info;
	HashedStr _name;
	std::vector<SystemToken> _systems;
	SparseMap<HookId, PhaseRoutine> _pre_run_hooks;
	SparseMap<HookId, PhaseRoutine> _post_run_hooks;
	bool _is_one_shot;
};

template<class T>
concept c_preset = c_class<T> && requires(const T preset, Runtime& runtime) {
	preset.build(runtime);
};

struct ProcessArgs {
	std::span<char*> argv;
};

template<class T>
struct RoutineParamTraits;

template<class Invoker>
struct RoutineInvokerTraits {
	static constexpr auto has_default_return = false;
};

template<>
struct RoutineInvokerTraits<std::remove_pointer_t<SystemRoutine>> {
	static constexpr auto has_default_return = true;
};

template<>
struct RoutineInvokerTraits<std::remove_pointer_t<PhaseRoutine>> {
	static constexpr auto has_default_return = true;
};

template<class CallbackType, class InvokerType>
struct RoutineInvoker;

/// TODO: https://godbolt.org/z/zxcG86dT1
template<class CallbackRet, class... CallbackArgs, class InvokerRet, class... InvokerArgs>
struct RoutineInvoker<CallbackRet (CallbackArgs...), InvokerRet (InvokerArgs...)> {
	using Traits = RoutineInvokerTraits<InvokerRet (InvokerArgs...)>;

	template<const auto& Callback>
	static
	auto fn(InvokerArgs... invoker_args) -> InvokerRet {
		// NOTE: Casting temporaries to lvalue references (by `forward_like`) is safe as long as
		// systems don't store these references somewhere that outlives the function call
		if constexpr (
			std::is_void_v<CallbackRet>
			&& !std::is_void_v<InvokerRet>
			&& Traits::has_default_return
		) {
			Callback(std::forward_like<CallbackArgs>(
				RoutineParamTraits<RemoveConstRef<CallbackArgs>>::get(
					std::forward<InvokerArgs>(invoker_args)...
				)
			)...);
			return InvokerRet{};
		}
		else {
			return Callback(std::forward_like<CallbackArgs>(
				RoutineParamTraits<RemoveConstRef<CallbackArgs>>::get(
					std::forward<InvokerArgs>(invoker_args)...
				)
			)...);
		}
	}
};

struct RunSystemConfig {
	bool log_enabled = false;
};

/// @todo
///   TODO: Rename to Framework/Engine?
class Runtime {
public:
	Runtime() = default;

	Runtime(int argc, char* argv[]);

	Runtime(const Runtime&) = delete;
	auto operator=(const Runtime&) -> Runtime& = delete;

	Runtime(Runtime&&) noexcept = default;
	auto operator=(Runtime&&) noexcept -> Runtime& = default;

	~Runtime() = default;

	template<class U>
	requires c_part<std::remove_cvref_t<U>>
	auto add_part(U&& part) -> Runtime& {
		using T = std::remove_cvref_t<U>;
		auto new_part = _parts.try_emplace<T>(std::forward<U>(part));
		FR_PANIC_CHECK_FMT(new_part, "Runtime: part '{}' already exists",
			refl_custom_name<T>);
		FR_LOG_INFO("Runtime: added part '{}' with type_index = {}", refl_custom_name<T>,
			PartTypeIdx::of<T>.value());
		return *this;
	}

	/// @brief Same as `add_part` except don't panic if the part already exists
	template<class U>
	requires c_part<std::remove_cvref_t<U>>
	auto try_add_part(U&& part) -> Runtime& {
		using T = std::remove_cvref_t<U>;
		auto new_part = _parts.try_emplace<T>(std::forward<U>(part));
		if (new_part.success)
			FR_LOG_INFO("Runtime: added part '{}' with type_index = {}", refl_custom_name<T>,
				PartTypeIdx::of<T>.value());
		else
			FR_LOG_INFO("Runtime: part '{}' already exists", refl_custom_name<T>);
		return *this;
	}

	template<class T, class Self>
	auto get_part(this Self& self) noexcept -> CopyConst<T, Self>& {
		auto* ptr = self._parts.template try_get<T>();
		FR_PANIC_CHECK_FMT(ptr, "Runtime: part '{}' doesn't exist", refl_custom_name<T>);
		return *ptr;
	}

	auto add_phase(AnyPhase phase) -> Runtime&;

	template<c_phase Phase>
	auto add_phase() -> Runtime& {
		return add_phase(AnyPhase{in_place_as<Phase>});
	}

	template<c_mp_list PhaseList>
	auto add_phases() -> Runtime& {
		[this]<class... Phases>(MpList<Phases...>) {
			(this->add_phase<Phases>(), ...);
		}(PhaseList{});
		return *this;
	}

	template<c_phase... Phases>
	auto add_phases() -> Runtime& {
		(add_phase<Phases>(), ...);
		return *this;
	}

	auto add_system(PhaseToken phase_token, AnySystem system) -> Runtime&;

	template<c_phase Phase>
	FR_FORCE_INLINE
	auto add_system(AnySystem system) -> Runtime& {
		return add_system(PhaseToken::make<Phase>(), std::move(system));
	}

	template<c_phase Phase, class System>
	FR_FORCE_INLINE
	auto add_system() -> Runtime& {
		return add_system(PhaseToken::make<Phase>(), AnySystem{in_place_as<System>});
	}

	auto set_runner(AnySystem runner) noexcept -> Runtime& {
		_runner = std::move(runner);
		FR_LOG_INFO("Runtime: assigned '{}' as a runner", _runner->name());
		return *this;
	}

	template<c_system System>
	auto set_runner() noexcept -> Runtime& {
		return set_runner(AnySystem{in_place_as<System>});
	}

	template<c_preset Preset>
	auto add_preset(const Preset& preset) -> Runtime& {
		FR_LOG_INFO("Runtime: adding preset '{}'", refl_custom_name<Preset>);
		preset.build(*this);
		return *this;
	}

	auto run_system(SystemToken token, const RunSystemConfig& cfg = {}) -> ErrorOr<bool>;
	auto run_phase(PhaseToken token) -> ErrorOr<>;

	template<c_phase Phase>
	auto run_phase() -> ErrorOr<> {
		return run_phase(PhaseToken::make<Phase>());
	}

	template<class Self>
	auto current_system(this Self& self) noexcept -> CopyConst<AnySystem, Self>* {
		return self._curr_system ? &self.get_system(self._curr_system) : nullptr;
	}

	template<class Self>
	auto current_phase(this Self& self) noexcept -> CopyConst<AnyPhase, Self>* {
		return self._curr_phase ? &self.get_phase(self._curr_phase) : nullptr;
	}

	auto run() -> ErrorOr<>;

	template<const auto& Callback>
	static constexpr
	auto make_system_routine() -> SystemRoutine {
		return &RoutineInvoker<
			typename FuncTraits<decltype(Callback)>::Stripped,
			std::remove_pointer_t<SystemRoutine>
		>::template fn<Callback>;
	}

	template<const auto& Callback>
	static constexpr
	auto make_system_predicate() -> SystemPredicate {
		return &RoutineInvoker<
			typename FuncTraits<decltype(Callback)>::Stripped,
			std::remove_pointer_t<SystemPredicate>
		>::template fn<Callback>;
	}

	template<const auto& Callback>
	static constexpr
	auto make_phase_routine() -> PhaseRoutine {
		return &RoutineInvoker<
			typename FuncTraits<decltype(Callback)>::Stripped,
			std::remove_pointer_t<PhaseRoutine>
		>::template fn<Callback>;
	}

	template<c_phase Phase, class Self>
	auto try_get_phase(
		this Self& self, PhaseTypeIdx idx
	) noexcept -> CopyConst<AnyPhase, Self>* {
		return self._phases.template try_get_at<Phase>(idx);
	}

	template<class Self>
	auto try_get_phase(
		this Self& self, PhaseToken token
	) noexcept -> CopyConst<AnySystem, Self>* {
		return self._phases.try_get_at(token.type_idx());
	}

	template<c_phase Phase, class Self>
	auto try_get_phase(this Self& self) noexcept -> CopyConst<AnyPhase, Self>* {
		return self._phases.template try_get<Phase>();
	}

	template<class Self>
	auto get_phase(
		this Self& self, PhaseTypeIdx idx
	) noexcept -> CopyConst<AnyPhase, Self>& {
		auto* const phase = self._phases.try_get_at(idx);
		FR_PANIC_CHECK_FMT(phase, "Runtime: phase with type_idx '{}' doesn't exist",
			idx.value());
		return *phase;
	}

	template<class Self>
	auto get_phase(
		this Self& self, PhaseToken token
	) noexcept -> CopyConst<AnyPhase, Self>& {
		auto* const phase = self._phases.try_get_at(token.type_idx());
		FR_PANIC_CHECK_FMT(phase, "Runtime: phase '{}' doesn't exist",
			token.type_info().custom_name());
		return *phase;
	}

	template<c_phase Phase, class Self>
	auto get_phase(this Self& self) noexcept -> CopyConst<AnyPhase, Self>& {
		auto* phase = self._phases.template try_get<Phase>();
		FR_PANIC_CHECK_FMT(phase, "Runtime: phase '{}' doesn't exist",
			refl_custom_name<Phase>);
		return *phase;
	}

	template<class Self>
	auto try_get_system(
		this Self& self, SystemTypeIdx idx
	) noexcept -> CopyConst<AnySystem, Self>* {
		return self._systems.try_get_at(idx);
	}

	template<class Self>
	auto try_get_system(
		this Self& self, SystemToken token
	) noexcept -> CopyConst<AnySystem, Self>* {
		return self._systems.try_get_at(token.type_idx());
	}

	template<c_system System, class Self>
	auto try_get_system(this Self& self) noexcept -> CopyConst<AnySystem, Self>* {
		return self._systems.template try_get<System>();
	}

	template<class Self>
	auto get_system(
		this Self& self, SystemTypeIdx idx
	) noexcept -> CopyConst<AnySystem, Self>& {
		auto* const system = self._systems.try_get_at(idx);
		FR_PANIC_CHECK_FMT(system, "Runtime: system with type_idx = '{}' doesn't exist",
			idx.value());
		return *system;
	}

	template<class Self>
	auto get_system(
		this Self& self, SystemToken token
	) noexcept -> CopyConst<AnySystem, Self>& {
		auto* const system = self._systems.try_get_at(token.type_idx());
		FR_PANIC_CHECK_FMT(system, "Runtime: system '{}' doesn't exist",
			token.type_info().custom_name());
		return *system;
	}

	template<c_system System, class Self>
	auto get_system(this Self& self) noexcept -> CopyConst<AnySystem, Self>& {
		auto* const system = self._systems.template try_get<System>();
		FR_PANIC_CHECK_FMT(system, "Runtime: system '{}' doesn't exist",
			refl_custom_name<System>);
		return *system;
	}

	auto message_manager() const noexcept -> const MessageManager& { return _message_manager; }
	auto message_manager() noexcept -> MessageManager& { return _message_manager; }

	auto tracer() const noexcept -> const Tracer& { return _tracer; }
	auto tracer() noexcept -> Tracer& { return _tracer; }

	auto real_clock() const noexcept -> const Timeline& { return _real_clock; }
	auto real_clock() noexcept -> Timeline& { return _real_clock; }

private:
	// TODO: Make sure that parts are destroyed in the reverse order of construction
	TypeAnyObjectMap<PartTypeIdxDomain> _parts;
	TypeObjectMap<PhaseTypeIdxDomain, AnyPhase> _phases;
	TypeObjectMap<SystemTypeIdxDomain, AnySystem> _systems;
	std::optional<AnySystem> _runner;
	PhaseTypeIdx _curr_phase;
	SystemTypeIdx _curr_system;
	MessageManager _message_manager;
	Tracer _tracer;
	Timeline _real_clock;
};

template<c_system System>
inline
AnySystem::AnySystem(InPlaceAsInit<System>) noexcept:
	_type_idx{SystemTypeIdx::of<System>},
	_type_info{&::fr::type_info<System>},
	_name{std::string(refl_custom_name<System>)},
	_run_fn{Runtime::make_system_routine<System::run>()}
{
	if constexpr (requires { System::condition; })
		_condition_fn = Runtime::make_system_predicate<System::condition>();
}

template<const auto& Condition, c_mutable Self>
requires c_system_condition_callback<decltype(Condition)>
auto AnySystem::set_condition(this Self&& self) noexcept -> Self&& {
	self._condition_fn = Runtime::make_system_predicate<Condition>();
	return std::forward<Self>(self);
}

template<>
struct RoutineParamTraits<Runtime> {
	template<c_maybe_const_of<Runtime> TRuntime>
	static FR_FORCE_INLINE
	auto get(TRuntime& runtime, auto&) noexcept -> TRuntime& {
		return runtime;
	}
};

template<>
struct RoutineParamTraits<AnySystem> {
	template<c_maybe_const_of<Runtime> TRuntime, c_maybe_const_of<AnySystem> TAnySystem>
	static FR_FORCE_INLINE
	auto get(TRuntime&, TAnySystem& system) noexcept -> TAnySystem& {
		return system;
	}
};
template<>
struct RoutineParamTraits<AnyPhase> {
	template<c_maybe_const_of<Runtime> TRuntime, c_maybe_const_of<AnySystem> TAnySystem>
	static FR_FORCE_INLINE
	auto get(TRuntime& runtime, TAnySystem& system) noexcept -> CopyConst<AnyPhase, TRuntime>& {
		return unwrap_fmt(runtime.current_phase(),
			"Can't construct AnyPhase parameter: system '{}' routine invoked outside of any phase",
			system.name());
	}

	template<c_maybe_const_of<Runtime> TRuntime, c_maybe_const_of<AnyPhase> TAnyPhase>
	static FR_FORCE_INLINE
	auto get(TRuntime&, TAnyPhase& phase) noexcept -> TAnyPhase& {
		return phase;
	}
};

template<>
struct RoutineParamTraits<MessageManager> {
	template<c_maybe_const_of<Runtime> TRuntime>
	static FR_FORCE_INLINE
	auto get(TRuntime& runtime, auto&) noexcept -> CopyConst<MessageManager, TRuntime>& {
		return runtime.message_manager();
	}
};

template<>
struct RoutineParamTraits<Tracer> {
	template<c_maybe_const_of<Runtime> TRuntime>
	static FR_FORCE_INLINE
	auto get(TRuntime& runtime, auto&) noexcept -> CopyConst<Tracer, TRuntime>& {
		return runtime.tracer();
	}
};

template<>
struct RoutineParamTraits<Timeline> {
	template<c_maybe_const_of<Runtime> TRuntime>
	static FR_FORCE_INLINE
	auto get(TRuntime& runtime, auto&) noexcept -> CopyConst<Timeline, TRuntime>& {
		return runtime.real_clock();
	}
};

template<class M>
struct RoutineParamTraits<MessageQueue<M>> {
	template<c_maybe_const_of<Runtime> TRuntime>
	static FR_FORCE_INLINE
	auto get(TRuntime& runtime, auto&) noexcept -> CopyConst<MessageQueue<M>, TRuntime>& {
		return runtime.message_manager().template get_queue_of<M>();
	}
};

template<class... Ms>
struct RoutineParamTraits<MessageReader<Ms...>> {
	/// @note `MessageReader` provides mutable access, so no const version
	static FR_FORCE_INLINE
	auto get(Runtime& runtime, auto&) -> MessageReader<Ms...> {
		return runtime.message_manager().template make_reader<Ms...>();
	}
};

template<class... Ms>
struct RoutineParamTraits<MessageWriter<Ms...>> {
	/// @note `MessageWriter` provides mutable access, so no const version
	static FR_FORCE_INLINE
	auto get(Runtime& runtime, auto&) -> MessageWriter<Ms...> {
		return runtime.message_manager().template make_writer<Ms...>();
	}
};

template<c_part Part>
struct RoutineParamTraits<Part> {
	template<c_maybe_const_of<Runtime> TRuntime>
	static FR_FORCE_INLINE
	auto get(TRuntime& runtime, auto&) noexcept -> CopyConst<Part, TRuntime>& {
		return runtime.template get_part<Part>();
	}
};

template<c_ephemeral_part Part>
struct RoutineParamTraits<Part> {
	/// @note Ephemeral parts may or may not provide mutable access, but only mutable version is
	/// supported for now
	/// @note No ephemeral parts for phase routines yet
	static FR_FORCE_INLINE
	auto get(Runtime& runtime, AnySystem& system) -> Part {
		if constexpr (std::constructible_from<Part, Runtime&, AnySystem&>) {
			return Part{runtime, system};
		}
		else  if constexpr (std::constructible_from<Part, Runtime&>) {
			return Part{runtime};
		}
		else
			static_assert(false, "Can't construct ephemeral part");
	}
};

} // namespace fr
#endif // include guard
