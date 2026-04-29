#include "fractal_box/core/io/io_concepts.hpp"
#include "fractal_box/core/io/span_io.hpp"
#include "fractal_box/core/io/string_io.hpp"
#include "fractal_box/core/io/vector_io.hpp"

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
		CHECK(res.has_error<fr::BufferOverrun>());
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

	SECTION("read(..)") {
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
	SECTION("read_exact(..)") {
		{
			auto chunk = std::array<char, buf_size>{};
			const auto res = reader.read_exact(std::span(chunk.data(), 5));
			REQUIRE(res.has_value());
			CHECK(chunk == padded_str<buf_size>("abcde"));
		}
		{
			auto chunk = std::array<char, buf_size>{};
			const auto res = reader.read_exact(std::span(chunk.data(), 50));
			CHECK(res.has_error<fr::BufferOverrun>());
		}
	}
}

TEST_CASE("StringWriter", "[u][engine][core][io]") {
	SECTION("type properties") {
		STATIC_CHECK(fr::c_writer<fr::StringWriter<std::string>>);
		STATIC_CHECK(fr::c_writer<fr::StringWriter<std::u16string>>);

		STATIC_CHECK_FALSE(std::is_copy_constructible_v<fr::StringWriter<std::string>>);
		STATIC_CHECK_FALSE(std::is_copy_assignable_v<fr::StringWriter<std::string>>);

		STATIC_CHECK(std::is_move_constructible_v<fr::StringWriter<std::string>>);
		STATIC_CHECK(std::is_nothrow_move_constructible_v<fr::StringWriter<std::string>>);
		STATIC_CHECK(std::is_move_assignable_v<fr::StringWriter<std::string>>);
		STATIC_CHECK(std::is_nothrow_move_assignable_v<fr::StringWriter<std::string>>);

		STATIC_CHECK(std::is_destructible_v<fr::StringWriter<std::string>>);
		STATIC_CHECK(std::is_trivially_destructible_v<fr::StringWriter<std::string>>);
	}

	auto buf = std::string{};
	auto writer = fr::StringWriter{buf};

	CHECK(writer.buffer().empty());
	{
		const char chunk[] = {'a', 'b', 'c'};
		CHECK(writer.write(chunk) == 3);
		CHECK(buf == "abc");
		CHECK(writer.buffer().empty());
	}
	{
		const char chunk[] = {'d', 'e', 'f', 'g', 'h', 'i'};
		CHECK(writer.write(chunk) == 6);
		CHECK(buf == "abcdefghi");
		CHECK(writer.buffer().empty());
	}
	{
		INFO("write zero bytes");
		CHECK(writer.write(std::span<const char>{}) == 0);
		CHECK(buf == "abcdefghi");
		CHECK(writer.buffer().empty());
	}
	{
		INFO("commit from a buffer");
		const char chunk[] = {'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w'};
		writer.resize_buffer(std::size(chunk) + 3);
		CHECK(writer.buffer().size() == std::size(chunk) + 3);
		std::ranges::copy(chunk, writer.buffer().begin());
		writer.commit_buffer(std::size(chunk));
		CHECK(writer.buffer().size() == 3);
	}
	{
		INFO("fill up the rest of the buffer");
		const char chunk[] = {'x', 'y', 'z'};
		std::ranges::copy(chunk, writer.buffer().begin());
		writer.commit_buffer(std::size(chunk));
		CHECK(buf == "abcdefghijklmnopqrstuvwxyz");
		CHECK(writer.buffer().empty());
	}
	{
		INFO("write after resizing buffer (buffer is larger)");
		const char chunk[] = {'A', 'B', 'C'};
		writer.resize_buffer(5);
		writer.write(chunk);
		CHECK(buf == "abcdefghijklmnopqrstuvwxyzABC");
		CHECK(writer.buffer().empty());
	}
	{
		INFO("write after resizing buffer (buffer is smaller)");
		const char chunk[] = {'D', 'E', 'F'};
		writer.resize_buffer(2);
		writer.write(chunk);
		CHECK(buf == "abcdefghijklmnopqrstuvwxyzABCDEF");
		CHECK(writer.buffer().empty());
	}
}

