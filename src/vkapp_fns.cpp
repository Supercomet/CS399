
#include <array>
#include <iostream>     // std::cout
#include <fstream>      // std::ifstream
#include <exception> // runtime_error

#include <unordered_set>
#include <unordered_map>

#include "vkapp.h"
#include "app.h"
#include "extensions_vk.hpp"
#include "vktools.h" // custom helpers

#ifdef GUI
#include "backends/imgui_impl_glfw.h"
#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"
#endif

#define VK_CHK(x) do{\
VkResult result = x;\
assert(result == VK_SUCCESS);\
if(result != VK_SUCCESS){\
throw std::runtime_error("Failed Vulkan Check");\
}\
}while(0)\


void VkApp::destroyAllVulkanResources()
{
    // @@
     vkDeviceWaitIdle(m_device);  // Uncomment this when you have an m_device created.

     vkDestroyPipelineLayout(m_device, m_scanlinePipelineLayout, nullptr);
     vkDestroyPipeline(m_device, m_scanlinePipeline, nullptr);

     m_scDesc.destroy(m_device);

     vkDestroyRenderPass(m_device, m_scanlineRenderPass, nullptr);
     vkDestroyFramebuffer(m_device, m_scanlineFramebuffer, nullptr);

     m_objDescriptionBW.destroy(m_device);

     m_matrixBW.destroy(m_device);

     for (size_t i = 0; i < m_objText.size(); i++)
     {
         m_objText[i].destroy(m_device);
     }

     for (size_t i = 0; i < m_objData.size(); i++)
     {
		 ObjData& obj = m_objData[i];
		 obj.vertexBuffer.destroy(m_device);
		 obj.indexBuffer.destroy(m_device);
		 obj.matColorBuffer.destroy(m_device);
		 obj.matIndexBuffer.destroy(m_device);

     }

     for (size_t i = 0; i < m_objDesc.size(); i++)
     {
         // TODO::destroy description
         //m_objDesc[i].destroy(m_device);
     }

#ifdef GUI
     destroyGUI();
#endif

     m_scImageBuffer.destroy(m_device);
     m_postDesc.destroy(m_device);

     vkDestroyPipelineLayout(m_device, m_postPipelineLayout, nullptr);
     vkDestroyPipeline(m_device, m_postPipeline, nullptr);

     for(uint32_t i = 0; i < m_framebuffers.size(); i++) 
     {
         vkDestroyFramebuffer(m_device, m_framebuffers[i], nullptr);
     }

     vkDestroyRenderPass(m_device, m_postRenderPass, nullptr);
     m_depthImage.destroy(m_device);

     destroySwapchain();

     vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
     vkDestroyCommandPool(m_device, m_cmdPool, nullptr);
    // Destroy all vulkan objects.
    // ...  All objects created on m_device must be destroyed before m_device.
    vkDestroyDevice(m_device, nullptr);
    vkDestroyInstance(m_instance, nullptr);
}

void VkApp::recreateSizedResources(VkExtent2D size)
{
    assert(false && "Not ready for onResize events.");
    // Destroy everything related to the window size
    // (RE)Create them all at the new size
}
 
void VkApp::createInstance(bool doApiDump)
{
    uint32_t countGLFWextensions{0};
    const char** reqGLFWextensions = glfwGetRequiredInstanceExtensions(&countGLFWextensions);
    const char** glfwExts = reqGLFWextensions;
    // @@
    // Append each GLFW required extension in reqGLFWextensions to reqInstanceExtensions
    // Print them out while your are at it
    std::cout << "GLFW required extensions : " << std::endl;
    for (uint32_t x = 0; x < countGLFWextensions; ++x)
    {
        std::cout << '\t' << reqGLFWextensions[x] << std::endl;
        reqInstanceExtensions.push_back(reqGLFWextensions[x]);
    }
    printf("\n");

    // Suggestion: Parse a command line argument to set/unset doApiDump
    if (doApiDump)
        reqInstanceLayers.push_back("VK_LAYER_LUNARG_api_dump");
    
    uint32_t count;
    // The two step procedure for getting a variable length list
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    std::vector<VkLayerProperties> availableLayers(count);
    vkEnumerateInstanceLayerProperties(&count, availableLayers.data());

    // @@
    // Print out the availableLayers
    printf("InstanceLayer count: %d\n", count);
    for (uint32_t x = 0; x < count; ++x)
    {
        // ...  use availableLayers[i].layerName
        std::cout << '\t' << availableLayers[x].layerName << std::endl;
    }
    printf("\n");

    // Another two step dance
    vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(count);
    vkEnumerateInstanceExtensionProperties(nullptr, &count, availableExtensions.data());

    // @@
    // Print out the availableExtensions
    printf("InstanceExtensions count: %d\n", count);
    for (uint32_t x = 0; x < count; ++x)
    {
        // ...  use availableExtensions[i].extensionName
        std::cout << '\t' << availableExtensions[x].extensionName << std::endl;
    }
    printf("\n");

    VkApplicationInfo applicationInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    applicationInfo.pApplicationName = "rtrt";
    applicationInfo.pEngineName      = "no-engine";
    applicationInfo.apiVersion       = VK_MAKE_VERSION(1, 3, 0);

    VkInstanceCreateInfo instanceCreateInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instanceCreateInfo.pNext                   = nullptr;
    instanceCreateInfo.pApplicationInfo        = &applicationInfo;
    
    instanceCreateInfo.enabledExtensionCount   = reqInstanceExtensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = reqInstanceExtensions.data();
    
    instanceCreateInfo.enabledLayerCount       = reqInstanceLayers.size();
    instanceCreateInfo.ppEnabledLayerNames     = reqInstanceLayers.data();

    VK_CHK(vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance));

    // @@
    // Verify VkResult is VK_SUCCESS
    // Document with a cut-and-paste of the three list printouts above.
    // To destroy: vkDestroyInstance(m_instance, nullptr);
}

