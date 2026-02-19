#ifndef FR_TEST_TEST_COMMON_TEST_HELPERS_HPP
#define FR_TEST_TEST_COMMON_TEST_HELPERS_HPP

#include <array>
#include <concepts>
#include <string_view>
#include <type_traits>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <catch2/catch_test_macros.hpp>

#include "fractal_box/core/meta/reflection.hpp"

namespace frt {

inline constexpr char lorem_text[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, "
	"sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.";

inline constexpr char lorem_text_long[] =
	R"(Sed ut perspiciatis, unde omnis iste natus error sit voluptatem accusantium doloremque
laudantium, totam rem aperiam eaque ipsa, quae ab illo inventore veritatis et quasi architecto
beatae vitae dicta sunt, explicabo. Nemo enim ipsam voluptatem, quia voluptas sit, aspernatur aut
odit aut fugit, sed quia consequuntur magni dolores eos, qui ratione voluptatem sequi nesciunt,
neque porro quisquam est, qui dolorem ipsum, quia dolor sit amet consectetur adipisci[ng] velit,
sed quia non numquam [do] eius modi tempora inci[di]dunt, ut labore et dolore magnam aliquam
quaerat voluptatem. Ut enim ad minima veniam, quis nostrum[d] exercitationem ullam corporis
suscipit laboriosam, nisi ut aliquid ex ea commodi consequatur? [D]Quis autem vel eum i[r]ure
reprehenderit, qui in ea voluptate velit esse, quam nihil molestiae consequatur, vel illum, qui
dolorem eum fugiat, quo voluptas nulla pariatur?

At vero eos et accusamus et iusto odio dignissimos ducimus, qui blanditiis praesentium
voluptatum deleniti atque corrupti, quos dolores et quas molestias excepturi sint, obcaecati
cupiditate non provident, similique sunt in culpa, qui officia deserunt mollitia animi, id est
laborum et dolorum fuga. Et harum quidem reru[d]um facilis est e[r]t expedita distinctio.
Nam libero tempore, cum soluta nobis est eligendi optio, cumque nihil impedit, quo minus id,
quod maxime placeat facere possimus, omnis voluptas assumenda est, omnis dolor repellend[a]us.
Temporibus autem quibusdam et aut officiis debitis aut rerum necessitatibus saepe eveniet, ut et
voluptates repudiandae sint et molestiae non recusandae. Itaque earum rerum hic tenetur a sapiente
delectus, ut aut reiciendis voluptatibus maiores alias consequatur aut perferendis doloribus
asperiores repellat.)";

// TODO: Replace with Catch v3 matchers
template<class T, class U = T>
inline constexpr
auto points_to_value(T* ptr, const U& value) -> bool {
	return ptr && *ptr == value;
}

struct Empty {
	friend constexpr auto operator==(Empty, Empty) noexcept -> bool = default;
};

struct FuncCallStats {
	void reset() noexcept { *this = {}; }

public:
	int default_ctor_count = 0;
	int custom_ctor_count = 0;
	int many_args_ctor_count = 0;
	int copy_ctor_count = 0;
	int move_ctor_count = 0;
	int copy_assign_count = 0;
	int move_assign_count = 0;
	int dtor_count = 0;
};

inline constinit FuncCallStats global_call_stats {};

template<class T>
concept c_func_call_spy = requires {
	{ typename std::remove_cvref_t<T>::tracks_func_calls{} } -> std::same_as<std::true_type>;
};

static_assert(!c_func_call_spy<std::string>);

/// @brief "Spy" in Martin Fowler's terms. Records calls to special member functions. Suitable
/// to test SBO implementations, as it is small enough to fit in any small buffer
struct SmallCallSpy {
	using Data = char;
	static constexpr char default_value = '#';
	using tracks_func_calls = std::true_type;

	SmallCallSpy() noexcept {
		++global_call_stats.default_ctor_count;
	}

	explicit(false)
	SmallCallSpy(Data d) noexcept:
		data{d}
	{
		++global_call_stats.custom_ctor_count;
	}

	SmallCallSpy(Data x, Data y) noexcept:
		data{static_cast<Data>(x + y)}
	{
		++global_call_stats.many_args_ctor_count;
	}

	SmallCallSpy(const SmallCallSpy& other):
		data(other.data)
	{
		++global_call_stats.copy_ctor_count;
	}

	auto operator=(const SmallCallSpy& other) -> SmallCallSpy& {
		++global_call_stats.copy_assign_count;
		data = other.data;
		return *this;
	}

	SmallCallSpy(SmallCallSpy&& other) noexcept:
		data(std::exchange(other.data, default_value))
	{
		++global_call_stats.move_ctor_count;
	}

	auto operator=(SmallCallSpy&& other) noexcept -> SmallCallSpy& {
		++global_call_stats.move_assign_count;
		data = std::exchange(other.data, default_value);
		return *this;
	}

	~SmallCallSpy() {
		++global_call_stats.dtor_count;
	}

