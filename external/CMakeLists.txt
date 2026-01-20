cmake_minimum_required(VERSION 3.21)

include(FetchContent)

# --- LUA ---
message(STATUS "Fetching Lua...")
FetchContent_Declare(
    lua
    GIT_REPOSITORY https://github.com/lua/lua.git
    GIT_TAG        v5.4.6
)
FetchContent_MakeAvailable(lua)

add_library(lua_static STATIC
    ${lua_SOURCE_DIR}/lapi.c
    ${lua_SOURCE_DIR}/lauxlib.c
    ${lua_SOURCE_DIR}/lbaselib.c
    ${lua_SOURCE_DIR}/lcode.c
    ${lua_SOURCE_DIR}/lcorolib.c
    ${lua_SOURCE_DIR}/lctype.c
    ${lua_SOURCE_DIR}/ldblib.c
    ${lua_SOURCE_DIR}/ldebug.c
    ${lua_SOURCE_DIR}/ldo.c
    ${lua_SOURCE_DIR}/ldump.c
    ${lua_SOURCE_DIR}/lfunc.c
    ${lua_SOURCE_DIR}/lgc.c
    ${lua_SOURCE_DIR}/linit.c
    ${lua_SOURCE_DIR}/liolib.c
    ${lua_SOURCE_DIR}/llex.c
    ${lua_SOURCE_DIR}/lmathlib.c
    ${lua_SOURCE_DIR}/lmem.c
    ${lua_SOURCE_DIR}/loadlib.c
    ${lua_SOURCE_DIR}/lobject.c
    ${lua_SOURCE_DIR}/lopcodes.c
    ${lua_SOURCE_DIR}/loslib.c
    ${lua_SOURCE_DIR}/lparser.c
    ${lua_SOURCE_DIR}/lstate.c
    ${lua_SOURCE_DIR}/lstring.c
    ${lua_SOURCE_DIR}/lstrlib.c
    ${lua_SOURCE_DIR}/ltable.c
    ${lua_SOURCE_DIR}/ltablib.c
    ${lua_SOURCE_DIR}/ltm.c
    ${lua_SOURCE_DIR}/lundump.c
    ${lua_SOURCE_DIR}/lvm.c
    ${lua_SOURCE_DIR}/lzio.c
    ${lua_SOURCE_DIR}/lutf8lib.c
)
target_include_directories(lua_static PUBLIC ${lua_SOURCE_DIR})
target_compile_definitions(lua_static PUBLIC LUA_USE_WINDOWS)

# --- SOL2 ---
message(STATUS "Fetching Sol2...")
FetchContent_Declare(
    sol2
    GIT_REPOSITORY https://github.com/ThePhD/sol2.git
    GIT_TAG        v3.3.0
)
FetchContent_MakeAvailable(sol2)

message(STATUS "Fetching FastNoiseLite...")
FetchContent_Declare(
    fast_noise
    GIT_REPOSITORY https://github.com/Auburn/FastNoiseLite.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(fast_noise)

# Создаем библиотеку (Header-only)
add_library(FastNoiseLite INTERFACE)

# !!! ВАЖНО: Тут должно быть Cpp, а не C !!!
target_include_directories(FastNoiseLite INTERFACE ${fast_noise_SOURCE_DIR}/Cpp)

# --- RAYLIB ---
message(STATUS "Using local Raylib...")
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory(raylib)

# --- ФИНАЛЬНЫЙ СПИСОК БИБЛИОТЕК ---
set(EIDOS_ENGINE_LIBS
    lua_static
    sol2
    FastNoiseLite  # Добавили сюда
    raylib
    CACHE INTERNAL ""
)