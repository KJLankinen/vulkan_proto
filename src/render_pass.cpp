#include "render_pass.h"
#include "device.h"
#include "renderer.h"
#include "swapchain.h"

namespace vulkan_proto {
RenderPass::RenderPass(Renderer &renderer) : m_renderer(renderer) {}
RenderPass::~RenderPass() {}

void RenderPass::create(bool recycle) {
    LOG("=Create render pass=");
    if (recycle) {
        destroy();
    }
    VkAttachmentDescription colorAttchDes = {};
    colorAttchDes.flags = 0;
    colorAttchDes.format = m_renderer.getSurfaceFormat();
    colorAttchDes.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttchDes.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttchDes.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttchDes.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttchDes.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttchDes.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttchDes.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttchDes = {};
    depthAttchDes.flags = 0;
    depthAttchDes.format = m_renderer.getDepthFormat();
    depthAttchDes.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttchDes.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttchDes.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttchDes.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttchDes.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttchDes.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttchDes.finalLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttchDes,
                                                          depthAttchDes};

    VkAttachmentReference colorAttchRef = {};
    colorAttchRef.attachment = 0;
    colorAttchRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttchRef = {};
    depthAttchRef.attachment = 1;
    depthAttchRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttchRef;
    subpass.pDepthStencilAttachment = &depthAttchRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependency.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                               VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassCi = {};
    renderPassCi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCi.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassCi.pAttachments = attachments.data();
    renderPassCi.subpassCount = 1;
    renderPassCi.pSubpasses = &subpass;
    renderPassCi.dependencyCount = 1;
    renderPassCi.pDependencies = &dependency;

    VK_CHECK(vkCreateRenderPass(m_renderer.getDevice(), &renderPassCi,
                                m_renderer.getAllocator(), &m_handle));
}

void RenderPass::destroy() {
    LOG("=Destroy render pass=");
    vkDestroyRenderPass(m_renderer.getDevice(), m_handle,
                        m_renderer.getAllocator());
    m_handle = VK_NULL_HANDLE;
}

Logger &RenderPass::getLogger() { return m_renderer.getLogger(); }
} // namespace vulkan_proto
