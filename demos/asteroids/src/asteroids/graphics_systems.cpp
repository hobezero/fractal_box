#include "asteroids/graphics_systems.hpp"

#include <glad/glad.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL.h>

#include "fractal_box/components/collision.hpp"
#include "fractal_box/components/sprite.hpp"
#include "fractal_box/graphics/debug_draw_preset.hpp"
#include "fractal_box/graphics/gl_common.hpp"

namespace aster {

void WindowMessagesRenderSystem::run(fr::MessageListReader<fr::WindowMessages>& messages) {
	messages.for_each(fr::Overload{
		[&](const fr::WindowResized& msg) {
			glViewport(0, 0, msg.framebuffer_size.x, msg.framebuffer_size.y);
		},
		[](const auto&) { }
	});
}

void ClearFramebufferSystem::run(const GameConsts& consts) {
	const auto bg = consts.world.background;
	glClearColor(bg.r, bg.g, bg.b, bg.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

auto RenderSystem::run(
	World& world,
	const MainCamera& camera,
	GameResources& resources
) -> fr::ErrorOr<> {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Prevent the visual artifact on sprite edges observed when a fragment doesn't pass depth test
	// if drawn behind (and after) a translucent fragment with the same z index
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	const auto vp_mat = camera.value.make_view_proj_matrix();
	auto renderables = world.build_uncached_query<const fr::Transform, fr::Sprite>();
	auto& shader = resources.sprite_shader;
	shader.use();
	renderables.for_each([&](const fr::Transform& transform, fr::Sprite& sprite) {
		if (!sprite.is_visible)
			return;
		const auto mvp_mat = vp_mat * transform.make_model_matrix();
		shader.bindTexture(*sprite.texture);
		shader.setViewProjMatUniform(mvp_mat);
		shader.setDepthUniform(1.f - sprite.z_index);
		shader.draw(*sprite.mesh);
	});

	if (const auto error_flags = fr::get_all_gl_error_flags()) {
		return make_error_fmt(fr::Errc::OpenGlError, "RenderSystem failed: OpenGL error ({})",
			error_flags);
	}

	return {};
}

auto DebugDrawRenderSystem::run(
	const fr::DebugDrawConfig& config,
	const fr::DebugDrawAdHocData& adhoc,
	World& world,
	const MainCamera& camera,
	GameResources& resources
) -> fr::ErrorOr<> {
	if (!config.master_enabled)
		return {};

	const auto state_guard = fr::GlStateGuard{};

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST); // TODO: Implement depth testing of debug shapes

	const auto vp_mat = camera.value.make_view_proj_matrix();
	auto renderables = world.build_uncached_query<const fr::Transform, fr::Sprite>();

	auto& color_shader = resources.color_shader;
	color_shader.use();

	// AABB
	const auto aabbs = world.build_uncached_query<const fr::Collider>();
	if (config.aabb_wire_enabled) {
		color_shader.setDepthUniform(0.5f);
		color_shader.setColorUniform(fr::Color4{1.f, 1.f, 0.1f, 0.95f});
		aabbs.for_each([&](const fr::Collider& collider) {
			collider.visit(fr::Overload{
				[&](fr::FAaRect aabb) {
					const auto t = fr::ParallelTransform{aabb.center(), aabb.size()};
					const auto mvp_mat = vp_mat * t.make_model_matrix();
					color_shader.setViewProjMatUniform(mvp_mat);
					color_shader.draw(resources.mesh_square_wire);
				},
				[](auto&&) { }
			});
		});
	}
	if (config.aabb_solid_enabled) {
		color_shader.setDepthUniform(0.5f);
		color_shader.setColorUniform(fr::Color4{1.f, 1.f, 0.1f, 0.2f});
		aabbs.for_each([&](const fr::Collider& collider) {
			collider.visit(fr::Overload{
				[&](fr::FAaRect aabb) {
					const auto t = fr::ParallelTransform{aabb.center(),aabb.size()};
					const auto mvp_mat = vp_mat * t.make_model_matrix();
					color_shader.setViewProjMatUniform(mvp_mat);
					color_shader.draw(resources.mesh_square_solid);
				},
				[](auto&&) { }
			});
		});
	}

	// Mesh
	if (config.mesh_solid_enabled) {
		color_shader.setDepthUniform(0.1f);
		color_shader.setColorUniform(fr::Color4{0.3f, 1.f, 0.3f, 0.2f});
		renderables.for_each([&](const fr::Transform& transform, fr::Sprite& sprite) {
			const auto mvp_mat = vp_mat * transform.make_model_matrix();
			color_shader.setViewProjMatUniform(mvp_mat);
			color_shader.draw(*sprite.mesh);
		});
	}

#if FR_TARGET_GL_DESKTOP
	// WebGL doesn't suport glPolygonMode(..)
	if (config.mesh_wire_enabled) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		color_shader.setDepthUniform(0.1f);
		color_shader.setColorUniform(fr::Color4{0.1f, 1.f, 0.1f, 0.95f});
		renderables.for_each([&](const fr::Transform& transform, fr::Sprite& sprite) {
			const auto mvp_mat = vp_mat * transform.make_model_matrix();
			color_shader.setViewProjMatUniform(mvp_mat);
			color_shader.draw(*sprite.mesh);
		});
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
#endif

	// Collision
	if (config.collider_solid_enabled) {
		color_shader.setDepthUniform(0.5f);
		color_shader.setColorUniform(fr::Color4{1.f, 0.4f, 0.4f, 0.2f});
		aabbs.for_each([&](const fr::Collider& collider) {
			collider.visit(fr::Overload{
				[&](fr::FAaRect aabb) {
					const auto t = fr::ParallelTransform{aabb.center(), aabb.size()};
					const auto mvp_mat = vp_mat * t.make_model_matrix();
					color_shader.setViewProjMatUniform(mvp_mat);
					color_shader.draw(resources.mesh_square_solid);
				},
				[&](fr::FCircle circle) {
					const auto t = fr::ParallelTransform{circle.center(),
						{2.f * circle.radius(), 2.f * circle.radius()}};
					const auto mvp_mat = vp_mat * t.make_model_matrix();
					color_shader.setViewProjMatUniform(mvp_mat);
					color_shader.draw(resources.mesh_circle_solid);
				},
				[](auto&&) { }
			});
		});
	}

	if (config.collider_wire_enabled) {
		color_shader.setDepthUniform(0.5f);
		color_shader.setColorUniform(fr::Color4{1.f, 0.1f, 0.1f, 0.95f});
		aabbs.for_each([&](const fr::Collider& collider) {
			collider.visit(fr::Overload{
				[&](fr::FPoint2d point) {
					const auto t = fr::ParallelTransform{point, {5.f, 5.f}};
					const auto mvp_mat = vp_mat * t.make_model_matrix();
					color_shader.setViewProjMatUniform(mvp_mat);
					color_shader.draw(resources.mesh_circle_solid);
				},
				[&](fr::FAaRect aabb) {
					const auto t = fr::ParallelTransform{aabb.center(), aabb.size()};
					const auto mvp_mat = vp_mat * t.make_model_matrix();
					color_shader.setViewProjMatUniform(mvp_mat);
					color_shader.draw(resources.mesh_square_wire);
				},
				[&](fr::FCircle circle) {
					const auto t = fr::ParallelTransform{circle.center(),
						{2.f * circle.radius(), 2.f * circle.radius()}};
					const auto mvp_mat = vp_mat * t.make_model_matrix();
					color_shader.setViewProjMatUniform(mvp_mat);
					color_shader.draw(resources.mesh_circle_wire);
				},
				[](auto&&) { }
			});
		});
	}

	if (config.adhoc_enabled) {
		for (const auto& line : adhoc.shapes().lines.values()) {
			color_shader.setDepthUniform(line.z_index);
			color_shader.setColorUniform(line.color);
			color_shader.setViewProjMatUniform(vp_mat * line.make_transform().make_model_matrix());
			color_shader.draw(resources.mesh_line);
		}
		for (const auto& rect : adhoc.shapes().rects.values()) {
			color_shader.setDepthUniform(rect.z_index);
			color_shader.setColorUniform(rect.color);
			color_shader.setViewProjMatUniform(vp_mat * rect.make_transform().make_model_matrix());
			color_shader.draw(rect.shape_fill == fr::ShapeFill::Solid ? resources.mesh_square_solid
				: resources.mesh_square_wire);
		}
		for (const auto& circle : adhoc.shapes().circles.values()) {
			color_shader.setDepthUniform(circle.z_index);
			color_shader.setColorUniform(circle.color);
			color_shader.setViewProjMatUniform(vp_mat
				* circle.make_transform().make_model_matrix());
			color_shader.draw(circle.shape_fill == fr::ShapeFill::Solid
				? resources.mesh_circle_solid : resources.mesh_circle_wire);
		}
		for (const auto& point : adhoc.shapes().points.values()) {
			color_shader.setDepthUniform(point.z_index);
			color_shader.setColorUniform(point.color);
			// TODO: calculate scale in screen space
			color_shader.setViewProjMatUniform(vp_mat
				* point.make_transform(1.f).make_model_matrix());
			color_shader.draw(resources.mesh_circle_solid);
		}
	}

	glEnable(GL_DEPTH_TEST);

	if (const auto error_flags = fr::get_all_gl_error_flags()) {
		return make_error_fmt(fr::Errc::OpenGlError,
			"DebugDrawRenderSystem failed: OpenGL error ({})", error_flags);
	}
	return {};
}

void VfxSystem::run(World& world) {
	world.build_uncached_query<const Ship>().for_each([&](const Ship& ship) {
		auto* plume_surface = world.try_get_component<fr::Sprite>(ship.plume);
		if (!plume_surface) {
			FR_LOG_WARN("{}", "Ship: invalid plume entity id");
			return;
		}
		plume_surface->is_visible = ship.control.engine_burn_main > 0.f;
	});
}

} // namespace aster
