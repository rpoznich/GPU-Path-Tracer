#ifndef CONFIG_H
#define CONFIG_H
#include <cstddef>
/*
 * Global variables go here.
 */

#define VERTICES_PER_CUBE 8
#define HEIGHT_MARGIN 0.15f

const float kCylinderRadius = 0.25;
const int kMaxBones = 128;
/*
 * Extra credit: what would happen if you set kNear to 1e-5? How to solve it?
 */
const float kNear = 0.5f;
const float kFar = 100.0f;
const float kFov = 60.0f;

// Floor info.
const float kFloorEps = 0.5 * (0.025 + 0.0175);
const float kFloorXMin = -100.0f;
const float kFloorXMax = 100.0f;
const float kFloorZMin = -100.0f;
const float kFloorZMax = 100.0f;
const float kFloorY = -0.75617 - kFloorEps;
const float maxTerrainHeight = 10.0f;
const float minTerrainHeight = -15.0f;

const float kScrollSpeed = 64.0f;

const float  BOX_SIZE = 1.0f;
const size_t num_x = 50;
const size_t num_z = 50;
const float GROUND = -(BOX_SIZE/2);
#endif
