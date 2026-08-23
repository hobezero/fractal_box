#include "fractal_box/core/relocation.hpp"

#include <array>
#include <memory>
#include <utility>

#include <fmt/format.h>

#include <catch2/catch_test_macros.hpp>

#include "test_common/test_helpers.hpp"

namespace {

struct TrivialAggregate {
	[[maybe_unused]] friend constexpr
	auto operator==(const TrivialAggregate&, const TrivialAggregate&) -> bool = default;

public:
	int x = 0;
	int y = 0;
};

static_assert(std::is_trivially_copyable_v<TrivialAggregate>);

/// Per-object move/destroy counters. Unlike `frt::SmallCallSpy`/`frt::LargeCallSpy`
/// (which record into a `constinit` global and therefore can't be touched during constant
/// evaluation), these are meant to be owned by the caller as a plain local variable, so spies
/// pointing at them work both at compile time and at runtime
struct FuncCallStats {
	int move_ctor_count = 0;
	int dtor_count = 0;
};

/// Move-only and NOT trivially copyable (user-provided special members), so it always
/// takes the `move_and_destroy` path in `relocate_at`, never the `memmove` one
struct MoveOnlySpy {
	constexpr
	MoveOnlySpy(FuncCallStats& c, int v) noexcept: stats{&c}, value{v} { }

	MoveOnlySpy(const MoveOnlySpy&) = delete;
	auto operator=(const MoveOnlySpy&) -> MoveOnlySpy& = delete;
	auto operator=(MoveOnlySpy&&) noexcept -> MoveOnlySpy& = delete;

	constexpr
	MoveOnlySpy(MoveOnlySpy&& other) noexcept:
		stats{other.stats},
		value{std::exchange(other.value, -1)}
	{
		if (stats)
			++this->stats->move_ctor_count;
	}

	constexpr
	~MoveOnlySpy() {
		if (stats)
			++this->stats->dtor_count;
	}

public:
	FuncCallStats* stats = nullptr;
	int value = 0;
};

static_assert(std::move_constructible<MoveOnlySpy>);
static_assert(!std::is_trivially_copyable_v<MoveOnlySpy>);
static_assert(std::is_nothrow_move_constructible_v<MoveOnlySpy>);
static_assert(std::is_nothrow_destructible_v<MoveOnlySpy>);

/// @brief Structurally the same as `MoveOnlySpy`, but opts into trivial relocation via the
/// customization point
struct CustomTriviallyRelocatable {
	explicit constexpr
	CustomTriviallyRelocatable(int v) noexcept: value{v} { }

	CustomTriviallyRelocatable(const CustomTriviallyRelocatable&) = delete;
	auto operator=(const CustomTriviallyRelocatable&) -> CustomTriviallyRelocatable& = delete;
	auto operator=(CustomTriviallyRelocatable&&) noexcept -> CustomTriviallyRelocatable& = delete;

	constexpr
	CustomTriviallyRelocatable(CustomTriviallyRelocatable&& other) noexcept:
		value{std::exchange(other.value, -1)}
	{ }

	constexpr
	~CustomTriviallyRelocatable() = default;

	friend
	auto fr_custom_is_trivially_relocatable(const CustomTriviallyRelocatable&) -> fr::TrueC;

public:
	int value = 0;
};

static_assert(std::move_constructible<CustomTriviallyRelocatable>);
static_assert(!std::is_trivially_copyable_v<CustomTriviallyRelocatable>);

/// Opts into trivial relocation via the customization point, but is NOT move-constructible nor
/// copy-constructible
struct NonMovableTaggedTrivial {
	NonMovableTaggedTrivial() = default;
	NonMovableTaggedTrivial(const NonMovableTaggedTrivial&) = delete;
	NonMovableTaggedTrivial(NonMovableTaggedTrivial&&) = delete;
	auto operator=(const NonMovableTaggedTrivial&) -> NonMovableTaggedTrivial& = delete;
	auto operator=(NonMovableTaggedTrivial&&) noexcept -> NonMovableTaggedTrivial& = delete;

