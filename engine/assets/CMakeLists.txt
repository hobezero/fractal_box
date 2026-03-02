include("${PROJECT_SOURCE_DIR}/cmake/cmake_rc.cmake")

cmrc_add_resource_library(fractal_box_assets ALIAS fractal_box::assets NAMESPACE fr
	fonts/Inconsolata-Medium.otf

	shaders/screen_quad.vert.glsl
	shaders/screen_quad.frag.glsl
)
