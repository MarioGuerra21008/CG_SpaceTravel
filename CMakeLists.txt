cmake_minimum_required(VERSION 3.25)
project(SpaceTravel)

set(CMAKE_CXX_STANDARD 23)
set(SDL_INCLUDE_DIR C:/SDL/include)
set(SDL2_LIB_DIR C:/SDL/lib/x64)

include_directories(${SDL_INCLUDE_DIR})
link_directories(${SDL2_LIB_DIR})
include_directories("C:/MinGW/include")

add_executable(SpaceTravel main.cpp extensions/color.h extensions/framebuffer.h extensions/point.h
        extensions/line.h extensions/triangle.h extensions/fragment.h extensions/uniform.h extensions/shaders.h
        extensions/vertexArray.h extensions/loadOBJFile.h extensions/FastNoiseLite.h)

target_link_libraries(SpaceTravel SDL2main SDL2)
