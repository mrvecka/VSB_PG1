# cmake_minimum_required(VERSION <specify CMake version here>)
project(pg)

set(CMAKE_CXX_STANDARD 14)

include_directories(pg1_embree)

add_executable(pg
        pg1_embree/camera.cpp
        pg1_embree/camera.h
        pg1_embree/material.cpp
        pg1_embree/material.h
        pg1_embree/matrix3x3.cpp
        pg1_embree/matrix3x3.h
        pg1_embree/mymath.cpp
        pg1_embree/mymath.h
        pg1_embree/objloader.cpp
        pg1_embree/objloader.h
        pg1_embree/pg1_embree.cpp
        pg1_embree/raytracer.cpp
        pg1_embree/raytracer.h
        pg1_embree/simpleguidx11.cpp
        pg1_embree/simpleguidx11.h
        pg1_embree/stdafx.cpp
        pg1_embree/stdafx.h
        pg1_embree/structs.cpp
        pg1_embree/structs.h
        pg1_embree/surface.cpp
        pg1_embree/surface.h
        pg1_embree/targetver.h
        pg1_embree/texture.cpp
        pg1_embree/texture.h
        pg1_embree/triangle.cpp
        pg1_embree/triangle.h
        pg1_embree/tutorials.cpp
        pg1_embree/tutorials.h
        pg1_embree/utils.cpp
        pg1_embree/utils.h
        pg1_embree/vector3.cpp
        pg1_embree/vector3.h
        pg1_embree/vertex.cpp
        pg1_embree/vertex.h)
