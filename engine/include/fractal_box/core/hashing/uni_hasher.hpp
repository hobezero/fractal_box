#ifndef FRACTAL_BOX_CORE_HASHING_UNI_HASHER_HPP
#define FRACTAL_BOX_CORE_HASHING_UNI_HASHER_HPP

#include <climits>
#include <cmath>

#include <vector>
#include <array>

#include "fractal_box/core/algorithm.hpp"
#include "fractal_box/core/assert.hpp"
#include "fractal_box/core/byte_utils.hpp"
#include "fractal_box/core/enum_utils.hpp"
#include "fractal_box/core/hashing/hash_digest.hpp"
#include "fractal_box/core/hashing/hasher_utils.hpp"
#include "fractal_box/core/hashing/hasher_visitor_base.hpp"
#include "fractal_box/core/hashing/hashing_concepts.hpp"
#include "fractal_box/core/hashing/rapidhash.hpp"
#include "fractal_box/core/meta/reflection.hpp"

/// Hashing algorithm
/// 1. Build a typed representation of the hashing tree (preserve positional info)
/// 2. Sort the tree in order:
///    - Uncoditional: primitives, arrays, enums, aggregates, reflected classes
///    - Containerss
///    - Customized classes
/// 3. Group uncoditional items into blocks
/// 4. For each uncoditional block, pull the corresponding value from the hashing tree using
///    positional info
/// 5. Hash the remainder using push model. Accumulate data into blocks at runtime, then flush
/// 6. Finalize

namespace fr {

enum class UniHasherSeeding {
	Stable,
	Unstable,
	Provided,
};

struct UniHasherOpts {
	constexpr
	auto validate_or_panic() const noexcept -> bool {
		if (this->bits != 16 && this->bits != 32 && this->bits != 64) {
			FR_PANIC_MSG("Unsupported number of bits in hash digest");
		}
		if (this->is_dos_resistant && this->seeding == UniHasherSeeding::Stable) {
			FR_PANIC_MSG("DOS resistance is incompatible with stable seeding");
		}
		if (this->is_dos_resistant && !this->is_avalanching) {
			FR_PANIC_MSG("DOS resistance requires avalanche effect");
		}
		if (this->seed != 0 && this->seeding != UniHasherSeeding::Stable) {
			FR_PANIC_MSG("Custom static seed requires stable seeding");
		}
		return true;
	}

public:
	int bits = 64;
	bool is_avalanching = false;
	UniHasherSeeding seeding = UniHasherSeeding::Stable;
	/// @brief Initial seed. Affects hashing only if `seeding == Stable`
	HashDigest64 seed = 0;
	/// @brief Enables a minimal protection against DOS attacks by utilizing protected rapidhash
	/// mixing
	bool is_dos_resistant = false;
	RapidhashAlgo algorithm = RapidhashAlgo::Nano;
};

namespace detail {

struct UniHashableLens1 {
	constexpr
	auto operator==(const UniHashableLens1&) const -> bool = default;

	FR_FORCE_INLINE constexpr
	auto num_words() const noexcept -> size_t {
		return byte_size / sizeof(uint64_t) + (byte_size % sizeof(uint64_t) == 0zu ? 0zu : 1zu);
	}

public:
	std::vector<size_t> path;
	size_t byte_size = 0zu;
};

template<size_t MaxSize>
struct UniHashableLens2 {
	UniHashableLens2() = default;

	consteval
	UniHashableLens2(
		const UniHashableLens1& lens,
		size_t offset,
		size_t start_word,
		size_t end_word
	) noexcept:
		path{},
		path_size{lens.path.size()},
		byte_size{lens.byte_size},
		byte_offset{offset},
		start_word_idx{start_word},
		end_word_idx{end_word}
	{
		for (auto i = 0zu; i < this->path_size; ++i)
			this->path[i] = lens.path[i];
	}

	explicit consteval
	UniHashableLens2(const UniHashableLens1& lens) noexcept:
		UniHashableLens2(lens, 0zu, 0zu, 0zu)
	{ }