	friend
	auto fr_custom_is_trivially_relocatable(const NonMovableTaggedTrivial&) -> fr::TrueC;
};

static_assert(!std::move_constructible<NonMovableTaggedTrivial>);

/// Thrown by `ThrowOnMoveSpy`'s move constructor
struct BoomException { };

/// @brief Move-constructible, but the move can be configured to throw. Used to exercise the
/// exception-safety cleanup logic in `move_and_destroy`, `relocate_at`, `uninitialized_relocate`.
/// Not trivially copyable, so it always goes through `move_and_destroy`, both at compile time
/// and at runtime - there's no consteval/runtime divergence
struct ThrowOnMoveSpy {
	constexpr
	ThrowOnMoveSpy(FuncCallStats& s, int v, bool should_throw = false) noexcept:
		stats{&s},
		value{v},
		throw_on_move{should_throw}
	{}

	ThrowOnMoveSpy(const ThrowOnMoveSpy&) = delete;
	auto operator=(const ThrowOnMoveSpy&) -> ThrowOnMoveSpy& = delete;
	auto operator=(ThrowOnMoveSpy&&) noexcept -> ThrowOnMoveSpy& = delete;

	constexpr
	ThrowOnMoveSpy(ThrowOnMoveSpy&& other):
		stats{other.stats},
		value{other.value},
		throw_on_move{false}
	{
		if (other.throw_on_move)
			throw BoomException{};
		if (this->stats)
			++this->stats->move_ctor_count;
	}

	constexpr
	~ThrowOnMoveSpy() {
		if (this->stats)
			++this->stats->dtor_count;
	}

public:
	FuncCallStats* stats = nullptr;
	int value = 0;
	bool throw_on_move = false;
};

static_assert(std::move_constructible<ThrowOnMoveSpy>);
static_assert(!std::is_nothrow_move_constructible_v<ThrowOnMoveSpy>);
static_assert(!std::is_trivially_copyable_v<ThrowOnMoveSpy>);

} // namespace

template<>
struct fmt::formatter<TrivialAggregate>: formatter<char> {
	auto format(const TrivialAggregate& v, format_context& ctx) const {
		return fmt::format_to(ctx.out(), "{{{}, {}}}", v.x, v.y);
	}
};

template<>
struct Catch::StringMaker<TrivialAggregate> {
	static
	auto convert(const TrivialAggregate& v) -> std::string {
		return fmt::format("{}", v);
	}
};

