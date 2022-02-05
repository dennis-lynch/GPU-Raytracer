#pragma once
#include "CUDACall.h"

namespace CUDAContext {
	inline int compute_capability = -1;

	inline size_t total_memory;

	// Creates a new CUDA Context
	void init();
	void free();

	size_t get_available_memory(); // Available memory on GPU in bytes

	unsigned get_shared_memory(); // Available shared memory in bytes (per Block)

	unsigned get_sm_count(); // Number of Streaming Multiprocessors on the current Device
}