	FR_FORCE_INLINE constexpr
	auto num_words() const noexcept -> size_t {
		return end_word_idx - start_word_idx;
	}

public:
	std::array<size_t, MaxSize> path;
	size_t path_size;
	size_t byte_size;
	size_t byte_offset;
	size_t start_word_idx;
	size_t end_word_idx;
};

template<auto Lens, size_t PathIdx = 0, class T>
FR_FORCE_INLINE constexpr FR_FLATTEN
auto apply_uni_hashable_lens(const T& obj) -> decltype(auto) {
	static_assert(PathIdx <= Lens.path_size);
	using enum HashableCategory;

	static constexpr auto hashability = get_hashability<T>();
	static_assert(hashability);

	constexpr auto category = hashability.category();

	if constexpr (PathIdx == Lens.path_size) {
		return obj;
	}
	else if constexpr (is_hvb_wrapper_tuple<T>) {
		return apply_uni_hashable_lens<Lens, PathIdx + 1>(std::get<Lens.path[PathIdx]>(obj.args));
	}
	else if constexpr (category == Described) {
		static constexpr auto base_count = mp_size<ReflBases<T>>;
		if constexpr (Lens.path[PathIdx] < base_count) {
			using Base = MpAt<ReflBases<T>, Lens.path[PathIdx]>;
			return apply_uni_hashable_lens<Lens, PathIdx + 1>(static_cast<const Base&>(obj));
		}
		else {
			using FP = MpAt<ReflFieldsAndProperties<T>, Lens.path[PathIdx] - base_count>;
			if constexpr (PathIdx + 1 == Lens.path_size) {
				// Can't recurse further or some property returning a temporary might produce a
				// dangling reference
				return get_field_or_property<FP>(obj);
			}
			else {
				return apply_uni_hashable_lens<Lens, PathIdx + 1>(get_field_or_property<FP>(obj));
			}
		}
	}
	else if constexpr ((category == Array || category == Range) && c_constexpr_sized_range<T>) {
		return apply_uni_hashable_lens<Lens, PathIdx + 1>(
			*(std::ranges::begin(obj) + Lens.path[PathIdx]));
	}
	else if constexpr (category == Record) {
		return apply_uni_hashable_lens<Lens, PathIdx + 1>(get<Lens.path[PathIdx]>(obj));
	}
	else {
		static_assert(false);
	}
}

template<auto Lens, size_t PathIdx = 0zu, class... Ts>
requires (sizeof...(Ts) > 1zu)
FR_FORCE_INLINE constexpr
auto apply_uni_hashable_lens(const Ts&... args) -> decltype(auto) {
	static_assert(PathIdx < Lens.path_size);
	return apply_uni_hashable_lens<Lens, PathIdx + 1>(mp_pack_at<Lens.path[PathIdx]>(args...));
}

class UniHashableLenses1 {
public:
	template<class... Ts>
	explicit constexpr
	UniHashableLenses1(MpList<Ts...>) {
		build<Ts...>();
		std::ranges::sort(_byte_hashables, std::ranges::greater{}, [](const auto& lens) {
			return lens.byte_size;
		});
	}

	constexpr
	auto byte_hashables() const noexcept -> const std::vector<UniHashableLens1>& {
		return _byte_hashables;
	}

	constexpr
	auto others() const noexcept -> const std::vector<UniHashableLens1>& {
		return _others;
	}

private:
	template<class... Ts>
	requires (sizeof...(Ts) > 1)
	constexpr
	void build(const std::vector<size_t>& path = {}) noexcept {
		[&]<size_t... Is>(std::index_sequence<Is...>) {
			(..., build<Ts>(appended(path, Is)));
		}(std::make_index_sequence<sizeof...(Ts)>{});
	}

	template<class T>
	constexpr
	void build(const std::vector<size_t>& path = {}) {
		using PT = std::remove_cvref_t<T>;
		using enum HashableCategory;
		using enum HashableMode;

		static constexpr auto hashability = get_hashability<T>();
		static_assert(hashability);

		constexpr auto mode = hashability.mode();
		constexpr auto category = hashability.category();

		if constexpr (mode == AsBytes) {
			add_byte_hashable(path, sizeof(PT));
		}
		else if constexpr (category == Primitive) {
			if constexpr (std::is_same_v<PT, float> || std::is_same_v<PT, double>)
				add_byte_hashable(path, sizeof(PT));
			else
				add_other(path);
		}
		else if constexpr (category == Wrapper) {
			if constexpr (is_hvb_wrapper_tuple<PT>) {
				[&]<size_t... Is>(std::index_sequence<Is...>) {
					(..., build<MpAt<typename PT::Args, Is>>(appended(path, Is)));
				}(std::make_index_sequence<T::size>{});
			}
			else {
				add_other(path);
			}
		}
		else if constexpr (category == Custom) {
			add_other(path);
		}
		else if constexpr (category == Described) {
			static constexpr auto base_count = mp_size<ReflBases<T>>;
			[&]<class... Bases, size_t... Is>(
				MpList<Bases...>,
				std::index_sequence<Is...>
			) FR_FORCE_INLINE_L {
				(..., build_base<mode, Bases, Is>(path));
			}(ReflBases<T>{}, std::make_index_sequence<base_count>{});

			using FieldsAndProps = ReflFieldsAndProperties<PT>;
			[&]<class... FPs, size_t... Is>(MpList<FPs...>, std::index_sequence<Is...>) {
				(..., build_child<mode, FPs, base_count + Is>(path));
			}(FieldsAndProps{}, std::make_index_sequence<mp_size<FieldsAndProps>>{});
		}
		else if constexpr (category == Enum) {
			// Non-byte-hashable enums exist?
			add_other(path);
		}
		else if constexpr (category == Optional) {
			add_other(path);
		}
		else if constexpr (category == String) {
			add_other(path);
		}
		else if constexpr (category == Array || category == Range) {
			// Non-byte-hashable ranges
			if constexpr (c_constexpr_sized_range<PT>) {
				for (auto i = 0zu; i < constexpr_size<PT>; ++i) {
					build<std::ranges::range_value_t<PT>>(appended(path, i));
				}
			}
			else {
				add_other(path);
			}
		}
		else if constexpr (category == Record) {
			// Non-byte-hashable records
			using Decomposed = ReflDecomposition<T>;
			[&]<class... Fields, size_t... Is>(MpList<Fields...>, std::index_sequence<Is...>) {
				(..., build<Fields>(appended(path, Is)));
			}(Decomposed{}, std::make_index_sequence<mp_size<Decomposed>>{});
		}
		else {
			static_assert(false);
		}
	}

