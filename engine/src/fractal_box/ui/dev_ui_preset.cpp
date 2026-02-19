#include "fractal_box/ui/dev_ui_preset.hpp"

#include <cmrc/cmrc.hpp>

#include "fractal_box/core/out_ptr.hpp"
#include "fractal_box/graphics/debug_draw_config.hpp"
#include "fractal_box/graphics/debug_draw_preset.hpp"
#include "fractal_box/resources/resource_utils.hpp"
#include "fractal_box/runtime/core_preset.hpp"
#include "fractal_box/runtime/tracer_profiler.hpp"
#include "fractal_box/ui/dimgui_preset.hpp"
#include "fractal_box/ui/dimgui_utils.hpp"

CMRC_DECLARE(fr);

namespace fr {

using namespace std::string_view_literals;

struct DebugDrawUiSystem {
	static constexpr char display_name[] = "Debug Draw";

	static
	void run(AnySystem& self, DebugDrawConfig& debug_draw, const DebugDrawAdHocData& ad_hoc) {
		const auto window = im_scoped_window(display_name, out_ptr_for_enabled(self));
		if (!window)
			return;

		im_text("Ad hoc lines: {}", ad_hoc.shapes().lines.size());
		im_text("Ad hoc rects: {}", ad_hoc.shapes().rects.size());
		im_text("Ad hoc circles: {}", ad_hoc.shapes().circles.size());
		im_text("Ad hoc points: {}", ad_hoc.shapes().points.size());
		ImGui::Separator();

		ImGui::Checkbox("Master", &debug_draw.master_enabled);

		ImGui::Checkbox("##mesh_wire", &debug_draw.mesh_wire_enabled);
		ImGui::SameLine();
		ImGui::Checkbox("Mesh", &debug_draw.mesh_solid_enabled);

		ImGui::Checkbox("##collider_wire", &debug_draw.collider_wire_enabled);
		ImGui::SameLine();
		ImGui::Checkbox("Collider", &debug_draw.collider_solid_enabled);

		ImGui::Checkbox("##aabb_wf", &debug_draw.aabb_wire_enabled);
		ImGui::SameLine();
		ImGui::Checkbox("AABB", &debug_draw.aabb_solid_enabled);

		ImGui::Checkbox("Ad hoc", &debug_draw.adhoc_enabled);
	}
};

struct FpsOverlayUiSystem {
	static constexpr auto display_name = "FPS Overlay"sv;