TEST_CASE("relocation concepts and traits", "[u][engine][core][relocation]") {
	SECTION("trivially copyable type") {
		STATIC_CHECK(fr::is_relocatable<TrivialAggregate>);
		STATIC_CHECK(fr::c_relocatable<TrivialAggregate>);
		STATIC_CHECK(fr::is_trivially_relocatable<TrivialAggregate>);
		STATIC_CHECK(fr::c_trivially_relocatable<TrivialAggregate>);
		STATIC_CHECK(fr::c_nothrow_relocatable<TrivialAggregate>);
		STATIC_CHECK((fr::c_relocatable_from<TrivialAggregate, TrivialAggregate>));
	}
	SECTION("movable, but not trivially copyable, and no custom opt-in") {
		STATIC_CHECK(fr::is_relocatable<MoveOnlySpy>);
		STATIC_CHECK(fr::c_relocatable<MoveOnlySpy>);
		STATIC_CHECK_FALSE(fr::is_trivially_relocatable<MoveOnlySpy>);
		STATIC_CHECK_FALSE(fr::c_trivially_relocatable<MoveOnlySpy>);
		STATIC_CHECK(fr::c_nothrow_relocatable<MoveOnlySpy>);
	}
	SECTION("movable, not trivially copyable, opted into trivial relocation") {
		STATIC_CHECK(fr::is_relocatable<CustomTriviallyRelocatable>);
		STATIC_CHECK(fr::c_relocatable<CustomTriviallyRelocatable>);
		STATIC_CHECK(fr::is_trivially_relocatable<CustomTriviallyRelocatable>);
		STATIC_CHECK(fr::c_trivially_relocatable<CustomTriviallyRelocatable>);
		STATIC_CHECK(fr::c_nothrow_relocatable<CustomTriviallyRelocatable>);
	}
	SECTION("not move-constructible, opted into trivial relocation anyway") {
		STATIC_CHECK_FALSE(fr::is_relocatable<NonMovableTaggedTrivial>);
		STATIC_CHECK_FALSE(fr::c_relocatable<NonMovableTaggedTrivial>);
		STATIC_CHECK_FALSE(fr::is_trivially_relocatable<NonMovableTaggedTrivial>);
		STATIC_CHECK_FALSE(fr::c_trivially_relocatable<NonMovableTaggedTrivial>);
		STATIC_CHECK_FALSE(fr::c_nothrow_relocatable<NonMovableTaggedTrivial>);
	}
	SECTION("movable, not trivially copyable, potentially-throwing move") {
		STATIC_CHECK(fr::is_relocatable<ThrowOnMoveSpy>);
		STATIC_CHECK(fr::c_relocatable<ThrowOnMoveSpy>);
		STATIC_CHECK_FALSE(fr::is_trivially_relocatable<ThrowOnMoveSpy>);
		STATIC_CHECK_FALSE(fr::c_nothrow_relocatable<ThrowOnMoveSpy>);
	}
	SECTION("c_relocatable_from requires the same type on both sides") {
		STATIC_CHECK((fr::c_relocatable_from<MoveOnlySpy, MoveOnlySpy>));
		STATIC_CHECK_FALSE((fr::c_relocatable_from<TrivialAggregate, MoveOnlySpy>));
	}
}

TEST_CASE("move_and_destroy", "[u][engine][core][relocation]") {
	frt::double_test("moves the value and destroys the source", [] {
		auto stats = FuncCallStats{};
		auto src_storage = frt::RawStorage<MoveOnlySpy>{1};
		auto dst_storage = frt::RawStorage<MoveOnlySpy>{1};
		std::construct_at(src_storage.data(), stats, 42);

		auto* result = fr::move_and_destroy(dst_storage.data(), src_storage.data());
		FRT_CHECK(result == dst_storage.data());
		FRT_CHECK(result->value == 42);
		FRT_CHECK(stats.move_ctor_count == 1);
		FRT_CHECK(stats.dtor_count == 1);

		std::destroy_at(result);
	});
	// TODO: Enable compile time testing in C++26 once we get `throw` in constexpr contexts
	frt::double_test<false>("destroys the source even if the move constructor throws", [] {
		auto stats = FuncCallStats{};
		auto src_storage = frt::RawStorage<ThrowOnMoveSpy>{1};
		auto dst_storage = frt::RawStorage<ThrowOnMoveSpy>{1};
		std::construct_at(src_storage.data(), stats, 42, /* should_throw =*/ true);

		auto threw = false;
		try {
			fr::move_and_destroy(dst_storage.data(), src_storage.data());
		}
		catch (const BoomException &) {
			threw = true;
		}

		FRT_CHECK(threw);
		// The source is destroyed by the internal `Guard`, even though construction of the
		// destination never completed
		FRT_CHECK(stats.dtor_count == 1);
		FRT_CHECK(stats.move_ctor_count == 0);
		// `dst_storage`'s slot was never constructed into (the move threw), so there's nothing
		// left to clean up beyond deallocating the raw storage itself
	});
}