	constexpr
	void add_byte_hashable(std::vector<size_t> path, size_t byte_size) {
		_byte_hashables.emplace_back(std::move(path), byte_size);
	}

	constexpr
	void add_other(std::vector<size_t> path) {
		_others.emplace_back(std::move(path));
	}

	template<HashableMode mode, class Base, size_t Idx>
	constexpr
	void build_base(const std::vector<size_t>& path) {
		if constexpr (mode == HashableMode::OptOut) {
			build<Base>(appended(path, Idx));
		}
		else if constexpr (mode == HashableMode::OptIn) {
			if constexpr (get_hashability<Base>()) {
				build<Base>(appended(path, Idx));
			}
		}
		else {
			static_assert(false);
		}
	}

	template<HashableMode mode, class Child, size_t Idx>
	constexpr
	void build_child(
		const std::vector<size_t>& path
	) {
		using Type = ReflFieldOrPropertyType<Child>;
		static constexpr auto should_hash = mode == HashableMode::OptOut
			? refl_attribute_or<Child, Hashable, Hashable{true}>
			: refl_attribute_or<Child, Hashable, Hashable{false}>;
		if constexpr (should_hash) {
			build<Type>(appended(path, Idx));
		}
	}

private:
	std::vector<UniHashableLens1> _byte_hashables;
	std::vector<UniHashableLens1> _others;
};

struct UniHashableLenses2Sizes {
	size_t num_byte_hashables;
	size_t num_others;
	size_t max_byte_hashables_path_size;
	size_t max_others_path_size;
};

template<UniHashableLenses2Sizes Sizes>
struct UniHashableLenses2 {
	using ByteHashableLens = UniHashableLens2<Sizes.max_byte_hashables_path_size>;
	using OtherLens = UniHashableLens2<Sizes.max_others_path_size>;
	static constexpr auto sizes = Sizes;

	explicit consteval
	UniHashableLenses2(const UniHashableLenses1& lenses) noexcept {
		FR_ASSERT(lenses.byte_hashables().size() == Sizes.num_byte_hashables);
		FR_ASSERT(lenses.others().size() == Sizes.num_others);

		for (auto i = 0zu; i < Sizes.num_byte_hashables; ++i) {
			const auto& lens1 = lenses.byte_hashables()[i];
			this->byte_hashables[i] = ByteHashableLens{lens1, this->buffer_size, this->num_words,
				this->num_words + lens1.num_words()};
			this->buffer_size += lens1.byte_size;
			this->num_words += lens1.num_words();
		}

		for (auto i = 0zu; i < Sizes.num_others; ++i) {
			this->others[i] = OtherLens{lenses.others()[i]};
		}
	}

public:
	std::array<ByteHashableLens, Sizes.num_byte_hashables> byte_hashables {};
	std::array<OtherLens, Sizes.num_others> others {};
	size_t buffer_size = 0zu;
	size_t num_words = 0;
};

template<class... Ts>
inline consteval
auto build_uni_hashable_lenses2_sizes(MpList<Ts...> types) {
	const auto lenses1 = UniHashableLenses1{types};

	const auto max_byte_hashables_path_size
		= lenses1.byte_hashables().empty()
		? 1zu
		: std::ranges::max_element(lenses1.byte_hashables(), {}, [](const auto& lens) {
			return lens.path.size();
		})->path.size();

	const auto max_others_path_size = lenses1.others().empty()
		? 1zu
		: std::ranges::max_element(lenses1.others(), {}, [](const auto& lens) {
			return lens.path.size();
		})->path.size();

	return UniHashableLenses2Sizes{
		lenses1.byte_hashables().size(),
		lenses1.others().size(),
		max_byte_hashables_path_size,
		max_others_path_size
	};
}

template<class... Ts>
inline consteval
auto build_uni_hashable_lenses2(MpList<Ts...> types) {
	constexpr auto sizes = build_uni_hashable_lenses2_sizes(types);
	const auto lenses1 = UniHashableLenses1{types};
	return UniHashableLenses2<sizes>{lenses1};
}

template<bool IsSeedProvided>
struct UniHasherBase { };

template<>
struct UniHasherBase<true> {
	HashDigest64 _seed;
};

} // namespace detail

/// @brief "Universal" hasher
template<UniHasherOpts Opts = {}>
class UniHasher: private detail::UniHasherBase<Opts.seeding == UniHasherSeeding::Provided> {
	using Base = detail::UniHasherBase<Opts.seeding == UniHasherSeeding::Provided>;
	using RapidAlgo = RapidhashAlgoType<Opts.algorithm, Opts.is_avalanching, Opts.is_dos_resistant>;
	using Word = typename RapidAlgo::Word;

public:
	using Digest = HashDigestOfSize<static_cast<size_t>(Opts.bits / CHAR_BIT)>;
	using Seed = HashDigest64;

