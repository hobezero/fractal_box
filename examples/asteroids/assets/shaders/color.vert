layout(location=0) in highp vec2 in_pos;
layout(location=1) in highp vec2 in_tex_coords;

uniform highp mat3 u_view_proj_mat;
uniform mediump float u_depth;

void main() {
	gl_Position.xy = (u_view_proj_mat * vec3(in_pos.xy, 1.f)).xy;
	gl_Position.z = u_depth;
	gl_Position.w = 1.f;
}
