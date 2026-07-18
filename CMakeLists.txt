cmake_minimum_required(VERSION 4.0)

project(melv)

add_library(melv
	src/util/common.hpp
	src/util/common.cpp
	src/util/template.hpp
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
	src/app/serialize.hpp
	src/app/serialize.cpp
)

target_include_directories(melv PRIVATE src/)

add_subdirectory(vendor/SDL EXCLUDE_FROM_ALL)

# turn unused image formats off
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

target_include_directories(melv PRIVATE vendor/SDL/include/)
target_include_directories(melv PRIVATE vendor/SDL_image/include/)
target_include_directories(melv PRIVATE vendor/SDL_mixer/include)
target_include_directories(melv PRIVATE vendor/SDL_ttf/include/)

target_link_libraries(
	melv
	PRIVATE
	SDL3::SDL3
	SDL3_image::SDL3_image
	SDL3_mixer::SDL3_mixer
	SDL3_ttf::SDL3_ttf
)