void VkApp::createPhysicalDevice()
{
    uint physicalDevicesCount;
    vkEnumeratePhysicalDevices(m_instance, &physicalDevicesCount, nullptr);
    std::vector<VkPhysicalDevice> physicalDevices(physicalDevicesCount);
    vkEnumeratePhysicalDevices(m_instance, &physicalDevicesCount, physicalDevices.data());

	std::vector<uint32_t> compatibleDevices;

	printf("%d devices\n", physicalDevicesCount);
	int i = 0;

	// For each GPU:
	for (auto physicalDevice : physicalDevices)
	{

		// Get the GPU's properties
		VkPhysicalDeviceProperties GPUproperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &GPUproperties);

		if (GPUproperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			continue;
		}

		// Get the GPU's extension list;  Another two-step list retrieval procedure:
		uint extCount;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, nullptr);
		std::vector<VkExtensionProperties> extensionProperties(extCount);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr,
			&extCount, extensionProperties.data());

		// @@ This code is in a loop iterating variable physicalDevice
		// through a list of all physicalDevices.  The
		// physicalDevice's properties (GPUproperties) and a list of
		// its extension properties (extensionProperties) are retrieve
		// above, and here we judge if the physicalDevice (i.e.. GPU)
		// is compatible with our requirements. We consider a GPU to be
		// compatible if it satisfies both:

		bool deviceOK = true;
		for (size_t i = 0; i < reqDeviceExtensions.size(); i++)
		{
			bool exists = false;
			for (size_t j = 0; j < extCount; ++j)
			{
				//    All reqDeviceExtensions can be found in the GPUs extensionProperties list
				if (strcmp(extensionProperties[j].extensionName, reqDeviceExtensions[i]) == 0)
					exists = true;
			}
			if (exists == false)
			{
				// we failed to find a suitable extension
				deviceOK = false;
				std::cout << "Device [" << GPUproperties.deviceName << "] missing required Ext " << "[" << reqDeviceExtensions[i] << "]" << std::endl;
				break;
			}
		}
		if (deviceOK)
		{
			compatibleDevices.push_back(i);
		}
		++i;

	}

    //  If a GPU is found to be compatible
    //  Return it (physicalDevice), or continue the search and then return the best GPU.
    //    raise an exception of none were found
    //    tell me all about your system if more than one was found.
    if (compatibleDevices.empty())
    {
        throw std::runtime_error("Could not find suitable GPU");
    }

    if (compatibleDevices.size() > 1)
    {
        std::cout << "[Interest] This system has [" << compatibleDevices.size() << "] compatible devices" << std::endl;
    }

    // we take first one for now. maybe find a better way to select GPU
    m_physicalDevice = physicalDevices[compatibleDevices.front()];
    {
        VkPhysicalDeviceProperties GPUproperties;
        vkGetPhysicalDeviceProperties(m_physicalDevice, &GPUproperties);
        std::cout << "Selected GPU : [" << GPUproperties.deviceName << "]" << std::endl << std::endl;
    }

}

void VkApp::chooseQueueIndex()
{
    VkQueueFlags requiredQueueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT
        | VK_QUEUE_TRANSFER_BIT;

    uint32_t mpCount;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &mpCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueProperties(mpCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &mpCount, queueProperties.data());

    // @@ Use the api_dump to document the results of the above two
    // step.  How many queue families does your Vulkan offer.  Which
    // of them, by index, has the above three required flags?
    uint32_t selectedQueue = -1;
    for (size_t i = 0; i < mpCount; i++)
    {
        if ((queueProperties[i].queueFlags & requiredQueueFlags) == requiredQueueFlags)
        {
            selectedQueue = i;
            break;
        }
    }

    if (selectedQueue == static_cast<uint32_t>(-1))
    {
        throw std::runtime_error("Could not find suitable queue in physical device.");
    }

    // Select this queue
    m_graphicsQueueIndex = selectedQueue;

}


