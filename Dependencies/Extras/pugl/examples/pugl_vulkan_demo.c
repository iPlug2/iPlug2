// Copyright 2019-2023 David Robillard <d@drobilla.net>
// Copyright 2019 Jordan Halase <jordan@halase.me>
// SPDX-License-Identifier: ISC

/*
  A simple example of drawing with Vulkan.

  For a more advanced demo that actually draws something interesting, see
  pugl_vulkan_cpp_demo.cpp.
*/

#include "demo_utils.h"
#include "test/test_utils.h"

#include "pugl/pugl.h"
#include "pugl/vulkan.h"

#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CLAMP(x, l, h) ((x) <= (l) ? (l) : (x) >= (h) ? (h) : (x))

// Vulkan allocation callbacks which can be used for debugging
#define ALLOC_VK NULL

// Helper macro for allocating arrays by type, with C++ compatible cast
#define AALLOC(size, Type) ((Type*)calloc(size, sizeof(Type)))

// Helper macro for counted array arguments to make clang-format behave
#define COUNTED(count, ...) count, __VA_ARGS__

/// Dynamically loaded Vulkan API functions
typedef struct {
  PFN_vkCreateDebugReportCallbackEXT  vkCreateDebugReportCallbackEXT;
  PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT;
} InstanceAPI;

/// Vulkan swapchain and everything that depends on it
typedef struct {
  VkSwapchainKHR   rawSwapchain;
  uint32_t         nImages;
  VkExtent2D       extent;
  VkImage*         images;
  VkImageView*     imageViews;
  VkFence*         fences;
  VkCommandBuffer* commandBuffers;
} Swapchain;

/// Synchronization semaphores
typedef struct {
  VkSemaphore presentComplete;
  VkSemaphore renderFinished;
} Sync;

/// Vulkan state, purely Vulkan functions can depend on only this
typedef struct {
  InstanceAPI                api;
  VkInstance                 instance;
  VkDebugReportCallbackEXT   debugCallback;
  VkSurfaceKHR               surface;
  VkSurfaceFormatKHR         surfaceFormat;
  VkPresentModeKHR           presentMode;
  VkPhysicalDeviceProperties deviceProperties;
  VkPhysicalDevice           physicalDevice;
  uint32_t                   graphicsIndex;
  VkDevice                   device;
  VkQueue                    graphicsQueue;
  VkCommandPool              commandPool;
  Swapchain*                 swapchain;
  Sync                       sync;
} VulkanState;

/// Complete application
typedef struct {
  PuglTestOptions opts;
  PuglWorld*      world;
  PuglView*       view;
  VulkanState     vk;
  uint32_t        framesDrawn;
  uint32_t        width;
  uint32_t        height;
  bool            quit;
} VulkanApp;

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugReportFlagsEXT      flags,
              VkDebugReportObjectTypeEXT objType,
              uint64_t                   obj,
              size_t                     location,
              int32_t                    code,
              const char*                layerPrefix,
              const char*                msg,
              void*                      userData)
{
  (void)userData;
  (void)objType;
  (void)obj;
  (void)location;
  (void)code;

  if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
    fprintf(stderr, "note: ");
  } else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
    fprintf(stderr, "warning: ");
  } else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
    fprintf(stderr, "performance warning: ");
  } else if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
    fprintf(stderr, "error: ");
  } else if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
    fprintf(stderr, "debug: ");
  }

  fprintf(stderr, "%s: ", layerPrefix);
  fprintf(stderr, "%s\n", msg);
  return VK_FALSE;
}

static bool
hasExtension(const char* const                  name,
             const VkExtensionProperties* const properties,
             const uint32_t                     count)
{
  for (uint32_t i = 0; i < count; ++i) {
    if (!strcmp(properties[i].extensionName, name)) {
      return true;
    }
  }

  return false;
}

static bool
hasLayer(const char* const              name,
         const VkLayerProperties* const properties,
         const uint32_t                 count)
{
  for (uint32_t i = 0; i < count; ++i) {
    if (!strcmp(properties[i].layerName, name)) {
      return true;
    }
  }

  return false;
}

static void
pushString(const char*** const array,
           uint32_t* const     count,
           const char* const   string)
{
  *array = (const char**)realloc(*array, (*count + 1) * sizeof(const char*));
  (*array)[*count] = string;
  ++*count;
}

