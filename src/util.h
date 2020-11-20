#pragma once
#include <fstream>
#include <memory>
#include <stdexcept>
#include <stdio.h>
#include <string>

#define FORMAT_STR(...) vulkan_proto::stringFormat(__VA_ARGS__)

#ifndef NDEBUG
#define VK_CHECK(call)                                                         \
    vulkan_proto::checkVulkanCallResult(params, call, #call, __FILE__, __LINE__)
#else
#define VK_CHECK(call) call
#endif

#define ABORT_IF(condition, ...)                                               \
    vulkan_proto::abortIf(params, condition, FORMAT_STR(__VA_ARGS__),          \
                          __FILE__, __LINE__)

#define LOG(...)                                                               \
    vulkan_proto::log(params, params.fileStream, FORMAT_STR(__VA_ARGS__))

namespace vulkan_proto {
template <typename... Args>
std::string stringFormat(const char *format, Args... args) {
    size_t size = snprintf(nullptr, 0, format, args...) + 1;
    if (size <= 0) {
        throw std::runtime_error("Error while formatting string");
    }
    std::unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), size, format, args...);
    return std::string(buf.get(), buf.get() + size - 1);
}

void formatTimeSinceStart(Params &params) {
    params.timess.clear();
    params.timess.str(std::string());
    auto now = std::chrono::steady_clock::now();
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - params.startingTime);
    auto hours = std::chrono::duration_cast<std::chrono::hours>(milliseconds);
    milliseconds -=
        std::chrono::duration_cast<std::chrono::milliseconds>(hours);
    auto minutes =
        std::chrono::duration_cast<std::chrono::minutes>(milliseconds);
    milliseconds -=
        std::chrono::duration_cast<std::chrono::milliseconds>(minutes);
    auto seconds =
        std::chrono::duration_cast<std::chrono::seconds>(milliseconds);
    milliseconds -=
        std::chrono::duration_cast<std::chrono::milliseconds>(seconds);
    params.timess << "[";
    if (hours.count() < 10) {
        params.timess << "0";
    }
    params.timess << hours.count() << ":";

    if (minutes.count() < 10) {
        params.timess << "0";
    }
    params.timess << minutes.count() << ":";

    if (seconds.count() < 10) {
        params.timess << "0";
    }
    params.timess << seconds.count() << ":";

    if (milliseconds.count() < 100) {
        params.timess << "0";
    }
    if (milliseconds.count() < 10) {
        params.timess << "0";
    }
    params.timess << milliseconds.count() << "]";
}

inline void log(Params &params, std::ofstream *fileStream, std::string msg) {
#ifndef NDEBUG
    formatTimeSinceStart(params);
    if (fileStream != nullptr) {
        *fileStream << params.timess.str().c_str() << " " << msg.c_str()
                    << std::endl;
    }
    std::cout << params.timess.str().c_str() << " " << msg.c_str() << std::endl;
#endif
}

void terminate(Params &params);
inline void abort(Params &params, const char *msg) {
    LOG(msg);
    LOG("Aborting");
    terminate(params);

    if (params.fileStream != nullptr) {
        params.fileStream->flush();
    }

    exit(EXIT_FAILURE);
}

inline void abortIf(Params &params, bool condition, std::string msg,
                    const char *fileName, int line) {
    if (condition) {
        abort(params,
              FORMAT_STR("'%s' at %s:%d", msg.c_str(), fileName, line).c_str());
    }
}

inline void checkVulkanCallResult(Params &params, VkResult result,
                                  const char *callStr, const char *fileName,
                                  int line) {
    abortIf(params, VK_SUCCESS != result,
            FORMAT_STR("Vulkan error '%d':\n%s", result, callStr), fileName,
            line);
}
} // namespace vulkan_proto
