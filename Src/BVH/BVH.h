#pragma once
#include <stdlib.h>

#include "Config.h"

#include "Pathtracer/Triangle.h"

typedef unsigned char byte;

struct BVHNode2 {
	AABB aabb;
	union {
		int left;
		int first;
	};
	unsigned count : 30;
	unsigned axis  : 2;

	inline bool is_leaf() const {
		return count > 0;
	}
};

struct BVHNode4 {
	float aabb_min_x[4] = { 0.0f };
	float aabb_min_y[4] = { 0.0f };
	float aabb_min_z[4] = { 0.0f };
	float aabb_max_x[4] = { 0.0f };
	float aabb_max_y[4] = { 0.0f };
	float aabb_max_z[4] = { 0.0f };

	struct {
		int index;
		int count;
	} index_and_count[4];

	inline       int & get_index(int i)       { return index_and_count[i].index; }
	inline const int & get_index(int i) const { return index_and_count[i].index; }
	inline       int & get_count(int i)       { return index_and_count[i].count; }
	inline const int & get_count(int i) const { return index_and_count[i].count; }

	inline bool is_leaf(int i) { return get_count(i) > 0; }

	inline int get_child_count() const {
		int result = 0;

		for (int i = 0; i < 4; i++) {
			if (get_count(i) == -1) break;

			result++;
		}

		return result;
	}

};

static_assert(sizeof(BVHNode4) == 128);

struct BVHNode8 {
	Vector3 p;
	byte e[3];
	byte imask;

	unsigned base_index_child;
	unsigned base_index_triangle;

	byte meta[8] = { };

	byte quantized_min_x[8] = { }, quantized_max_x[8] = { };
	byte quantized_min_y[8] = { }, quantized_max_y[8] = { };
	byte quantized_min_z[8] = { }, quantized_max_z[8] = { };

	inline bool is_leaf(int child_index) {
		return (meta[child_index] & 0b00011111) < 24;
	}
};

static_assert(sizeof(BVHNode8) == 80);

union BVHNodePtr {
	BVHNode2 * _2;
	BVHNode4 * _4;
	BVHNode8 * _8;
};

struct BVH {
	int   index_count;
	int * indices;

	int        node_count;
	BVHNodePtr nodes;

	// Each individual BVH needs to put its Nodes in a shared aggregated array of BVH Nodes before being upload to the GPU
	// The procedure to do this is different for each BVH type
	void aggregate(BVHNodePtr aggregated_bvh_nodes, int index_offset, int bvh_offset) const;

	// Helper Methods
	static BVHType underlying_bvh_type() {
		// All BVH use standard BVH as underlying type, only SBVH uses SBVH
		if (config.bvh_type == BVHType::SBVH) {
			return BVHType::SBVH;
		} else {
			return BVHType::BVH;
		}
	}
};
