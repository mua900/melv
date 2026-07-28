cmake_minimum_required(VERSION 4.0)

project(melv)

add_library(melv
	src/util/common.hpp
	src/util/common.cpp
	src/util/template.hpp
	src/util/hash_table.hpp
	src/util/value.hpp
	src/util/log.hpp
	src/util/log.cpp
	src/util/string_util.hpp
	src/util/string_util.cpp
	src/util/math_util.hpp
	src/util/math_util.cpp
	src/util/file_util.hpp
	src/util/file_util.cpp

	src/app/application.hpp
	src/app/application.cpp
	src/app/audio.hpp
	src/app/audio.cpp
	src/app/input.hpp
	src/app/input.cpp
	src/app/time.hpp
	src/app/ui.hpp
	src/app/ui.cpp
	src/app/text.hpp
	src/app/asset.hpp
	src/app/asset.cpp
	src/app/draw.hpp
	src/app/draw.cpp
	src/app/camera.hpp
	src/app/camera.cpp
	src/app/animation.hpp
	src/app/animation.cpp
	src/app/serialize.hpp
	src/app/serialize.cpp
)

option(BUILD_EXAMPLE_PROGRAM OFF)
if (BUILD_EXAMPLE_PROGRAM)
	add_executable(
		example
		example/src/main.cpp
	)

	target_include_directories(example PRIVATE src)
	target_link_libraries(example PRIVATE melv)
endif()

target_include_directories(melv PUBLIC src/)

add_subdirectory(vendor/SDL EXCLUDE_FROM_ALL)

# SDL's helper libraries themselves depend on other libraries like libpng, zlib, freetype, harfbuzz etc.
# SDL's own configuration defaults to using system libraries if it's not targeting windows or android
# it can silently skip building support for a backend if it doesn't find required libraries on the system
# it is nice to have this explicitly under control under an option that is easier to control
option(USE_VENDORED_DEPENDENCIES ON)
if (USE_VENDORED_DEPENDENCIES)
	set(SDLIMAGE_VENDORED ON)
	set(SDLTTF_VENDORED ON)
	set(SDLMIXER_VENDORED ON)
endif()

# turn the ones you want formats on
set(SDLIMAGE_ANI  OFF)
set(SDLIMAGE_AVIF OFF)
set(SDLIMAGE_BMP  OFF)
set(SDLIMAGE_GIF  OFF)
set(SDLIMAGE_JPG  OFF)
set(SDLIMAGE_JXL  OFF)
set(SDLIMAGE_LBM  OFF)
set(SDLIMAGE_PCX  OFF)
set(SDLIMAGE_PNM  OFF)
set(SDLIMAGE_QOI  OFF)
set(SDLIMAGE_TGA  OFF)
set(SDLIMAGE_TIF  OFF)
set(SDLIMAGE_WEBP OFF)
set(SDLIMAGE_XCF  OFF)
set(SDLIMAGE_XPM  OFF)
set(SDLIMAGE_XV   OFF)
set(SDLIMAGE_PNG  ON)
set(SDLIMAGE_SVG  ON)
add_subdirectory(vendor/SDL_image EXCLUDE_FROM_ALL)

add_subdirectory(vendor/SDL_mixer EXCLUDE_FROM_ALL)
add_subdirectory(vendor/SDL_ttf EXCLUDE_FROM_ALL)

target_include_directories(melv PUBLIC vendor/SDL/include/)
target_include_directories(melv PUBLIC vendor/SDL_image/include/)
target_include_directories(melv PUBLIC vendor/SDL_mixer/include)
target_include_directories(melv PUBLIC vendor/SDL_ttf/include/)

target_link_libraries(
	melv
	PUBLIC
	SDL3::SDL3
	SDL3_image::SDL3_image
	SDL3_mixer::SDL3_mixer
	SDL3_ttf::SDL3_ttf
)
