cmake_minimum_required(VERSION 3.15)
project(Project1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # helps VS Code IntelliSense

option(ENABLE_WARNINGS "Enable extra compiler warnings" ON)
if(ENABLE_WARNINGS)
	if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		add_compile_options(-Wall -Wextra -Wpedantic)
	elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
		add_compile_options(/W4 /permissive-)
	endif()
endif()

# ----------------------------------------------------------------------------
# Dependencies: OpenGL + GLFW (system) + GLAD (vendored) + ImGui (vendored)
# ----------------------------------------------------------------------------
find_package(OpenGL REQUIRED)

# Try config-mode first, fall back to pkg-config if needed
find_package(glfw3 QUIET)
if (NOT glfw3_FOUND)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(GLFW REQUIRED glfw3)
	set(GLFW_TARGETS ${GLFW_LIBRARIES})
	set(GLFW_INCLUDE_DIRS ${GLFW_INCLUDE_DIRS})
else()
	# Modern GLFW exports the target glfw
	set(GLFW_TARGETS glfw)
endif()

# ----------------------------------------------------------------------------
# Optional: Assimp (model import)
# Try config-mode (CMake package) first, fall back to pkg-config if not found.
# This allows users to build with system assimp or a vendored/packaged one.
# ----------------------------------------------------------------------------
find_package(assimp QUIET)
if (NOT assimp_FOUND)
	# pkg-config fallback (some systems only provide assimp.pc)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(ASSIMP REQUIRED assimp)
	# pkg_check_modules populates ASSIMP_INCLUDE_DIRS and ASSIMP_LIBRARIES
endif()

# ----------------------------------------------------------------------------
# Source files
# ----------------------------------------------------------------------------
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(INC_DIR ${CMAKE_SOURCE_DIR}/include)
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/include/imgui)

set(IMGUI_SOURCES
		${IMGUI_DIR}/imgui.cpp
		${IMGUI_DIR}/imgui_demo.cpp
		${IMGUI_DIR}/imgui_draw.cpp
		${IMGUI_DIR}/imgui_tables.cpp
		${IMGUI_DIR}/imgui_widgets.cpp
		${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
		${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_executable(${PROJECT_NAME}
		${SRC_DIR}/main.cpp
		${SRC_DIR}/stb_image.cpp
		${INC_DIR}/glad/src/glad.c
		${IMGUI_SOURCES}
)

# Ensure the debugger (VS) starts in the executable directory so relative paths resolve to "./data"
set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)

# ----------------------------------------------------------------------------
# Include directories
# ----------------------------------------------------------------------------
target_include_directories(${PROJECT_NAME} PUBLIC     # make GLAD headers visible to all sources including headers
    ${INC_DIR}
    ${INC_DIR}/glad/include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# Prevent GLFW from pulling in system GL headers (we use GLAD)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    IMGUI_IMPL_OPENGL_LOADER_GLAD
    GLFW_INCLUDE_NONE
)

# If pkg-config or find_package provided ASSIMP include dirs, add them
if (ASSIMP_INCLUDE_DIRS)
    target_include_directories(${PROJECT_NAME} PRIVATE ${ASSIMP_INCLUDE_DIRS})
endif()

# Use absolute path macros so shader loads don't depend on process CWD
# DATA_DIR = original source asset directory
# RUNTIME_DATA_DIR = copied directory beside the built executable
unset(_old_data_def)
get_target_property(_old_data_def ${PROJECT_NAME} COMPILE_DEFINITIONS)
# Replace previous relative DATA_DIR definition
target_compile_definitions(${PROJECT_NAME} PRIVATE
    DATA_DIR=\"${CMAKE_SOURCE_DIR}/data\"
    RUNTIME_DATA_DIR=\"$<TARGET_FILE_DIR:${PROJECT_NAME}>/data\"
)

# Optional: verify at configure time that at least one critical shader exists
set(_required_shader "${CMAKE_SOURCE_DIR}/data/shaders/model.vs")
if(NOT EXISTS "${_required_shader}")
    message(WARNING "Expected shader not found: ${_required_shader}")
endif()

# ----------------------------------------------------------------------------
# Linking
# ----------------------------------------------------------------------------
target_link_libraries(${PROJECT_NAME} PRIVATE
		OpenGL::GL
		${GLFW_TARGETS}
)

# Link Assimp if it was found. Prefer the modern imported target when available.
if (TARGET assimp::assimp)
    target_link_libraries(${PROJECT_NAME} PRIVATE assimp::assimp)
elseif (ASSIMP_LIBRARIES)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${ASSIMP_LIBRARIES})
endif()

# ----------------------------------------------------------------------------
# Runtime assets: copy data/ (shaders, textures) next to the binary for relative paths
# ----------------------------------------------------------------------------
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
						${CMAKE_SOURCE_DIR}/data
						$<TARGET_FILE_DIR:${PROJECT_NAME}>/data
		COMMENT "Copying data directory (shaders, textures) into build output"
)
# Enable relative paths in the generated build system (optional)
set(CMAKE_USE_RELATIVE_PATHS ON)

# ----------------------------------------------------------------------------
# Convenience: organize source groups (useful for IDEs)
# ----------------------------------------------------------------------------
source_group(TREE ${CMAKE_SOURCE_DIR} FILES
		${SRC_DIR}/main.cpp
		${SRC_DIR}/stb_image.cpp
		${INC_DIR}/glad/src/glad.c
		${IMGUI_SOURCES}
)

message(STATUS "Configured ${PROJECT_NAME} with C++${CMAKE_CXX_STANDARD}")
message(STATUS "Data directory: ${CMAKE_SOURCE_DIR}/data")
message(STATUS "Runtime directory: ${CMAKE_CURRENT_SOURCE_DIR}")