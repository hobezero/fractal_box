#ifndef ASTEROIDS_RESOURCES_HPP
#define ASTEROIDS_RESOURCES_HPP

#include "fractal_box/core/error.hpp"
#include "fractal_box/graphics/gl_mesh.hpp"
#include "fractal_box/graphics/gl_texture.hpp"
#include "asteroids/shaders.hpp"

namespace aster {

class GameResources {
public:
	auto init() -> fr::ErrorOr<>;

public:
	// Shaders
	ColorShader color_shader {};
	SpriteShader sprite_shader {};

	// Meshes
	fr::GlMesh mesh_line {};
	fr::GlMesh mesh_square_solid {};
	fr::GlMesh mesh_square_wire {};
	fr::GlMesh mesh_circle_solid {};
	fr::GlMesh mesh_circle_wire {};

	// Textures
	fr::GlTexture2d tex_spaceship {};
	fr::GlTexture2d tex_bullet {};
	std::array<fr::GlTexture2d, 4> tex_asteroids {};
	fr::GlTexture2d tex_plume {};
};

} // namespace aster
#endif // include guard
