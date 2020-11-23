#include "graphics_pipeline.h"
#include "DirStackFileIncluder.h"
#include "logger.h"
#include "renderer.h"
#include <glslang/SPIRV/GlslangToSpv.h>

bool vulkan_proto::GraphicsPipeline::glslangInitialized = false;

namespace vulkan_proto {
GraphicsPipeline::GraphicsPipeline(Renderer &renderer) : m_renderer(renderer) {}
GraphicsPipeline::~GraphicsPipeline() {}

void GraphicsPipeline::create(bool recycle) {
    LOG("=Create graphics pipeline=");
    if (recycle) {
        destroy(recycle);
    }

    auto shaders = m_renderer.getProgramInput().at("shaders");
    const uint32_t numShaderStages = static_cast<uint32_t>(shaders.size());
    m_shaderModules.resize(numShaderStages);
    std::vector<VkPipelineShaderStageCreateInfo> shaderCIs(numShaderStages);
    std::vector<std::string> entryPoints(numShaderStages);

    // These values are updated by the lambda below
    VkShaderStageFlagBits stageFlag = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
    EShLanguage stage = EShLangCount;
    auto setShaderStage = [&stageFlag, &stage](const std::string &type) {
        if (type == "vertex") {
            stageFlag = VK_SHADER_STAGE_VERTEX_BIT;
            stage = EShLangVertex;
        } else if (type == "tessellation_control") {
            stageFlag = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
            stage = EShLangTessControl;
        } else if (type == "tessellation_evaluation") {
            stageFlag = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
            stage = EShLangTessEvaluation;
        } else if (type == "geometry") {
            stageFlag = VK_SHADER_STAGE_GEOMETRY_BIT;
            stage = EShLangGeometry;
        } else if (type == "fragment") {
            stageFlag = VK_SHADER_STAGE_FRAGMENT_BIT;
            stage = EShLangFragment;
        } else if (type == "compute") {
            stageFlag = VK_SHADER_STAGE_COMPUTE_BIT;
            stage = EShLangCompute;
        } else {
            THROW_IF(true, "Invalid shader type string!");
        }
    };

    // Shaders
    uint32_t shaderStageIndex = 0;
    for (const auto &it : shaders) {
        setShaderStage(it.at("type").get<std::string>());

        VkShaderModule sm = createShaderModuleFromGLSL(
            std::string(m_renderer.getDataPath() +
                        it.at("path").get<std::string>()),
            stage);
        THROW_IF(sm == VK_NULL_HANDLE,
                 "Shader module is VK_NULL_HANDLE for shader file %s",
                 it.at("path").get<std::string>().c_str());

        m_shaderModules[shaderStageIndex] = sm;
        entryPoints[shaderStageIndex] = it.at("entryPoint").get<std::string>();

        shaderCIs[shaderStageIndex].sType =
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderCIs[shaderStageIndex].pNext = nullptr;
        shaderCIs[shaderStageIndex].flags = 0;
        shaderCIs[shaderStageIndex].stage = stageFlag;
        shaderCIs[shaderStageIndex].module = m_shaderModules[shaderStageIndex];
        shaderCIs[shaderStageIndex].pName =
            entryPoints[shaderStageIndex].c_str();
        shaderCIs[shaderStageIndex].pSpecializationInfo = nullptr;

        ++shaderStageIndex;
    }

    // Vertex input binding
    VkVertexInputBindingDescription inputBinding = {};
    inputBinding.binding = 0;
    inputBinding.stride = 32;
    inputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::vector<VkVertexInputAttributeDescription> inputAttributes(3);
    inputAttributes[0].location = 0;
    inputAttributes[0].binding = inputBinding.binding;
    inputAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    inputAttributes[0].offset = 0;

    inputAttributes[1].location = 1;
    inputAttributes[1].binding = inputBinding.binding;
    inputAttributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    inputAttributes[1].offset = 12;

    inputAttributes[2].location = 2;
    inputAttributes[2].binding = inputBinding.binding;
    inputAttributes[2].format = VK_FORMAT_R32G32_SFLOAT;
    inputAttributes[2].offset = 24;

    VkPipelineVertexInputStateCreateInfo vertexInputCI = {};
    vertexInputCI.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCI.vertexBindingDescriptionCount = 1;
    vertexInputCI.pVertexBindingDescriptions = &inputBinding;
    vertexInputCI.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(inputAttributes.size());
    vertexInputCI.pVertexAttributeDescriptions = inputAttributes.data();

    // Color blending
    VkPipelineColorBlendAttachmentState colBlendAttch = {};
    colBlendAttch.blendEnable = false;
    colBlendAttch.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colBlendAttch.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colBlendAttch.colorBlendOp = VK_BLEND_OP_ADD;
    colBlendAttch.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colBlendAttch.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colBlendAttch.alphaBlendOp = VK_BLEND_OP_ADD;
    colBlendAttch.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlendingCI = {};
    colorBlendingCI.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendingCI.logicOpEnable = false;
    colorBlendingCI.logicOp = VK_LOGIC_OP_COPY;
    colorBlendingCI.attachmentCount = 1;
    colorBlendingCI.pAttachments = &colBlendAttch;
    colorBlendingCI.blendConstants[0] = 0.0f;
    colorBlendingCI.blendConstants[1] = 0.0f;
    colorBlendingCI.blendConstants[2] = 0.0f;
    colorBlendingCI.blendConstants[3] = 0.0f;

    // Pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutCI = {};
    pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCI.setLayoutCount =
        static_cast<uint32_t>(m_renderer.getDescriptorSetLayouts().size());
    pipelineLayoutCI.pSetLayouts = m_renderer.getDescriptorSetLayouts().data();
    pipelineLayoutCI.pushConstantRangeCount =
        static_cast<uint32_t>(m_renderer.getPushConstantRanges().size());
    pipelineLayoutCI.pPushConstantRanges =
        m_renderer.getPushConstantRanges().data();

    VK_CHECK(vkCreatePipelineLayout(m_renderer.getDevice(), &pipelineLayoutCI,
                                    m_renderer.getAllocator(), &m_layout));

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCI = {};
    inputAssemblyCI.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCI.primitiveRestartEnable = false;

    // Viewport state
    VkViewport vp = {};
    vp.x = 0.0f;
    vp.y = 0.0f;
    vp.width = m_renderer.getSwapchainExtent().width;
    vp.height = m_renderer.getSwapchainExtent().height;
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.extent = {m_renderer.getSwapchainExtent()};
    scissor.offset = {0, 0};

    VkPipelineViewportStateCreateInfo viewPortStateCI = {};
    viewPortStateCI.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewPortStateCI.viewportCount = 1;
    viewPortStateCI.pViewports = &vp;
    viewPortStateCI.scissorCount = 1;
    viewPortStateCI.pScissors = &scissor;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizerStateCI = {};
    rasterizerStateCI.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerStateCI.depthClampEnable = false;
    rasterizerStateCI.rasterizerDiscardEnable = false;
    rasterizerStateCI.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizerStateCI.lineWidth = 1.0f;
    rasterizerStateCI.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizerStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizerStateCI.depthBiasEnable = false;
    rasterizerStateCI.depthBiasConstantFactor = 0.0f;
    rasterizerStateCI.depthBiasClamp = 0.0f;
    rasterizerStateCI.depthBiasSlopeFactor = 0.0f;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisamplingStateCI = {};
    multisamplingStateCI.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisamplingStateCI.sampleShadingEnable = false;
    multisamplingStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisamplingStateCI.minSampleShading = 1.0f;
    multisamplingStateCI.pSampleMask = nullptr;
    multisamplingStateCI.alphaToCoverageEnable = false;
    multisamplingStateCI.alphaToOneEnable = false;

    // Depth & stencil
    VkStencilOpState stencilOpFront = {};
    stencilOpFront.failOp = VK_STENCIL_OP_KEEP;
    stencilOpFront.passOp = VK_STENCIL_OP_KEEP;
    stencilOpFront.depthFailOp = VK_STENCIL_OP_KEEP;
    stencilOpFront.compareOp = VK_COMPARE_OP_NEVER;
    stencilOpFront.compareMask = 0;
    stencilOpFront.writeMask = 0;
    stencilOpFront.reference = 0;

    VkStencilOpState stencilOpBack = {};
    stencilOpBack.failOp = VK_STENCIL_OP_KEEP;
    stencilOpBack.passOp = VK_STENCIL_OP_KEEP;
    stencilOpBack.depthFailOp = VK_STENCIL_OP_KEEP;
    stencilOpBack.compareOp = VK_COMPARE_OP_NEVER;
    stencilOpBack.compareMask = 0;
    stencilOpBack.writeMask = 0;
    stencilOpBack.reference = 0;

    VkPipelineDepthStencilStateCreateInfo depthStencilCI = {};
    depthStencilCI.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilCI.depthTestEnable = true;
    depthStencilCI.depthWriteEnable = true;
    depthStencilCI.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilCI.depthBoundsTestEnable = false;
    depthStencilCI.minDepthBounds = 0.0f;
    depthStencilCI.maxDepthBounds = 1.0f;
    depthStencilCI.stencilTestEnable = false;
    depthStencilCI.front = stencilOpFront;
    depthStencilCI.back = stencilOpBack;

    // Pipeline
    VkGraphicsPipelineCreateInfo pipelineCI = {};
    pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCI.stageCount = static_cast<uint32_t>(shaderCIs.size());
    pipelineCI.pStages = shaderCIs.data();
    pipelineCI.pVertexInputState = &vertexInputCI;
    pipelineCI.pInputAssemblyState = &inputAssemblyCI;
    pipelineCI.pViewportState = &viewPortStateCI;
    pipelineCI.pRasterizationState = &rasterizerStateCI;
    pipelineCI.pMultisampleState = &multisamplingStateCI;
    pipelineCI.pDepthStencilState = &depthStencilCI;
    pipelineCI.pColorBlendState = &colorBlendingCI;
    pipelineCI.pDynamicState = nullptr;
    pipelineCI.layout = m_layout;
    pipelineCI.renderPass = m_renderer.getRenderPass();
    pipelineCI.subpass = 0;
    pipelineCI.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCI.basePipelineIndex = -1;

    VK_CHECK(vkCreateGraphicsPipelines(m_renderer.getDevice(), VK_NULL_HANDLE,
                                       1, &pipelineCI,
                                       m_renderer.getAllocator(), &m_handle));

    // Clean up shader modules
    for (auto &it : m_shaderModules) {
        vkDestroyShaderModule(m_renderer.getDevice(), it,
                              m_renderer.getAllocator());
    }
    m_shaderModules.clear();
}

void GraphicsPipeline::destroy(bool recycle) {
    LOG("=Destroy graphics pipeline=");
    if (!recycle && glslangInitialized) {
        glslang::FinalizeProcess();
        glslangInitialized = false;
    }
    vkDestroyPipelineLayout(m_renderer.getDevice(), m_layout,
                            m_renderer.getAllocator());
    vkDestroyPipeline(m_renderer.getDevice(), m_handle,
                      m_renderer.getAllocator());

    for (auto &it : m_shaderModules) {
        vkDestroyShaderModule(m_renderer.getDevice(), it,
                              m_renderer.getAllocator());
    }
    m_shaderModules.clear();

    m_layout = VK_NULL_HANDLE;
    m_handle = VK_NULL_HANDLE;
}

Logger &GraphicsPipeline::getLogger() { return m_renderer.getLogger(); }

VkShaderModule GraphicsPipeline::createShaderModule(const uint32_t *code,
                                                    uint32_t bytes) {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = bytes;
    createInfo.pCode = code;

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    VK_CHECK(vkCreateShaderModule(m_renderer.getDevice(), &createInfo,
                                  m_renderer.getAllocator(), &shaderModule));

    return shaderModule;
}

VkShaderModule
GraphicsPipeline::createShaderModuleFromSpirV(const char *fName) {
    std::ifstream file(fName, std::ios::ate | std::ios::binary);
    THROW_IF(file.is_open() == false, "Failed to open file!");
    const uint32_t fileSize = (uint32_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return createShaderModule(reinterpret_cast<uint32_t *>(buffer.data()),
                              static_cast<uint32_t>(buffer.size()));
}

VkShaderModule
GraphicsPipeline::createShaderModuleFromGLSL(std::string &&filename,
                                             EShLanguage shaderStage) {
    LOG("=Creating shader module from %s=", filename.c_str());
    // Initialize only once per process
    if (!glslangInitialized) {
        glslang::InitializeProcess();
        glslangInitialized = true;
    }

    std::filesystem::path f{filename.c_str()};
    THROW_IF(!std::filesystem::exists(f), "File %s does not exist",
             filename.c_str());
    std::ifstream file(filename.c_str(), std::ios::in);
    THROW_IF(!file.is_open(), "Problem opening file %s", filename.c_str());

    std::string sourceStr((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());

    const char *sourceCStr = sourceStr.c_str();
    std::string pathStr(filename.substr(0, filename.find_last_of("/")));
    DirStackFileIncluder includer;
    includer.pushExternalLocalDirectory(pathStr);

    glslang::TShader shader(shaderStage);
    shader.setStrings(&sourceCStr, 1);

    // Specify Vulkan/SpirV environment
    // Get these from somewhere?
    const int defaultVersion = 100;
    const int clientInputSemanticsVersion = 100;
    glslang::EShTargetClientVersion vulkanVersion =
        glslang::EShTargetVulkan_1_0;
    glslang::EShTargetLanguageVersion targetVersion = glslang::EShTargetSpv_1_0;

    shader.setEnvInput(glslang::EShSourceGlsl, shaderStage,
                       glslang::EShClientVulkan, clientInputSemanticsVersion);
    shader.setEnvClient(glslang::EShClientVulkan, vulkanVersion);
    shader.setEnvTarget(glslang::EShTargetSpv, targetVersion);

    TBuiltInResource resources = DefaultTBuiltInResource;
    EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
    std::string tempStr;

    if (!shader.preprocess(&resources, defaultVersion, ENoProfile, false, false,
                           messages, &tempStr, includer)) {
        LOG("GLSL preprocessing failed for %s\n%s\n%s\n", filename.c_str(),
            shader.getInfoLog(), shader.getInfoDebugLog());

        return VK_NULL_HANDLE;
    }

    const char *preprocessedCStr = tempStr.c_str();
    shader.setStrings(&preprocessedCStr, 1);

    if (!shader.parse(&resources, defaultVersion, false, messages)) {
        LOG("GLSL parse failed for %s\n%s\n%s\n", filename.c_str(),
            shader.getInfoLog(), shader.getInfoDebugLog());

        return VK_NULL_HANDLE;
    }

    glslang::TProgram program;
    program.addShader(&shader);

    if (!program.link(messages)) {
        LOG("GLSL linking failed for %s\n%s\n%s\n", filename.c_str(),
            shader.getInfoLog(), shader.getInfoDebugLog());

        return VK_NULL_HANDLE;
    }

    std::vector<uint32_t> spirvCode;
    spv::SpvBuildLogger logger;
    glslang::SpvOptions spvOptions;
    glslang::GlslangToSpv(*program.getIntermediate(shaderStage), spirvCode,
                          &logger, &spvOptions);

    if (logger.getAllMessages().length() > 0) {
        LOG(logger.getAllMessages().c_str());
    }

    return createShaderModule(
        spirvCode.data(),
        static_cast<uint32_t>(spirvCode.size() * sizeof(spirvCode[0])));
}
} // namespace vulkan_proto