static VkResult
createInstance(VulkanApp* const app)
{
  const VkApplicationInfo appInfo = {
    VK_STRUCTURE_TYPE_APPLICATION_INFO,
    NULL,
    "Pugl Vulkan Test",
    VK_MAKE_VERSION(0, 1, 0),
    "Pugl Vulkan Test Engine",
    VK_MAKE_VERSION(0, 1, 0),
    VK_MAKE_VERSION(1, 0, 0),
  };

  // Get the number of supported extensions and layers
  VkResult vr          = VK_SUCCESS;
  uint32_t nExtProps   = 0;
  uint32_t nLayerProps = 0;
  if ((vr = vkEnumerateInstanceLayerProperties(&nLayerProps, NULL)) ||
      (vr = vkEnumerateInstanceExtensionProperties(NULL, &nExtProps, NULL))) {
    return vr;
  }

  // Get properties of supported extensions
  VkExtensionProperties* extProps = AALLOC(nExtProps, VkExtensionProperties);
  vkEnumerateInstanceExtensionProperties(NULL, &nExtProps, extProps);

  uint32_t     nExtensions = 0;
  const char** extensions  = NULL;

  // Add extensions required by pugl
  uint32_t           nPuglExts = 0;
  const char* const* puglExts  = puglGetInstanceExtensions(&nPuglExts);
  for (uint32_t i = 0; i < nPuglExts; ++i) {
    pushString(&extensions, &nExtensions, puglExts[i]);
  }

  // Add extra extensions we want to use if they are supported
  if (hasExtension("VK_EXT_debug_report", extProps, nExtProps)) {
    pushString(&extensions, &nExtensions, "VK_EXT_debug_report");
  }

  // Get properties of supported layers
  VkLayerProperties* layerProps = AALLOC(nLayerProps, VkLayerProperties);
  vkEnumerateInstanceLayerProperties(&nLayerProps, layerProps);

  // Add validation layers if error checking is enabled
  uint32_t     nLayers = 0;
  const char** layers  = NULL;
  if (app->opts.errorChecking) {
    const char* debugLayers[] = {"VK_LAYER_KHRONOS_validation",
                                 "VK_LAYER_LUNARG_standard_validation",
                                 NULL};

    for (const char** l = debugLayers; *l; ++l) {
      if (hasLayer(*l, layerProps, nLayerProps)) {
        pushString(&layers, &nLayers, *l);
      }
    }
  }

  for (uint32_t i = 0; i < nExtensions; ++i) {
    printf("Using instance extension:    %s\n", extensions[i]);
  }

  for (uint32_t i = 0; i < nLayers; ++i) {
    printf("Using instance layer:        %s\n", layers[i]);
  }

  const VkInstanceCreateInfo createInfo = {
    VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    NULL,
    0,
    &appInfo,
    COUNTED(nLayers, layers),
    COUNTED(nExtensions, extensions),
  };

  if ((vr = vkCreateInstance(&createInfo, ALLOC_VK, &app->vk.instance))) {
    logError("Could not create Vulkan Instance: %d\n", vr);
  }

  free(layers);
  free(extensions);
  free(layerProps);
  free(extProps);

  return vr;
}

static VkResult
enableDebugging(VulkanState* const vk)
{
  vk->api.vkCreateDebugReportCallbackEXT =
    (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
      vk->instance, "vkCreateDebugReportCallbackEXT");

  vk->api.vkDestroyDebugReportCallbackEXT =
    (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(
      vk->instance, "vkDestroyDebugReportCallbackEXT");

  if (vk->api.vkCreateDebugReportCallbackEXT) {
    const VkDebugReportCallbackCreateInfoEXT info = {
      VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
      NULL,
      VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT,
      debugCallback,
      NULL,
    };

    VkResult vr = VK_SUCCESS;
    if ((vr = vk->api.vkCreateDebugReportCallbackEXT(
           vk->instance, &info, ALLOC_VK, &vk->debugCallback))) {
      logError("Could not create debug reporter: %d\n", vr);
      return vr;
    }
  }

  return VK_SUCCESS;
}

static VkResult
getGraphicsQueueIndex(VkSurfaceKHR     surface,
                      VkPhysicalDevice device,
                      uint32_t*        graphicsIndex)
{
  VkResult r = VK_SUCCESS;

  uint32_t nProps = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &nProps, NULL);

  VkQueueFamilyProperties* props = AALLOC(nProps, VkQueueFamilyProperties);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &nProps, props);

  for (uint32_t q = 0; q < nProps; ++q) {
    if (props[q].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      VkBool32 supported = false;
      if ((r = vkGetPhysicalDeviceSurfaceSupportKHR(
             device, q, surface, &supported))) {
        free(props);
        return r;
      }

      if (supported) {
        *graphicsIndex = q;
        free(props);
        return VK_SUCCESS;
      }
    }
  }

  free(props);
  return VK_ERROR_FEATURE_NOT_PRESENT;
}