	static constexpr auto opts = Opts;
	static_assert(Opts.validate_or_panic());

	struct State {

	public:
		explicit FR_FORCE_INLINE constexpr
		State(HashDigest64 seed) noexcept: _result{seed} { }

		FR_FORCE_INLINE constexpr
		void absorb_primitive(bool obj) noexcept {
			_result = city_hash_128_to_64(_result, obj
				? UINT64_C(0x66006600660066) : UINT64_C(0x99009900990099));
		}

		FR_FORCE_INLINE constexpr
		void absorb_primitive(std::nullptr_t) noexcept {
			return absorb_primitive(std::uintptr_t{});
		}

		template<std::integral T>
		FR_FORCE_INLINE constexpr
		void absorb_primitive(T obj) noexcept {
			if constexpr (Opts.bits <= 64) {
				_result = RapidAlgo::template hash_obj_seeded<sizeof(obj)>(obj, _result);
			}
			else if constexpr (Opts.bits == 128) {
				static_assert(false, "Unsupported");
			}
		}

		template<std::floating_point T>
		FR_FORCE_INLINE constexpr
		void absorb_primitive(T obj) noexcept {
			// The issue with `long double` is that it might have unused bytes
			if constexpr (std::is_same_v<T, long double>) {
				// Based on the Abseil algorithm.
				// PERF: About 10x slower than the `float` version
				const auto category = std::fpclassify(obj);
				absorb_primitive(category); // To minimize collisions across different FP classes
				switch (category) {
					case FP_NORMAL:
					case FP_SUBNORMAL: {
						const auto buff = LongDoubleBuffer{obj};
						_result = RapidAlgo::template hash_obj_seeded<buff.size()>(buff, _result);
						break;
					}
					case FP_INFINITE: {
						absorb_primitive(std::signbit(obj));
						break;
					}
					case FP_NAN:
					case FP_ZERO:
					default:
						break;
				}
			}
			else {
				// Ensure that -0.0 and +0.0 have the same hash code
				if (obj == T{})
					obj = T{};
				absorb_primitive(std::bit_cast<UIntOfSize<sizeof(T)>>(obj));
			}
		}

		template<c_hash_digest T>
		FR_FORCE_INLINE constexpr
		void absorb_digest(T digest) noexcept {
			if constexpr (sizeof(Digest) <= sizeof(HashDigest64)) {
				_result = detail::rapidhash_mix<opts.is_dos_resistant>(_result,
					static_cast<HashDigest64>(digest));
			}
			else
				static_assert(false);
		}

		template<c_hash_digest T>
		FR_FORCE_INLINE constexpr
		void absorb_digest_commutative(T digest) noexcept {
			if constexpr (sizeof(Digest) <= sizeof(HashDigest64)) {
				_result = hash_mix_commutative(_result, static_cast<HashDigest64>(digest));
			}
			else
				static_assert(false);
		}

		template<c_byte_like B, class SizeType>
		FR_FORCE_INLINE constexpr
		void absorb_bytes(const B* data, SizeType size) noexcept {
			absorb_primitive(size);
			_result = RapidAlgo::hash_bytes_seeded(data, static_cast<size_t>(size), _result);
		}

		template<auto Size, c_byte_like B>
		FR_FORCE_INLINE constexpr
		void absorb_fixed_bytes(const B* data) noexcept {
			if constexpr (Size <= RapidAlgo::max_short_size_bytes) {
				_result = RapidAlgo::template hash_bytes_short<Size>(data, _result);
			}
			else {
				_result = RapidAlgo::hash_bytes_seeded(data, static_cast<size_t>(Size), _result);
			}
		}

		template<class T>
		void absorb_object_bytes(const T& obj) noexcept {
			_result = RapidAlgo::template hash_obj_seeded<sizeof(T)>(obj, _result);
		}

		FR_FORCE_INLINE constexpr
		auto result() -> HashDigest64 { return _result; }

		FR_FORCE_INLINE constexpr
		void set_result(HashDigest64 result) noexcept { _result = result; }

