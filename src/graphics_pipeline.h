#pragma once

#include "headers.h"
#include <glslang/Include/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>

namespace vulkan_proto {

struct Renderer;
struct Logger;

struct GraphicsPipeline {
    const Renderer &m_renderer;
    VkPipeline m_handle = VK_NULL_HANDLE;
    VkPipelineLayout m_layout = VK_NULL_HANDLE;
    std::vector<VkShaderModule> m_shaderModules;

    static bool glslangInitialized;

    GraphicsPipeline(Renderer &renderer);
    ~GraphicsPipeline();
    void create(bool recycle = false);
    void destroy(bool recycle = false);
    Logger &getLogger();
    VkShaderModule createShaderModule(const uint32_t *code, uint32_t bytes);
    VkShaderModule createShaderModuleFromSpirV(const char *fName);
    VkShaderModule createShaderModuleFromGLSL(std::string &&filename,
                                              EShLanguage shaderStage);
};
} // namespace vulkan_proto
