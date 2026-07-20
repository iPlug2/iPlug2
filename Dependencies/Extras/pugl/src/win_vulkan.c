// Copyright 2012-2021 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#define VK_NO_PROTOTYPES 1

#include "stub.h"
#include "types.h"
#include "win.h"

#include "pugl/vulkan.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

#include <stdlib.h>

struct PuglVulkanLoaderImpl {
  HMODULE                   libvulkan;
  PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
  PFN_vkGetDeviceProcAddr   vkGetDeviceProcAddr;
};

PuglVulkanLoader*
puglNewVulkanLoader(PuglWorld*        PUGL_UNUSED(world),
                    const char* const libraryName)
{
  PuglVulkanLoader* loader =
    (PuglVulkanLoader*)calloc(1, sizeof(PuglVulkanLoader));
  if (!loader) {
    return NULL;
  }

  const char* const filename = libraryName ? libraryName : "vulkan-1.dll";
  if (!(loader->libvulkan =
          LoadLibraryEx(filename, NULL, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS))) {
    free(loader);
    return NULL;
  }

  loader->vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetProcAddress(
    loader->libvulkan, "vkGetInstanceProcAddr");

  loader->vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr)GetProcAddress(
    loader->libvulkan, "vkGetDeviceProcAddr");

  return loader;
}

void
puglFreeVulkanLoader(PuglVulkanLoader* loader)
{
  if (loader) {
    FreeLibrary(loader->libvulkan);
    free(loader);
  }
}

PFN_vkGetInstanceProcAddr
puglGetInstanceProcAddrFunc(const PuglVulkanLoader* loader)
{
  return loader->vkGetInstanceProcAddr;
}

PFN_vkGetDeviceProcAddr
puglGetDeviceProcAddrFunc(const PuglVulkanLoader* loader)
{
  return loader->vkGetDeviceProcAddr;
}

const PuglBackend*
puglVulkanBackend(void)
{
  static const PuglBackend backend = {puglWinConfigure,
                                      puglStubCreate,
                                      puglStubDestroy,
                                      puglWinEnter,
                                      puglWinLeave,
                                      puglStubGetContext};

  return &backend;
}

const char* const*
puglGetInstanceExtensions(uint32_t* const count)
{
  static const char* const extensions[] = {"VK_KHR_surface",
                                           "VK_KHR_win32_surface"};

  *count = 2;
  return extensions;
}

VkResult
puglCreateSurface(PFN_vkGetInstanceProcAddr          vkGetInstanceProcAddr,
                  PuglView* const                    view,
                  VkInstance                         instance,
                  const VkAllocationCallbacks* const pAllocator,
                  VkSurfaceKHR* const                pSurface)
{
  PuglInternals* const impl = view->impl;

  PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR =
    (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(
      instance, "vkCreateWin32SurfaceKHR");

  const VkWin32SurfaceCreateInfoKHR createInfo = {
    VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
    NULL,
    0,
    GetModuleHandle(NULL),
    impl->hwnd,
  };

  return vkCreateWin32SurfaceKHR(instance, &createInfo, pAllocator, pSurface);
}
