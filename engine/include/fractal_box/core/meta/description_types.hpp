#ifndef FRACTAL_BOX_CORE_REFLECTION_DESCRIBE_TYPES_HPP
#define FRACTAL_BOX_CORE_REFLECTION_DESCRIBE_TYPES_HPP

#include "fractal_box/core/concepts.hpp"
#include "fractal_box/core/meta/meta_basics.hpp"
#include "fractal_box/core/string_literal.hpp"

namespace fr {

// c_class_description_part
// ^^^^^^^^^^^^^^^^^^^^^^^^

template<class T>
inline constexpr auto is_class_description_part = false;

template<class T>
using IsClassDescriptionPart = BoolC<is_class_description_part<T>>;

template<class T>
concept c_class_description_part = is_class_description_part<T>;

// c_field_description_part
// ^^^^^^^^^^^^^^^^^^^^^^^^

template<class T>
inline constexpr auto is_field_description_part = false;

template<class T>
using IsFieldDescriptionPart = BoolC<is_field_description_part<T>>;

template<class T>
concept c_field_description_part = is_class_description_part<T>;

// c_property_description_part
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^

template<class T>
inline constexpr auto is_property_description_part = false;

template<class T>
using IsPropertyDescriptionPart = BoolC<is_property_description_part<T>>;

template<class T>
concept c_property_description_part = is_property_description_part<T>;

// Name
// ^^^^

template<StringLiteral S>
struct Name {
	static constexpr auto value = S;
};

template<class T>
inline constexpr auto is_description_name = false;

template<StringLiteral S>
inline constexpr auto is_description_name<Name<S>> = true;

template<class T>
using IsDescriptionName = BoolC<is_description_name<T>>;

template<class T>
concept c_description_name = is_description_name<T>;

template<StringLiteral S>
inline constexpr auto is_class_description_part<Name<S>> = true;

template<StringLiteral S>
inline constexpr auto is_field_description_part<Name<S>> = true;

// DisplayName
// ^^^^^^^^^^^

template<StringLiteral S>
struct DisplayName {
	static constexpr auto value = S;
};

template<class T>
inline constexpr auto is_description_display_name = false;

template<StringLiteral S>
inline constexpr auto is_description_display_name<DisplayName<S>> = true;

template<class T>
using IsDescriptionDisplayName = BoolC<is_description_display_name<T>>;

template<class T>
concept c_description_display_name = is_description_display_name<T>;

template<StringLiteral S>
inline constexpr auto is_class_description_part<DisplayName<S>> = true;

template<StringLiteral S>
inline constexpr auto is_field_description_part<DisplayName<S>> = true;

template<StringLiteral S>
inline constexpr auto is_property_description_part<DisplayName<S>> = true;

// Bases
// ^^^^^

template<c_class... Ts>
struct Bases {
	using Parts = MpList<Ts...>;
};

template<class T>
inline constexpr auto is_description_bases = false;

template<class... Ts>
inline constexpr auto is_description_bases<Bases<Ts...>> = true;

template<class T>
using IsDescriptionBases = BoolC<is_description_bases<T>>;

template<class T>
concept c_description_bases = is_description_bases<T>;

template<class... Ts>
inline constexpr auto is_class_description_part<Bases<Ts...>> = true;

// Attributes
// ^^^^^^^^^^

template<auto... VParts>
struct Attributes {
	using Parts = MpValueList<VParts...>;
	static constexpr auto parts = Parts{};
};

template<class T>
inline constexpr auto is_description_attributes = false;

template<auto... Parts>
inline constexpr auto is_description_attributes<Attributes<Parts...>> = true;

template<class T>
using IsDescriptionAttributes = BoolC<is_description_attributes<T>>;

template<class T>
concept c_description_attributes = is_description_attributes<T>;

template<auto... Parts>
inline constexpr auto is_class_description_part<Attributes<Parts...>> = true;

template<auto... Parts>
inline constexpr auto is_field_description_part<Attributes<Parts...>> = true;

template<auto... Parts>
inline constexpr auto is_property_description_part<Attributes<Parts...>> = true;

// Field
// ^^^^^

template<auto Ptr, c_field_description_part... TParts>
struct Field {
	using PtrType = decltype(Ptr);
	static constexpr auto ptr = Ptr;
	using Parts = MpList<TParts...>;
};

template<class T>
inline constexpr auto is_description_field = false;

template<auto Ptr, class... Parts>
inline constexpr auto is_description_field<Field<Ptr, Parts...>> = true;

template<class T>
using IsDescriptionField = BoolC<is_description_field<T>>;

template<class T>
concept c_description_field = is_description_field<T>;

template<auto Ptr, class... Parts>
inline constexpr auto is_class_description_part<Field<Ptr, Parts...>> = true;

// Property
// ^^^^^^^^

template<
	StringLiteral Name,
	c_object T,
	auto Getter,
	auto Setter,
	c_property_description_part... TParts
>
struct Property {
	static constexpr auto name = Name;
	using Type = T;
	static constexpr auto getter = Getter;
	static constexpr auto setter = Setter;
	using Parts = MpList<TParts...>;

	using GetterType = std::remove_cvref_t<decltype(Getter)>;
	using SetterType = std::remove_cvref_t<decltype(Setter)>;

	static constexpr auto has_setter = [] -> bool {
		if constexpr (std::is_same_v<SetterType, std::nullptr_t>)
			return false;
		else if constexpr (std::is_pointer_v<SetterType> || std::is_member_function_pointer_v<T>)
			return Setter != nullptr;
		else
			return true;
	}();
};

template<class T>
inline constexpr auto is_description_property = false;

template<StringLiteral Name, class T, auto Getter, auto Setter, class... TParts>
inline constexpr auto is_description_property<Property<Name, T, Getter, Setter, TParts...>>
	= true;

template<class T>
using IsDescriptionProperty = BoolC<is_description_property<T>>;

template<class T>
concept c_description_property = is_description_property<T>;

template<StringLiteral Name, class T, auto Getter, auto Setter, class... TParts>
inline constexpr auto is_class_description_part<Property<Name, T, Getter, Setter, TParts...>>
	= true;

// ClassDesc
// ^^^^^^^^^

template<c_class_description_part... TParts>
struct ClassDesc {
	using Parts = MpList<TParts...>;
};

template<c_class_description_part... Parts>
inline constexpr auto class_desc = ClassDesc<Parts...>{};

template<class T>
inline constexpr auto is_class_desc = false;

template<class... Parts>
inline constexpr auto is_class_desc<ClassDesc<Parts...>> = true;

template<class T>
using IsClassDesc = BoolC<is_class_desc<T>>;

template<class T>
concept c_class_desc = is_class_desc<T>;

} // namespace fr
#endif // include guard
