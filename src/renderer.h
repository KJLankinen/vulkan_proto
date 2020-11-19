#pragma once
#include "data_types.h"
#include "util.h"
#include <array>
#include <assert.h>
#include <cstring>
#include <glm/glm.hpp>
#include <stdio.h>

namespace vulkan_proto {

void init(Params &params);
void initGLFW(Params &params);
void createInstance(Params &params);
void createDevice(Params &params);

void abortIf(Params &params, bool condition, const char *msg, const char *file,
             int line);
void run();

void terminate(Params &params);
void terminateGLFW(Params &params);
} // namespace vulkan_proto
