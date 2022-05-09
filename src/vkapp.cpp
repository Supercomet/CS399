#include <array>
#include <iostream>     // std::cout
#include <fstream>      // std::ifstream


#ifdef WIN64
#else
#include <unistd.h>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "vkapp.h"

#include "app.h"

 
template <class integral>
constexpr integral align_up(integral x, size_t a) noexcept
{
    return integral((x + (integral(a) - 1)) & ~integral(a - 1));
}

#define GLM_FORCE_RADIANS
#define GLM_SWIZZLE
#include <glm/glm.hpp>

VkApp::VkApp(App* _app) : app(_app)
{
     createInstance(app->doApiDump);
     assert (m_instance);
     createPhysicalDevice(); // i.e. the GPU
     assert (m_physicalDevice);
     chooseQueueIndex();
     createDevice();
     getCommandQueue();

    loadExtensions();

    getSurface();
    createCommandPool();
    
    createSwapchain();
    createDepthResource();
    createPostRenderPass();
    createPostFrameBuffers();

    //createPostDescriptor();
    createPostPipeline();

    #ifdef GUI
    initGUI();
    #endif
    
    // myloadModel("models/living_room.obj", glm::mat4());

    // createScBuffer();
    // createRtBuffers();
    // createDenoiseBuffer();

    // createScanlineRenderPass();
    // createStuff();

    // createMatrixBuffer();
    // createObjDescriptionBuffer();
    // createScDescriptorSet();
    // createScPipeline();

    // //init ray tracing capabilities
    // initRayTracing();
    // createRtAccelerationStructure();
    // createRtDescriptorSet();
    // createRtPipeline();
    // createRtShaderBindingTable();

    // createDenoiseDescriptorSet();
    // createDenoiseCompPipeline();

}

void VkApp::drawFrame()
{        
    prepareFrame();
    
    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(m_commandBuffer, &beginInfo);
    {   // Extra indent for recording commands into m_commandBuffer
        //updateCameraBuffer();
        
        // Draw scene
        // if (useRaytracer) {
        //     raytrace();`
        //     denoise(); }
        // else {
        //     rasterize(); }
        
        postProcess(); //  tone mapper and output to swapchain image.
        
        vkEndCommandBuffer(m_commandBuffer);
    }   // Done recording;  Execute!
    
        submitFrame();  // Submit for display
}

VkAccessFlags accessFlagsForImageLayout(VkImageLayout layout)
{
    switch(layout)
        {
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            return VK_ACCESS_HOST_WRITE_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            return VK_ACCESS_TRANSFER_WRITE_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            return VK_ACCESS_TRANSFER_READ_BIT;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            return VK_ACCESS_SHADER_READ_BIT;
        default:
            return VkAccessFlags();
        }
}

VkPipelineStageFlags pipelineStageForLayout(VkImageLayout layout)
{
    switch(layout)
        {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            return VK_PIPELINE_STAGE_TRANSFER_BIT;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;  // Allow queue other than graphic
            // return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;  // Allow queue other than graphic
            // return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            return VK_PIPELINE_STAGE_HOST_BIT;
        case VK_IMAGE_LAYOUT_UNDEFINED:
            return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        default:
            return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        }
}

//-------------------------------------------------------------------------------------------------
// Post processing pass: tone mapper, UI
//void VkApp::postProcess()
//{
//    std::array<VkClearValue, 2> clearValues{};
//    clearValues[0].color        = {{1,1,1,1}};
//    clearValues[1].depthStencil = {1.0f, 0};
//            
//    VkRenderPassBeginInfo _i{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
//    _i.clearValueCount = 2;
//    _i.pClearValues    = clearValues.data();
//    _i.renderPass      = m_postRenderPass;
//    _i.framebuffer     = m_framebuffers[m_swapchainIndex];
//    _i.renderArea      = {{0, 0}, windowSize};
//    vkCmdBeginRenderPass(m_commandBuffer, &_i, VK_SUBPASS_CONTENTS_INLINE);
//    {   // extra indent for renderpass commands
//        VkViewport viewport{0.0f, 0.0f,
//            static_cast<float>(windowSize.width), static_cast<float>(windowSize.height),
//            0.0f, 1.0f};
//        vkCmdSetViewport(m_commandBuffer, 0, 1, &viewport);
//        
//        VkRect2D scissor{{0, 0}, {windowSize.width, windowSize.height}};
//        vkCmdSetScissor(m_commandBuffer, 0, 1, &scissor);
//
//        auto aspectRatio = static_cast<float>(windowSize.width)
//            / static_cast<float>(windowSize.height);
//        vkCmdPushConstants(m_commandBuffer, m_postPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
//                           sizeof(float), &aspectRatio);
//        vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_postPipeline);
//        vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
//                                m_postPipelineLayout, 0, 0, nullptr, 0, nullptr);
//
//        // Weird! This draws 3 vertices but with no vertices/triangles buffers bound in.
//        // Hint: The vertex shader fabricates vertices from gl_VertexIndex
//        vkCmdDraw(m_commandBuffer, 3, 1, 0, 0);
//
//        #ifdef GUI
//        ImGui::Render();  // Rendering UI
//        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_commandBuffer);
//        #endif
//    }
//    vkCmdEndRenderPass(m_commandBuffer);
//}


