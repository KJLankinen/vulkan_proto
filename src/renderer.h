#pragma once
#include "data_types.h"
#include "util.h"
#include <array>
#include <assert.h>
#include <cstring>
#include <glm/glm.hpp>
#include <stdio.h>

namespace vulkan_proto {
void initGLFW(Params &params);
void terminateGLFW(Params &params);
void init(Params &params);
void terminate(Params &params);
void run();
} // namespace vulkan_proto
