#include "fractal_box/core/serialization/sbs_data_format.hpp"
#include "fractal_box/core/serialization/serialization_concepts.hpp"

#include <catch2/catch_test_macros.hpp>

#include "fractal_box/core/io/span_io.hpp"
#include "fractal_box/core/io/vector_io.hpp"

#include "test_common/test_helpers.hpp"

namespace {

struct CustomFriend {
	auto operator<=>(const CustomFriend&) const -> bool = default;

	template<class Ar>
	friend constexpr
	auto fr_custom_serialize(Ar& archive, fr::AddConstIf<CustomFriend, Ar::is_encoding>& self) {
		return archive(self.x, self.y);
	}

public:
	int x;
	std::string y;
};

struct CustomStatic {
	auto operator<=>(const CustomStatic&) const -> bool = default;

	static constexpr
	auto fr_custom_serialize(auto& archive, auto& self) {
		return archive(self.x, self.y);
	}

public:
	int x;
	std::string y;
};

} // namespace

TEST_CASE("Serialization-concepts", "[u][engine][core][serialization]") {
	STATIC_CHECK(fr::c_data_format<fr::SbsDataFormat>);

	STATIC_CHECK(fr::c_has_custom_serialize<CustomFriend>);
	STATIC_CHECK(fr::c_has_custom_serialize<CustomStatic>);

	STATIC_CHECK(fr::c_serializable<int>);
	STATIC_CHECK(fr::c_serializable<CustomFriend>);
	STATIC_CHECK(fr::c_serializable<CustomStatic>);
	STATIC_CHECK(fr::c_serializable<std::string>);
	STATIC_CHECK(fr::c_serializable<std::vector<std::string>>);
}

TEST_CASE("SbsDataFormat.primitives", "[u][engine][core][serialization]") {
	SECTION("serializing into a vector") {
		constexpr auto do_test = [] {
			const auto in_value1 = uint16_t{0x0A0B};
			const auto in_value2 = uint32_t{0x01020304};
			const auto in_value3 = 2.35;

			static constexpr auto value1_size = sizeof(in_value1);
			static constexpr auto value2_size = sizeof(in_value2);
			static constexpr auto value3_size = sizeof(in_value3);

			auto buf = std::vector<unsigned char>{};
			auto writer = fr::VectorWriter{buf};

			FRT_CHECK(fr::SbsDataFormat::encode(writer, in_value1) == value1_size);
			FRT_CHECK(buf == std::vector<unsigned char>{0x0B, 0x0A});

			FRT_CHECK(fr::SbsDataFormat::encode(writer, in_value2) == value2_size);
			FRT_CHECK(buf == std::vector<unsigned char>{0x0B, 0x0A, 0x04, 0x03, 0x02, 0x01});

			FRT_CHECK(fr::SbsDataFormat::encode(writer, in_value3) == value3_size);

			auto out_value1 = uint16_t{};
			auto out_value2 = uint32_t{};
			auto out_value3 = double{};

			auto reader = fr::SpanReader{buf};

			auto res1 = fr::SbsDataFormat::decode(reader, out_value1);
			FRT_REQUIRE(res1);
			FRT_CHECK(*res1 == value1_size);
			FRT_CHECK(out_value1 == in_value1);

			auto res2 = fr::SbsDataFormat::decode(reader, out_value2);
			FRT_REQUIRE(res2);
			FRT_CHECK(*res2 == value2_size);
			FRT_CHECK(out_value2 == in_value2);

			auto res3 = fr::SbsDataFormat::decode(reader, out_value3);
			FRT_REQUIRE(res3);
			FRT_CHECK(*res3 == value3_size);
			FRT_CHECK(out_value3 == in_value3);

			return true;
		};

		do_test();
		STATIC_CHECK(do_test());
	}
	SECTION("serializing into an array which is too small") {
		constexpr auto do_test = [] {
			auto in_value1 = uint16_t{0x0A0B};
			auto in_value2 = uint32_t{0x01020304};

			auto buf = std::array<unsigned char, 5>{};
			auto writer = fr::SpanWriter{buf};

			auto res1 = fr::SbsDataFormat::encode(writer, in_value1);
			FRT_REQUIRE(res1);
			FRT_CHECK(*res1 == 2);

			auto res2 = fr::SbsDataFormat::encode(writer, in_value2);
			FRT_CHECK(!res2);
			FRT_CHECK(res2.has_error<fr::BufferOverrun>());

			return true;
		};

		do_test();
		STATIC_CHECK(do_test());
	}
	SECTION("deserializing from an array which is too small") {
		constexpr auto do_test = [] {
			auto out_value = uint64_t{};

			auto buf = std::array<unsigned char, 5>{};
			auto reader = fr::SpanReader{buf};

			auto res = fr::SbsDataFormat::decode(reader, out_value);
			FRT_CHECK(!res);
			FRT_CHECK(res.has_error<fr::BufferOverrun>());

			return true;
		};

		do_test();
		STATIC_CHECK(do_test());
	}
}

