#pragma once
#include <memory>
#include <stdexcept>
#include <string>
#include <vulkan/vulkan.h>

#define FORMAT_STR(...) vulkan_proto::stringFormat(__VA_ARGS__)

#ifndef NDEBUG
#define VK_CHECK(call)                                                         \
    vulkan_proto::checkVulkanCallResult(call, #call, __FILE__, __LINE__)
#else
#define VK_CHECK(call) call
#endif

#define THROW_IF(condition, ...)                                               \
    vulkan_proto::throwIf(condition, FORMAT_STR(__VA_ARGS__), __FILE__,        \
                          __LINE__)

#define LOG(...) getLogger().log(FORMAT_STR(__VA_ARGS__));

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

inline void throwIf(bool condition, std::string msg, const char *fileName,
                    int line) {
    if (condition) {
        throw std::runtime_error(
            FORMAT_STR("@%s:%d\n\t'%s'", fileName, line, msg.c_str()).c_str());
    }
}

inline void checkVulkanCallResult(VkResult result, const char *callStr,
                                  const char *fileName, int line) {
    throwIf(VK_SUCCESS != result,
            FORMAT_STR("Vulkan error '%d':\n%s", result, callStr), fileName,
            line);
}
} // namespace vulkan_proto