	static
	void run(const DevUi& dev, const Profiler& profiler) {
		const auto node_it = profiler.nodes().find(CorePreset::frame_iter_label);
		const auto extra_node_it = profiler.extra_nodes().find(CorePreset::frame_iter_label);
		if (node_it == profiler.nodes().end() || extra_node_it == profiler.extra_nodes().end())
			return;
		const auto& node = node_it->second;
		const auto& extra_node = extra_node_it->second;

		const auto margin = 0.f;
		const auto* viewport = ImGui::GetMainViewport();
		// Bottom-left corner
		const auto win_pos = ImVec2{
			viewport->WorkPos.x + margin,
			viewport->WorkPos.y + viewport->WorkSize.y - margin
		};
		ImGui::SetNextWindowPos(win_pos, ImGuiCond_Always, ImVec2{0.f, 1.f});
		ImGui::SetNextWindowBgAlpha(0.3f);
		const auto no_border = im_scoped_style_var(ImGuiStyleVar_WindowBorderSize, 0.f);
		const auto padding = im_scoped_style_var(ImGuiStyleVar_WindowPadding, ImVec2{4.f, 4.f});
		const auto window = im_scoped_window("Metrics", nullptr,
			im_overlay_win_flags | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
		if (!window)
			return;
		const auto font = im_scoped_font(dev.metrics_font);

		const auto avg_fps = 1.f / chrono_cast<FDuration>(node.stats.avg).count();
		const auto p95_fps = 1.f / chrono_cast<FDuration>(extra_node.stats.p95).count();
		im_text("{:>4.0f} {:>4.0f}", avg_fps, p95_fps);
	}
};

FR_DIAGNOSTIC_PUSH
FR_DIAGNOSTIC_ERROR_MISSING_FIELD_INITIALIZERS

auto ProfilerUiConfig::make() noexcept -> ProfilerUiConfig {
	return {
		.stat_prefix_chars = 4,
		.stat_precision = 3,
		.spf_prefix_chars = 4,
		.spf_precision = 3,
		.hot_color_root = rgba_to_im_vec4(0x4B30BA'EFu),
		.hot_color_branch = rgba_to_im_vec4(0x901DB0'EFu),
		.hot_color_leaf = rgba_to_im_vec4(0xE40808'EFu),
		.hot_color_exponent = 0.9f,
	};
}

FR_DIAGNOSTIC_POP

struct ProfilerUiSystem {
	static constexpr auto display_name = "Profiler"sv;

	static
	void run(
		AnySystem& self,
		Runtime& runtime,
		const DevUi& dev,
		Profiler& profiler,
		const ProfilerUiConfig& config
	) {
		const auto char_width = ImGui::CalcTextSize("0").x;
		const auto stat_column_width = char_width
			* static_cast<float>(config.stat_prefix_chars + 1 + config.stat_precision);
		const auto spf_column_width = char_width
			* static_cast<float>(config.spf_prefix_chars + 1 + config.spf_precision);

		auto max_subtree_stats = MaxStats{};
		const auto max_roots_stats = MaxStats{profiler, StringId{}};

		const auto im_text_aligned_right = [&](float column_width, std::string_view text) {
			const auto text_width = static_cast<float>(text.size()) * char_width;
			ImGui::SetCursorPosX(ImGui::GetCursorPosX()
				+ std::max(0.f, column_width - text_width));
			im_text(text);
		};

		// Defer insertion/removal of extra nodes untill we are done iterating the container
		auto extras_requests = std::vector<ExtraNodeChange>{};

		const auto begin_tree_node = [&](
			StringId node_id,
			const Profiler::Node& node,
			const Profiler::ExtraNode* extra_node,
			int depth
		) -> std::pair<bool, TreeWalkControl> {
			const auto is_root = depth == 0;
			const auto is_leaf = node.children.empty();

			if (is_root) {
				// NOTE: Max stats are calculated per subtree exluding the tree root
				max_subtree_stats = MaxStats{profiler, node_id};
			}

			const auto stat_widget = [&](FDuration stat, FDuration max_stat = FDuration::zero()) {
				if (!ImGui::TableNextColumn())
					return;
				if (std::isnan(stat.count()))
					return;
				if (max_stat != FDuration::zero()) {
					const auto alpha = std::pow(stat.count() / max_stat.count(), 0.9f);
					auto color
						= is_root ? config.hot_color_root
						: is_leaf ? config.hot_color_leaf
						: config.hot_color_branch;
					color.w *= alpha;
					ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32(color));
				}

				im_text_aligned_right(stat_column_width, fmt::format("{:0.{}f}",
					chrono_cast<FDurationMs>(stat).count(), config.stat_precision));
			};

			ImGui::TableNextRow();

			const auto row_id = im_scoped_id(static_cast<int>(node_id.hash()));

			ImGui::TableNextColumn();
			const auto should_recurse = node.children.empty()
				? (ImGui::TreeNodeEx(node.name.c_str(), tree_node_flags_leaf), false)
				: ImGui::TreeNodeEx(node.name.c_str(), tree_node_flags_default);

			if (ImGui::TableNextColumn()) {
				if (node.user_data.semantics() == TraceEventSemantics::System) {
					static constexpr auto checkbox_id_value = StringId{"On"}.hash();
					const auto checkbox_id = im_scoped_id(static_cast<int>(checkbox_id_value));
					auto& system = runtime.get_system(SystemTypeIdx{adopt, node.user_data.index()});
					ImGui::Checkbox("", out_ptr_for_enabled(system));
				}
			}

			const auto& max_stats = is_root ? max_roots_stats : max_subtree_stats;
			stat_widget(node.last_sample, max_stats.last);
			stat_widget(node.stats.min, max_stats.min);
			stat_widget(node.stats.avg, max_stats.avg);
			stat_widget(node.stats.max, max_stats.max);

			if (ImGui::TableNextColumn()) {
				if (!std::isnan(node.stats.samples_per_tick)) {
					im_text_aligned_right(spf_column_width, fmt::format("{:0.{}f}",
						node.stats.samples_per_tick, config.spf_precision));
				}
			}

			if (ImGui::TableNextColumn()) {
				static constexpr auto checkbox_id_value = StringId{"Extras"}.hash();
				const auto checkbox_id = im_scoped_id(static_cast<int>(checkbox_id_value));

				auto has_extras = extra_node != nullptr;
				if (ImGui::Checkbox("", &has_extras))
					extras_requests.push_back({.node_id = node_id, .enabled = has_extras});
			}
			if (extra_node) {
				stat_widget(extra_node->stats.p50, max_stats.p50);
				stat_widget(extra_node->stats.p95, max_stats.p95);
			}

			return {should_recurse, {.recurse = should_recurse}};
		};

		const auto end_tree_node = [&](bool should_pop, auto&&...) {
			if (should_pop)
				ImGui::TreePop();
		};

		const auto window = im_scoped_window("Profiler", out_ptr_for_enabled(self));
		if (!window)
			return;
		// WARNING: Font MUST be monospace
		const auto font = im_scoped_font(dev.main_font);
		const auto table_size = ImVec2{0.f, ImGui::GetContentRegionAvail().y};
		if (const auto table = im_scoped_table("ProfilerTable", std::size(columns), table_flags,
			table_size)
		) {
			for (auto i = 0uz; i < std::size(columns); ++i)
				ImGui::TableSetupColumn(columns[i].name, columns[i].flags, 0.f);
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableHeadersRow();

			profiler.walk_tree_stateful(begin_tree_node, end_tree_node);
		}

		for (const auto& req : extras_requests) {
			if (req.enabled)
				profiler.add_extra_node(req.node_id);
			else
				profiler.remove_extra_node(req.node_id);
		}
	}

private:
	static constexpr auto table_flags
		= ImGuiTableFlags_Borders
		| ImGuiTableFlags_Resizable
		| ImGuiTableFlags_SizingFixedFit
		| ImGuiTableFlags_Reorderable
		| ImGuiTableFlags_Hideable
		| ImGuiTableFlags_ScrollY
		| ImGuiTableFlags_ScrollX
		// | ImGuiTableFlags_NoHostExtendX
		| ImGuiTableFlags_NoSavedSettings
	;

	struct ColumnMetadata {
		const char* name;
		ImGuiTableColumnFlags flags;
	};

	static constexpr auto column_flags_default = ImGuiTableColumnFlags_NoResize;
	static constexpr ColumnMetadata columns[] = {
		{"Label", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide},
		{"On", column_flags_default},
		{"Last", column_flags_default},
		{"Min", column_flags_default},
		{"Avg", column_flags_default},
		{"Max", column_flags_default},
		{"SPF", column_flags_default}, // Samples Per Frame
		{"Extras", column_flags_default},
		{"Median", column_flags_default},
		{"P95th", column_flags_default},
		{"", column_flags_default | ImGuiTableColumnFlags_NoHide},
	};

	static constexpr auto tree_node_flags_default
		= ImGuiTreeNodeFlags_SpanFullWidth
		| ImGuiTreeNodeFlags_FramePadding
		| ImGuiTreeNodeFlags_DefaultOpen
	;
	static constexpr auto tree_node_flags_leaf
		= tree_node_flags_default
		| ImGuiTreeNodeFlags_Leaf
		| ImGuiTreeNodeFlags_NoTreePushOnOpen
	;

	struct MaxStats {
		MaxStats() = default;

		/// @param root The root node of the subtree for which to calculated the stats.
		/// If `root == StringId{}`, then walk over only root nodes ignoring any children
		MaxStats(const Profiler& profiler, StringId root) {
			const auto update_one = [](FDuration& target, FDuration source) {
				if (std::isnan(source.count()))
					return;
				target = std::max(target, source);
			};

			const auto update_tree = [&] (
				StringId,
				const Profiler::Node& node,
				const Profiler::ExtraNode* extra_node,
				int depth
			) -> TreeWalkControl {
				if (depth > 0 || !root) {
					update_one(this->last, node.last_sample);
					update_one(this->min, node.stats.min);
					update_one(this->avg, node.stats.avg);
					update_one(this->max, node.stats.max);
					if (extra_node) {
						update_one(this->p50, extra_node->stats.p50);
						update_one(this->p95, extra_node->stats.p95);
					}
				}
				return {.recurse = bool{root}};
			};

			profiler.walk_tree_preorder(update_tree, root);
		}

	public:
		FDuration last = FDuration::zero();
		FDuration min = FDuration::zero();
		FDuration avg = FDuration::zero();
		FDuration max = FDuration::zero();
		FDuration p50 = FDuration::zero();
		FDuration p95 = FDuration::zero();
	};

	struct ExtraNodeChange {
		StringId node_id;
		bool enabled;
	};
};

struct DevUiInitSystem {
	static
	auto run(Runtime& runtime, Input& input, const DevUiConsts& consts) -> ErrorOr<> {
		auto fs = cmrc::fr::get_filesystem();
		const auto scale = 1.f; // TODO: DPI scaling
		const auto inconsolata = get_resource_data(fs, "fonts/Inconsolata-Medium.otf");

		auto* main_font = make_im_font_from_data(inconsolata, scale * consts.main_font_size);
		auto* metrics_font = make_im_font_from_data(inconsolata, scale
			* consts.metrics_font_size);

		if (!main_font)
			return make_error(Errc::ImGuiError, "DevUi: failed to create main font");
		if (!metrics_font)
			return make_error(Errc::ImGuiError, "DevUi: failed to create metrics font");

		DevUi tools;
		tools.window_shown = false;
		tools.main_font = main_font;
		tools.metrics_font = metrics_font;

		tools.add_tool<FpsOverlayUiSystem>(runtime, true);

		tools.add_tool<DebugDrawUiSystem>(runtime);

		runtime.try_add_part(ProfilerUiConfig::make());
		tools.add_tool<ProfilerUiSystem>(runtime);

		runtime.add_part(std::move(tools));

		const auto& keys = consts.shortcuts;
		input.map_to_action(keys.loop_toggle, SwitchEvent::JustPressed, action_loop_toggle);
		input.map_to_action(keys.loop_step, SwitchEvent::JustPressed, action_loop_step);
		input.map_to_action(keys.dev_ui_toggle, SwitchEvent::JustPressed, action_dev_ui_toggle);
		input.map_to_action(keys.debug_draw_toggle, SwitchEvent::JustPressed,
			action_debug_draw_toggle);

		return {};
	}
};

struct DevUiTranslateActionsSystem {
	static
	auto run(
		const Input& input,
		MessageListWriter<LoopRequests, DevUiRequests>& messages
	) {
		if (input.get_action(action_loop_toggle)) messages.push(ReqLoopToggle{});
		if (input.get_action(action_loop_step)) messages.push(ReqLoopStep{});
		if (input.get_action(action_dev_ui_toggle)) { messages.push(ReqDevUiToggle{}); }
		if (input.get_action(action_debug_draw_toggle)) messages.push(ReqDebugDrawToggle{});
	}
};

struct DevUiMessagesSystem {
	static
	auto run(DevUi& dev, DebugDrawConfig& debug_draw, MessageListReader<DevUiRequests>& messages) {
		messages.for_each_consume(Overload{
			[&](ReqDevUiToggle) { dev.window_shown = !dev.window_shown; },
			[&](ReqDebugDrawToggle) { debug_draw.master_enabled = !debug_draw.master_enabled; }
		});
	}
};

struct DevUiUpdateSystem {
	static
	auto run(
		Runtime& runtime,
		DevUi& dev,
		const DImGuiData& imgui,
		LoopStatus loop,
		MessageListWriter<LoopRequests>& messages
	) -> ErrorOr<> {
		ImGui::SetCurrentContext(imgui.dev_context);

		ImGui::StyleColorsDark();
		const auto font = im_scoped_font(dev.main_font);

		if (dev.window_shown)
			tools_window(runtime, dev, loop, messages);

		if (auto res = runtime.run_phase<DevToolsUiPhase>(); !res)
			return res;
		return {};
	}

private:
	static
	void loop_controls(LoopStatus loop, MessageListWriter<LoopRequests>& messages) {
		if (is_loop_advancing(loop)) {
			if (ImGui::Button("Interrupt"))
				messages.push(ReqLoopInterrupt{});
		}
		else {
			if (ImGui::Button("Continue"))
				messages.push(ReqLoopContinue{});
			ImGui::SameLine();
			if (ImGui::Button("Step"))
				messages.push(ReqLoopStep{.with_const_delta = false});
			ImGui::SameLine();
			if (ImGui::Button("Const Step"))
				messages.push(ReqLoopStep{.with_const_delta = true});
		}
	}

	static
	void tools_window(
		Runtime& runtime, DevUi& dev, LoopStatus loop,
		MessageListWriter<LoopRequests>& messages
	) {
		ImGui::SetNextWindowSize({370, 400}, ImGuiCond_Once);
		const auto window = im_scoped_window("Developer Tools", &dev.window_shown,
			ImGuiWindowFlags_NoFocusOnAppearing);
		if (!window)
			return;

		loop_controls(loop, messages);

		ImGui::Separator();
		for (const auto& tool : dev.tools()) {
			auto& system = runtime.get_system(tool.system_idx());
			ImGui::Checkbox(tool.display_name().c_str(), out_ptr_for_enabled(system));
		}
	}
};

void DevUiPreset::build(Runtime& runtime) {
	runtime
		.add_phase<DevToolsUiPhase>()
		.try_add_part(DevUiConsts{})
		.add_system<SetupPhase, DevUiInitSystem>()
		.add_system<FrameStartPhase, DevUiTranslateActionsSystem>()
		.add_system<FrameStartPhase, DevUiMessagesSystem>()
		.add_system<FrameRenderEarlyPhase, DevUiUpdateSystem>()
	;
}

} // namespace fr
