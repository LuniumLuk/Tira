cmake_minimum_required(VERSION 3.11.0)

set(NAME Tira)

project(${NAME} VERSION 1.0.0)

include_directories(Tira)
include_directories(Thirdparty)

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/Tira/*.cpp)
file(GLOB_RECURSE THIRDSOURCES ${PROJECT_SOURCE_DIR}/Thirdparty/*.cpp)

add_executable(${PROJECT_NAME} 
    ${SOURCES}
    ${THIRDSOURCES}
    Platform/win32.cpp
    Platform/macos.mm)

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    target_link_libraries(${PROJECT_NAME} PUBLIC OpenMP::OpenMP_CXX)
endif()

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
add_compile_definitions(ASSET_DIR="../Asset/")
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
