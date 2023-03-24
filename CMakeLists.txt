cmake_minimum_required(VERSION 3.11.0)

set(NAME Tira_CPU)

project(${NAME} VERSION 1.0.0)

include_directories(Tira)

add_executable(${PROJECT_NAME} 
    Tira/integrator/integrator.cpp
    Tira/integrator/whitted.cpp
    Tira/integrator/montecarlo.cpp
    Tira/integrator/bidirectional.cpp
    Tira/misc/image.cpp
    Tira/scene/bvh.cpp
    Tira/scene/octree.cpp
    Tira/scene/material.cpp
    Tira/scene/scene.cpp
    Tira/scene/texture.cpp
    Tira/thirdparty/pugixml.cpp
    Tira/thirdparty/stb_impl.cpp
    Tira/window/gui.cpp
    Tira/window/win32.cpp
    # [TODO] Fix objective-c++ compile error with cmake
    # Tira/window/macos.mm
    Tira_CPU/main.cpp)

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    target_link_libraries(${PROJECT_NAME} PUBLIC OpenMP::OpenMP_CXX)
endif()

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
add_compile_definitions(ROOT_DIR="../")
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
