# ============================================================
# Collect all source files recursively
# ============================================================
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

# ============================================================
# Visual Studio folders (filters)
# ============================================================
source_group(
    TREE ${CMAKE_CURRENT_SOURCE_DIR}
    PREFIX "Code"
    FILES ${SOURCES}
)

# ============================================================
# Executable (CREATE ONCE!)
# ============================================================
add_executable(EidosApp ${SOURCES})

# ============================================================
# Include directories
# ============================================================
target_include_directories(EidosApp
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# ============================================================
# Link libraries
# ============================================================
target_link_libraries(EidosApp
    PRIVATE
        raylib
        lua_static
        sol2
	FastNoiseLite
)

# ============================================================
# Copy assets after build
# ============================================================
add_custom_command(
    TARGET EidosApp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:EidosApp>/assets
)
