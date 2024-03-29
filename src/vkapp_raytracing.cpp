
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <array>
#include <math.h>

#include "vkapp.h"

#define GLM_FORCE_RADIANS
#define GLM_SWIZZLE
#include <glm/glm.hpp>
using namespace glm;

#include "app.h"
#include "shaders/shared_structs.h"


void VkApp::createRtBuffers()
{
    m_rtColCurrBuffer = createBufferImage(windowSize);
    transitionImageLayout(m_rtColCurrBuffer.image, VK_FORMAT_R32G32B32A32_SFLOAT,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_GENERAL, 1);

    m_rtColPrevBuffer = createBufferImage(windowSize);
    transitionImageLayout(m_rtColPrevBuffer.image, VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL, 1);

    m_rtNdCurrBuffer = createBufferImage(windowSize);
    transitionImageLayout(m_rtNdCurrBuffer.image, VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL, 1);

    m_rtNdPrevBuffer = createBufferImage(windowSize);
    transitionImageLayout(m_rtNdPrevBuffer.image, VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL, 1);

    m_rtKdCurrBuffer = createBufferImage(windowSize);
    transitionImageLayout(m_rtKdCurrBuffer.image, VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL, 1);

}

// Initialize ray tracing
void VkApp::initRayTracing()
{

    m_pcRay.rr = 0.8f;

    // Requesting ray tracing properties
    VkPhysicalDeviceProperties2 prop2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProps
        {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
    prop2.pNext = &rtProps;
    vkGetPhysicalDeviceProperties2(m_physicalDevice, &prop2);

    handleSize      = rtProps.shaderGroupHandleSize;
    handleAlignment = rtProps.shaderGroupHandleAlignment;
    baseAlignment   = rtProps.shaderGroupBaseAlignment;
    
    m_rtBuilder.setup(this, m_device, m_graphicsQueueIndex);

    // @@ Call  m_rtBuilder.destroy() after the acceleration building is done.
}

//-------------------------------------------------------------------------------------------------
// This descriptor set holds the Acceleration structure and the output image
//
void VkApp::createRtDescriptorSet()
{

    m_rtDesc.setBindings(m_device, {
            {RtBindings::eTlas, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1,  // TLAS
             VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR},
            {RtBindings::eOutCurrImage, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1,  // Output 4 images
             VK_SHADER_STAGE_RAYGEN_BIT_KHR},
            {RtBindings::eLights, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR 
            | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT},

            {RtBindings::eOutPrevImage, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR},
            {RtBindings::eOutCurrNd, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR},
            {RtBindings::eOutPrevNd, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR},
            {RtBindings::eOutCurrKd, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR},
        });
    

    m_rtDesc.write(m_device, RtBindings::eTlas, m_rtBuilder.getAccelerationStructure());
    m_rtDesc.write(m_device, RtBindings::eOutCurrImage, m_rtColCurrBuffer.Descriptor());
    m_rtDesc.write(m_device, RtBindings::eLights, m_lightBuffer.buffer);
    m_rtDesc.write(m_device, RtBindings::eOutPrevImage, m_rtColPrevBuffer.Descriptor());
    m_rtDesc.write(m_device, RtBindings::eOutCurrNd, m_rtNdCurrBuffer.Descriptor());
    m_rtDesc.write(m_device, RtBindings::eOutPrevNd, m_rtNdPrevBuffer.Descriptor());
    m_rtDesc.write(m_device, RtBindings::eOutCurrKd, m_rtKdCurrBuffer.Descriptor());
}

// Pipeline for the ray tracer: all shaders, raygen, chit, miss
//
void VkApp::createRtPipeline()
{
    ////////////////////////////////////////////////////////////////////////////////////////////
    // stages: Array of shaders: 1 raygen, 1 miss, 1 hit (later: an additional hit/miss pair.)

    ////////////////////////////////////////////////////////////////////////////////////////////
    // Group the shaders.  Raygen and miss shaders get their own
    // groups. Hit shaders can group with any-hit and intersection
    // shaders -- of which we have none -- so the hit shader(s) get
    // their own group also.
    std::vector<VkPipelineShaderStageCreateInfo> stages{};
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> groups{};

    VkPipelineShaderStageCreateInfo stage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stage.pName = "main";  // All the same entry point

    VkRayTracingShaderGroupCreateInfoKHR group
        {VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR};
    group.anyHitShader       = VK_SHADER_UNUSED_KHR;
    group.closestHitShader   = VK_SHADER_UNUSED_KHR;
    group.generalShader      = VK_SHADER_UNUSED_KHR;
    group.intersectionShader = VK_SHADER_UNUSED_KHR;

    // Raygen shader stage and group appended to stages and groups lists
    stage.module = createShaderModule(loadFile("spv/raytrace.rgen.spv"));
    stage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    stages.push_back(stage);
    
    group.type          = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    group.generalShader = stages.size()-1;    // Index of raygen shader
    groups.push_back(group);
    group.generalShader    = VK_SHADER_UNUSED_KHR;
    
    // Miss shader stage and group appended to stages and groups lists
    stage.module = createShaderModule(loadFile("spv/raytrace.rmiss.spv"));
    stage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
    stages.push_back(stage);

    group.type          = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    group.generalShader = stages.size()-1;    // Index of miss shader
    groups.push_back(group);
    group.generalShader    = VK_SHADER_UNUSED_KHR;

    stage.module = createShaderModule(loadFile("spv/raytraceShadow.rmiss.spv"));
    stage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
    stages.push_back(stage);
    
    group.type          = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    group.generalShader = stages.size()-1;    // Index of miss shader
    groups.push_back(group);
    group.generalShader    = VK_SHADER_UNUSED_KHR;

    // Closest hit shader stage and group appended to stages and groups lists
    stage.module = createShaderModule(loadFile("spv/raytrace.rchit.spv"));
    stage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    stages.push_back(stage);   

    group.type             = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    group.closestHitShader = stages.size()-1;   // Index of hit shader
    groups.push_back(group);


    ////////////////////////////////////////////////////////////////////////////////////////////
    // Create the ray tracing pipeline layout.
    // Push constant: we want to be able to update constants used by the shaders
    VkPushConstantRange pushConstant{VK_SHADER_STAGE_RAYGEN_BIT_KHR
        | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR,
        0, sizeof(PushConstantRay)};

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo
        {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges    = &pushConstant;

    // Descriptor sets: one specific to ray tracing, and one shared with the rasterization pipeline
    std::vector<VkDescriptorSetLayout> rtDescSetLayouts =
        {m_rtDesc.descSetLayout, m_scDesc.descSetLayout};
    pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(rtDescSetLayouts.size());
    pipelineLayoutCreateInfo.pSetLayouts = rtDescSetLayouts.data();

    vkCreatePipelineLayout(m_device, &pipelineLayoutCreateInfo, nullptr, &m_rtPipelineLayout);

    ////////////////////////////////////////////////////////////////////////////////////////////
    // Create the ray tracing pipeline.
    // Assemble the shader stages and recursion depth info into the ray tracing pipeline
    VkRayTracingPipelineCreateInfoKHR rayPipelineInfo
        {VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR};
    rayPipelineInfo.stageCount = static_cast<uint32_t>(stages.size());  // Stages are shaders
    rayPipelineInfo.pStages    = stages.data();

    rayPipelineInfo.groupCount = static_cast<uint32_t>(groups.size());
    rayPipelineInfo.pGroups    = groups.data();

    rayPipelineInfo.maxPipelineRayRecursionDepth = 10;  // Ray depth
    rayPipelineInfo.layout                       = m_rtPipelineLayout;

    vkCreateRayTracingPipelinesKHR(m_device, {}, {}, 1, &rayPipelineInfo, nullptr, &m_rtPipeline);
    for (auto& s : stages)
        vkDestroyShaderModule(m_device, s.module, nullptr);

}

//--------------------------------------------------------------------------------------------------
// The Shader Binding Table (SBT)
// - getting all shader handles and write them in a SBT buffer
//
template <class integral>
constexpr integral align_up(integral x, size_t a) noexcept
{
    return integral((x + (integral(a) - 1)) & ~integral(a - 1));
}

void VkApp::createRtShaderBindingTable()
{
    uint32_t missCount{2};
    uint32_t hitCount{1};
    auto     handleCount = 1 + missCount + hitCount;

    // The SBT (buffer) needs to have starting group to be aligned
    // and handles in the group to be aligned.
    uint32_t handleSizeAligned = align_up(handleSize, handleAlignment);  // handleAlignment==32

    m_rgenRegion.stride = align_up(handleSizeAligned, baseAlignment); //baseAlignment==64
    m_rgenRegion.size = m_rgenRegion.stride;  // The size member must be equal to its stride member
    
    m_missRegion.stride = handleSizeAligned;
    m_missRegion.size   = align_up(missCount * handleSizeAligned, baseAlignment);
    
    m_hitRegion.stride  = handleSizeAligned;
    m_hitRegion.size    = align_up(hitCount * handleSizeAligned, baseAlignment);

    printf("Shader binding table:\n");
    printf("  alignments:\n");
    printf("    handleAlignment: %d\n", handleAlignment);
    printf("    baseAlignment:   %d\n", baseAlignment);
    printf("  counts:\n");
    printf("    miss:   %d\n", missCount);
    printf("    hit:    %d\n", hitCount);
    printf("    handle: %d = 1+missCount+hitCount\n", handleCount);
    printf("  regions stride:size:\n");
    printf("    rgen %2ld:%2ld\n", m_rgenRegion.stride, m_rgenRegion.size);
    printf("    miss %2ld:%2ld\n", m_missRegion.stride, m_missRegion.size);
    printf("    hit  %2ld:%2ld\n", m_hitRegion.stride,  m_hitRegion.size);
    printf("    call %2ld:%2ld\n", m_callRegion.stride, m_callRegion.size);

    // Get the shader group handles.  This is a byte array retrieved
    // from the pipeline.
    uint32_t             dataSize = handleCount * handleSize;
    std::vector<uint8_t> handles(dataSize);
    auto result = vkGetRayTracingShaderGroupHandlesKHR(m_device, m_rtPipeline,
                                                       0, handleCount, dataSize, handles.data());
    assert(result == VK_SUCCESS);

    // Allocate a buffer for storing the SBT, and a staging buffer for transferring data to it.
    VkDeviceSize sbtSize = m_rgenRegion.size + m_missRegion.size
        + m_hitRegion.size + m_callRegion.size;
    
    BufferWrap staging = createBufferWrap(sbtSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                      | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    m_shaderBindingTableBW = createBufferWrap(sbtSize,
                                  VK_BUFFER_USAGE_TRANSFER_DST_BIT
                                  | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
                                  | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // Find the SBT addresses of each group
    VkBufferDeviceAddressInfo info = {VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
    info.buffer                    = m_shaderBindingTableBW.buffer;
    VkDeviceAddress sbtAddress = vkGetBufferDeviceAddress(m_device, &info);
    
    m_rgenRegion.deviceAddress = sbtAddress;
    m_missRegion.deviceAddress = sbtAddress + m_rgenRegion.size;
    m_hitRegion.deviceAddress  = sbtAddress + m_rgenRegion.size + m_missRegion.size;

    // Helper to retrieve the handle data
    auto getHandle = [&](int i) { return handles.data() + i * handleSize; };

    // Map the SBT buffer and write in the handles.
    uint8_t* mappedMemAddress;
    vkMapMemory(m_device, staging.memory, 0, sbtSize, 0, (void**)&mappedMemAddress);
    uint8_t offset = 0;

    // Raygen
    uint32_t handleIdx{0};
    memcpy(mappedMemAddress+offset, getHandle(handleIdx++), handleSize);

    // Miss
    offset = m_rgenRegion.size;
    for(uint32_t c = 0; c < missCount; c++) {
        memcpy(mappedMemAddress+offset, getHandle(handleIdx++), handleSize);
        offset += m_missRegion.stride; }

    // Hit
    offset = m_rgenRegion.size + m_missRegion.size;
    for(uint32_t c = 0; c < hitCount; c++) {
        memcpy(mappedMemAddress+offset, getHandle(handleIdx++), handleSize);
        offset += m_hitRegion.stride; }

    vkUnmapMemory(m_device, staging.memory);
    
    copyBuffer(staging.buffer, m_shaderBindingTableBW.buffer, sbtSize);

    staging.destroy(m_device);

    // @@ destroy acceleration structure with m_shaderBindingTableBW.destroy(m_device);
}

void VkApp::raytrace()
{

    m_pcRay.moved = app->myCamera.moved;

    if (useHistory) m_pcRay.moved = false;
    if (prevUseHistory != useHistory)
    {
        // we just changed lets refresh
        m_pcRay.moved = true;
        prevUseHistory = useHistory;
    }
    if (prevUseDenoise != useDenoise)
    {
        // we just changed lets refresh
        m_pcRay.moved = true;
        prevUseDenoise = useDenoise;
    }

    if (m_pcRay.moved)
    {
        currIterations = 0;
    }
    app->myCamera.moved = false;
    ++currIterations;
    m_pcRay.frameSeed = rand() % 32768;
    m_pcRay.depth=1;
    m_pcRay.ExplicitLightRays = useExplicit;
    m_pcRay.n_threshold = f_nThreshold;
    m_pcRay.d_threshold = f_dThreshold;
    while (float(rand())/RAND_MAX < m_pcRay.rr)   m_pcRay.depth++;

    // Bind the ray tracing pipeline
    vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_rtPipeline);

    // Bind the descriptor sets (the ray tracing specific one, and the
    // full model descriptor)
    std::vector<VkDescriptorSet> descSets{m_rtDesc.descSet, m_scDesc.descSet};
    vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                            m_rtPipelineLayout, 0, (uint32_t)descSets.size(),
                            descSets.data(), 0, nullptr);

    // Push the push constants
    vkCmdPushConstants(m_commandBuffer, m_rtPipelineLayout,
                       VK_SHADER_STAGE_RAYGEN_BIT_KHR
                       | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
                       | VK_SHADER_STAGE_MISS_BIT_KHR,
                       0, sizeof(PushConstantRay), &m_pcRay);

    vkCmdTraceRaysKHR(m_commandBuffer, &m_rgenRegion, &m_missRegion, &m_hitRegion,
                      &m_callRegion, windowSize.width, windowSize.height, 1);


    // Copy the ray tracer output image to the scanline output image
    // -- because we already have the operations needed to display
    // that image on the screen.
    CmdCopyImage(m_rtColCurrBuffer, m_scImageBuffer);
    CmdCopyImage(m_rtColCurrBuffer, m_rtColPrevBuffer);
    CmdCopyImage(m_rtNdCurrBuffer, m_rtNdPrevBuffer);

    
}

