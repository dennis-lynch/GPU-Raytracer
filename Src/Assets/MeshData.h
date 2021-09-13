#pragma once
#include "Pathtracer/Triangle.h"

#include "BVH/BVH.h"

struct MeshData {
	int        triangle_count;
	Triangle * triangles;

	BVH bvh;

	void init_bvh(BVH & bvh);
};

struct MeshDataHandle { int handle = INVALID; };