void VkApp::createDevice()
{
   
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeature{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
    
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelFeature{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
    
    VkPhysicalDeviceVulkan13Features features13{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    
    VkPhysicalDeviceVulkan12Features features12{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    
    VkPhysicalDeviceVulkan11Features features11{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
    
    VkPhysicalDeviceFeatures2 features2{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};

    // @@
    // Build a pNext chain of the following six "feature" structures:
    //   features2->features11->features12->features13->accelFeature->rtPipelineFeature->NULL
    features2.pNext = &features11;
    features11.pNext = &features12;
    features12.pNext = &features13;
    features13.pNext = &accelFeature;
    accelFeature.pNext = &rtPipelineFeature;

    // Fill in all structures on the pNext chain
    vkGetPhysicalDeviceFeatures2(m_physicalDevice, &features2);
    // @@
    // Document the whole filled in pNext chain using an api_dump
    // Examine all the many features.  Do any of them look familiar?

    // Turn off robustBufferAccess (WHY?)
    features2.features.robustBufferAccess = VK_FALSE;

    float priority = 1.0;
    VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queueInfo.queueFamilyIndex = m_graphicsQueueIndex;
    queueInfo.queueCount       = 1;
    queueInfo.pQueuePriorities = &priority;
    
    VkDeviceCreateInfo deviceCreateInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    deviceCreateInfo.pNext            = &features2; // This is the whole pNext chain
  
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos    = &queueInfo;
    
    deviceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(reqDeviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = reqDeviceExtensions.data();

    VK_CHK(vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device));
}


void VkApp::getCommandQueue()
{
    vkGetDeviceQueue(m_device, m_graphicsQueueIndex, 0, &m_queue);
    // Returns void -- nothing to verify
    // Nothing to destroy -- the queue is owned by the device.
}

// Calling load_VK_EXTENSIONS from extensions_vk.cpp.  A Python script
// from NVIDIA created extensions_vk.cpp from the current Vulkan spec
// for the purpose of loading the symbols for all registered
// extension.  This be (indistinguishable from) magic.
void VkApp::loadExtensions()
{
    load_VK_EXTENSIONS(m_instance, vkGetInstanceProcAddr, m_device, vkGetDeviceProcAddr);
}

//  VkSurface is Vulkan's name for the screen.  Since GLFW creates and
//  manages the window, it creates the VkSurface at our request.
void VkApp::getSurface()
{
    VkBool32 isSupported;   // Supports drawing(presenting) on a screen

    // @@ Verify VK_SUCCESS from both the glfw... and the vk... calls.
    VK_CHK(glfwCreateWindowSurface(m_instance, app->GLFW_window, nullptr, &m_surface);
    vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, m_graphicsQueueIndex,
        m_surface, &isSupported));

    if (isSupported != VK_TRUE)
    {
    // @@ Verify isSupported==VK_TRUE, meaning that Vulkan supports presenting on this surface.
        throw std::runtime_error("Surface does not support presentation");
    }
}

// Create a command pool, used to allocate command buffers, which in
// turn are use to gather and send commands to the GPU.  The flag
// makes it possible to reuse command buffers.  The queue index
// determines which queue the command buffers can be submitted to.
// Use the command pool to also create a command buffer.
void VkApp::createCommandPool()
{
    VkCommandPoolCreateInfo poolCreateInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolCreateInfo.queueFamilyIndex = m_graphicsQueueIndex;
    // @@ Verify VK_SUCCESS
    VK_CHK(vkCreateCommandPool(m_device, &poolCreateInfo, nullptr, &m_cmdPool));

    // Create a command buffer
    VkCommandBufferAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocateInfo.commandPool        = m_cmdPool;
    allocateInfo.commandBufferCount = 1;
    allocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    VK_CHK(vkAllocateCommandBuffers(m_device, &allocateInfo, &m_commandBuffer));
    // @@ Verify VK_SUCCESS
    // Nothing to destroy -- the pool owns the command buffer.
}

// 
void VkApp::createSwapchain()
{
    VkResult       err;
    VkSwapchainKHR oldSwapchain = m_swapchain;

    vkDeviceWaitIdle(m_device);  // Probably unnecessary

    // Get the surface's capabilities
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &capabilities);

    uint32_t cnt;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &cnt, nullptr);
    std::vector<VkPresentModeKHR> presentModes(cnt);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &cnt, presentModes.data());

    // assume it can present considering we passed a check ealier
    std::cout << "Present modes supported:" << std::endl;
    for (size_t i = 0; i < presentModes.size(); i++)
    {
        switch (presentModes[i])
        {
            case VK_PRESENT_MODE_IMMEDIATE_KHR:
            std::cout<< '\t' << "IMMEDIATE" << std::endl;
            break;
            case VK_PRESENT_MODE_MAILBOX_KHR:
            std::cout<< '\t' << "MAILBOX" << std::endl;
            break;
            case VK_PRESENT_MODE_FIFO_KHR:
            std::cout<< '\t' << "FIFO" << std::endl;
            break;
            case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
            std::cout<< '\t' << "FIFO RELAXED" << std::endl;
            break;
            case VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR:
            std::cout<< '\t' << "SHARED DEMAND REFRESH" << std::endl;
            break;
            case VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR:
            std::cout<< '\t' << "SHARED CONTINUOUS REFRESH" << std::endl;
            break;
        }
    }

    // Choose VK_PRESENT_MODE_FIFO_KHR as a default (this must be supported)
    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR; // Support is required by specification.
    for (size_t i = 0; i < presentModes.size(); i++)
    {
        if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            // @@ But choose VK_PRESENT_MODE_MAILBOX_KHR if it can be found in presentModes
            swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }

    // Get the list of VkFormat's that are supported:
    // @@ Document the list you get.
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &cnt, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(cnt);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &cnt, formats.data());
    std::cout << "Surface formats supported: " << std::endl;
    for (size_t i = 0; i < formats.size(); i++)
    {
        std::cout << "\tFormat: " << vk::tools::VkFormatString(formats[i].format)
                << "\tColourSpace: " << vk::tools::VkColorSpaceKHRString(formats[i].colorSpace) << std::endl;
    }
    std::cout << std::endl;

    VkFormat surfaceFormat       = VK_FORMAT_UNDEFINED;               // Temporary value.
    VkColorSpaceKHR surfaceColor = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR; // Temporary value
                                                                      // @@ Replace the above two temporary lines with the following two
                                                                      // to choose the first format and its color space as defaults:
                                                                      //  VkFormat surfaceFormat = formats[0].format;
                                                                      //  VkColorSpaceKHR surfaceColor  = formats[0].colorSpace;
    surfaceFormat = formats.front().format;
    m_surfaceFormat = surfaceFormat;

    surfaceColor = formats.front().colorSpace;
    for (size_t i = 0; i < formats.size(); i++)
    {
        if (formats[i].format = VK_FORMAT_B8G8R8A8_UNORM)
        {
            surfaceFormat = formats[i].format;
            surfaceColor = formats[i].colorSpace;
            break;
        }
    }
    std::cout<< "Chosen Format:" << vk::tools::VkFormatString(surfaceFormat)
        << "\tColourSpace: " << vk::tools::VkColorSpaceKHRString(surfaceColor) << std::endl;
                                                                      // @@ Then search the formats (from several lines up) to choose
                                                                      // format VK_FORMAT_B8G8R8A8_UNORM (and its color space) if such
                                                                      // exists.  Document your list of formats/color-spaces, and your
                                                                      // particular choice.

                                                                      // Get the swap chain extent
    VkExtent2D swapchainExtent = capabilities.currentExtent;
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        swapchainExtent = capabilities.currentExtent; }
    else 
    {
        // Does this case ever happen?
        int width, height;
        glfwGetFramebufferSize(app->GLFW_window, &width, &height);

        swapchainExtent = VkExtent2D{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

        swapchainExtent.width = std::clamp(swapchainExtent.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width);
        swapchainExtent.height = std::clamp(swapchainExtent.height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height); 
    }

    // Test against valid size, typically hit when windows are minimized.
    // The app must prevent triggering this code in such a case
    assert(swapchainExtent.width && swapchainExtent.height);
    // @@ If this assert fires, we have some work to do to better deal
    // with the situation.

    // Choose the number of swap chain images, within the bounds supported.
    uint imageCount = capabilities.minImageCount + 1; // Recommendation: minImageCount+1
    m_minImageCount = capabilities.minImageCount;
    if (capabilities.maxImageCount > 0
        && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount; }

    assert (imageCount == 3);
    // If this triggers, disable the assert, BUT help me understand
    // the situation that caused it.  

    // Create the swap chain
    VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
        | VK_IMAGE_USAGE_STORAGE_BIT
        | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    VkSwapchainCreateInfoKHR _i = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    _i.surface                  = m_surface;
    _i.minImageCount            = imageCount;
    _i.imageFormat              = surfaceFormat;
    _i.imageColorSpace          = surfaceColor;
    _i.imageExtent              = swapchainExtent;
    _i.imageUsage               = imageUsage;
    _i.preTransform             = capabilities.currentTransform;
    _i.compositeAlpha           = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    _i.imageArrayLayers         = 1;
    _i.imageSharingMode         = VK_SHARING_MODE_EXCLUSIVE;
    _i.queueFamilyIndexCount    = 1;
    _i.pQueueFamilyIndices      = &m_graphicsQueueIndex;
    _i.presentMode              = swapchainPresentMode;
    _i.oldSwapchain             = oldSwapchain;
    _i.clipped                  = true;

    VK_CHK(vkCreateSwapchainKHR(m_device, &_i, nullptr, &m_swapchain));
    // @@ Verify VK_SUCCESS


    // Verify success
    VK_CHK(vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_imageCount, nullptr));
    m_swapchainImages.resize(m_imageCount);
    VK_CHK(vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_imageCount, m_swapchainImages.data())); 

    // Verify and document that you retrieved the correct number of images.
    std::cout << "Swapchain Images: [" << m_swapchainImages.size() << "]" << std::endl;

    m_barriers.resize(m_imageCount);
    m_imageViews.resize(m_imageCount);

    // Create an VkImageView for each swap chain image.
    for (uint i=0;  i<m_imageCount;  i++) 
    {
        VkImageViewCreateInfo createInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        createInfo.image = m_swapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = surfaceFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VK_CHK(vkCreateImageView(m_device, &createInfo, nullptr, &m_imageViews[i])); 
    }

    // Create three VkImageMemoryBarrier structures (one for each swap
    // chain image) and specify the desired
    // layout (VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) for each.
    for (uint i=0;  i<m_imageCount;  i++) 
    {
        VkImageSubresourceRange range = {0};
        range.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel            = 0;
        range.levelCount              = VK_REMAINING_MIP_LEVELS;
        range.baseArrayLayer          = 0;
        range.layerCount              = VK_REMAINING_ARRAY_LAYERS;

        VkImageMemoryBarrier memBarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        memBarrier.dstAccessMask        = 0;
        memBarrier.srcAccessMask        = 0;
        memBarrier.oldLayout            = VK_IMAGE_LAYOUT_UNDEFINED;
        memBarrier.newLayout            = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        memBarrier.image                = m_swapchainImages[i];
        memBarrier.subresourceRange     = range;
        m_barriers[i] = memBarrier;
    }

    // Create a temporary command buffer. submit the layout conversion
    // command, submit and destroy the command buffer.
    VkCommandBuffer cmd = createTempCmdBuffer();
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0,
        nullptr, m_imageCount, m_barriers.data());
    submitTempCmdBuffer(cmd);

    // Create the three synchronization objects.  These are not
    // technically part of the swap chain, but they are used
    // exclusively for synchronizing the swap chain, so I include them
    // here.
    VkFenceCreateInfo fenceCreateInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VK_CHK(vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_waitFence));

    VkSemaphoreCreateInfo semCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VK_CHK(vkCreateSemaphore(m_device, &semCreateInfo, nullptr, &m_readSemaphore));
    VK_CHK(vkCreateSemaphore(m_device, &semCreateInfo, nullptr, &m_writtenSemaphore));
    //NAME(m_readSemaphore, VK_OBJECT_TYPE_SEMAPHORE, "m_readSemaphore");
    //NAME(m_writtenSemaphore, VK_OBJECT_TYPE_SEMAPHORE, "m_writtenSemaphore");
    //NAME(m_queue, VK_OBJECT_TYPE_QUEUE, "m_queue");

    windowSize = swapchainExtent;
    // To destroy:  Complete and call function destroySwapchain
}