TEST_CASE("trivially_relocate", "[u][engine][core][relocation]") {
	// NOTE: No double_test, as trivially_relocate is non-constexpr by design
	constexpr auto n = 7zu;

	auto src_storage = frt::RawStorage<TrivialAggregate>{n};
	auto dst_storage = frt::RawStorage<TrivialAggregate>{n};
	for (auto i = 0; i < static_cast<int>(n); ++i) {
		std::construct_at(src_storage.data() + i, i, i * i);
	}

	auto* after = fr::trivially_relocate(
		src_storage.data(), src_storage.data() + n, dst_storage.data());
	CHECK(after == dst_storage.data() + n);

	for (auto i = 0; i < static_cast<int>(n); ++i) {
		FRT_INFO("i = " << i);
		CHECK(dst_storage.data()[i] == (TrivialAggregate{i, i * i}));
	}

	std::destroy(dst_storage.data(), dst_storage.data() + n);
}

TEST_CASE("relocate_at", "[u][engine][core][relocation]") {
	frt::double_test("relocating to the same address is a no-op", [] {
		auto stats = FuncCallStats{};
		auto storage = frt::RawStorage<MoveOnlySpy>{1};
		std::construct_at(storage.data(), stats, 42);

		auto* result = fr::relocate_at(storage.data(), storage.data());
		FRT_CHECK(result == storage.data());
		FRT_CHECK(result->value == 42);
		FRT_CHECK(stats.move_ctor_count == 0);
		FRT_CHECK(stats.dtor_count == 0);

		std::destroy_at(result);
	});
	frt::double_test("trivially relocatable type", [] {
		auto src_storage = frt::RawStorage<TrivialAggregate>{1};
		auto dst_storage = frt::RawStorage<TrivialAggregate>{1};
		std::construct_at(src_storage.data(), TrivialAggregate{3, 7});

		auto* result = fr::relocate_at(dst_storage.data(), src_storage.data());
		FRT_REQUIRE(result == dst_storage.data());
		FRT_CHECK(*result == (TrivialAggregate{3, 7}));

		std::destroy_at(result);
	});
	frt::double_test("custom trivially relocatable type", [] {
		auto src_storage = frt::RawStorage<CustomTriviallyRelocatable>{1};
		auto dst_storage = frt::RawStorage<CustomTriviallyRelocatable>{1};
		std::construct_at(src_storage.data(), 5);

		auto* result = fr::relocate_at(dst_storage.data(), src_storage.data());
		FRT_CHECK(result == dst_storage.data());
		FRT_CHECK(result->value == 5);

		std::destroy_at(result);
	});
	frt::double_test("non-trivially relocatable, movable type", [] {
		auto counters = FuncCallStats{};
		auto src_storage = frt::RawStorage<MoveOnlySpy>{1};
		auto dst_storage = frt::RawStorage<MoveOnlySpy>{1};
		std::construct_at(src_storage.data(), counters, 42);

		auto* result = fr::relocate_at(dst_storage.data(), src_storage.data());
		FRT_CHECK(result == dst_storage.data());
		FRT_CHECK(result->value == 42);
		FRT_CHECK(counters.move_ctor_count == 1);
		FRT_CHECK(counters.dtor_count == 1);

		std::destroy_at(result);
	});
}

static constexpr
auto make_unitialized_relocate_tester(size_t n, size_t failing_idx) {
	return [n, failing_idx] {
		auto stats = std::vector<FuncCallStats>(n);
		auto src_storage = frt::RawStorage<ThrowOnMoveSpy>{n};
		auto dst_storage = frt::RawStorage<ThrowOnMoveSpy>{n};
		for (auto i = 0zu; i < n; ++i) {
			std::construct_at(src_storage.data() + i, stats[i], static_cast<int>(i),
				/* should_throw = */ i == failing_idx);
		}

		auto threw = false;
		try {
			fr::uninitialized_relocate(src_storage.data(), src_storage.data() + n,
				dst_storage.data());
		}
		catch (const BoomException&) {
			threw = true;
		}
		FRT_REQUIRE(threw);

		for (auto i = 0zu; i < n; ++i) {
			FRT_INFO("i = " << i);
			if (i < failing_idx) {
				// Only the elements before the failure were actually (successfully) relocated
				FRT_CHECK(stats[i].move_ctor_count == 1);
				// Element has been relocated before the exception. So two destructor calls:
				//  - at the source by `Guard` in `move_and_destroy`;
				//  - at the destination by `catch` cleanup in `relocate_range_throwing`
				FRT_CHECK(stats[i].dtor_count == 2);
			}
			else {
				// No relocations = no moves
				FRT_CHECK(stats[i].move_ctor_count == 0);
				// Element has not been relocated yet or the relocation attempt has failed.
				// Only one destructor call:
				//   - The failing element gets destroyed by `Guard` in `move_and_destroy`
				//   - The unrelocated elements in source get destroyed by `catch` cleanup
				//     in `relocate_range_throwing`
				FRT_CHECK(stats[i].dtor_count == 1);
			}
		}
		// `relocate_range_throwing` does all of the cleanup: unrelocated elements at the source
		// and relocated elements at the destination
	};
}

