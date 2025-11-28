CMAKE_MINIMUM_REQUIRED(VERSION 3.18)

project(MaterialParser CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_compile_options(-fdiagnostics-color=always)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options(-fcolor-diagnostics)
endif()

if (MINGW)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc -static-libstdc++")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()

set(JSON_BuildTests OFF CACHE INTERNAL "")
set(ZSTD_BUILD_SHARED OFF CACHE INTERNAL "")
set(ZSTD_BUILD_PROGRAMS OFF CACHE INTERNAL "")
set(ZSTD_STATIC_LINKING_ONLY ON CACHE INTERNAL "")

add_subdirectory(lib/MeshCodec)
add_subdirectory(lib/json)

add_executable(
    mat-tool
    
    src/include/types.h
    src/include/math_types.h

    src/include/binary_file.h

    src/include/bfres.h
    src/include/bfsha.h
    src/include/shader.h

    src/include/app.h

    src/binary_file.cpp
    src/bfres.cpp

    src/shader.cpp

    src/app.cpp

    src/main.cpp
)

target_include_directories(mat-tool PRIVATE src/include)

target_link_libraries(mat-tool PRIVATE MeshCodec nlohmann_json::nlohmann_json)

if (MSVC)
    target_compile_options(mat-tool PRIVATE /W4 /wd4244 /wd4127 /Zc:__cplusplus)
else()
    target_compile_options(mat-tool PRIVATE -Wall -Wextra -fno-plt)
endif()