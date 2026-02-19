layout(location=0) in highp vec2 in_pos;
layout(location=1) in highp vec2 in_tex_coords;

out highp vec2 v_tex_coords;

void main() {
	gl_Position = vec4(in_pos, 0., 1.);
	v_tex_coords = in_tex_coords;
}
