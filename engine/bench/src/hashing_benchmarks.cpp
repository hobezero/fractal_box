#include "fractal_box/core/hashing/hasher_utils.hpp"
#include "fractal_box/core/hashing/rapidhash.hpp"
#include "fractal_box/core/hashing/uni_hasher.hpp"

#include <random>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>
#include <nanobench.h>

#include "fractal_box/core/platform.hpp"
#include "fractal_box/core/meta/type_name.hpp"

#include "bench_common/bench_helpers.hpp"
#include "bench_common/old_hash.hpp"

namespace nanobench = ankerl::nanobench;

template<class Algo>
static // FR_NOINLINE
auto calc_rapidhash(const std::vector<unsigned char>& str) noexcept -> uint64_t {
	return Algo::hash_bytes(str.data(), str.size());
}

template<class T>
static constexpr FR_FORCE_INLINE
auto calc_std_hash(const T& obj) noexcept -> size_t {
	return std::hash<T>{}(obj);
}

template<class Algo>
static
auto bench_rapidhash(const std::string& algo_name) {
	auto bench = nanobench::Bench{};
	bench.title(algo_name)
		.performanceCounters(true)
		.relative(true)
		.warmup(100)
		.minEpochIterations(20'000)
	;

	auto vec = std::vector<unsigned char>{};
	auto rng = nanobench::Rng{};
	constexpr size_t sizes[] = {
		0zu, 2zu, 4zu, 5zu, 8zu,
		16zu, 19zu, 24zu, 32zu, 41zu, 48zu, 64zu, 77zu,
		128zu, 250zu, 520zu, 1030zu,
		2050zu, 8192zu
	};
	vec.reserve(sizes[std::size(sizes) - 1zu]);
	for (size_t size : sizes) {
		vec.clear();
		auto dist = std::uniform_int_distribution<unsigned char>{};
		for (auto i = 0zu; i < size; ++i) {
			vec.push_back(dist(rng));
		}
		bench.run(fmt::format("{} bytes", size), [&] {
			nanobench::doNotOptimizeAway(Algo::hash_bytes(vec.data(), vec.size()));
		});
	}
	REQUIRE_FALSE(bench.results().empty());
}

TEST_CASE("bench:rapidhash", "[b][engine][hashing]") {
	bench_rapidhash<fr::Rapidhash<true, false, true>>("rapidhash (compact)");
	bench_rapidhash<fr::Rapidhash<true, false, false>>("rapidhash (non-compact)");
	bench_rapidhash<fr::Rapidhash<true, true, true>>("rapidhash (compact protected)");
	bench_rapidhash<fr::Rapidhash<false, false, true>>("rapidhash (non-avalanching)");

	bench_rapidhash<fr::RapidhashMicro<true, false>>("rapidhash_micro");
	bench_rapidhash<fr::RapidhashMicro<true, true>>("rapidhash_micro (protected))");
	bench_rapidhash<fr::RapidhashMicro<false, false>>("rapidhash_micro (non-avalanching)");

	bench_rapidhash<fr::RapidhashNano<true, false>>("rapidhash_nano");
	bench_rapidhash<fr::RapidhashNano<true, true>>("rapidhash_nano (protected))");
	bench_rapidhash<fr::RapidhashNano<false, false>>("rapidhash_nano (non-avalanching)");
}

namespace {

template<fr::c_hash_digest T>
struct MyDigest {
	template<class H>
	friend constexpr
	auto fr_old_custom_hash(const MyDigest& self) noexcept {
		return H::hash_expansion(self.value);
	}

	template<class H>
	friend constexpr
	void fr_custom_hash(const MyDigest& self, H& visitor) noexcept {
		visitor(H::digest(self.value));
	}

public:
	T value;
};

template<class T>
struct MyWrap {
	using Self = MyWrap;

	template<class H>
	friend constexpr
	auto fr_old_custom_hash(const Self& self) noexcept {
		return H::hash_expansion(self.value);
	}

	[[maybe_unused]] friend consteval
	auto fr_describe(const Self&) noexcept {
		return fr::class_desc<
			fr::Attributes<fr::Hashable{}>,
			fr::Field<&Self::value>
		>;
	}

public:
	T value;
};

template<class T>
struct MyPair {
	using Self = MyPair;

	template<class H>
	friend constexpr
	auto fr_old_custom_hash(const Self& self) noexcept {
		return H::hash_expansion(self.first, self.second);
	}

	[[maybe_unused]] friend consteval
	auto fr_describe(const Self&) noexcept {
		return fr::class_desc<
			fr::Attributes<fr::Hashable{}>,
			fr::Field<&Self::first>,
			fr::Field<&Self::second>
		>;
	}

public:
	T first;
	T second;
};

struct CustomClass {
	template<class H>
	[[maybe_unused]] friend constexpr
	void fr_custom_hash(const CustomClass& self, H visitor) noexcept {
		visitor(
			self.a,
			self.b,
			self.c,
			self.d,
			self.e,
			self.f,
			self.g,
			self.h
		);
	}

public:
	int64_t a;
	std::string b;
	std::array<int16_t, 3> c;
	bool d;
	double e;
	std::optional<uint32_t> f;
	std::string g;
	std::vector<int> h;
};

struct DescribedClass {
	[[maybe_unused]] friend constexpr
	auto fr_describe(const DescribedClass&) noexcept {
		using Self = DescribedClass;
		return fr::class_desc<
			fr::Attributes<fr::Hashable{}>,
			fr::Field<&Self::a>,
			fr::Field<&Self::b>,
			fr::Field<&Self::c>,
			fr::Field<&Self::d>,
			fr::Field<&Self::e>,
			fr::Field<&Self::f>,
			fr::Field<&Self::g>,
			fr::Field<&Self::h>
		>;
	}

public:
	int64_t a;
	std::string b;
	std::array<int16_t, 3> c;
	bool d;
	double e;
	std::optional<uint32_t> f;
	std::string g;
	std::vector<int> h;
};

struct AggregateClass {
	int64_t a;
	std::string b;
	std::array<int16_t, 3> c;
	bool d;
	double e;
	std::optional<uint32_t> f;
	std::string g;
	std::vector<int> h;
};

template<class T>
inline constexpr
auto make_class() -> T {
	return T{
		378431998,
		"abcd ef asdkfsj alksjdfd lajsalsfj ajflljj llj234 as233rufjjasdff dda",
		{2, 54, -2300},
		true,
		45.94,
		{8343112},
		"abc",
		{2344, 12, -934, 0, 13, 23, 11087}
	};
}

template<class H, class T>
requires std::same_as<T, CustomClass>
	|| std::same_as<T, DescribedClass>
	|| std::same_as<T, AggregateClass>
constexpr
auto fr_old_custom_hash(const T& self) noexcept {
	return H::hash_expansion(
		self.a,
		H::hash_range(self.b),
		H::hash_range(self.c),
		self.d,
		self.e,
		self.f,
		H::hash_range(self.g),
		H::hash_range(self.h)
	);
}

} // namespace

template<class T>
struct std::hash<MyDigest<T>> {
	constexpr
	auto operator()(const MyDigest<T>& self) const noexcept -> size_t {
		return static_cast<size_t>(self.value);
	}
};

template<class T>
struct std::hash<MyWrap<T>> {
	constexpr
	auto operator()(const MyWrap<T>& self) const noexcept -> size_t {
		return calc_std_hash(self.value);
	}
};

template<class T>
struct std::hash<MyPair<T>> {
	constexpr
	auto operator()(const MyPair<T>& self) const noexcept -> size_t {
		const auto sub = std::hash<T>{};
		return fr::hash_mix_boost(sub(self.first), sub(self.second));
	}
};

template<class T>
requires std::same_as<T, CustomClass>
	|| std::same_as<T, DescribedClass>
	|| std::same_as<T, AggregateClass>
struct std::hash<T> {
	constexpr
	auto operator()(const T& self) const noexcept -> size_t {
		auto h = calc_std_hash(self.a);
		h = fr::hash_mix_boost(h, calc_std_hash(self.b));
		h = fr::hash_mix_boost(h, calc_std_hash(self.c[0]));
		h = fr::hash_mix_boost(h, calc_std_hash(self.c[1]));
		h = fr::hash_mix_boost(h, calc_std_hash(self.c[2]));
		h = fr::hash_mix_boost(h, calc_std_hash(self.d));
		h = fr::hash_mix_boost(h, calc_std_hash(self.e));
		h = fr::hash_mix_boost(h, calc_std_hash(self.f));
		h = fr::hash_mix_boost(h, calc_std_hash(self.g));
		h = fr::hash_mix_boost(h, calc_std_hash(self.h.size()));
		for (const auto& x : self.h) {
			h = fr::hash_mix_boost(h, calc_std_hash(x));
		}
		return h;
	}
};

template<class Hasher, class T>
static
auto run_hasher(nanobench::Bench& bench, const char* name, T& obj) {
	auto hasher = [] {
		if constexpr (std::is_default_constructible_v<Hasher>)
			return Hasher{};
		else
			return Hasher{0xDEADBEEF};
	}();

	bench.run(name, [&] {
		nanobench::doNotOptimizeAway(hasher(obj));
	});
}

template<class T>
static
auto bench_hasher(std::string_view tag, const T& obj) {
	auto bench = nanobench::Bench{};
	const auto tname = fr::unqualified_type_name<T>;
	const auto title = tag.empty()
		? fmt::format("Hashers hashing `{}` ({} bytes)", tname, sizeof(T))
		: fmt::format("Hashers hashing `{}` ({} bytes): {}", tname, sizeof(T), tag);

	bench.title(title)
		.performanceCounters(true)
		.relative(true)
		.warmup(100)
		.minEpochIterations(40'000)
	;

	run_hasher<std::hash<T>>(bench, "std::hash + boost mixing", obj);
	run_hasher<fr::StableHash32<>>(bench, "StableHash32", obj);
	run_hasher<fr::StableHash64<>>(bench, "StableHash64", obj);
	run_hasher<fr::UniHasherFast32>(bench, "UniHasherFast32", obj);
	run_hasher<fr::UniHasherFast64>(bench, "UniHasherFast64", obj);
	run_hasher<fr::UniHasherQuality32>(bench, "UniHasherQuality32", obj);
	run_hasher<fr::UniHasherQuality64>(bench, "UniHasherQuality64", obj);
	run_hasher<fr::UniHasherProtected32>(bench, "UniHasherProtected32", obj);
	run_hasher<fr::UniHasherProtected64>(bench, "UniHasherProtected64", obj);

	REQUIRE_FALSE(bench.results().empty());
}

TEST_CASE("bench:hashers", "[b][engine][hashing]") {
	// TODO: Randomly generate a stream of objects to see branch prediction effects
	bench_hasher({}, MyDigest<fr::HashDigest64>{67831});

	bench_hasher({}, MyWrap<uint32_t>{67831});
	bench_hasher({}, MyWrap<uint64_t>{67832});

	bench_hasher("short", MyWrap<std::string>{"abcdefghABCDE"});
	bench_hasher("medium", MyWrap<std::string>{frb::lorem_text});
	bench_hasher("long", MyWrap<std::string>{frb::lorem_text_long});

	bench_hasher("engaged", MyWrap<std::optional<uint64_t>>{2345});
	bench_hasher("disengaged", MyWrap<std::optional<uint64_t>>{std::nullopt});

	bench_hasher({}, MyPair<bool>{true, false});

	bench_hasher({}, MyPair<int32_t>{234, -54321});

	bench_hasher("normals", MyPair<float>{8.9f, 2.3f});
	bench_hasher("special values", MyPair<float>{std::numeric_limits<float>::infinity(),
		std::numeric_limits<float>::quiet_NaN()});

	bench_hasher("normals", MyPair<long double>{8.9l, 2.3l});
	bench_hasher("subnormals", MyPair<long double>{0x0.000002p-126l, 0x0.000002p-126l});
	bench_hasher("special values", MyPair<long double>{std::numeric_limits<long double>::infinity(),
		std::numeric_limits<long double>::quiet_NaN()});

	bench_hasher({}, make_class<CustomClass>());
	bench_hasher({}, make_class<DescribedClass>());
	bench_hasher({}, make_class<AggregateClass>());
}

extern "C"
auto fr_hash_int32(int32_t v) noexcept -> fr::HashDigest64 {
	return fr::UniHasher<>{}(v);
}

extern "C"
auto fr_hash_bool(bool v) noexcept -> fr::HashDigest64 {
	return fr::UniHasher<>{}(v);
}

extern "C"
auto fr_hash_bool_std(bool v) noexcept -> fr::HashDigest64 {
	return calc_std_hash(v);
}

extern "C"
auto fr_hash_pair_bool(std::pair<bool, bool> v) noexcept -> fr::HashDigest64 {
	return fr::UniHasher<>{}(v);
}

extern "C" FR_FLATTEN
auto fr_hash_custom(const CustomClass& v) noexcept -> fr::HashDigest64 {
	return fr::UniHasher<>{}(v);
}

extern "C" FR_FLATTEN
auto fr_hash_described(const DescribedClass& v) noexcept -> fr::HashDigest64 {
	return fr::UniHasher<>{}(v);
}

extern "C" FR_FLATTEN
auto fr_hash_aggregate(const AggregateClass& v) noexcept -> fr::HashDigest64 {
	return fr::UniHasher<>{}(v);
}
