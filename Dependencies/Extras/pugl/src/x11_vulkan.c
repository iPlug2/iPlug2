// Copyright 2012-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#define VK_NO_PROTOTYPES 1

#include "attributes.h"
#include "stub.h"
#include "types.h"
#include "x11.h"

#include "pugl/pugl.h"
#include "pugl/vulkan.h"

#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_xlib.h>

#include <dlfcn.h>

#include <stdint.h>
#include <stdlib.h>

struct PuglVulkanLoaderImpl {
  void*                     libvulkan;
  PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
  PFN_vkGetDeviceProcAddr   vkGetDeviceProcAddr;
};

PuglVulkanLoader*
puglNewVulkanLoader(PuglWorld*        PUGL_UNUSED(world),
                    const char* const libraryName)
{
  const char* const filename  = libraryName ? libraryName : "libvulkan.so";
  void* const       libvulkan = dlopen(filename, RTLD_LAZY);
  if (!libvulkan) {
    return NULL;
  }

  PuglVulkanLoader* const loader =
    (PuglVulkanLoader*)calloc(1, sizeof(PuglVulkanLoader));

  if (!loader) {
    dlclose(libvulkan);
    return NULL;
  }

  loader->libvulkan = libvulkan;

  loader->vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(
    loader->libvulkan, "vkGetInstanceProcAddr");

  loader->vkGetDeviceProcAddr =
    (PFN_vkGetDeviceProcAddr)dlsym(loader->libvulkan, "vkGetDeviceProcAddr");

  return loader;
}

void
puglFreeVulkanLoader(PuglVulkanLoader* loader)
{
  if (loader) {
    dlclose(loader->libvulkan);
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
  static const PuglBackend backend = {puglX11Configure,
                                      puglStubCreate,
                                      puglStubDestroy,
                                      puglStubEnter,
                                      puglStubLeave,
                                      puglStubGetContext};

  return &backend;
}

const char* const*
puglGetInstanceExtensions(uint32_t* const count)
{
  static const char* const extensions[] = {"VK_KHR_surface",
                                           "VK_KHR_xlib_surface"};

  *count = 2;
  return extensions;
}

VkResult
puglCreateSurface(PFN_vkGetInstanceProcAddr          vkGetInstanceProcAddr,
                  PuglView* const                    view,
                  VkInstance                         instance,
                  const VkAllocationCallbacks* const allocator,
                  VkSurfaceKHR* const                surface)
{
  PuglInternals* const impl       = view->impl;
  PuglWorldInternals*  world_impl = view->world->impl;

  PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR =
    (PFN_vkCreateXlibSurfaceKHR)vkGetInstanceProcAddr(instance,
                                                      "vkCreateXlibSurfaceKHR");

  const VkXlibSurfaceCreateInfoKHR info = {
    VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
    NULL,
    0,
    world_impl->display,
    impl->win,
  };

  return vkCreateXlibSurfaceKHR(instance, &info, allocator, surface);
}