	friend auto operator==(const SmallCallSpy&, const SmallCallSpy&) -> bool = default;

public:
	char data = default_value;
};

static_assert(c_func_call_spy<SmallCallSpy>);

/// @brief "Spy" in Martin Fowler's terms. Records calls to special member functions. Suitable
/// to test non-SBO paths in SBO implementations, as it is large enough to NOT fit in basically
/// any small buffer
struct LargeCallSpy {
	using Data = std::array<std::string, 20>;
	using tracks_func_calls = std::true_type;

	LargeCallSpy() noexcept {
		++global_call_stats.default_ctor_count;
	}

	explicit(false)
	LargeCallSpy(Data&& d) noexcept:
		data(std::move(d))
	{
		++global_call_stats.custom_ctor_count;
	}

	LargeCallSpy(Data&& d, std::string first) noexcept:
		data(std::move(d))
	{
		++global_call_stats.many_args_ctor_count;
		this->data[0] = std::move(first);
	}

	LargeCallSpy(const LargeCallSpy& other):
		data(other.data)
	{
		++global_call_stats.copy_ctor_count;
	}

	LargeCallSpy(LargeCallSpy&& other) noexcept:
		data(std::move(other.data))
	{
		++global_call_stats.move_ctor_count;
	}

	~LargeCallSpy() {
		++global_call_stats.dtor_count;
	}

	auto operator=(const LargeCallSpy& other) -> LargeCallSpy& {
		++global_call_stats.copy_assign_count;
		this->data = other.data;
		return *this;
	}

	auto operator=(LargeCallSpy&& other) noexcept -> LargeCallSpy& {
		++global_call_stats.move_assign_count;
		this->data = std::move(other.data);
		return *this;
	}

	friend auto operator==(const LargeCallSpy&, const LargeCallSpy&) -> bool
		= default;

public:
	std::array<std::string, 20> data = {};
};

template<class First, class... Rest, class F, class... Args>
inline
void named_typed_section(std::string_view name_prefix, F&& callback, Args&&... args) {
	using namespace std::string_view_literals;

	fmt::memory_buffer buf;
	if (!name_prefix.empty()) {
		buf.append(name_prefix);
		buf.append(" ("sv);
	}
	buf.append(sizeof...(Rest) > 0 ? "with types: "sv : "with type: "sv);

	fmt::format_to(std::back_inserter(buf), "'{}'", fr::type_name_lit<First>);
	(fmt::format_to(std::back_inserter(buf), ", '{}'", fr::type_name_lit<Rest>), ...);
	(fmt::format_to(std::back_inserter(buf), ", '{}' (deduced)", fr::type_name_lit<Args>), ...);
	if (!name_prefix.empty())
		buf.append(")"sv);

	SECTION(fmt::to_string(buf)) {
		std::forward<F>(callback).template operator()<First, Rest...>(std::forward<Args>(args)...);
	}
}

template<class First, class... Rest, class F, class... Args>
FR_FORCE_INLINE
void typed_section(F&& callback, Args&&... args) {
	named_typed_section<First, Rest...>({}, std::forward<F>(callback), std::forward<Args>(args)...);
}

} // namespace frt

// fmt specializations
// -------------------

template<>
struct fmt::formatter<frt::Empty>: formatter<char> {
	auto format(frt::Empty, format_context& ctx) const {
		return fmt::format_to(ctx.out(), "{{}}");
	}
};

template<>
struct fmt::formatter<frt::SmallCallSpy>: formatter<frt::SmallCallSpy::Data> {
	auto format(const frt::SmallCallSpy& spy, format_context& ctx) const {
		return formatter<frt::SmallCallSpy::Data>::format(spy.data, ctx);
	}
};

template<>
struct fmt::formatter<frt::LargeCallSpy>: formatter<frt::LargeCallSpy::Data> {
	auto format(const frt::LargeCallSpy& spy, format_context& ctx) const {
		return formatter<frt::LargeCallSpy::Data>::format(spy.data, ctx);
	}
};

// Catch2 specializations
// ----------------------

// NOTE: Unfortunatelly, can't reuse the fmt specializations because there is no apparent way to
// detect if Catch2 already provides special printing rules for a given type. fmt has
// `fmt::formattable` for this purpose. Note that `Catch::StringMaker` is defined for *any* type:
// it prints `{?}` if no other logic was specified.
// There is another minor issue. Ideally, we want debug formatting to be slightly different from
// *normal* formatting
// Some links
//   - https://github.com/catchorg/Catch2/issues/2780
//   - https://brevzin.github.io/c++/2023/01/19/debug-fmt-catch/

template<>
struct Catch::StringMaker<frt::Empty> {
	static
	auto convert(frt::Empty) -> std::string {
		return "{}";
	}
};

template<>
struct Catch::StringMaker<frt::SmallCallSpy> {
	static
	auto convert(const frt::SmallCallSpy& spy) -> std::string {
		return StringMaker<decltype(spy.data)>::convert(spy.data);
	}
};

template<>
struct Catch::StringMaker<frt::LargeCallSpy> {
	static
	auto convert(const frt::LargeCallSpy& spy) -> std::string {
		return StringMaker<frt::LargeCallSpy::Data>::convert(spy.data);
	}
};

#endif // include guard
