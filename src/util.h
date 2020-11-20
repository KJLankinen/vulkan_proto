#pragma once
#include <fstream>
#include <memory>
#include <stdexcept>
#include <stdio.h>
#include <string>

#define FORMAT_STR(...) vulkan_proto::stringFormat(__VA_ARGS__)

#ifndef NDEBUG
#define VK_CHECK(call)                                                         \
    vulkan_proto::checkVulkanCallResult(log, call, #call, __FILE__, __LINE__)
#else
#define VK_CHECK(call) call
#endif

#define THROW_IF(condition, ...)                                               \
    vulkan_proto::throwIf(log, condition, FORMAT_STR(__VA_ARGS__), __FILE__,   \
                          __LINE__)

#define LOG(...) vulkan_proto::log(log, log.fileStream, FORMAT_STR(__VA_ARGS__))

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

void formatTimeSinceStart(Log &log) {
    log.timess.clear();
    log.timess.str(std::string());
    auto now = std::chrono::steady_clock::now();
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - log.startingTime);
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
    log.timess << "[";
    if (hours.count() < 10) {
        log.timess << "0";
    }
    log.timess << hours.count() << ":";

    if (minutes.count() < 10) {
        log.timess << "0";
    }
    log.timess << minutes.count() << ":";

    if (seconds.count() < 10) {
        log.timess << "0";
    }
    log.timess << seconds.count() << ":";

    if (milliseconds.count() < 100) {
        log.timess << "0";
    }
    if (milliseconds.count() < 10) {
        log.timess << "0";
    }
    log.timess << milliseconds.count() << "]";
}

inline void log(Log &log, std::ofstream *fileStream, std::string msg) {
#ifndef NDEBUG
    formatTimeSinceStart(log);
    if (fileStream != nullptr) {
        *fileStream << log.timess.str().c_str() << " " << msg.c_str()
                    << std::endl;
    }
    std::cout << log.timess.str().c_str() << " " << msg.c_str() << std::endl;
#endif
}

inline void throwIf(Log &log, bool condition, std::string msg,
                    const char *fileName, int line) {
    if (condition) {
        throw std::runtime_error(
            FORMAT_STR("'%s' at %s:%d.", msg.c_str(), fileName, line).c_str());
    }
}

inline void checkVulkanCallResult(Log &log, VkResult result,
                                  const char *callStr, const char *fileName,
                                  int line) {
    throwIf(log, VK_SUCCESS != result,
            FORMAT_STR("Vulkan error '%d':\n%s", result, callStr), fileName,
            line);
}
} // namespace vulkan_proto