void VkApp::destroySwapchain()
{
    vkDeviceWaitIdle(m_device);

    // @@
    // Destroy all (3)  m_imageView'Ss with vkDestroyImageView(m_device, imageView, nullptr)
    for (size_t i = 0; i < m_imageViews.size(); i++)
    {
        vkDestroyImageView(m_device, m_imageViews[i], nullptr);
    }
    // Destroy the synchronization items:  // TODO: Do we actually need to do this??
    vkDestroyFence(m_device, m_waitFence, nullptr);
    vkDestroySemaphore(m_device, m_readSemaphore, nullptr);
    vkDestroySemaphore(m_device, m_writtenSemaphore, nullptr);

    // Destroy the actual swapchain with: vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

    m_swapchain = VK_NULL_HANDLE;
    m_imageViews.clear();
    m_barriers.clear();
}

bool VkApp::recreateSwapchain()
{
    vkDeviceWaitIdle(m_device);

    VkSwapchainKHR oldSwapchain = m_swapchain;

    // Get the surface's capabilities
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &capabilities);
   
    uint32_t cnt;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &cnt, nullptr);
    std::vector<VkPresentModeKHR> presentModes(cnt);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &cnt, presentModes.data());

    // Choose VK_PRESENT_MODE_FIFO_KHR as a default (this must be supported)
    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR; // Support is required by specification.
    for (size_t i = 0; i < presentModes.size(); i++)
    {
        if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            // @@ But choose VK_PRESENT_MODE_MAILBOX_KHR if it can be found in presentModes
            swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }

    // Get the list of VkFormat's that are supported:
    // @@ Document the list you get.
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &cnt, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(cnt);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &cnt, formats.data());

    VkFormat surfaceFormat       = VK_FORMAT_UNDEFINED;               // Temporary value.
    VkColorSpaceKHR surfaceColor = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR; // Temporary value
                                                                      // @@ Replace the above two temporary lines with the following two
                                                                      // to choose the first format and its color space as defaults:
                                                                      //  VkFormat surfaceFormat = formats[0].format;
                                                                      //  VkColorSpaceKHR surfaceColor  = formats[0].colorSpace;
    surfaceFormat = formats.front().format;
    surfaceColor = formats.front().colorSpace;
    for (size_t i = 0; i < formats.size(); i++)
    {
        if (formats[i].format = VK_FORMAT_B8G8R8A8_UNORM)
        {
            surfaceFormat = formats[i].format;
            surfaceColor = formats[i].colorSpace;
            break;
        }
    }
    // @@ Then search the formats (from several lines up) to choose
    // format VK_FORMAT_B8G8R8A8_UNORM (and its color space) if such
    // exists.  Document your list of formats/color-spaces, and your
    // particular choice.

    // Get the swap chain extent
    VkExtent2D swapchainExtent = capabilities.currentExtent;
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        swapchainExtent = capabilities.currentExtent; }
    else 
    {
        // Does this case ever happen?
        int width, height;
        glfwGetFramebufferSize(app->GLFW_window, &width, &height);

        swapchainExtent = VkExtent2D{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

        swapchainExtent.width = std::clamp(swapchainExtent.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width);
        swapchainExtent.height = std::clamp(swapchainExtent.height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height); 
    }

    // Test against valid size, typically hit when windows are minimized.
    // The app must prevent triggering this code in such a case
    assert(swapchainExtent.width && swapchainExtent.height);
    // @@ If this assert fires, we have some work to do to better deal
    // with the situation.

    // Choose the number of swap chain images, within the bounds supported.
    uint imageCount = capabilities.minImageCount + 1; // Recommendation: minImageCount+1
    if (capabilities.maxImageCount > 0
        && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount; }

    assert (imageCount == 3);
    // If this triggers, disable the assert, BUT help me understand
    // the situation that caused it.  

    // Create the swap chain
    VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
        | VK_IMAGE_USAGE_STORAGE_BIT
        | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    VkSwapchainCreateInfoKHR _i = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    _i.surface                  = m_surface;
    _i.minImageCount            = imageCount;
    _i.imageFormat              = surfaceFormat;
    _i.imageColorSpace          = surfaceColor;
    _i.imageExtent              = swapchainExtent;
    _i.imageUsage               = imageUsage;
    _i.preTransform             = capabilities.currentTransform;
    _i.compositeAlpha           = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    _i.imageArrayLayers         = 1;
    _i.imageSharingMode         = VK_SHARING_MODE_EXCLUSIVE;
    _i.queueFamilyIndexCount    = 1;
    _i.pQueueFamilyIndices      = &m_graphicsQueueIndex;
    _i.presentMode              = swapchainPresentMode;
    _i.oldSwapchain             = oldSwapchain;
    _i.clipped                  = true;

    VkSwapchainKHR newSwapchain;
    VK_CHK(vkCreateSwapchainKHR(m_device, &_i, nullptr, &newSwapchain));
    // @@ Verify VK_SUCCESS
    destroySwapchain();
    m_swapchain = newSwapchain;

    // Verify success
    VK_CHK(vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_imageCount, nullptr));
    m_swapchainImages.resize(m_imageCount);
    VK_CHK(vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_imageCount, m_swapchainImages.data())); 

    // Verify and document that you retrieved the correct number of images.
    std::cout << "Swapchain Images: [" << m_swapchainImages.size() << "]" << std::endl;

    m_barriers.resize(m_imageCount);
    m_imageViews.resize(m_imageCount);

    // Create an VkImageView for each swap chain image.
    for (uint i=0;  i<m_imageCount;  i++) 
    {
        VkImageViewCreateInfo createInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        createInfo.image = m_swapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = surfaceFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VK_CHK(vkCreateImageView(m_device, &createInfo, nullptr, &m_imageViews[i])); 
    }

    // Create three VkImageMemoryBarrier structures (one for each swap
    // chain image) and specify the desired
    // layout (VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) for each.
    for (uint i=0;  i<m_imageCount;  i++) 
    {
        VkImageSubresourceRange range = {0};
        range.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel            = 0;
        range.levelCount              = VK_REMAINING_MIP_LEVELS;
        range.baseArrayLayer          = 0;
        range.layerCount              = VK_REMAINING_ARRAY_LAYERS;

        VkImageMemoryBarrier memBarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        memBarrier.dstAccessMask        = 0;
        memBarrier.srcAccessMask        = 0;
        memBarrier.oldLayout            = VK_IMAGE_LAYOUT_UNDEFINED;
        memBarrier.newLayout            = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        memBarrier.image                = m_swapchainImages[i];
        memBarrier.subresourceRange     = range;
        m_barriers[i] = memBarrier;
    }

    // Create a temporary command buffer. submit the layout conversion
    // command, submit and destroy the command buffer.
    VkCommandBuffer cmd = createTempCmdBuffer();
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0,
        nullptr, m_imageCount, m_barriers.data());
    submitTempCmdBuffer(cmd);

    // Create the three synchronization objects.  These are not
    // technically part of the swap chain, but they are used
    // exclusively for synchronizing the swap chain, so I include them
    // here.
    VkFenceCreateInfo fenceCreateInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VK_CHK(vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_waitFence));

    VkSemaphoreCreateInfo semCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VK_CHK(vkCreateSemaphore(m_device, &semCreateInfo, nullptr, &m_readSemaphore));
    VK_CHK(vkCreateSemaphore(m_device, &semCreateInfo, nullptr, &m_writtenSemaphore));
    //NAME(m_readSemaphore, VK_OBJECT_TYPE_SEMAPHORE, "m_readSemaphore");
    //NAME(m_writtenSemaphore, VK_OBJECT_TYPE_SEMAPHORE, "m_writtenSemaphore");
    //NAME(m_queue, VK_OBJECT_TYPE_QUEUE, "m_queue");

    windowSize = swapchainExtent;
}