template<size_t N>
static constexpr
auto make_str_vec(const char (&arr)[N]) {
	return std::vector<char>(arr, arr + N - 1zu);
}

TEST_CASE("VectorWriter", "[u][engine][core][io]") {
	SECTION("type properties") {
		STATIC_CHECK(fr::c_writer<fr::VectorWriter<std::vector<char>>>);
		STATIC_CHECK(fr::c_writer<fr::VectorWriter<std::vector<std::byte>>>);

		STATIC_CHECK_FALSE(std::is_copy_constructible_v<fr::VectorWriter<std::vector<char>>>);
		STATIC_CHECK_FALSE(std::is_copy_assignable_v<fr::VectorWriter<std::vector<char>>>);

		STATIC_CHECK(std::is_move_constructible_v<fr::VectorWriter<std::vector<char>>>);
		STATIC_CHECK(std::is_nothrow_move_constructible_v<fr::VectorWriter<std::vector<char>>>);
		STATIC_CHECK(std::is_move_assignable_v<fr::VectorWriter<std::vector<char>>>);
		STATIC_CHECK(std::is_nothrow_move_assignable_v<fr::VectorWriter<std::vector<char>>>);

		STATIC_CHECK(std::is_destructible_v<fr::VectorWriter<std::vector<char>>>);
		STATIC_CHECK(std::is_trivially_destructible_v<fr::VectorWriter<std::vector<char>>>);
	}

	auto buf = std::vector<char>{};
	auto writer = fr::VectorWriter{buf};

	CHECK(writer.buffer().empty());
	{
		const char chunk[] = {'a', 'b', 'c'};
		CHECK(writer.write(chunk) == 3);
		CHECK(buf == make_str_vec("abc"));
		CHECK(writer.buffer().empty());
	}
	{
		const char chunk[] = {'d', 'e', 'f', 'g', 'h', 'i'};
		CHECK(writer.write(chunk) == 6);
		CHECK(buf == make_str_vec("abcdefghi"));
		CHECK(writer.buffer().empty());
	}
	{
		INFO("write zero bytes");
		CHECK(writer.write(std::span<const char>{}) == 0);
		CHECK(buf == make_str_vec("abcdefghi"));
		CHECK(writer.buffer().empty());
	}
	{
		INFO("commit from a buffer");
		const char chunk[] = {'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w'};
		writer.resize_buffer(std::size(chunk) + 3);
		CHECK(writer.buffer().size() == std::size(chunk) + 3);
		std::ranges::copy(chunk, writer.buffer().begin());
		writer.commit_buffer(std::size(chunk));
		CHECK(writer.buffer().size() == 3);
	}
	{
		INFO("fill up the rest of the buffer");
		const char chunk[] = {'x', 'y', 'z'};
		std::ranges::copy(chunk, writer.buffer().begin());
		writer.commit_buffer(std::size(chunk));
		CHECK(buf == make_str_vec("abcdefghijklmnopqrstuvwxyz"));
		CHECK(writer.buffer().empty());
	}
	{
		INFO("write after resizing buffer (buffer is larger)");
		const char chunk[] = {'A', 'B', 'C'};
		writer.resize_buffer(5);
		writer.write(chunk);
		CHECK(buf == make_str_vec("abcdefghijklmnopqrstuvwxyzABC"));
		CHECK(writer.buffer().empty());
	}
	{
		INFO("write after resizing buffer (buffer is smaller)");
		const char chunk[] = {'D', 'E', 'F'};
		writer.resize_buffer(2);
		writer.write(chunk);
		CHECK(buf == make_str_vec("abcdefghijklmnopqrstuvwxyzABCDEF"));
		CHECK(writer.buffer().empty());
	}
}
