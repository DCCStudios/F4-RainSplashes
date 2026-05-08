cmake_minimum_required(VERSION 3.21)

macro(set_from_environment VARIABLE)
	if(NOT DEFINED ${VARIABLE} AND DEFINED ENV{${VARIABLE}})
		set(${VARIABLE} $ENV{${VARIABLE}})
	endif()
endmacro()

set_from_environment(VCPKG_ROOT)
set_from_environment(Fallout4Path)

if(DEFINED VCPKG_ROOT)
	set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
	set(VCPKG_TARGET_TRIPLET "x64-windows-static-md" CACHE STRING "")
else()
	message(WARNING "VCPKG_ROOT is not set.")
endif()

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" CACHE STRING "")

project(RainSplashesF4SE VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Compile/F4SE/Plugins")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Compile/F4SE/Plugins")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/Compile/F4SE/Plugins")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/Compile/F4SE/Plugins")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/Compile/F4SE/Plugins")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/Compile/F4SE/Plugins")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../PluginTemplate/CommonLibF4/cmake")

if(NOT TARGET CommonLibF4)
	add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../PluginTemplate/CommonLibF4/CommonLibF4" CommonLibF4)
endif()

find_package(spdlog REQUIRED CONFIG)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(mmio REQUIRED CONFIG)

set(SOURCES
	src/PCH.cpp
	src/main.cpp
	src/Hooks.cpp
	src/RelSanity.cpp
	src/RainSplashes.cpp
	src/RayCast.cpp
	src/TerrainHeight.cpp
	src/Settings.cpp
	src/PluginPaths.cpp
	src/WeatherOverrides.cpp
	src/UI.cpp
)

set(HEADERS
	src/PCH.h
	src/Hooks.h
	src/RelSanity.h
	src/RainSplashes.h
	src/RayCast.h
	src/TerrainHeight.h
	src/Settings.h
	src/PluginPaths.h
	src/WeatherOverrides.h
	src/UI.h
	src/SimpleIni.h
	include/F4SEMenuFramework.h
)

add_library(${PROJECT_NAME} SHARED ${SOURCES} ${HEADERS})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

target_compile_definitions(${PROJECT_NAME} PRIVATE
	BEAUTIFUL_NAME="Rain Splashes F4SE"
	MOD_NAME="Rain Splashes F4SE"
	PRODUCT_NAME="RainSplashesF4SE"
	_UNICODE
	UNICODE
	NOMINMAX
	_CRT_SECURE_NO_WARNINGS
	_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
)

target_include_directories(${PROJECT_NAME} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
	CommonLibF4::CommonLibF4
	mmio::mmio
	nlohmann_json::nlohmann_json
	spdlog::spdlog
)

target_precompile_headers(${PROJECT_NAME} PRIVATE src/PCH.h)

if(MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE
		/sdl /utf-8 /Zi /permissive- /Zc:preprocessor /EHsc /W4 /WX-
		/wd4100 /wd4189 /wd4244 /wd4702
	)
	target_link_options(${PROJECT_NAME} PRIVATE
		"$<$<CONFIG:RELEASE>:/INCREMENTAL:NO;/OPT:REF;/OPT:ICF;/DEBUG:FULL>"
	)
endif()

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/RainSplashesF4SE.ini
	${CMAKE_SOURCE_DIR}/Compile/F4SE/Plugins/RainSplashesF4SE.ini
	COPYONLY
)

option(COPY_BUILD "Copy DLL to Fallout 4 Data folder." OFF)
if(COPY_BUILD AND DEFINED Fallout4Path)
	add_custom_command(
		TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> "${Fallout4Path}/Data/F4SE/Plugins/"
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PDB_FILE:${PROJECT_NAME}> "${Fallout4Path}/Data/F4SE/Plugins/"
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"${CMAKE_SOURCE_DIR}/Compile/F4SE/Plugins/RainSplashesF4SE.ini"
			"${Fallout4Path}/Data/F4SE/Plugins/RainSplashesF4SE.ini"
	)
endif()
