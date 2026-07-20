// Copyright 2012-2023 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

/*
  Note that this header includes Vulkan headers, so if you are writing a
  program or plugin that dynamically loads vulkan, you should first define
  `VK_NO_PROTOTYPES` before including it.
*/

#ifndef PUGL_VULKAN_HPP
#define PUGL_VULKAN_HPP

#include "pugl/pugl.h"
#include "pugl/pugl.hpp"
#include "pugl/vulkan.h"

#include <vulkan/vulkan_core.h>

#include <cstdint>

namespace pugl {

/**
   @defgroup puglpp_vulkan Vulkan
   Vulkan graphics support.

   Note that the Pugl C++ wrapper does not use vulkan-hpp because it is a
   heavyweight dependency which not everyone uses, and its design is not very
   friendly to dynamic loading in plugins anyway.  However, if you do use
   vulkan-hpp smart handles, it is relatively straightforward to wrap the
   result of createSurface() manually.

   @ingroup puglpp
   @{
*/

/// @copydoc PuglVulkanLoader
class VulkanLoader final
  : public detail::Wrapper<PuglVulkanLoader, puglFreeVulkanLoader>
{
public:
  /**
     Create a new dynamic loader for Vulkan functions.

     This dynamically loads the Vulkan library and gets the load functions
     from it.

     Note that this constructor does not throw exceptions, though failure is
     possible.  To check if the Vulkan library failed to load, test this
     loader, which is explicitly convertible to `bool`.  It is safe to use a
     failed loader, but the accessors will always return null.
  */
  explicit VulkanLoader(World& world) noexcept
    : VulkanLoader{world, nullptr}
  {}

  /**
     Create a new dynamic loader for Vulkan functions from a specific library.

     This is the same as the simpler constructor, but allows the user to
     specify the name of the Vulkan library to load, for certain packaging
     scenarios or unusual/unsupported system configurations.  The `libraryName`
     is passed to the system library loading function (`dlopen` or
     `LoadLibrary`), and supports an absolute path.
  */
  explicit VulkanLoader(World& world, const char* const libraryName) noexcept
    : Wrapper{puglNewVulkanLoader(world.cobj(), libraryName)}
  {}

  /**
     Return the `vkGetInstanceProcAddr` function.

     @return Null if the Vulkan library failed to load, or does not contain
     this function (which is unlikely and indicates a broken system).
  */
  PFN_vkGetInstanceProcAddr getInstanceProcAddrFunc() const noexcept
  {
    return cobj() ? puglGetInstanceProcAddrFunc(cobj()) : nullptr;
  }

  /**
     Return the `vkGetDeviceProcAddr` function.

     @return Null if the Vulkan library failed to load, or does not contain
     this function (which is unlikely and indicates a broken system).
  */
  PFN_vkGetDeviceProcAddr getDeviceProcAddrFunc() const noexcept
  {
    return cobj() ? puglGetDeviceProcAddrFunc(cobj()) : nullptr;
  }

  /// Return true if this loader is valid to use
  explicit operator bool() const noexcept { return cobj(); }
};

/**
   A simple wrapper for an array of static C strings.

   This provides a minimal API that supports iteration, like `std::vector`, but
   avoids allocation, exceptions, and a dependency on the C++ standard library.
*/
class StaticStringArray final
{
public:
  using value_type     = const char*;
  using const_iterator = const char* const*;
  using size_type      = uint32_t;

  StaticStringArray(const char* const* strings, const uint32_t size) noexcept
    : _strings{strings}
    , _size{size}
  {}

  const char* const* begin() const noexcept { return _strings; }
  const char* const* end() const noexcept { return _strings + _size; }
  const char* const* data() const noexcept { return _strings; }
  uint32_t           size() const noexcept { return _size; }

private:
  const char* const* _strings;
  uint32_t           _size;
};

/**
   Return the Vulkan instance extensions required to draw to a PuglView.

   If successful, the returned array always contains "VK_KHR_surface", along
   with whatever other platform-specific extensions are required.

   @return An array of extension name strings.
*/
inline StaticStringArray
getInstanceExtensions() noexcept
{
  uint32_t                 count      = 0;
  const char* const* const extensions = puglGetInstanceExtensions(&count);

  return StaticStringArray{extensions, count};
}

/// @copydoc puglCreateSurface
inline VkResult
createSurface(PFN_vkGetInstanceProcAddr          vkGetInstanceProcAddr,
              View&                              view,
              VkInstance                         instance,
              const VkAllocationCallbacks* const allocator,
              VkSurfaceKHR* const                surface) noexcept
{
  const VkResult r = puglCreateSurface(
    vkGetInstanceProcAddr, view.cobj(), instance, allocator, surface);

  return (!r && !surface) ? VK_ERROR_INITIALIZATION_FAILED : r;
}

/// @copydoc puglVulkanBackend
inline const PuglBackend*
vulkanBackend() noexcept
{
  return puglVulkanBackend();
}

/**
   @}
*/

} // namespace pugl

#endif // PUGL_VULKAN_HPP
