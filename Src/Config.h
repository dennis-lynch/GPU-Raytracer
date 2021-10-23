#pragma once
#include "../CUDA_Source/Common.h"

#include "Util/Array.h"

inline Config config = { };

struct SceneConfig {
	Array<const char *> scenes;
	const char * sky = nullptr;
};
inline SceneConfig scene_config = { };