static constexpr auto test_custom = []<class T> {
	SECTION("serializing into a vector") {
		constexpr auto do_test = [] {
			auto in_value = T{55, "abcdef"};
			static constexpr auto value_size = sizeof(int) + sizeof(size_t) + 6zu;

			auto buf = std::vector<unsigned char>{};
			auto writer = fr::VectorWriter{buf};

			FRT_CHECK(fr::SbsDataFormat::encode(writer, in_value) == value_size);

			auto out_value = T{};
			auto reader = fr::SpanReader{buf};

			auto res = fr::SbsDataFormat::decode(reader, out_value);
			FRT_REQUIRE(res);
			FRT_CHECK(*res == value_size);
			FRT_CHECK(out_value == in_value);

			return true;
		};

		do_test();
		STATIC_CHECK(do_test());
	}
	SECTION("serialializing into an array which is too small") {
		constexpr auto do_test = [] {
			auto in_value = T{55, "abcdef"};
			static constexpr auto value_size = sizeof(int) + sizeof(size_t) + 6zu;

			auto buf = std::array<std::byte, value_size - 2>{};
			auto writer = fr::SpanWriter{buf};

			auto res = fr::SbsDataFormat::encode(writer, in_value);
			FRT_CHECK(!res);
			FRT_CHECK(res.template has_error<fr::BufferOverrun>());

			return true;
		};
		do_test();
		STATIC_CHECK(do_test());
	}
	SECTION("deserializing from a span which is too small") {
		constexpr auto do_test = [] {
			auto in_value = T{55, "abcdef"};
			static constexpr auto value_size = sizeof(int) + sizeof(size_t) + 6zu;

			auto buf = std::vector<char>{};
			auto writer = fr::VectorWriter{buf};

			FRT_REQUIRE(fr::SbsDataFormat::encode(writer, in_value));

			auto out_value = T{};

			auto reader1 = fr::SpanReader{std::span<char>(buf.data(), buf.data() + 3)};
			auto res1 = fr::SbsDataFormat::decode(reader1, out_value);
			FRT_CHECK(!res1);
			FRT_CHECK(res1.template has_error<fr::BufferOverrun>());

			auto reader2 = fr::SpanReader{std::span<char>(buf.data(), buf.data() + value_size - 2)};
			auto res2 = fr::SbsDataFormat::decode(reader2, out_value);
			FRT_CHECK(!res2);
			FRT_CHECK(res2.template has_error<fr::BufferOverrun>());

			return true;
		};
		do_test();
		STATIC_CHECK(do_test());
	}
};

TEST_CASE("SbsDataFormat.custom", "[u][engine][core][serialization]") {
	frt::named_typed_section<CustomFriend>("through friend function", test_custom);
	frt::named_typed_section<CustomStatic>("through static memer function", test_custom);
}

template<class C>
constexpr auto string_lit1 = fr::detail::MpIllegal{};

template<>
constexpr char string_lit1<char>[] = "lorem ipsum dolor sit amet, consectetur";

template<>
constexpr char16_t string_lit1<char16_t>[] = u"lorem ipsum dolor sit amet, consectetur";

template<>
constexpr char32_t string_lit1<char32_t>[] = U"lorem ipsum dolor sit amet, consectetur";

template<class C>
constexpr auto string_lit2 = fr::detail::MpIllegal{};

template<>
constexpr char string_lit2<char>[] = "1234567890";

template<>
constexpr char16_t string_lit2<char16_t>[] = u"1234567890";

template<>
constexpr char32_t string_lit2<char32_t>[] = U"1234567890";

