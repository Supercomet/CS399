
#include <array>
#include <iostream>     // std::cout
#include <fstream>      // std::ifstream
#include <exception> // runtime_error

#include <unordered_set>
#include <unordered_map>

#include "vkapp.h"
#include "app.h"
#include "extensions_vk.hpp"


void VkApp::destroyAllVulkanResources()
{
    // @@
    // vkDeviceWaitIdle(m_device);  // Uncomment this when you have an m_device created.

    // Destroy all vulkan objects.
    // ...  All objects created on m_device must be destroyed before m_device.
    //vkDestroyDevice(m_device, nullptr);
    //vkDestroyInstance(m_instance, nullptr);
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
    printf("GLFW required extensions:\n");
    for (uint32_t x = 0; x < countGLFWextensions; ++x)
    {
        printf("%s\n",reqGLFWextensions[x]);
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
        std::cout << availableLayers[x].layerName << std::endl;
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
        std::cout << availableExtensions[x].extensionName << std::endl;
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

    vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance);

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
    for (auto physicalDevice : physicalDevices) {

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
                if (strcmp(extensionProperties[j].extensionName, reqDeviceExtensions[i]) == 0)
                    exists = true;
            }
            if (exists == false)
            {
                // we failed to find a suitable extension
                deviceOK = false;
                std::cout << "Device ["<< GPUproperties.deviceName <<"] missing required Ext " << "[" << reqDeviceExtensions[i] << "]" << std::endl;
                break;
            }
        }
        if (deviceOK)
        {
            compatibleDevices.push_back(i);
        }
        ++i;

        //    GPUproperties.deviceType==VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
        
        // and
        
        //    All reqDeviceExtensions can be found in the GPUs extensionProperties list
        //      That is: for all i, there exists a j such that:
        //                 reqDeviceExtensions[i] == extensionProperties[j].extensionName

        //  If a GPU is found to be compatible
        //  Return it (physicalDevice), or continue the search and then return the best GPU.
        //    raise an exception of none were found
        //    tell me all about your system if more than one was found.
    }

    if (compatibleDevices.empty())
    {
        throw std::runtime_error("Could not find suitable GPU");
    }

    // we take first one for now. maybe find a better way to select GPU
    m_physicalDevice = physicalDevices[compatibleDevices.front()];
    {
        VkPhysicalDeviceProperties GPUproperties;
        vkGetPhysicalDeviceProperties(m_physicalDevice, &GPUproperties);
        std::cout << "Selected GPU : [" << GPUproperties.deviceName << "]" << std::endl;
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

    //@@ Search the list for (the index of) the first queue family that has the required flags.
    // Verity that your search choose the correct queue family.
    // Record the index in m_graphicsQueueIndex.
    // Nothing to destroy as m_graphicsQueueIndex is just an integer.
    //m_graphicsQueueIndex = you chosen index;
}


void VkApp::createDevice()
{
    // @@
    // Build a pNext chain of the following six "feature" structures:
    //   features2->features11->features12->features13->accelFeature->rtPipelineFeature->NULL

    // Hint: Keep it simple; add a second parameter (the pNext pointer) to each
    // structure point up to the previous structure.
    
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

    vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device);
    // @@
    // Verify VK_SUCCESS
    // To destroy: vkDestroyDevice(m_device, nullptr);
}

// That's all for now!
// Many more procedures will follow ...