	private:
		HashDigest64 _result;
	};

	/// @todo FIXME: Make SFINAE-friendly
	class Visitor: public HasherVisitorBase {
	public:
		using Hasher = UniHasher;

		explicit FR_FORCE_INLINE constexpr
		Visitor(const UniHasher& hasher, State& state) noexcept: _hasher{hasher}, _state{state} { }

		template<c_hashable... Ts>
		FR_FORCE_INLINE constexpr
		void operator()(const Ts&... objects) const noexcept {
			if constexpr (sizeof...(Ts) == 1zu) {
				absorb_dispatch(objects...);
			}
			else {
				static constexpr auto lenses = detail::build_uni_hashable_lenses2(mp_list<Ts...>);
				if constexpr (0zu < lenses.buffer_size
					&& lenses.buffer_size <= RapidAlgo::max_short_size_bytes
				) {
					absorb_lensed_bytes<lenses>(objects...);
				}
				else if constexpr (lenses.byte_hashables.size() != 0zu) {
#if 1
					absorb_lensed_blocks<lenses>(objects...);
#else
					unroll<lenses.byte_hashables.size()>([&]<size_t I> FR_FORCE_INLINE_L {
						static constexpr auto lens = lenses.byte_hashables[I];
						const auto& obj = detail::apply_uni_hashable_lens<lens>(objects...);
						absorb_dispatch(obj);
					});
#endif
				}

				if constexpr (lenses.others.size() != 0zu) {
					absorb_lensed_other_objects<lenses>(objects...);
				}
			}
		}

		auto hasher() const noexcept -> const UniHasher& { return _hasher; }
		auto state() const noexcept -> State& { return _state; }

	private:
		template<class T>
		FR_FORCE_INLINE constexpr
		void handle_byte_hashable(unsigned char* buffer, const T& obj) const noexcept {
			if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
				const auto f = obj == T{} ? T{} : obj;
				write_obj_as_bytes(buffer, f);
			}
			else {
				write_obj_as_bytes(buffer, obj);
			}
		}

		template<auto Lenses, class... Ts>
		FR_FORCE_INLINE constexpr
		void absorb_lensed_bytes(const Ts&... objects) const noexcept {
			static_assert(Lenses.buffer_size <= RapidAlgo::max_short_size_bytes);
			alignas(Word) unsigned char buffer[Lenses.buffer_size];
			unroll<Lenses.byte_hashables.size()>([&]<size_t I> FR_FORCE_INLINE_L {
				static constexpr auto lens = Lenses.byte_hashables[I];
				const auto& obj = detail::apply_uni_hashable_lens<lens>(objects...);
				handle_byte_hashable(buffer + lens.byte_offset, obj);
			});
			_state.template absorb_fixed_bytes<Lenses.buffer_size>(buffer);
		}

		template<auto Lenses, class... Ts>
		FR_FORCE_INLINE constexpr
		void absorb_lensed_blocks(const Ts&... objects) const noexcept {
			Word words[Lenses.num_words];
			unroll<Lenses.byte_hashables.size()>([&]<size_t I> FR_FORCE_INLINE_L {
				static constexpr auto lens = Lenses.byte_hashables[I];
				const auto& obj = detail::apply_uni_hashable_lens<lens>(objects...);
				if consteval {
					const auto bytes = std::bit_cast<SimpleArray<unsigned char, sizeof(obj)>>(obj);
					unroll<lens.num_words()>([&]<size_t WordOffset> FR_FORCE_INLINE_L {
						static constexpr auto chunk = WordOffset + 1zu == lens.num_words()
							? (lens.byte_size - sizeof(Word) * WordOffset)
							: sizeof(Word);
						words[lens.start_word_idx + WordOffset] = partial_read64<chunk>(
							bytes.data() + sizeof(Word) * WordOffset);
					});
				}
				else {
					unroll<lens.num_words()>([&]<size_t WordOffset> FR_FORCE_INLINE_L {
						static constexpr auto chunk = WordOffset + 1zu == lens.num_words()
							? (lens.byte_size - sizeof(Word) * WordOffset)
							: sizeof(Word);
						words[lens.start_word_idx + WordOffset] = partial_read64<chunk>(
							reinterpret_cast<const unsigned char*>(std::addressof(obj))
							+ sizeof(Word) * WordOffset);
					});
				}
			});
			auto stream = typename RapidAlgo::Stream{_state.result()};
			_state.set_result(stream.absorb_words(words, Lenses.num_words));
		}

		template<auto Lenses, class... Ts>
		FR_FORCE_INLINE constexpr
		void absorb_lensed_other_objects(const Ts&... objects) const noexcept {
			unroll<Lenses.others.size()>([&]<size_t I> FR_FORCE_INLINE_L {
				static constexpr auto lens = Lenses.others[I];
				const auto& obj = detail::apply_uni_hashable_lens<lens>(objects...);
				absorb_dispatch(obj);
			});
		}

