MESSAGE( STATUS "Fetching SDL_image..." )

SET( SDL2IMAGE_INSTALL OFF )

FETCHCONTENT_DECLARE( sdl_image
	GIT_REPOSITORY https://github.com/libsdl-org/SDL_image
	GIT_TAG release-2.6.3
	)

FETCHCONTENT_MAKEAVAILABLE( sdl_image )

 