VkCommandBuffer VkApp::createTempCmdBuffer()
{
    VkCommandBufferAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocateInfo.commandBufferCount = 1;
    allocateInfo.commandPool        = m_cmdPool;
    allocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    VkCommandBuffer cmdBuffer;
    vkAllocateCommandBuffers(m_device, &allocateInfo, &cmdBuffer);

    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmdBuffer, &beginInfo);
    return cmdBuffer;
}

void VkApp::submitTempCmdBuffer(VkCommandBuffer cmdBuffer)
{
    vkEndCommandBuffer(cmdBuffer);

    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &cmdBuffer;
    vkQueueSubmit(m_queue, 1, &submitInfo, {});
    vkQueueWaitIdle(m_queue);
    vkFreeCommandBuffers(m_device, m_cmdPool, 1, &cmdBuffer);
}

void VkApp::prepareFrame()
{
    // Acquire the next image from the swap chain --> m_swapchainIndex
    VkResult result = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_readSemaphore,
                                            (VkFence)VK_NULL_HANDLE, &m_swapchainIndex);

    // Check if window has been resized -- or other(??) swapchain specific event
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreateSizedResources(windowSize); }

    // Use a fence to wait until the command buffer has finished execution before using it again
    while (VK_TIMEOUT == vkWaitForFences(m_device, 1, &m_waitFence, VK_TRUE, 1'000'000))
        {}
}

void VkApp::submitFrame()
{
    vkResetFences(m_device, 1, &m_waitFence);

    // Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
    const VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
     
    // The submit info structure specifies a command buffer queue submission batch
    VkSubmitInfo _si_{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    _si_.pNext             = nullptr;
    _si_.pWaitDstStageMask = &waitStageMask; //  pipeline stages to wait for
    _si_.waitSemaphoreCount   = 1;  
    _si_.pWaitSemaphores = &m_readSemaphore;  // waited upon before execution
    _si_.signalSemaphoreCount = 1;
    _si_.pSignalSemaphores    = &m_writtenSemaphore; // signaled when execution finishes
    _si_.commandBufferCount = 1;
    _si_.pCommandBuffers = &m_commandBuffer;
    if (vkQueueSubmit(m_queue, 1, &_si_, m_waitFence) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!"); }
    
    // Present frame
    VkPresentInfoKHR _i_{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    _i_.waitSemaphoreCount = 1;
    _i_.pWaitSemaphores    = &m_writtenSemaphore;;
    _i_.swapchainCount     = 1;
    _i_.pSwapchains        = &m_swapchain;
    _i_.pImageIndices      = &m_swapchainIndex;
    if (vkQueuePresentKHR(m_queue, &_i_) != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!"); }
}


VkShaderModule VkApp::createShaderModule(std::string code)
{
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize                 = code.size();
    createInfo.pCode                    = (uint32_t*) code.data();

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        assert(0 && "failed to create shader module!");

    return shaderModule;
}

VkPipelineShaderStageCreateInfo VkApp::createShaderStageInfo(const std::string&    code,
                                                                   VkShaderStageFlagBits stage,
                                                                   const char* entryPoint)
{
    VkPipelineShaderStageCreateInfo shaderStage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    shaderStage.stage  = stage;
    shaderStage.module = createShaderModule(code);
    shaderStage.pName  = entryPoint;
    return shaderStage;
}