void VkApp::createDepthResource() 
{
    uint mipLevels = 1;

    // Note m_depthImage is type ImageWrap; a tiny wrapper around
    // several related Vulkan objects.
    m_depthImage = createImageWrap(windowSize.width, windowSize.height,
        VK_FORMAT_X8_D24_UNORM_PACK32,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        mipLevels);
    m_depthImage.imageView = createImageView(m_depthImage.image,
        VK_FORMAT_X8_D24_UNORM_PACK32,
        VK_IMAGE_ASPECT_DEPTH_BIT);
}

// Gets a list of memory types supported by the GPU, and search
// through that list for one that matches the requested properties
// flag.  The (only?) two types requested here are:
//
// (1) VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT: For the bulk of the memory
// used by the GPU to store things internally.
//
// (2) VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT:
// for memory visible to the CPU  for CPU to GPU copy operations.
uint32_t VkApp::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i))
            && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i; } }

    throw std::runtime_error("failed to find suitable memory type!");
}


// A factory function for an ImageWrap, this creates a VkImage and
// creates and binds an associated VkDeviceMemory object.  The
// VkImageView and VkSampler are left empty to be created elsewhere as
// needed.
ImageWrap VkApp::createImageWrap(uint32_t width, uint32_t height,
    VkFormat format,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties, uint mipLevels)
{
    ImageWrap myImage;

    VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_CHK(vkCreateImage(m_device, &imageInfo, nullptr, &myImage.image));

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device, myImage.image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    VK_CHK(vkAllocateMemory(m_device, &allocInfo, nullptr, &myImage.memory));

    VK_CHK(vkBindImageMemory(m_device, myImage.image, myImage.memory, 0));

    myImage.imageView = VK_NULL_HANDLE;
    myImage.sampler = VK_NULL_HANDLE;

    return myImage;
    // @@ Verify success for vkCreateImage, and vkAllocateMemory
}