		template<c_hashable T>
		FR_FORCE_INLINE constexpr
		void absorb_dispatch(const T& obj) const noexcept {
			static constexpr auto hashability = get_hashability<T>();
			using enum HashableCategory;
			if constexpr (hashability) {
				if constexpr (hashability.category() == Primitive) {
					static_assert(!c_has_custom_hash<std::remove_cvref_t<T>>);
					_state.absorb_primitive(obj);
				}
				else if constexpr (hashability.category() == Wrapper) {
					static_assert(!c_has_custom_hash<std::remove_cvref_t<T>>);
					absorb_wrapper(obj);
				}
				else if constexpr (hashability.category() == Custom) {
					fr_custom_hash(obj, *this);
				}
				else if constexpr (hashability.category() == Described) {
					absorb_described(obj);
				}
				else if constexpr (hashability.category() == Enum) {
					absorb_enum(obj);
				}
				else if constexpr (hashability.category() == Optional) {
					absorb_optional(obj);
				}
				else if constexpr (hashability.category() == String) {
					absorb_string(obj);
				}
				else if constexpr (hashability.category() == Array) {
					absorb_array(obj);
				}
				else if constexpr (hashability.category() == Range) {
					absorb_range(obj);
				}
				else if constexpr (hashability.category() == Record) {
					absorb_record(obj);
				}
				else {
					static_assert(false);
				}
			}
			else {
				static_assert(false);
			}
		}

		template<class T>
		FR_FORCE_INLINE constexpr
		void absorb_wrapper(Digest<T> digest) const noexcept {
			_state.absorb_digest(digest.value);
		}

		template<class B, class SizeType>
		FR_FORCE_INLINE constexpr
		void absorb_wrapper(Bytes<B, SizeType> bytes) const noexcept {
			_state.absorb_bytes(bytes.data, bytes.size);
		}

		template<class B, auto Size>
		FR_FORCE_INLINE constexpr
		void absorb_wrapper(FixedBytes<B, Size> bytes) const noexcept {
			_state.template absorb_fixed_bytes<Size>(bytes.data);
		}

		template<class Char, class SizeType>
		FR_FORCE_INLINE constexpr
		auto absorb_wrapper(String<Char, SizeType> str) const noexcept {
			if constexpr (c_byte_like<Char>) {
				_state.absorb_bytes(str.data, str.size);
			}
			else {
				const auto new_size = sizeof(Char) * static_cast<size_t>(str.size);
				if consteval {
					auto* const data = new unsigned char[new_size];
					write_str_as_bytes(data, str.data, static_cast<size_t>(str.size));
					_state.absorb_bytes(data, new_size);
					delete[] data;
				}
				else {
					_state.absorb_bytes(reinterpret_cast<const unsigned char*>(str.data), new_size);
				}
			}
		}

		template<class Char, auto Size>
		FR_FORCE_INLINE constexpr
		auto absorb_wrapper(FixedString<Char, Size> str) const noexcept {
			if constexpr (c_byte_like<Char>) {
				_state.template absorb_fixed_bytes<Size>(str.data);
			}
			else {
				constexpr auto new_size = sizeof(Char) * static_cast<size_t>(Size);
				if consteval {
					unsigned char data[new_size];
					write_str_as_bytes(data, str.data, Size);
					_state.template absorb_fixed_bytes<new_size>(data);
				}
				else {
					_state.template absorb_fixed_bytes<new_size>(
						reinterpret_cast<const unsigned char*>(new_size));
				}
			}
		}

		template<class T>
		FR_FORCE_INLINE
		void absorb_wrapper(Ptr<T> ptr) const noexcept {
			// NOTE: Must sync with whatever `absorb_primitive(std::nullptr_t)` is doing
			_state.absorb_primitive(std::bit_cast<uintptr_t>(ptr.get()));
		}

		template<class... Ts>
		FR_FORCE_INLINE constexpr
		void absorb_wrapper(Tuple<Ts...> tuple) const noexcept {
			[&]<size_t... Is>(std::index_sequence<Is...>) FR_FORCE_INLINE_L {
				(..., absorb_dispatch(std::get<Is>(tuple.args)));
			}(std::make_index_sequence<sizeof...(Ts)>{});
		}

		FR_FORCE_INLINE constexpr
		void absorb_commutative_step(State& sub_state, const auto& arg) const noexcept {
			auto elem_state = _hasher.init();
			Visitor{_hasher, elem_state}(arg);
			sub_state.absorb_digest_commutative(elem_state.result());
		}