static constexpr
auto make_unitialized_relocate_n_tester(size_t n, size_t failing_idx) {
	return [n, failing_idx] {
		auto stats = std::vector<FuncCallStats>(n);
		auto src_storage = frt::RawStorage<ThrowOnMoveSpy>{n};
		auto dst_storage = frt::RawStorage<ThrowOnMoveSpy>{n};
		for (auto i = 0zu; i < n; ++i) {
			std::construct_at(src_storage.data() + i, stats[i], static_cast<int>(i),
				/* should_throw = */ i == failing_idx);
		}

		auto threw = false;
		try {
			fr::uninitialized_relocate_n(src_storage.data(), n, dst_storage.data());
		}
		catch (const BoomException&) {
			threw = true;
		}
		FRT_REQUIRE(threw);

		for (auto i = 0zu; i < n; ++i) {
			FRT_INFO("i = " << i);
			if (i < failing_idx) {
				// Only the elements before the failure were actually (successfully) relocated
				FRT_CHECK(stats[i].move_ctor_count == 1);
				// Element has been relocated before the exception. So two destructor calls:
				//  - at the source by `Guard` in `move_and_destroy`;
				//  - at the destination by `catch` cleanup in `relocate_n_throwing`
				FRT_CHECK(stats[i].dtor_count == 2);
			}
			else {
				// No relocations = no moves
				FRT_CHECK(stats[i].move_ctor_count == 0);
				// Element has not been relocated yet or the relocation attempt has failed.
				// Only one destructor call:
				//   - The failing element gets destroyed by `Guard` in `move_and_destroy`
				//   - The unrelocated elements in source get destroyed by `catch` cleanup
				//     in `relocate_n_throwing`
				FRT_CHECK(stats[i].dtor_count == 1);
			}
		}
		// `relocate_n_throwing` does all of the cleanup: unrelocated elements at the source
		// and relocated elements at the destination
	};
}

