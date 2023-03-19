//
// Created by Ziyi.Lu 2022/02/17
//

#ifndef MACRO_H
#define MACRO_H

#if defined(_MSC_VER)
// disable type conversion warning
#pragma warning(disable : 4305)
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
// sscanf warning
#pragma warning(disable : 4996)
// unknown pragma 'GCC'
#pragma warning(disable : 4068)
#endif

//// Global Settings.

// ┌─ Enable SIMD acceleration
// V 
// #define ENABLE_SIMD

// ┌─ Use GlassMaterial for highly specular and transparent object
// V 
#define USE_GLASS_MATERIAL

// ┌─ Enable light to pass through close enough surface
// V 
#define TRIANGLE_TOLERATE_LIGHT_CLOSE_TO_SURFACE

// ┌─ Default scene to load if no scene name provided
// V 
#define DEFAULT_SCENE "CornellBox-Original"

// ┌─ Enable SAH to construct BVH
// V 
#define BVH_WITH_SAH

// ┌─ Max search for one SAH subdivide
// V 
#define SAH_MAX_SEARCH 256

// ┌─ Traverse BVH iteratively rather than recursively
// V 
#define TRAVERSE_ITERATIVE

// ┌─ Traverse BVH iteratively with stack, or traverse with pointers
// V 
// #define TRAVERSE_ITERATIVE_STACK

// ┌─ Threshold of Ni for BlinnPhongMaterial's specular term to become a delta function
// V 
#define BLINN_PHONG_SHININESS_THRESHOLD 500

// GUI mode:
//  -0 Render offscreen.
//  -1 Render with scene preview.
//  -2 Update window every sample.
#define GUI_MODE 0

// Integrator version:
//  -1 ver.1
//  -2 ver.2
//  -3 ver.3
#define INTEGRATOR_VER 2

// ┌─ How many poisson points used for sampling eye direction
// V 
#define POISSON_POINTS_NUM 32

//// Macros for developers, do not modify if unsure of its usage

// Ver.1
#define MAX_BOUNCE 8

// Ver.2
#define USE_MIS
#define MAX_DEPTH 8
// #define USE_RUSSIAN_ROULETTE
#define RUSSIAN_ROULETTE .8f

// Ver.3
#define MAX_RAY_DEPTH 8
#define NUM_LIGHT_SAMPLES 8

#endif