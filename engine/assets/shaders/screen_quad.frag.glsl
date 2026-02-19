in highp vec2 v_tex_coords;

uniform sampler2D u_texture;

out highp vec4 o_frag_color;

void main() {
	o_frag_color = texture(u_texture, v_tex_coords);
}