static constexpr auto test_sbs_strings = []<class C> {
	SECTION("serializing into a vector") {
		constexpr auto do_test = [] {
			auto in_value1 = std::basic_string<C>(string_lit1<C>);
			auto in_value2 = std::basic_string<C>(string_lit2<C>);

			const auto value1_size = sizeof(size_t) + sizeof(C) * in_value1.size();
			const auto value2_size = sizeof(size_t) + sizeof(C) * in_value2.size();

			auto buf = std::vector<std::byte>{};
			auto writer = fr::VectorWriter{buf};

			FRT_CHECK(fr::SbsDataFormat::encode(writer, in_value1) == value1_size);
			FRT_CHECK(fr::SbsDataFormat::encode(writer, in_value2) == value2_size);

			auto out_value1 = std::basic_string<C>{};
			auto out_value2 = std::basic_string<C>{};

			auto reader = fr::SpanReader{buf};

			auto res1 = fr::SbsDataFormat::decode(reader, out_value1);
			FRT_REQUIRE(res1);
			FRT_CHECK(*res1 == value1_size);
			FRT_CHECK(out_value1 == in_value1);

			auto res2 = fr::SbsDataFormat::decode(reader, out_value2);
			FRT_REQUIRE(res2);
			FRT_CHECK(*res2 == value2_size);
			FRT_CHECK(out_value2 == in_value2);

			return true;
		};

		do_test();
		STATIC_CHECK(do_test());
	}
	SECTION("serializing into an array which is too small") {
		constexpr auto do_test = [] {
			auto in_value = std::basic_string<C>(string_lit1<C>);

			auto buf = std::array<std::byte, 6>{};
			auto writer = fr::SpanWriter{buf};

			auto res = fr::SbsDataFormat::encode(writer, in_value);
			FRT_CHECK(!res);
			FRT_CHECK(res.template has_error<fr::BufferOverrun>());

			return true;
		};

		do_test();
		STATIC_CHECK(do_test());
	}
	SECTION("deserializing from a span which is too small") {
		constexpr auto do_test = [] {
			auto in_value = std::basic_string<C>(string_lit1<C>);

			auto buf = std::vector<std::byte>{};
			auto writer = fr::VectorWriter{buf};

			FRT_REQUIRE(fr::SbsDataFormat::encode(writer, in_value));

			std::string out_value;

			auto reader1 = fr::SpanReader{std::span<std::byte>(buf.data(), buf.data() + 3)};
			auto res1 = fr::SbsDataFormat::decode(reader1, out_value);
			FRT_CHECK(!res1);
			FRT_CHECK(res1.template has_error<fr::BufferOverrun>());

			auto reader2 = fr::SpanReader{std::span<std::byte>(buf.data(), buf.data() + 10)};
			auto res2 = fr::SbsDataFormat::decode(reader2, out_value);
			FRT_CHECK(!res2);
			FRT_CHECK(res2.template has_error<fr::BufferOverrun>());

			return true;
		};

		do_test();
		STATIC_CHECK(do_test());
	}
};

TEST_CASE("SbsDataFormat.optionals", "[u][engine][core][serialization]") {
	SECTION("serializing into a vector") {
		constexpr auto do_test = [] {
			const auto in_value1 = std::optional<int>{};
			const auto in_value2 = std::optional<int>{67};

			static constexpr auto value1_size = sizeof(bool);
			static constexpr auto value2_size = sizeof(bool) + sizeof(int);

			auto buf = std::vector<unsigned char>{};
			auto writer = fr::VectorWriter{buf};

			FRT_CHECK(fr::SbsDataFormat::encode(writer, in_value1) == value1_size);
			FRT_CHECK(fr::SbsDataFormat::encode(writer, in_value2) == value2_size);

			auto out_value1 = std::optional<int>{};
			auto out_value2 = std::optional<int>{};
			auto out_value3 = std::optional<int>{5};
			auto out_value4 = std::optional<int>{5};

			auto reader1 = fr::SpanReader{buf};
			auto reader2 = fr::SpanReader{buf};

			auto res1 = fr::SbsDataFormat::decode(reader1, out_value1);
			FRT_REQUIRE(res1);
			FRT_CHECK(*res1 == value1_size);
			FRT_CHECK(out_value1 == in_value1);

			auto res2 = fr::SbsDataFormat::decode(reader1, out_value2);
			FRT_REQUIRE(res2);
			FRT_CHECK(*res2 == value2_size);
			FRT_CHECK(out_value2 == in_value2);

			auto res3 = fr::SbsDataFormat::decode(reader2, out_value3);
			FRT_REQUIRE(res3);
			FRT_CHECK(*res3 == value1_size);
			FRT_CHECK(out_value3 == in_value1);

			auto res4 = fr::SbsDataFormat::decode(reader2, out_value4);
			FRT_REQUIRE(res4);
			FRT_CHECK(*res4 == value2_size);
			FRT_CHECK(out_value4 == in_value2);

			return true;
		};

		do_test();
		STATIC_CHECK(do_test());
	}
	SECTION("serialializing into an array which is too small") {
		constexpr auto do_test = [] {
			const auto in_value = std::optional<int>{5};
			static constexpr auto value_size = sizeof(bool) + sizeof(int);

			auto buf = std::array<std::byte, value_size - 3>{};
			auto writer = fr::SpanWriter{buf};

			auto res = fr::SbsDataFormat::encode(writer, in_value);
			FRT_CHECK(!res);
			FRT_CHECK(res.template has_error<fr::BufferOverrun>());

			return true;
		};
		do_test();
		STATIC_CHECK(do_test());
	}
	SECTION("deserializing from a span which is too small") {
		constexpr auto do_test = [] {
			auto in_value = std::optional<int>{66};
			static constexpr auto value_size = sizeof(bool) + sizeof(int);

			auto buf = std::vector<char>{};
			auto writer = fr::VectorWriter{buf};

			FRT_REQUIRE(fr::SbsDataFormat::encode(writer, in_value));

			auto out_value = std::optional<int>{};

			auto reader = fr::SpanReader{std::span<char>(buf.data(), buf.data() + value_size - 2)};
			auto res = fr::SbsDataFormat::decode(reader, out_value);
			FRT_CHECK(!res);
			FRT_CHECK(res.template has_error<fr::BufferOverrun>());

			return true;
		};
		do_test();
		STATIC_CHECK(do_test());
	}

}