VkImageView VkApp::createImageView(VkImage image, VkFormat format,
    VkImageAspectFlagBits aspect)
{
    VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspect;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    VK_CHK(vkCreateImageView(m_device, &viewInfo, nullptr, &imageView));
   

    return imageView;
}

void VkApp::createPostRenderPass()
{  
    std::array<VkAttachmentDescription, 2> attachments{};
    // Color attachment
    attachments[0].format      = VK_FORMAT_B8G8R8A8_UNORM;
    attachments[0].loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachments[0].samples     = VK_SAMPLE_COUNT_1_BIT;
#ifdef GUI
    //we change this here becasuse we will be using imgui as a subpass
    attachments[0].storeOp      = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
#endif // GUI

    // Depth attachment
    attachments[1].format        = VK_FORMAT_X8_D24_UNORM_PACK32;
    attachments[1].loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].finalLayout   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments[1].samples       = VK_SAMPLE_COUNT_1_BIT;

    const VkAttachmentReference colorReference{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    const VkAttachmentReference depthReference{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    std::array<VkSubpassDependency, 1> subpassDependencies{};
    // Transition from final to initial (VK_SUBPASS_EXTERNAL refers to all commands executed outside of the actual renderpass)
    subpassDependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
    subpassDependencies[0].dstSubpass      = 0;
    subpassDependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpassDependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[0].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
    subpassDependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
        | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkSubpassDescription subpassDescription{};
    subpassDescription.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount    = 1;
    subpassDescription.pColorAttachments       = &colorReference;
    subpassDescription.pDepthStencilAttachment = &depthReference;

    VkRenderPassCreateInfo renderPassInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments    = attachments.data();
    renderPassInfo.subpassCount    = 1;
    renderPassInfo.pSubpasses      = &subpassDescription;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
    renderPassInfo.pDependencies   = subpassDependencies.data();

    VK_CHK(vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_postRenderPass));
}

