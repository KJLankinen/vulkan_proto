#pragma once
#include <assert.h>
#include <stdio.h>

#ifndef NDEBUG
#define VK_ASSERT_CALL(call)                                                   \
    vulkan_proto::assertVulkanCallResult(call, #call, __FILE__, __LINE__)
#else
#define VK_ASSERT_CALL(call) call
#endif

namespace vulkan_proto {
inline void assertVulkanCallResult(VkResult result, const char *callStr,
                                   const char *file, int line) {
    if (VK_SUCCESS != result) {
        printf("Vulkan error '%d':\n%s\nat %s:%d\n", result, callStr, file,
               line);
        assert(false);
    }
}
}
