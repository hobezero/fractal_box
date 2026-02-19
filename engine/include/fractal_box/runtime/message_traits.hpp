#ifndef FRACTAL_BOX_RUNTIME_MESSAGE_TRAITS_HPP
#define FRACTAL_BOX_RUNTIME_MESSAGE_TRAITS_HPP

#include <compare>
#include <limits>
#include <type_traits>

#include "fractal_box/core/concepts.hpp"
#include "fractal_box/core/int_types.hpp"
#include "fractal_box/core/meta/meta_basics.hpp"
#include "fractal_box/core/platform.hpp"

namespace fr {

struct MessageTtl {
	using ValueType = int16_t;

	enum Enum: ValueType {
		Tombstone = 0,
		OneTick,
		TwoTicks,
		Persistent = std::numeric_limits<ValueType>::max(),
		Default = TwoTicks,
	};

	MessageTtl() = default;

	explicit(false) FR_FORCE_INLINE constexpr
	MessageTtl(Enum v) noexcept: value{v} { }

	explicit FR_FORCE_INLINE constexpr
	MessageTtl(ValueType v) noexcept: value{v} { }

	auto operator<=>(this MessageTtl, MessageTtl) = default;

	constexpr FR_FORCE_INLINE
	auto is_alive() const noexcept { return this->value > 0; }

	constexpr FR_FORCE_INLINE
	auto is_dead() const noexcept { return this->value <= 0; }

	constexpr FR_FORCE_INLINE
	auto operator--() noexcept -> MessageTtl& {
		if (this->value != Persistent)
			--this->value;
		return *this;
	}

	constexpr FR_FORCE_INLINE
	auto operator-=(ValueType count) noexcept -> MessageTtl& {
		if (this->value != Persistent)
			this->value = static_cast<ValueType>(this->value - count);
		return *this;
	}

public:
	ValueType value = Tombstone;
};

template<class T>
concept c_message
	= c_user_object<T>
	&& !c_mp_list<T>
	&& c_nothrow_movable<T>
	&& (!requires { T::ttl; } || std::convertible_to<decltype(T::ttl), MessageTtl>);

template<c_message M>
struct MessageTraits {
	using TickAt = typename M::TickAt;

	static constexpr auto default_ttl = [] -> MessageTtl {
		if constexpr (requires { M::ttl; })
			return M::ttl;
		else
			return MessageTtl::Default;
	}();
};

} // namespace fr
#endif