TEST_CASE("uninitialized_relocate", "[u][engine][core][relocation]") {
	frt::double_test("trivially relocatable type", [] {
		constexpr auto n = 7zu;

		auto src_storage = frt::RawStorage<TrivialAggregate>{n};
		auto dst_storage = frt::RawStorage<TrivialAggregate>{n};
		for (auto i = 0; i < static_cast<int>(n); ++i) {
			std::construct_at(src_storage.data() + i, i, i * i);
		}

		auto* dst_end = fr::uninitialized_relocate(
			src_storage.data(), src_storage.data() + n, dst_storage.data());
		FRT_CHECK(dst_end == dst_storage.data() + n);

		for (auto i = 0; i < static_cast<int>(n); ++i) {
			FRT_CHECK(dst_storage.data()[i] == (TrivialAggregate{i, i * i}));
		}

		std::destroy(dst_storage.data(), dst_storage.data() + n);
	});
	frt::double_test("non-trivially relocatable, nothrow-movable type", [] {
		constexpr size_t n = 3;

		auto stats = std::array<FuncCallStats, n>{};
		auto src_storage = frt::RawStorage<MoveOnlySpy>{n};
		auto dst_storage = frt::RawStorage<MoveOnlySpy>{n};
		for (auto i = 0zu; i < n; ++i) {
			std::construct_at(src_storage.data() + i, stats[i], static_cast<int>(i));
		}

		auto* dst_end = fr::uninitialized_relocate(
			src_storage.data(), src_storage.data() + n, dst_storage.data());
		FRT_CHECK(dst_end == dst_storage.data() + n);

		for (auto i = 0zu; i < n; ++i) {
			FRT_INFO("i = " << i);
			FRT_CHECK(dst_storage.data()[i].value == static_cast<int>(i));
			FRT_CHECK(stats[i].move_ctor_count == 1);
			// The source was already destroyed as part of relocating it
			FRT_CHECK(stats[i].dtor_count == 1);
		}

		std::destroy(dst_storage.data(), dst_storage.data() + n);
	});
	// TODO: Enable compile time testing in C++26 once we get `throw` in constexpr contexts
	frt::double_test<false>("throw on move at the start of array relocation",
		make_unitialized_relocate_tester(8zu, 0zu));
	frt::double_test<false>("throw on move in the middle of array relocation",
		make_unitialized_relocate_tester(8zu, 5zu));
	frt::double_test<false>("throw on move at the end of array relocation",
		make_unitialized_relocate_tester(8zu, 7zu));
}

TEST_CASE("uninitialized_relocate_n", "[u][engine][core][relocation]") {
	frt::double_test("trivially relocatable type", [] {
		constexpr auto n = 7zu;

		auto src_storage = frt::RawStorage<TrivialAggregate>{n};
		auto dst_storage = frt::RawStorage<TrivialAggregate>{n};
		for (auto i = 0; i < static_cast<int>(n); ++i) {
			std::construct_at(src_storage.data() + i, i, i * i);
		}

		const auto [src_end, dst_end] = fr::uninitialized_relocate_n(
			src_storage.data(), n, dst_storage.data());
		FRT_CHECK(src_end == src_storage.data() + n);
		FRT_CHECK(dst_end == dst_storage.data() + n);

		for (auto i = 0; i < static_cast<int>(n); ++i) {
			FRT_CHECK(dst_storage.data()[i] == (TrivialAggregate{i, i * i}));
		}

		std::destroy(dst_storage.data(), dst_storage.data() + n);
	});
	frt::double_test("non-trivially relocatable, nothrow-movable type", [] {
		constexpr auto n = 7zu;

		auto stats = std::array<FuncCallStats, n>{};

		auto src_storage = frt::RawStorage<MoveOnlySpy>{n};
		auto dst_storage = frt::RawStorage<MoveOnlySpy>{n};
		for (auto i = 0zu; i < n; ++i) {
			std::construct_at(src_storage.data() + i, stats[i], static_cast<int>(i));
		}

		const auto [src_end, dst_end] = fr::uninitialized_relocate_n(src_storage.data(), n,
			dst_storage.data());
		FRT_CHECK(src_end == src_storage.data() + n);
		FRT_CHECK(dst_end == dst_storage.data() + n);

		for (auto i = 0zu; i < n; ++i) {
			FRT_INFO("i = " << i);
			FRT_CHECK(dst_storage.data()[i].value == static_cast<int>(i));
			FRT_CHECK(stats[i].move_ctor_count == 1);
			FRT_CHECK(stats[i].dtor_count == 1);
		}

		std::destroy(dst_storage.data(), dst_storage.data() + n);
	});
	// TODO: Enable compile time testing in C++26 once we get `throw` in constexpr contexts
	frt::double_test<false>("throw on move at the start of array relocation",
		make_unitialized_relocate_n_tester(8zu, 0zu));
	frt::double_test<false>("throw on move in the middle of array relocation",
		make_unitialized_relocate_n_tester(8zu, 5zu));
	frt::double_test<false>("throw on move at the end of array relocation",
		make_unitialized_relocate_n_tester(8zu, 7zu));
}