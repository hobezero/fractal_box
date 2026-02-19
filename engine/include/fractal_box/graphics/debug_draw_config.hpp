#ifndef FRACTAL_BOX_GRAPHICS_DEBUG_DRAW_CONFIG_HPP
#define FRACTAL_BOX_GRAPHICS_DEBUG_DRAW_CONFIG_HPP

namespace fr {

struct DebugDrawConfig {
	bool master_enabled = false;
	bool mesh_wire_enabled = false;
	bool mesh_solid_enabled = false;
	bool aabb_wire_enabled = false;
	bool aabb_solid_enabled = false;
	bool collider_wire_enabled = false;
	bool collider_solid_enabled = false;
	bool adhoc_enabled = true;
};

} // namespace fr
#endif
