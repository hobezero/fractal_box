#include "fractal_box/core/io/span_io.hpp"
#include "fractal_box/core/io/io_concepts.hpp"

#include <array>

#include <catch2/catch_test_macros.hpp>

#include "fractal_box/core/array_utils.hpp"

TEST_CASE("SpanWriter", "[u][engine][core][io]") {
	SECTION("type properties") {
		STATIC_CHECK(fr::c_writer<fr::SpanWriter<std::byte>>);
		STATIC_CHECK(fr::c_writer<fr::SpanWriter<char16_t>>);

		STATIC_CHECK_FALSE(std::is_copy_constructible_v<fr::SpanWriter<char>>);
		STATIC_CHECK_FALSE(std::is_copy_assignable_v<fr::SpanWriter<char>>);

		STATIC_CHECK(std::is_move_constructible_v<fr::SpanWriter<char>>);
		STATIC_CHECK(std::is_nothrow_move_constructible_v<fr::SpanWriter<char>>);
		STATIC_CHECK(std::is_move_assignable_v<fr::SpanWriter<char>>);
		STATIC_CHECK(std::is_nothrow_move_assignable_v<fr::SpanWriter<char>>);

		STATIC_CHECK(std::is_destructible_v<fr::SpanWriter<char>>);
		STATIC_CHECK(std::is_trivially_destructible_v<fr::SpanWriter<char>>);
	}

	auto buf = std::array<char, 15>{};
	auto writer = fr::SpanWriter{buf};

	{
		const char chunk[] = {'a', 'b', 'c'};
		const auto res = writer.write(chunk);
		REQUIRE(res.has_value());
		CHECK(*res == 3);
		CHECK(buf == std::array<char, 15>{'a', 'b', 'c'});
	}
	{
		const char chunk[] = {'d', 'e', 'f', 'g', 'h', 'i'};
		const auto res = writer.write(chunk);
		REQUIRE(res.has_value());
		CHECK(*res == 6);
		CHECK(buf == std::array<char, 15>{'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i'});
	}
	{
		const auto res = writer.write(std::span<const char>{});
		REQUIRE(res.has_value());
		CHECK(*res == 0);
		CHECK(buf == std::array<char, 15>{'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i'});
	}
	SECTION("exact fit") {
		const char chunk[] = {'j', 'k', 'l', 'm', 'n', 'o'};
		const auto res = writer.write(chunk);
		REQUIRE(res.has_value());
		CHECK(*res == 6);
		CHECK(buf == std::array<char, 15>{'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k',
			'l', 'm', 'n', 'o'});
	}
	SECTION("overfit") {
		const char chunk[] = {'j', 'k', 'l', 'm', 'n', 'o', 'p'};
		const auto res = writer.write(chunk);
		REQUIRE_FALSE(res.has_value());
		CHECK(res.has_error<fr::Eof>());
		CHECK(buf == std::array<char, 15>{'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i'});
	}
}

template<size_t N, size_t SrcSize>
static constexpr
auto padded_str(const char (&src)[SrcSize]) {
	return fr::make_array_n_from<N>(std::string_view(src, SrcSize - 1));
}

TEST_CASE("SpanReader", "[u][engine][core][io]") {
	SECTION("type properties") {
		STATIC_CHECK(fr::c_reader<fr::SpanReader<std::byte>>);
		STATIC_CHECK(fr::c_reader<fr::SpanReader<char16_t>>);

		STATIC_CHECK_FALSE(std::is_copy_constructible_v<fr::SpanReader<char>>);
		STATIC_CHECK_FALSE(std::is_copy_assignable_v<fr::SpanReader<char>>);

		STATIC_CHECK(std::is_move_constructible_v<fr::SpanReader<char>>);
		STATIC_CHECK(std::is_nothrow_move_constructible_v<fr::SpanReader<char>>);
		STATIC_CHECK(std::is_move_assignable_v<fr::SpanReader<char>>);
		STATIC_CHECK(std::is_nothrow_move_assignable_v<fr::SpanReader<char>>);

		STATIC_CHECK(std::is_destructible_v<fr::SpanReader<char>>);
		STATIC_CHECK(std::is_trivially_destructible_v<fr::SpanReader<char>>);
	}

	constexpr const char buf[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
		'n', 'o'};
	constexpr auto buf_size = std::size(buf);
	auto reader = fr::SpanReader{buf};

	{
		auto chunk = std::array<char, buf_size>{};
		const auto res = reader.read(std::span(chunk.data(), 3));
		REQUIRE(res.has_value());
		CHECK(*res == 3);
		CHECK(chunk == padded_str<buf_size>("abc"));
	}
	{
		auto chunk = std::array<char, buf_size>{};
		const auto res = reader.read(std::span(chunk.data(), 6));
		REQUIRE(res.has_value());
		CHECK(*res == 6);
		CHECK(chunk == padded_str<buf_size>("defghi"));
	}
	{
		auto chunk = std::array<char, buf_size>{};
		const auto res = reader.read(std::span(chunk.data(), 0));
		REQUIRE(res.has_value());
		CHECK(*res == 0);
		CHECK(chunk == padded_str<buf_size>(""));
	}
	SECTION("exact fit") {
		auto chunk = std::array<char, buf_size>{};
		const auto res = reader.read(std::span(chunk.data(), 6));
		REQUIRE(res.has_value());
		CHECK(*res == 6);
		CHECK(chunk == padded_str<buf_size>("jklmno"));
	}
	SECTION("overfit") {
		auto chunk = std::array<char, buf_size>{};
		const auto res = reader.read(std::span(chunk.data(), 8));
		REQUIRE(res.has_value());
		CHECK(*res == 6);
		CHECK(chunk == padded_str<buf_size>("jklmno"));
	}
}