		template<class T0, class... TRest>
		FR_FORCE_INLINE constexpr
		void absorb_wrapper(CommutativeTuple<T0, TRest...> tuple) const noexcept {
			// S_next = mix(S, cmix(cmix(cmix(a0, a1), a2), ..., an)))
			auto sub_state = _hasher.init();
			Visitor{_hasher, sub_state}(std::get<0>(tuple.args));

			[&]<size_t... Is>(std::index_sequence<Is...>) FR_FORCE_INLINE_L {
				(..., absorb_commutative_step(sub_state, std::get<1 + Is>(tuple.args)));
			}(std::make_index_sequence<sizeof...(TRest)>{});

			_state.absorb_digest(sub_state.result());
		}

		template<class Iter, class Sentinel, size_t Size>
		FR_FORCE_INLINE constexpr
		void absorb_wrapper(Range<Iter, Sentinel, Size> range) const noexcept {
			using V = std::iter_value_t<Iter>;
			if constexpr (std::sized_sentinel_for<Sentinel, Iter>) {
				const auto real_size = Size == npos
					? static_cast<size_t>(std::ranges::distance(range.begin, range.end))
					: Size;
				if constexpr (Size == npos) {
					_state.absorb_primitive(real_size);
				}
				if constexpr (std::contiguous_iterator<Iter>
					&& get_hashability<V>().mode() == HashableMode::AsBytes
				) {
					const auto size_bytes = sizeof(V) * real_size;
					if consteval {
						auto* const data = new unsigned char[size_bytes];
						write_range_as_bytes(data, range.begin, range.end);
						_state.absorb_bytes(data, size_bytes);
						delete[] data;
					}
					else {
						_state.absorb_bytes(
							reinterpret_cast<const unsigned char*>(std::to_address(range.begin)),
							size_bytes
						);
					}
				}
				else {
					for (; range.begin != range.end; ++range.begin) {
						absorb_dispatch(*range.begin);
					}
				}
			}
			else {
				for (; range.begin != range.end; ++range.begin) {
					// _state.absorb_digest(UINT64_C(0x1010101010101010));
					absorb_dispatch(*range.begin);
				}
				// End marker to prevent collision with the subsequent element
				_state.absorb_digest(UINT64_C(0xE1E2E3E4E5E6E7E8));
			}
		}

		/// @warning WARN: Commutative mixing is incompatible with DOS resistance
		template<class Iter, class Sentinel, size_t Size>
		FR_FORCE_INLINE constexpr
		void absorb_wrapper(CommutativeRange<Iter, Sentinel, Size> range) const noexcept {
			if constexpr (std::sized_sentinel_for<Sentinel, Iter> && Size == npos) {
				_state.absorb_primitive(range.end - range.begin);
			}

			if (range.begin == range.end)
				return;
			auto sub_state = _hasher.init();
			Visitor{_hasher, sub_state}(*range.begin);
			for (++range.begin; range.begin != range.end; ++range.begin) {
				absorb_commutative_step(sub_state, *range.begin);
			}

			if constexpr (!std::sized_sentinel_for<Sentinel, Iter>) {
				// End marker to prevent collision with the subsequent element
				_state.absorb_digest(UINT64_C(0xECE2E3E4E5E6E7E8));
			}
		}

		template<class T>
		FR_FORCE_INLINE constexpr
		void absorb_wrapper(Optional<T> opt) const noexcept {
			if (opt.value) {
				_state.absorb_digest(UINT64_C(0x8181818181818181));
				absorb_dispatch(*opt.value);
			}
			else {
				_state.absorb_digest(UINT64_C(0x8080808080808080));
			}
		}

		template<c_described_class T>
		FR_FORCE_INLINE constexpr
		void absorb_described(const T& obj) const noexcept {
			static constexpr auto mode = get_hashability<T>().mode();
			if constexpr (mode == HashableMode::AsBytes) {
				_state.absorb_object_bytes(obj);
			}
			else {
				if constexpr (mode == HashableMode::OptOut) {
					[&]<class... Bases>(MpList<Bases...>) FR_FORCE_INLINE_L {
						static_assert((true && ... && get_hashability<Bases>()));
						(..., absorb_dispatch(static_cast<const Bases&>(obj)));
					}(ReflBases<T>{});
				}
				else if constexpr (mode == HashableMode::OptIn) {
					for_each_type<ReflBases<T>>([&]<class Base> FR_FORCE_INLINE_L {
						if constexpr (get_hashability<Base>()) {
							absorb_dispatch(static_cast<const Base&>(obj));
						}
					});
				}

				for_each_type<ReflFieldsAndProperties<T>>([&]<class Child> FR_FORCE_INLINE_L {
					static constexpr auto should_hash = mode == HashableMode::OptOut
						? refl_attribute_or<Child, Hashable, Hashable{true}>
						: refl_attribute_or<Child, Hashable, Hashable{false}>;
					if constexpr (should_hash) {
						absorb_dispatch(get_field_or_property<Child>(obj));
					}
				});
			}
		}

		template<c_enum T>
		FR_FORCE_INLINE constexpr
		void absorb_enum(T e) const noexcept {
			_state.absorb_primitive(to_underlying(e));
		}

