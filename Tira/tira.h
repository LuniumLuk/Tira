//
// * * * * * * * * * * * * * * * * * * * * * * * * * * * //
//	Tira : A Tiny physically based renderer
//  Designed for ZJU Computer Graphics 2022 Course Project
//	Created by Ziyi.Lu 2023/03/20
// * * * * * * * * * * * * * * * * * * * * * * * * * * * //
//

#include <macro.h>

#include <misc/utils.h>
#include <misc/timer.h>
#include <misc/image.h>

#include <math/simd.h>
#include <math/vector.h>
#include <math/matrix.h>

#include <geometry/ray.h>
#include <geometry/sphere.h>
#include <geometry/triangle.h>
#include <geometry/object.h>

#include <scene/scene.h>
#include <scene/accel.h>
#include <scene/bvh.h>
#include <scene/octree.h>
#include <scene/camera.h>
#include <scene/material.h>
#include <scene/texture.h>

#include <window/platform.h>
#include <window/gui.h>

#include <integrator/integrator.h>
#include <integrator/whitted.h>
#include <integrator/montecarlo.h>
#include <integrator/bidirectional.h>