TEST_CASE("SbsDataFormat.strings", "[u][engine][core][serialization]") {
	frt::named_typed_section<char>("std::string", test_sbs_strings);
	frt::named_typed_section<char16_t>("std::u16string", test_sbs_strings);
	frt::named_typed_section<char32_t>("std::u32string", test_sbs_strings);
}

TEST_CASE("SbsDataFormat.vectors", "[u][engine][core][serialization]") {
	SECTION("serializing into a vector") {
		constexpr auto do_test = [] {
			static constexpr int values[] = {22, 44, 66, 88};
			auto in_value = std::vector<int>(std::from_range, values);
			static constexpr auto value_size = sizeof(size_t) + sizeof(int) * std::size(values);

			auto buf = std::vector<unsigned char>{};
			auto writer = fr::VectorWriter{buf};

			FRT_CHECK(fr::SbsDataFormat::encode(writer, in_value) == value_size);

			auto out_value = std::vector<int>();
			auto reader = fr::SpanReader{buf};

			auto res = fr::SbsDataFormat::decode(reader, out_value);
			FRT_REQUIRE(res);
			FRT_CHECK(*res == value_size);
			FRT_CHECK(out_value == in_value);

			return true;
		};

		do_test();
		STATIC_CHECK(do_test());
	}
	SECTION("serialializing into an array which is too small") {
		constexpr auto do_test = [] {
			static constexpr int values[] = {22, 44, 66, 88};
			auto in_value = std::vector<int>(std::from_range, values);
			static constexpr auto value_size = sizeof(size_t) + sizeof(int) * std::size(values);

			auto buf = std::array<std::byte, value_size - 5>{};
			auto writer = fr::SpanWriter{buf};

			auto res = fr::SbsDataFormat::encode(writer, in_value);
			FRT_CHECK(!res);
			FRT_CHECK(res.template has_error<fr::BufferOverrun>());

			return true;
		};
		do_test();
		STATIC_CHECK(do_test());
	}
	SECTION("deserializing from a span which is too small") {
		constexpr auto do_test = [] {
			static constexpr int values[] = {22, 44, 66, 88};
			auto in_value = std::vector<int>(std::from_range, values);
			static constexpr auto value_size = sizeof(size_t) + sizeof(int) * std::size(values);

			auto buf = std::vector<char>{};
			auto writer = fr::VectorWriter{buf};

			FRT_REQUIRE(fr::SbsDataFormat::encode(writer, in_value));

			auto out_value = std::vector<int>();

			auto reader1 = fr::SpanReader{std::span<char>(buf.data(), buf.data() + 3)};
			auto res1 = fr::SbsDataFormat::decode(reader1, out_value);
			FRT_CHECK(!res1);
			FRT_CHECK(res1.template has_error<fr::BufferOverrun>());

			auto reader2 = fr::SpanReader{std::span<char>(buf.data(), buf.data() + value_size - 2)};
			auto res2 = fr::SbsDataFormat::decode(reader2, out_value);
			FRT_CHECK(!res2);
			FRT_CHECK(res2.template has_error<fr::BufferOverrun>());

			return true;
		};
		do_test();
		STATIC_CHECK(do_test());
	}
}
