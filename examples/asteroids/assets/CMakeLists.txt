include("${PROJECT_SOURCE_DIR}/cmake/CMakeRC.cmake")

cmrc_add_resource_library(asteroids_game_assets ALIAS asteroids::assets NAMESPACE aster
	fonts/Oxanium-SemiBold.ttf

	textures/spaceship.png
	textures/bullet.png
	textures/asteroid_00.png
	textures/asteroid_01.png
	textures/asteroid_02.png
	textures/asteroid_03.png
	textures/plume.png

	shaders/sprite.vert
	shaders/sprite.frag
	shaders/color.vert
	shaders/color.frag
)
