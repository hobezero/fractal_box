#ifndef FRACTAL_BOX_GRAPHICS_SPRITE_HPP
#define FRACTAL_BOX_GRAPHICS_SPRITE_HPP

#include "fractal_box/core/ref.hpp"
#include "fractal_box/graphics/gl_mesh.hpp"
#include "fractal_box/graphics/gl_texture.hpp"

namespace fr {

/// @brief A visible sprite renderable on the screen
struct Sprite {
	// TODO: replace with material ID or something
	Ref<GlTexture2d> texture;
	Ref<GlMesh> mesh;
	/// @brief Z index in range [0; 1]. Controls the order of rendering.
	/// Sprite with a higher z index will be displayed in front of others
	float z_index = 0.5f;
	bool is_visible = true;
};

} // namespace fr
#endif // include guard