// A VkFrameBuffer wraps several images into a render target --
// usually a color buffer and a depth buffer.
void VkApp::createPostFrameBuffers()
{
    std::array<VkImageView, 2> fbattachments{};

    // Create frame buffers for every swap chain image
    VkFramebufferCreateInfo _ci{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    _ci.renderPass      = m_postRenderPass;
    _ci.width           = windowSize.width;
    _ci.height          = windowSize.height;
    _ci.layers          = 1;
    _ci.attachmentCount = 2;
    _ci.pAttachments    = fbattachments.data();

    // Each of the three swapchain images gets an associated frame
    // buffer, all sharing one depth buffer.
    m_framebuffers.resize(m_imageCount);
    for(uint32_t i = 0; i < m_imageCount; i++) 
    {
        fbattachments[0] = m_imageViews[i];         // A color attachment from the swap chain
        fbattachments[1] = m_depthImage.imageView;  // A depth attachment
        VK_CHK(vkCreateFramebuffer(m_device, &_ci, nullptr, &m_framebuffers[i])); 
    }
}


void VkApp::createPostPipeline()
{

    // Creating the pipeline layout
    VkPipelineLayoutCreateInfo createInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};

    // What we eventually want:
    //createInfo.setLayoutCount         = 1;
    //createInfo.pSetLayouts            = &m_scDesc.descSetLayout;
    // Push constants in the fragment shader
    //VkPushConstantRange pushConstantRanges = {VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float)};
    //createInfo.pushConstantRangeCount = 1;
    //createInfo.pPushConstantRanges    = &pushConstantRanges;

    // What we can do now as a first pass:
    createInfo.setLayoutCount         = 1;
    createInfo.pSetLayouts            = &m_postDesc.descSetLayout;
    createInfo.pushConstantRangeCount = 0;
    createInfo.pPushConstantRanges    = nullptr;

    vkCreatePipelineLayout(m_device, &createInfo, nullptr, &m_postPipelineLayout);

    ////////////////////////////////////////////
    // Create the shaders
    ////////////////////////////////////////////
    VkShaderModule vertShaderModule = createShaderModule(loadFile("spv/post.vert.spv"));
    VkShaderModule fragShaderModule = createShaderModule(loadFile("spv/post.frag.spv"));

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    //auto bindingDescription = Vertex::getBindingDescription();
    //auto attributeDescriptions = Vertex::getAttributeDescriptions();

    // No geometry in this pipeline's draw.
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;

    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) windowSize.width;
    viewport.height = (float) windowSize.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = VkExtent2D{windowSize.width, windowSize.height};

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE; //??
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;// BEWARE!!  NECESSARY!!
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = m_postPipelineLayout;
    pipelineInfo.renderPass = m_postRenderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    

    VK_CHK(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
        &m_postPipeline));

    // The pipeline has fully compiled copies of the shaders, so these
    // intermediate (SPV) versions can be destroyed.
    vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
    vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
    // Document the vkCreateGraphicsPipelines call with an api_dump.  
}