static bool
supportsRequiredExtensions(const VkPhysicalDevice device)
{
  uint32_t nExtProps = 0;
  vkEnumerateDeviceExtensionProperties(device, NULL, &nExtProps, NULL);

  VkExtensionProperties* extProps = AALLOC(nExtProps, VkExtensionProperties);
  vkEnumerateDeviceExtensionProperties(device, NULL, &nExtProps, extProps);

  for (uint32_t i = 0; i < nExtProps; ++i) {
    if (!strcmp(extProps[i].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
      free(extProps);
      return true;
    }
  }

  free(extProps);
  return false;
}

static bool
isDeviceSuitable(const VulkanState* const vk,
                 const VkPhysicalDevice   device,
                 uint32_t* const          graphicsIndex)
{
  if (!supportsRequiredExtensions(device) ||
      getGraphicsQueueIndex(vk->surface, device, graphicsIndex)) {
    return false;
  }

  return true;
}

/**
   Selects a physical graphics device.

   This doesn't try to be clever, and just selects the first suitable device.
*/
static VkResult
selectPhysicalDevice(VulkanState* const vk)
{
  VkResult vr = VK_SUCCESS;
  if (!vk->surface) {
    logError("Cannot select a physical device without a surface\n");
    return VK_ERROR_SURFACE_LOST_KHR;
  }

  uint32_t nDevices = 0;
  if ((vr = vkEnumeratePhysicalDevices(vk->instance, &nDevices, NULL))) {
    logError("Failed to get count of physical devices: %d\n", vr);
    return vr;
  }

  if (!nDevices) {
    logError("No physical devices found\n");
    return VK_ERROR_DEVICE_LOST;
  }

  VkPhysicalDevice* devices = AALLOC(nDevices, VkPhysicalDevice);
  if ((vr = vkEnumeratePhysicalDevices(vk->instance, &nDevices, devices))) {
    logError("Failed to enumerate physical devices: %d\n", vr);
    free(devices);
    return vr;
  }

  uint32_t i = 0;
  for (i = 0; i < nDevices; ++i) {
    VkPhysicalDeviceProperties deviceProps = {0};
    vkGetPhysicalDeviceProperties(devices[i], &deviceProps);

    uint32_t graphicsIndex = 0;
    if (isDeviceSuitable(vk, devices[i], &graphicsIndex)) {
      printf("Using device %u/%u:            \"%s\"\n",
             i + 1,
             nDevices,
             deviceProps.deviceName);
      vk->deviceProperties = deviceProps;
      vk->physicalDevice   = devices[i];
      vk->graphicsIndex    = graphicsIndex;
      printf("Using graphics queue family: %u\n", vk->graphicsIndex);
      break;
    }

    printf("Device \"%s\" not suitable\n", deviceProps.deviceName);
  }

  if (i >= nDevices) {
    logError("No suitable devices found\n");
    vr = VK_ERROR_DEVICE_LOST;
  }

  free(devices);
  return vr;
}

/// Opens the logical device and sets up the queue and command pool
static VkResult
openDevice(VulkanState* const vk)
{
  if (vk->device) {
    logError("Renderer already has an opened device\n");
    return VK_NOT_READY;
  }

  const float       graphicsQueuePriority = 1.0f;
  const char* const swapchainName         = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

  const VkDeviceQueueCreateInfo queueCreateInfo = {
    VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    NULL,
    0,
    vk->graphicsIndex,
    COUNTED(1, &graphicsQueuePriority),
  };

  const VkDeviceCreateInfo createInfo = {
    VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    NULL,
    0,
    COUNTED(1, &queueCreateInfo),
    COUNTED(0, NULL),
    COUNTED(1, &swapchainName),
    NULL,
  };

  VkDevice device = NULL;
  VkResult vr     = VK_SUCCESS;
  if ((vr =
         vkCreateDevice(vk->physicalDevice, &createInfo, ALLOC_VK, &device))) {
    logError("Could not open device \"%s\": %d\n",
             vk->deviceProperties.deviceName,
             vr);
    return vr;
  }

  vk->device = device;
  vkGetDeviceQueue(vk->device, vk->graphicsIndex, 0, &vk->graphicsQueue);

  const VkCommandPoolCreateInfo commandInfo = {
    VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    NULL,
    0,
    vk->graphicsIndex,
  };

  if ((vr = vkCreateCommandPool(
         vk->device, &commandInfo, ALLOC_VK, &vk->commandPool))) {
    logError("Could not create command pool: %d\n", vr);
    return vr;
  }

  return VK_SUCCESS;
}

static const char*
presentModeString(const VkPresentModeKHR presentMode)
{
  switch (presentMode) {
  case VK_PRESENT_MODE_IMMEDIATE_KHR:
    return "Immediate";
  case VK_PRESENT_MODE_MAILBOX_KHR:
    return "Mailbox";
  case VK_PRESENT_MODE_FIFO_KHR:
    return "FIFO";
  case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
    return "FIFO relaxed";
  default:
    return "Other";
  }
}

static bool
hasPresentMode(const VkPresentModeKHR        mode,
               const VkPresentModeKHR* const presentModes,
               const uint32_t                nPresentModes)
{
  for (uint32_t i = 0; i < nPresentModes; ++i) {
    if (presentModes[i] == mode) {
      return true;
    }
  }

  return false;
}

/// Configure the surface for the currently opened device
static VkResult
configureSurface(VulkanState* const vk)
{
  uint32_t nFormats = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(
    vk->physicalDevice, vk->surface, &nFormats, NULL);
  if (!nFormats) {
    logError("No surface formats available\n");
    return VK_ERROR_FORMAT_NOT_SUPPORTED;
  }

  VkSurfaceFormatKHR* surfaceFormats = AALLOC(nFormats, VkSurfaceFormatKHR);
  vkGetPhysicalDeviceSurfaceFormatsKHR(
    vk->physicalDevice, vk->surface, &nFormats, surfaceFormats);

  const VkSurfaceFormatKHR want = {VK_FORMAT_B8G8R8A8_UNORM,
                                   VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

  uint32_t formatIndex = 0;
  for (formatIndex = 0; formatIndex < nFormats; ++formatIndex) {
    if (surfaceFormats[formatIndex].format == want.format &&
        surfaceFormats[formatIndex].colorSpace == want.colorSpace) {
      vk->surfaceFormat = want;
      break;
    }
  }
  free(surfaceFormats);
  if (formatIndex >= nFormats) {
    logError("Could not find a suitable surface format\n");
    return VK_ERROR_FORMAT_NOT_SUPPORTED;
  }

  uint32_t nPresentModes = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(
    vk->physicalDevice, vk->surface, &nPresentModes, NULL);
  if (!nPresentModes) {
    logError("No present modes available\n");
    return VK_ERROR_FORMAT_NOT_SUPPORTED;
  }

  VkPresentModeKHR* presentModes = AALLOC(nPresentModes, VkPresentModeKHR);
  vkGetPhysicalDeviceSurfacePresentModesKHR(
    vk->physicalDevice, vk->surface, &nPresentModes, presentModes);

  const VkPresentModeKHR tryModes[] = {
    VK_PRESENT_MODE_MAILBOX_KHR,
    VK_PRESENT_MODE_FIFO_RELAXED_KHR,
    VK_PRESENT_MODE_FIFO_KHR,
    VK_PRESENT_MODE_IMMEDIATE_KHR,
  };

  VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
  for (uint32_t i = 0; i < sizeof(tryModes) / sizeof(VkPresentModeKHR); ++i) {
    if (hasPresentMode(tryModes[i], presentModes, nPresentModes)) {
      presentMode = tryModes[i];
      break;
    }
  }

  free(presentModes);
  vk->presentMode = presentMode;
  printf("Using present mode:          \"%s\" (%u)\n",
         presentModeString(presentMode),
         presentMode);

  return VK_SUCCESS;
}

static VkResult
createRawSwapchain(VulkanState* const vk,
                   const uint32_t     width,
                   const uint32_t     height)
{
  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  VkResult                 vr = VK_SUCCESS;
  if ((vr = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
         vk->physicalDevice, vk->surface, &surfaceCapabilities))) {
    logError("Could not get surface capabilities: %d\n", vr);
    return vr;
  }

  /* There is a known race condition with window/surface sizes, so we clamp
     to what Vulkan reports and hope for the best. */

  vk->swapchain->extent.width = CLAMP(width,
                                      surfaceCapabilities.minImageExtent.width,
                                      surfaceCapabilities.maxImageExtent.width);

  vk->swapchain->extent.height =
    CLAMP(height,
          surfaceCapabilities.minImageExtent.height,
          surfaceCapabilities.maxImageExtent.height);

  vk->swapchain->nImages = surfaceCapabilities.minImageCount;

  const VkSwapchainCreateInfoKHR createInfo = {
    VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    NULL,
    0,
    vk->surface,
    vk->swapchain->nImages,
    vk->surfaceFormat.format,
    VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
    vk->swapchain->extent,
    1,
    (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT),
    VK_SHARING_MODE_EXCLUSIVE,
    COUNTED(0, NULL),
    surfaceCapabilities.currentTransform,
    VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    vk->presentMode,
    VK_TRUE,
    0,
  };

  if ((vr = vkCreateSwapchainKHR(
         vk->device, &createInfo, ALLOC_VK, &vk->swapchain->rawSwapchain))) {
    logError("Could not create swapchain: %d\n", vr);
    return vr;
  }

  return VK_SUCCESS;
}

static VkResult
recordCommandBuffers(VulkanState* const vk)
{
  const VkClearColorValue clearValue = {{
    0xA4 / (float)0x100, // R
    0x1E / (float)0x100, // G
    0x22 / (float)0x100, // B
    0xFF / (float)0x100, // A
  }};

  const VkImageSubresourceRange range = {
    VK_IMAGE_ASPECT_COLOR_BIT,
    0,
    1,
    0,
    1,
  };

  const VkCommandBufferBeginInfo beginInfo = {
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    NULL,
    VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
    NULL,
  };

  for (uint32_t i = 0; i < vk->swapchain->nImages; ++i) {
    const VkImageMemoryBarrier toClearBarrier = {
      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      NULL,
      VK_ACCESS_MEMORY_READ_BIT,
      VK_ACCESS_TRANSFER_WRITE_BIT,
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      vk->graphicsIndex,
      vk->graphicsIndex,
      vk->swapchain->images[i],
      range,
    };

    const VkImageMemoryBarrier toPresentBarrier = {
      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      NULL,
      VK_ACCESS_TRANSFER_WRITE_BIT,
      VK_ACCESS_MEMORY_READ_BIT,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      vk->graphicsIndex,
      vk->graphicsIndex,
      vk->swapchain->images[i],
      range,
    };

    vkBeginCommandBuffer(vk->swapchain->commandBuffers[i], &beginInfo);

    vkCmdPipelineBarrier(vk->swapchain->commandBuffers[i],
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0,
                         COUNTED(0, NULL),
                         COUNTED(0, NULL),
                         COUNTED(1, &toClearBarrier));

    vkCmdClearColorImage(vk->swapchain->commandBuffers[i],
                         vk->swapchain->images[i],
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         &clearValue,
                         COUNTED(1, &range));

    vkCmdPipelineBarrier(vk->swapchain->commandBuffers[i],
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         0,
                         COUNTED(0, NULL),
                         COUNTED(0, NULL),
                         COUNTED(1, &toPresentBarrier));

    vkEndCommandBuffer(vk->swapchain->commandBuffers[i]);
  }

  return VK_SUCCESS;
}

static VkResult
createSwapchain(VulkanState* const vk,
                const uint32_t     width,
                const uint32_t     height)
{
  VkResult vr = VK_SUCCESS;

  vk->swapchain = AALLOC(1, Swapchain);
  if ((vr = createRawSwapchain(vk, width, height))) {
    return vr;
  }

  if ((vr = vkGetSwapchainImagesKHR(vk->device,
                                    vk->swapchain->rawSwapchain,
                                    &vk->swapchain->nImages,
                                    NULL))) {
    logError("Failed to query swapchain images: %d\n", vr);
    return vr;
  }

  vk->swapchain->images = AALLOC(vk->swapchain->nImages, VkImage);
  if ((vr = vkGetSwapchainImagesKHR(vk->device,
                                    vk->swapchain->rawSwapchain,
                                    &vk->swapchain->nImages,
                                    vk->swapchain->images))) {
    logError("Failed to get swapchain images: %d\n", vr);
    return vr;
  }

  const VkCommandBufferAllocateInfo allocInfo = {
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    NULL,
    vk->commandPool,
    VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    vk->swapchain->nImages,
  };

  vk->swapchain->commandBuffers =
    AALLOC(vk->swapchain->nImages, VkCommandBuffer);

  if ((vr = vkAllocateCommandBuffers(
         vk->device, &allocInfo, vk->swapchain->commandBuffers))) {
    logError("Could not allocate command buffers: %d\n", vr);
    return vr;
  }

  const VkFenceCreateInfo fenceInfo = {
    VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    NULL,
    VK_FENCE_CREATE_SIGNALED_BIT,
  };
  vk->swapchain->fences = AALLOC(vk->swapchain->nImages, VkFence);

  for (uint32_t i = 0; i < vk->swapchain->nImages; ++i) {
    if ((vr = vkCreateFence(
           vk->device, &fenceInfo, ALLOC_VK, &vk->swapchain->fences[i]))) {
      logError("Could not create render finished fence: %d\n", vr);
      return vr;
    }
  }

  if ((vr = recordCommandBuffers(vk))) {
    logError("Failed to record command buffers\n");
    return vr;
  }

  return VK_SUCCESS;
}

static void
destroySwapchain(VulkanState* const vk, Swapchain* const swapchain)
{
  if (!swapchain) {
    return;
  }

  for (uint32_t i = 0; i < swapchain->nImages; ++i) {
    if (swapchain->fences[i]) {
      vkDestroyFence(vk->device, swapchain->fences[i], ALLOC_VK);
    }

    if (swapchain->imageViews && swapchain->imageViews[i]) {
      vkDestroyImageView(vk->device, swapchain->imageViews[i], ALLOC_VK);
    }
  }

  free(swapchain->fences);
  swapchain->fences = NULL;
  free(swapchain->imageViews);
  swapchain->imageViews = NULL;

  if (swapchain->images) {
    free(swapchain->images);
    swapchain->images = NULL;
  }

  if (swapchain->commandBuffers) {
    vkFreeCommandBuffers(vk->device,
                         vk->commandPool,
                         swapchain->nImages,
                         swapchain->commandBuffers);
    free(swapchain->commandBuffers);
  }

  if (swapchain->rawSwapchain) {
    vkDestroySwapchainKHR(vk->device, swapchain->rawSwapchain, ALLOC_VK);
  }

  free(swapchain);
}

static VkResult
createSyncObjects(VulkanState* const vk)
{
  const VkSemaphoreCreateInfo info = {
    VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    NULL,
    0,
  };

  vkCreateSemaphore(vk->device, &info, ALLOC_VK, &vk->sync.presentComplete);
  vkCreateSemaphore(vk->device, &info, ALLOC_VK, &vk->sync.renderFinished);
  return VK_SUCCESS;
}

static void
destroySyncObjects(VulkanState* const vk)
{
  if (vk->sync.renderFinished) {
    vkDestroySemaphore(vk->device, vk->sync.renderFinished, ALLOC_VK);
    vk->sync.renderFinished = VK_NULL_HANDLE;
  }
  if (vk->sync.presentComplete) {
    vkDestroySemaphore(vk->device, vk->sync.presentComplete, ALLOC_VK);
    vk->sync.presentComplete = VK_NULL_HANDLE;
  }
}

static void
closeDevice(VulkanState* const vk)
{
  if (vk->device) {
    vkDeviceWaitIdle(vk->device);
    destroySyncObjects(vk);
    destroySwapchain(vk, vk->swapchain);
    if (vk->commandPool) {
      vkDestroyCommandPool(vk->device, vk->commandPool, ALLOC_VK);
      vk->commandPool = VK_NULL_HANDLE;
    }
    vk->graphicsQueue = VK_NULL_HANDLE;
    vkDestroyDevice(vk->device, ALLOC_VK);
    vk->device = VK_NULL_HANDLE;
  }
}

static void
destroyWorld(VulkanApp* const app)
{
  VulkanState* vk = &app->vk;

  if (vk) {
    closeDevice(vk);

    if (app->view) {
      puglHide(app->view);
      puglFreeView(app->view);
      app->view = NULL;
    }
    if (vk->debugCallback && vk->api.vkDestroyDebugReportCallbackEXT) {
      vk->api.vkDestroyDebugReportCallbackEXT(
        vk->instance, vk->debugCallback, ALLOC_VK);
      vk->debugCallback = VK_NULL_HANDLE;
    }
    if (vk->surface) {
      vkDestroySurfaceKHR(vk->instance, vk->surface, ALLOC_VK);
      vk->surface = VK_NULL_HANDLE;
    }
    if (vk->instance) {
      fflush(stderr);
      vkDestroyInstance(vk->instance, ALLOC_VK);
      vk->instance = VK_NULL_HANDLE;
    }
    if (app->world) {
      puglFreeWorld(app->world);
      app->world = NULL;
    }
  }
}

static PuglStatus
onConfigure(PuglView* const view, const double width, const double height)
{
  VulkanApp* const app = (VulkanApp*)puglGetHandle(view);

  // We just record the size here and lazily resize the surface when exposed
  app->width  = (uint32_t)width;
  app->height = (uint32_t)height;

  return PUGL_SUCCESS;
}

static PuglStatus
recreateSwapchain(VulkanState* const vk,
                  const uint32_t     width,
                  const uint32_t     height)
{
  vkDeviceWaitIdle(vk->device);
  destroySwapchain(vk, vk->swapchain);

  if (createSwapchain(vk, width, height)) {
    logError("Failed to recreate swapchain\n");
    return PUGL_UNKNOWN_ERROR;
  }

  return PUGL_SUCCESS;
}

static PuglStatus
onExpose(PuglView* const view)
{
  VulkanApp*   app        = (VulkanApp*)puglGetHandle(view);
  VulkanState* vk         = &app->vk;
  uint32_t     imageIndex = 0;
  VkResult     result     = VK_SUCCESS;

  // Recreate swapchain if the window size has changed
  const Swapchain* swapchain = vk->swapchain;
  if (swapchain->extent.width != app->width ||
      swapchain->extent.height != app->height) {
    recreateSwapchain(vk, app->width, app->height);
  }

  // Acquire the next image to render, rebuilding if necessary
  while ((result = vkAcquireNextImageKHR(vk->device,
                                         vk->swapchain->rawSwapchain,
                                         UINT64_MAX,
                                         vk->sync.presentComplete,
                                         VK_NULL_HANDLE,
                                         &imageIndex))) {
    switch (result) {
    case VK_SUCCESS:
      break;
    case VK_SUBOPTIMAL_KHR:
    case VK_ERROR_OUT_OF_DATE_KHR:
      recreateSwapchain(vk, app->width, app->height);
      continue;
    default:
      logError("Could not acquire swapchain image: %d\n", result);
      return PUGL_UNKNOWN_ERROR;
    }
  }

  // Wait until we can start rendering this frame
  vkWaitForFences(vk->device,
                  COUNTED(1, &vk->swapchain->fences[imageIndex]),
                  VK_TRUE,
                  UINT64_MAX);
  vkResetFences(vk->device, 1, &vk->swapchain->fences[imageIndex]);

  const VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

  // Submit command buffer to render this frame
  const VkSubmitInfo submitInfo = {
    VK_STRUCTURE_TYPE_SUBMIT_INFO,
    NULL,
    COUNTED(1, &vk->sync.presentComplete),
    &waitStage,
    COUNTED(1, &vk->swapchain->commandBuffers[imageIndex]),
    COUNTED(1, &vk->sync.renderFinished)};
  if ((result = vkQueueSubmit(vk->graphicsQueue,
                              1,
                              &submitInfo,
                              vk->swapchain->fences[imageIndex]))) {
    logError("Could not submit to queue: %d\n", result);
    return PUGL_FAILURE;
  }

  // Present this frame
  const VkPresentInfoKHR presentInfo = {
    VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    NULL,
    COUNTED(1, &vk->sync.renderFinished),
    COUNTED(1, &vk->swapchain->rawSwapchain, &imageIndex, NULL),
  };
  if ((result = vkQueuePresentKHR(vk->graphicsQueue, &presentInfo))) {
    logError("Could not present image: %d\n", result);
  }

  if (app->opts.continuous) {
    ++app->framesDrawn;
  }

  return PUGL_SUCCESS;
}

static PuglStatus
onEvent(PuglView* const view, const PuglEvent* const e)
{
  VulkanApp* const app = (VulkanApp*)puglGetHandle(view);

  printEvent(e, "Event: ", app->opts.verbose);

  switch (e->type) {
  case PUGL_UPDATE:
    return app->opts.continuous ? puglPostRedisplay(view) : PUGL_SUCCESS;
  case PUGL_EXPOSE:
    return onExpose(view);
  case PUGL_CONFIGURE:
    return onConfigure(view, e->configure.width, e->configure.height);
  case PUGL_CLOSE:
    app->quit = 1;
    break;
  case PUGL_KEY_PRESS:
    switch (e->key.key) {
    case PUGL_KEY_ESCAPE:
    case 'q':
      app->quit = 1;
      break;
    }
    break;
  default:
    break;
  }
  return PUGL_SUCCESS;
}

int
main(int argc, char** argv)
{
  VulkanApp app;
  memset(&app, 0, sizeof(app));

  VulkanState*   vk            = &app.vk;
  const PuglSpan defaultWidth  = 640;
  const PuglSpan defaultHeight = 360;

  // Parse command line options
  app.opts = puglParseTestOptions(&argc, &argv);
  if (app.opts.help) {
    puglPrintTestUsage(argv[0], "");
    return 0;
  }

  // Create world and view
  if (!(app.world = puglNewWorld(PUGL_PROGRAM, PUGL_WORLD_THREADS))) {
    return logError("Failed to create world\n");
  }

  if (!(app.view = puglNewView(app.world))) {
    puglFreeWorld(app.world);
    return logError("Failed to create Pugl World and View\n");
  }

  // Create Vulkan instance
  if (createInstance(&app)) {
    puglFreeWorld(app.world);
    return logError("Failed to create instance\n");
  }

  // Create window
  puglSetViewString(app.view, PUGL_WINDOW_TITLE, "Pugl Vulkan Demo");
  puglSetSizeHint(app.view, PUGL_DEFAULT_SIZE, defaultWidth, defaultHeight);
  puglSetHandle(app.view, &app);
  puglSetBackend(app.view, puglVulkanBackend());
  puglSetViewHint(app.view, PUGL_RESIZABLE, app.opts.resizable);
  puglSetEventFunc(app.view, onEvent);
  const PuglStatus st = puglRealize(app.view);
  if (st) {
    puglFreeWorld(app.world);
    puglFreeView(app.view);
    return logError("Failed to create window (%s)\n", puglStrerror(st));
  }

  // Create Vulkan surface for window
  PuglVulkanLoader* loader = puglNewVulkanLoader(app.world, NULL);
  if (puglCreateSurface(puglGetInstanceProcAddrFunc(loader),
                        app.view,
                        vk->instance,
                        ALLOC_VK,
                        &vk->surface)) {
    return logError("Failed to create surface\n");
  }

  // Set up Vulkan
  VkResult vr = VK_SUCCESS;
  if ((vr = enableDebugging(vk)) ||                              //
      (vr = selectPhysicalDevice(vk)) ||                         //
      (vr = openDevice(vk)) ||                                   //
      (vr = configureSurface(vk)) ||                             //
      (vr = createSwapchain(vk, defaultWidth, defaultHeight)) || //
      (vr = createSyncObjects(vk))) {
    destroyWorld(&app);
    return logError("Failed to set up graphics (%d)\n", vr);
  }

  printf("Swapchain images:            %u\n", app.vk.swapchain->nImages);

  PuglFpsPrinter fpsPrinter = {puglGetTime(app.world)};
  puglShow(app.view, PUGL_SHOW_RAISE);
  while (!app.quit) {
    puglUpdate(app.world, -1.0);

    if (app.opts.continuous) {
      puglPrintFps(app.world, &fpsPrinter, &app.framesDrawn);
    }
  }

  destroyWorld(&app);
  return 0;
}
