#ifndef FRACTAL_BOX_UI_DIMGUI_UTIL_HPP
#define FRACTAL_BOX_UI_DIMGUI_UTIL_HPP

#include <span>
#include <string_view>

#include <fmt/format.h>
#include <glm/vec2.hpp>
#include <imgui.h>

#include "fractal_box/core/int_types.hpp"
#include "fractal_box/core/scope.hpp"

namespace fr {

inline
auto to_vec(ImVec2 src) noexcept -> glm::vec2{
	return {src.x, src.y};
}

inline
auto to_im_vec(glm::vec2 src) noexcept -> ImVec2 {
	return {src.x, src.y};
}

inline constexpr
auto rgb_to_im_vec4(uint32_t rgb) noexcept -> ImVec4 {
	const auto r = (rgb >> 16) & 0xFFu;
	const auto g = (rgb >> 8) & 0xFFu;
	const auto b = rgb & 0xFFu;
	return ImVec4{
		static_cast<float>(r) / 255.f,
		static_cast<float>(g) / 255.f,
		static_cast<float>(b) / 255.f,
		1.f
	};
}

inline constexpr
auto rgba_to_im_vec4(uint32_t rgba) noexcept -> ImVec4 {
	const auto r = (rgba >> 24) & 0xFFu;
	const auto g = (rgba >> 16) & 0xFFu;
	const auto b = (rgba >> 8) & 0xFFu;
	const auto a = rgba & 0xFFu;
	return ImVec4{
		static_cast<float>(r) / 255.f,
		static_cast<float>(g) / 255.f,
		static_cast<float>(b) / 255.f,
		static_cast<float>(a) / 255.f
	};
}

[[nodiscard]] inline
auto make_im_font_from_data(std::span<const char> font_data, float font_size) -> ImFont* {
	if (font_data.empty() || font_data.size() > size_t{INT_MAX})
		return nullptr;
	auto font_config = ImFontConfig{};
	font_config.FontDataOwnedByAtlas = false;
	// ImGui isn't const-correct despite claiming to never write to the font data.
	// Have to either copy data into a persistent storage or const_cast data. This is dumb
	return ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
		const_cast<char*>(font_data.data()), static_cast<int>(font_data.size()),
		font_size,
		&font_config
	);
}

template<class... Args>
inline
void im_text(fmt::format_string<Args...> fmt_str, Args&&... args) {
	const auto str = fmt::format(fmt_str, std::forward<Args>(args)...);
	ImGui::TextUnformatted(str.data(), str.data() + str.size());
}

inline
void im_text(std::string_view str) {
	ImGui::TextUnformatted(str.data(), str.data() + str.size());
}

inline
auto im_calc_text_size(
	std::string_view str,
	bool hide_text_after_double_dash = false,
	float wrap_width = -1.f
) -> ImVec2 {
	return ImGui::CalcTextSize(str.data(), str.data() + str.size(), hide_text_after_double_dash,
		wrap_width);
}

struct [[nodiscard]] DImGuiScopedWindow {
	explicit constexpr
	DImGuiScopedWindow(bool visible) noexcept: _is_visible{visible} { }

	DImGuiScopedWindow(const DImGuiScopedWindow&) = delete;
	auto operator=(const DImGuiScopedWindow&) -> DImGuiScopedWindow& = delete;

	DImGuiScopedWindow(DImGuiScopedWindow&&) = delete;
	auto operator=(DImGuiScopedWindow&&) -> DImGuiScopedWindow& = delete;

	~DImGuiScopedWindow() { ImGui::End(); }

	explicit constexpr
	operator bool() const noexcept { return _is_visible; }

private:
	bool _is_visible;
};

struct [[nodiscard]] DImGuiScopedTable {
	explicit constexpr
	DImGuiScopedTable(bool visible) noexcept: _is_visible{visible} { }

	DImGuiScopedTable(const DImGuiScopedTable&) = delete;
	auto operator=(const DImGuiScopedTable&) -> DImGuiScopedTable& = delete;

	DImGuiScopedTable(DImGuiScopedTable&&) = delete;
	auto operator=(DImGuiScopedTable&&) -> DImGuiScopedTable& = delete;

	~DImGuiScopedTable() {
		if (_is_visible)
			ImGui::EndTable();
	}

	explicit constexpr
	operator bool() const noexcept { return _is_visible; }

private:
	bool _is_visible;
};

struct [[nodiscard]] DImGuiScopedTreeNode {
	explicit constexpr
	DImGuiScopedTreeNode(bool visible) noexcept: _is_visible{visible} { }

	DImGuiScopedTreeNode(const DImGuiScopedTreeNode&) = delete;
	auto operator=(const DImGuiScopedTreeNode&) -> DImGuiScopedTreeNode& = delete;

	DImGuiScopedTreeNode(DImGuiScopedTreeNode&&) = delete;
	auto operator=(DImGuiScopedTreeNode&&) -> DImGuiScopedTreeNode& = delete;

	~DImGuiScopedTreeNode() {
		if (_is_visible)
			ImGui::TreePop();
	}

	explicit constexpr
	operator bool() const noexcept { return _is_visible; }

private:
	bool _is_visible;
};

[[nodiscard]] inline
auto im_scoped_id(int id) {
	ImGui::PushID(id);
	return ScopeExit{[] { ImGui::PopID(); }};
}

[[nodiscard]] inline
auto im_scoped_window(
	const char* label,
	bool* open = nullptr,
	ImGuiWindowFlags flags = {}
) -> DImGuiScopedWindow {
	return DImGuiScopedWindow{ImGui::Begin(label, open, flags)};
}

[[nodiscard]] inline
auto im_scoped_table(
	const char* label,
	int num_columns,
	ImGuiTableFlags flags = {},
	ImVec2 outer_size = ImVec2{0.f, 0.f},
	float inner_width = 0.f
) -> DImGuiScopedTable {
	return DImGuiScopedTable{ImGui::BeginTable(label, num_columns, flags, outer_size, inner_width)};
}

[[nodiscard]] inline
auto im_scoped_tree_node_ex(const char* label, ImGuiTreeNodeFlags flags = {}) {
	return DImGuiScopedTreeNode{ImGui::TreeNodeEx(label, flags)};
}

[[nodiscard]] inline
auto im_scoped_font(ImFont* font) {
	ImGui::PushFont(font);
	return ScopeExit{[] { ImGui::PopFont(); }};
}

[[nodiscard]] inline
auto im_scoped_color(ImGuiCol index, ImVec4 color) {
	ImGui::PushStyleColor(index, color);
	return ScopeExit{[] { ImGui::PopStyleColor(); }};
}

[[nodiscard]] inline
auto im_scoped_style_var(ImGuiStyleVar_ index, auto value) {
	ImGui::PushStyleVar(index, value);
	return ScopeExit{[] { ImGui::PopStyleVar(); }};
}

inline constexpr ImGuiWindowFlags im_overlay_win_flags
	= ImGuiWindowFlags_NoDecoration
	| ImGuiWindowFlags_NoMove
	| ImGuiWindowFlags_NoScrollbar
	| ImGuiWindowFlags_NoNav // ImGui will grab keyboard without this one
	| ImGuiWindowFlags_NoFocusOnAppearing
;

} // namespace fr
#endif // include guard