#ifdef GUI
// This function helps intialize a bunch of structures that ImGui uses for their rendering.
void VkApp::initGUI()
{
    VkAttachmentDescription attachment = {};
    attachment.format = m_surfaceFormat;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // Draw GUI on what exitst
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // make sure we set our preceeding renderpass to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL 
    // since this will be the final pass (before presentation) instead
    attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // final layout for presentation.

    VkAttachmentReference color_attachment = {};
    color_attachment.attachment = 0;
    color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;                             // create dependancy outside current renderpass
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // make sure pixels have been fully rendered before performing this pass
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // same thing
    dependency.srcAccessMask = 0;                                            // or VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = 1;
    info.pAttachments = &attachment;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    info.dependencyCount = 1;
    info.pDependencies = &dependency;

    // We have to set up a renderpass for imgui to use for their windows
    VK_CHK(vkCreateRenderPass(m_device, &info, nullptr, &m_imguiRenderPass));


    // ImGui uses these descriptors in their backend
    // Unsure if all are being used or its just for example.
    std::vector<VkDescriptorPoolSize> pool_sizes
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo dpci = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    dpci.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());// Amount of pool sizes being passed
    dpci.pPoolSizes = pool_sizes.data();// Pool sizes to create pool with
    dpci.maxSets = 1; // Maximum number of descriptor sets that can be created from pool
    VK_CHK(vkCreateDescriptorPool(m_device, &dpci, nullptr, &m_imguiDescPool));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    // Now we bind all the information from our renderer into the ImGui IO
    ImGui_ImplGlfw_InitForVulkan(app->GLFW_window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = m_instance;
    init_info.PhysicalDevice = m_physicalDevice;
    init_info.Device = m_device;
    init_info.QueueFamily = m_graphicsQueueIndex;
    init_info.Queue = m_queue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = m_imguiDescPool;
    init_info.Allocator = nullptr;
    init_info.MinImageCount = m_minImageCount;
    init_info.ImageCount = m_imageCount;
    init_info.CheckVkResultFn = nullptr; // we can put our own check function if any

    ImGui_ImplVulkan_Init(&init_info, m_imguiRenderPass);
    
    // This uploads the ImGUI font package to the GPU
    VkCommandBuffer command_buffer = beginSingleTimeCommands();
    ImGui_ImplVulkan_CreateFontsTexture(command_buffer);
    endSingleTimeCommands(command_buffer); 

    // Create frame buffers for every swap chain image
    // We need to do this because ImGUI only cares about the colour attachment.
    std::array<VkImageView, 2> fbattachments{};
    VkFramebufferCreateInfo _ci{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    _ci.renderPass      = m_imguiRenderPass;
    _ci.width           = windowSize.width;
    _ci.height          = windowSize.height;
    _ci.layers          = 1;
    _ci.attachmentCount = 1;
    _ci.pAttachments    = fbattachments.data();

    // Each of the three swapchain images gets an associated frame
    // buffer, all sharing one depth buffer.
    m_imguiBuffers.resize(m_imageCount);
    for(uint32_t i = 0; i < m_imageCount; i++) 
    {
        fbattachments[0] = m_imageViews[i];         // A color attachment from the swap chain
        //fbattachments[1] = m_depthImage.imageView;  // A depth attachment
        VK_CHK(vkCreateFramebuffer(m_device, &_ci, nullptr, &m_imguiBuffers[i])); 
    }
}

// We need to release all the resources we created to use ImGUI
void VkApp::destroyGUI()
{
    // Outside of here we called vkDeviceWaitIdle
    for(uint32_t i = 0; i < m_imageCount; i++) 
    {        
        vkDestroyFramebuffer(m_device,  m_imguiBuffers[i], nullptr); 
    }
    vkDestroyRenderPass(m_device, m_imguiRenderPass, nullptr);
    vkDestroyDescriptorPool(m_device, m_imguiDescPool, nullptr);
    ImGui_ImplVulkan_Shutdown();
}
#endif // GUI

// Immediate command sending helper
VkCommandBuffer VkApp::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_cmdPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

// Immediate command sending helper
void VkApp::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_queue);

    vkFreeCommandBuffers(m_device, m_cmdPool, 1, &commandBuffer);
}

std::string VkApp::loadFile(const std::string& filename)
{
    std::string   result;
    std::ifstream stream(filename, std::ios::ate | std::ios::binary);  //ate: Open at file end

    if(!stream.is_open())
        return result;

    result.reserve(stream.tellg()); // tellg() is last char position in file (i.e.,  length)
    stream.seekg(0, std::ios::beg);

    result.assign((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    return result;
}

//-------------------------------------------------------------------------------------------------
// Post processing pass: tone mapper, UI
void VkApp::postProcess()
{
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color        = {{1,1,1,1}};
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo _i{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    _i.clearValueCount = 2;
    _i.pClearValues    = clearValues.data();
    _i.renderPass      = m_postRenderPass;
    _i.framebuffer     = m_framebuffers[m_swapchainIndex];
    _i.renderArea      = {{0, 0}, windowSize};

    vkCmdBeginRenderPass(m_commandBuffer, &_i, VK_SUBPASS_CONTENTS_INLINE);
    {   // extra indent for renderpass commands
        //VkViewport viewport{0.0f, 0.0f,
        //    static_cast<float>(windowSize.width), static_cast<float>(windowSize.height),
        //    0.0f, 1.0f};
        //vkCmdSetViewport(m_commandBuffer, 0, 1, &viewport);
        //
        //VkRect2D scissor{{0, 0}, {windowSize.width, windowSize.height}};
        //vkCmdSetScissor(m_commandBuffer, 0, 1, &scissor);

        auto aspectRatio = static_cast<float>(windowSize.width)
            / static_cast<float>(windowSize.height);
        //vkCmdPushConstants(m_commandBuffer, m_postPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
        //                   sizeof(float), &aspectRatio);
        vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_postPipeline);
        
        vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                               m_postPipelineLayout, 0, 1, &m_postDesc.descSet, 0, nullptr);

        // Weird! This draws 3 vertices but with no vertices/triangles buffers bound in.
        // Hint: The vertex shader fabricates vertices from gl_VertexIndex
        vkCmdDraw(m_commandBuffer, 3, 1, 0, 0);

    }
    vkCmdEndRenderPass(m_commandBuffer);
#ifdef GUI
    {
    VkRenderPassBeginInfo GUIpassInfo = {};
    GUIpassInfo.sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    GUIpassInfo.renderPass  = m_imguiRenderPass;
    GUIpassInfo.framebuffer = m_imguiBuffers[m_swapchainIndex];
    GUIpassInfo.renderArea  = {{0, 0}, windowSize};
    vkCmdBeginRenderPass(m_commandBuffer, &GUIpassInfo, VK_SUBPASS_CONTENTS_INLINE);
    ImGui::Render();  // Rendering UI
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_commandBuffer);
    vkCmdEndRenderPass(m_commandBuffer);
    }
#endif

}

// That's all for now!
// Many more procedures will follow ...