		template<c_optional_like O>
		FR_FORCE_INLINE constexpr
		void absorb_optional(const O& opt) const noexcept {
			using T = typename O::value_type;
			absorb_wrapper(Optional<T>{opt ? std::addressof(*opt) : nullptr});
		}

		template<class S>
		FR_FORCE_INLINE constexpr
		void absorb_string(const S& str) const noexcept {
			absorb_wrapper(String<typename S::value_type, size_t>{str.data(), str.size()});
		}

		template<c_array_like A>
		FR_FORCE_INLINE constexpr
		void absorb_array(const A& arr) const noexcept {
			absorb_range(arr);
		}

		template<std::ranges::input_range R>
		FR_FORCE_INLINE constexpr
		void absorb_range(R&& r) const noexcept {
			using It = std::ranges::iterator_t<R>;
			using St = std::ranges::sentinel_t<R>;
			if constexpr (c_constexpr_sized_range<R>) {
				absorb_wrapper(Range<It, St, std::ranges::size(r)>{
					std::ranges::begin(r),
					std::ranges::end(r)
				});
			}
			else {
				absorb_wrapper(Range<It, St>{std::ranges::begin(r), std::ranges::end(r)});
			}
		}

		template<c_record_like T>
		FR_FORCE_INLINE constexpr
		void absorb_record(const T& obj) const noexcept {
			static constexpr auto mode = get_hashability<T>().mode();
			if constexpr (mode == HashableMode::AsBytes) {
				_state.absorb_object_bytes(obj);
			}
			else if constexpr (mode == HashableMode::OptOut) {
				visit_record_fields(obj, [&]<class... Fs>(const Fs&... fields){
					(..., absorb_dispatch(fields));
				});
			}
			else {
				static_assert(false);
			}
		}

	private:
		const UniHasher& _hasher;
		State& _state;
	};

	FR_FORCE_INLINE constexpr
	UniHasher() noexcept
	requires (Opts.seeding != UniHasherSeeding::Provided) = default;

	explicit FR_FORCE_INLINE constexpr
	UniHasher(Seed seed) noexcept
	requires (Opts.seeding == UniHasherSeeding::Provided): Base{seed} { }

	template<c_hashable T>
	inline constexpr
	auto operator()(const T& obj) const noexcept {
		auto state = init();
		Visitor{*this, state}(obj);
		return finalize(state);
	}

	FR_FORCE_INLINE constexpr
	auto seed() const noexcept -> Seed {
		if constexpr (Opts.seeding == UniHasherSeeding::Stable)
			return Opts.seed;
		else if constexpr (Opts.seeding == UniHasherSeeding::Unstable)
			return hash_seed_unstable<Digest>;
		else if constexpr (Opts.seeding == UniHasherSeeding::Provided)
			return this->_seed;
		else
			static_assert(false);
	}

	FR_FORCE_INLINE constexpr
	void reseed(Seed new_seed) noexcept
	requires (Opts.seeding == UniHasherSeeding::Provided) {
		this->_seed = new_seed;
	}

private:
	FR_FORCE_INLINE constexpr
	auto init() const noexcept -> State { return State{seed()}; }

	static FR_FORCE_INLINE constexpr
	auto finalize(State& state) noexcept -> Digest {
		// Truncate 64-bit hash code if necessary
		return static_cast<Digest>(state.result());
	}
};

UniHasher() -> UniHasher<>;

using UniHasherFast32 = UniHasher<{
	.bits = 32,
	.is_avalanching = false,
	.seeding = UniHasherSeeding::Stable,
	.is_dos_resistant = false,
}>;

using UniHasherFast64 = UniHasher<{
	.bits = 64,
	.is_avalanching = false,
	.seeding = UniHasherSeeding::Stable,
	.is_dos_resistant = false,
}>;

using UniHasherFastStd = UniHasher<{
	.bits = CHAR_BIT * sizeof(size_t),
	.is_avalanching = false,
	.seeding = UniHasherSeeding::Stable,
	.is_dos_resistant = false,
}>;

using UniHasherQuality32 = UniHasher<{
	.bits = 32,
	.is_avalanching = true,
	.seeding = UniHasherSeeding::Stable,
	.is_dos_resistant = false,
}>;

using UniHasherQuality64 = UniHasher<{
	.bits = 64,
	.is_avalanching = true,
	.seeding = UniHasherSeeding::Stable,
	.is_dos_resistant = false,
}>;

using UniHasherProtected32 = UniHasher<{
	.bits = 32,
	.is_avalanching = true,
	.seeding = UniHasherSeeding::Provided,
	.is_dos_resistant = true,
}>;

using UniHasherProtected64 = UniHasher<{
	.bits = 64,
	.is_avalanching = true,
	.seeding = UniHasherSeeding::Provided,
	.is_dos_resistant = true,
}>;

} // namespace fr
#endif // include guard
