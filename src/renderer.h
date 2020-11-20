#pragma once
#include "data_types.h"
#include "util.h"
#include <array>
#include <assert.h>
#include <cstring>
#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <sstream>
#include <string>

namespace vulkan_proto {

void init(Params &params);
void initGLFW(Params &params);
void createInstance(Params &params);
void createDevice(Params &params);

void abortIf(Params &params, bool condition, const char *msg,
             const char *fileName, int line);
void log(Params &params, Verbosity verbosity, FILE *stream, const char *msg);
void run();

void terminate(Params &params);
void terminateGLFW(Params &params);
} // namespace vulkan_proto
