// Copyright 2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

/**
   @file sybok.hpp
   @brief A minimal C++ wrapper for the Vulkan API.

   This is a manually-written minimal wrapper for Vulkan.  It makes working
   with Vulkan a little easier in C++, but takes a different approach than
   vulkan.hpp.  In particular:

   - Works nicely with dynamic loading.  Since the API itself is an object, it
     is simple to ensure the dynamically loaded API (or a consistent API in
     general) is used everywhere.  Passing a dispatch parameter to every
     function as in vulkan.hpp makes dynamic loading extremely painful (not to
     mention ugly), and mistakes tend to become link time errors.  This is, in
     my opinion, a glaring design flaw, and the real reason why this wrapper
     reluctantly exists.

   - Explicit separation of the initial API that does not require an instance
     to load, from the rest of the API that does.

   - Opinionated use of scoped handles everywhere.

   - Remains close to the C API so that code can be easily ported.  This means
     that the pattern of return codes with output parameters is preserved,
     except with smart handles that make leaks impossible.  While less pretty,
     this does not require exceptions.

   - No exceptions or RTTI required.

   - A safe scoped API for commands that encodes the semantics of the Vulkan
     API.  For example, it is statically impossible to call render scope
     commands while not in a render scope.

   - A reasonable amount of relatively readable code.

   On the other hand, there are far fewer niceties, and the C API is used
   directly as much as possible, particularly for structs (although they are
   taken by const reference so they can be written inline).  There is only
   support for a minimal portable subset of Vulkan 1.1 with a few portable KHR
   extensions.

   In short, if the above sounds appealing, or you want a minimal wrapper that
   can be extended if necessary to suit your application, you might find this
   useful.  If you want a fully-featured wrapper for Vulkan and don't care
   about linker dependencies, you probably won't.
*/

#ifndef SYBOK_HPP
#define SYBOK_HPP

#ifdef VULKAN_CORE_H_
#  error "sybok.hpp must be included before or instead of vulkan headers"
#endif

#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wswitch-enum"
#endif

#define VK_NO_PROTOTYPES

// On 64-bit platforms, all handles are "dispatchable" pointers
#if defined(__LP64__) || defined(_WIN64) ||                          \
  (defined(__x86_64__) && !defined(__ILP32__)) || defined(_M_X64) || \
  defined(__ia64) || defined(_M_IA64) || defined(__aarch64__) ||     \
  defined(__powerpc64__)

#  define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) \
    typedef struct object##_T* object; // NOLINT(bugprone-macro-parentheses)

// On 32-bit platforms, some "non-dispatchable" handles are 64 bit integers
#else

/// Trivial wrapper class for a 64-bit integer handle for type safety
template<class Tag>
struct NonDispatchableHandle {
  explicit operator uint64_t() const noexcept { return handle; }
  explicit operator bool() const noexcept { return handle; }

  uint64_t handle;
};

#  define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) \
    using object = NonDispatchableHandle<struct Sk##object##Tag>;

#endif

#include <vulkan/vulkan_core.h> // IWYU pragma: export

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

#if __cplusplus >= 201703L
#  define SYBOK_NODISCARD [[nodiscard]]
#elif defined(__GNUC__)
#  define SYBOK_NODISCARD [[gnu::warn_unused_result]]
#else
#  define SYBOK_NODISCARD
#endif

/// Helper macro to make array arguments format nicely
#define SK_COUNTED(count, ...) count, __VA_ARGS__

namespace sk {

class CommandScope;
class RenderCommandScope;

inline const char*
string(const VkResult result)
{
  switch (result) {
  case VK_SUCCESS:
    return "Success";
  case VK_NOT_READY:
    return "Not Ready";
  case VK_TIMEOUT:
    return "Timeout";
  case VK_EVENT_SET:
    return "Event set";
  case VK_EVENT_RESET:
    return "Event reset";
  case VK_INCOMPLETE:
    return "Incomplete";
  case VK_ERROR_OUT_OF_HOST_MEMORY:
    return "Out of host memory";
  case VK_ERROR_OUT_OF_DEVICE_MEMORY:
    return "Out of device memory";
  case VK_ERROR_INITIALIZATION_FAILED:
    return "Initialization failed";
  case VK_ERROR_DEVICE_LOST:
    return "Device lost";
  case VK_ERROR_MEMORY_MAP_FAILED:
    return "Memory map failed";
  case VK_ERROR_LAYER_NOT_PRESENT:
    return "Layer not present";
  case VK_ERROR_EXTENSION_NOT_PRESENT:
    return "Extension not present";
  case VK_ERROR_FEATURE_NOT_PRESENT:
    return "Feature not present";
  case VK_ERROR_INCOMPATIBLE_DRIVER:
    return "Incompatible driver";
  case VK_ERROR_TOO_MANY_OBJECTS:
    return "Too many objects";
  case VK_ERROR_FORMAT_NOT_SUPPORTED:
    return "Format not supported";
  case VK_ERROR_FRAGMENTED_POOL:
    return "Fragmented pool";
  case VK_ERROR_OUT_OF_POOL_MEMORY: // Vulkan 1.1
    return "Out of pool memory";
  case VK_ERROR_INVALID_EXTERNAL_HANDLE: // Vulkan 1.1
    return "Invalid external handle";
  case VK_ERROR_SURFACE_LOST_KHR: // VK_KHR_surface
    return "Surface lost";
  case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: // VK_KHR_surface
    return "Native window in use";
  case VK_SUBOPTIMAL_KHR: // VK_KHR_swapchain
    return "Suboptimal";
  case VK_ERROR_OUT_OF_DATE_KHR: // VK_KHR_swapchain
    return "Out of date";
  case VK_ERROR_VALIDATION_FAILED_EXT: // VK_EXT_debug_report
    return "Validation failed";
  default:
    break;
  }

  return "Unknown error";
}

inline const char*
string(const VkPresentModeKHR presentMode)
{
  switch (presentMode) {
  case VK_PRESENT_MODE_IMMEDIATE_KHR:
    return "Immediate";
  case VK_PRESENT_MODE_MAILBOX_KHR:
    return "Mailbox";
  case VK_PRESENT_MODE_FIFO_KHR:
    return "FIFO";
  case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
    return "Relaxed FIFO";
  default:
    break;
  }

  return "Unknown present mode";
}

inline const char*
string(const VkDebugReportFlagBitsEXT flag)
{
  switch (flag) {
  case VK_DEBUG_REPORT_INFORMATION_BIT_EXT:
    return "Information";
  case VK_DEBUG_REPORT_WARNING_BIT_EXT:
    return "Warning";
  case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:
    return "Performance Warning";
  case VK_DEBUG_REPORT_ERROR_BIT_EXT:
    return "Error";
  case VK_DEBUG_REPORT_DEBUG_BIT_EXT:
    return "Debug";
  default:
    break;
  }

  return "Unknown report";
}

template<class T>
class GlobalDeleter
{
public:
  using DestroyFunc = void (*)(T, const VkAllocationCallbacks*);

  GlobalDeleter()  = default;
  ~GlobalDeleter() = default;

  // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
  GlobalDeleter(DestroyFunc destroyFunc) noexcept
    : _destroyFunc{destroyFunc}
  {}

  GlobalDeleter(const GlobalDeleter&)            = delete;
  GlobalDeleter& operator=(const GlobalDeleter&) = delete;

  GlobalDeleter(GlobalDeleter&& other) noexcept
  {
    std::swap(_destroyFunc, other._destroyFunc);
  }

  GlobalDeleter& operator=(GlobalDeleter&& other) noexcept
  {
    std::swap(_destroyFunc, other._destroyFunc);
    return *this;
  }

  void operator()(T handle) noexcept
  {
    if (_destroyFunc && handle) {
      _destroyFunc(handle, nullptr);
    }
  }

private:
  DestroyFunc _destroyFunc{};
};

template<class T, class Parent>
class DependantDeleter
{
public:
  using DestroyFunc = void (*)(Parent, T, const VkAllocationCallbacks*);

  DependantDeleter()  = default;
  ~DependantDeleter() = default;

  DependantDeleter(Parent parent, DestroyFunc destroyFunc) noexcept
    : _parent{parent}
    , _destroyFunc{destroyFunc}
  {}

  DependantDeleter(const DependantDeleter&)            = delete;
  DependantDeleter& operator=(const DependantDeleter&) = delete;

  DependantDeleter(DependantDeleter&& other) noexcept { swap(other); }

  DependantDeleter& operator=(DependantDeleter&& other) noexcept
  {
    swap(other);
    return *this;
  }

  void operator()(T handle) noexcept
  {
    if (_parent && _destroyFunc && handle) {
      _destroyFunc(_parent, handle, nullptr);
    }
  }

private:
  void swap(DependantDeleter& other) noexcept
  {
    std::swap(_parent, other._parent);
    std::swap(_destroyFunc, other._destroyFunc);
  }

  Parent      _parent{};
  DestroyFunc _destroyFunc{};
};

template<class T, class Pool, class FreeFuncResult>
class PoolDeleter
{
public:
  using FreeFunc = FreeFuncResult (*)(VkDevice, Pool, uint32_t, const T*);

  PoolDeleter() noexcept  = default;
  ~PoolDeleter() noexcept = default;

  PoolDeleter(VkDevice device,
              Pool     pool,
              uint32_t count,
              FreeFunc freeFunc) noexcept
    : _device{device}
    , _pool{pool}
    , _count{count}
    , _freeFunc{freeFunc}
  {}

  PoolDeleter(const PoolDeleter&)            = delete;
  PoolDeleter& operator=(const PoolDeleter&) = delete;

  PoolDeleter(PoolDeleter&& other) noexcept { swap(other); }

  PoolDeleter& operator=(PoolDeleter&& other) noexcept
  {
    swap(other);
    return *this;
  }

  void operator()(T* handle) noexcept
  {
    if (_device && _pool && handle) {
      _freeFunc(_device, _pool, _count, handle);
    }
  }

private:
  void swap(PoolDeleter& other) noexcept
  {
    std::swap(_device, other._device);
    std::swap(_pool, other._pool);
    std::swap(_count, other._count);
    std::swap(_freeFunc, other._freeFunc);
  }

  VkDevice _device{};
  Pool     _pool{};
  uint32_t _count{};
  FreeFunc _freeFunc{};
};

template<class T, class TDeleter>
class UniqueDispatchableHandle
{
public:
  using Deleter = TDeleter;
  using Handle  = T;

  static_assert(std::is_pointer<T>::value, "");

  UniqueDispatchableHandle() = default;

  UniqueDispatchableHandle(Handle handle, Deleter deleter) noexcept
    : _handle{handle}
    , _deleter{std::move(deleter)}
  {}

  ~UniqueDispatchableHandle() noexcept
  {
    if (_handle) {
      _deleter(_handle);
    }
  }

  UniqueDispatchableHandle(const UniqueDispatchableHandle&) noexcept = delete;
  UniqueDispatchableHandle& operator=(
    const UniqueDispatchableHandle&) noexcept = delete;

  UniqueDispatchableHandle(UniqueDispatchableHandle&& other) noexcept
  {
    swap(other);
  }

  UniqueDispatchableHandle& operator=(UniqueDispatchableHandle&& other) noexcept
  {
    swap(other);
    return *this;
  }

  const Handle& get() const noexcept { return _handle; }

  // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
  operator Handle() const noexcept { return _handle; }

private:
  void swap(UniqueDispatchableHandle& other) noexcept
  {
    std::swap(_handle, other._handle);
    std::swap(_deleter, other._deleter);
  }

  Handle  _handle{};
  Deleter _deleter{};
};

#if defined(__LP64__) || defined(_WIN64) ||                          \
  (defined(__x86_64__) && !defined(__ILP32__)) || defined(_M_X64) || \
  defined(__ia64) || defined(_M_IA64) || defined(__aarch64__) ||     \
  defined(__powerpc64__)

template<class T, class TDeleter>
using UniqueNonDispatchableHandle = UniqueDispatchableHandle<T, TDeleter>;

#else

template<class T, class TDeleter>
class UniqueNonDispatchableHandle
{
public:
  using Deleter = TDeleter;
  using Handle  = T;

  UniqueNonDispatchableHandle() = default;

  UniqueNonDispatchableHandle(T handle, Deleter deleter) noexcept
    : _handle{handle}
    , _deleter{std::move(deleter)}
  {
    assert(handle);
  }

  ~UniqueNonDispatchableHandle() noexcept
  {
    if (_handle) {
      _deleter(_handle);
    }
  }

  UniqueNonDispatchableHandle(const UniqueNonDispatchableHandle&) noexcept =
    delete;
  UniqueNonDispatchableHandle& operator=(
    const UniqueNonDispatchableHandle&) noexcept = delete;

  UniqueNonDispatchableHandle(UniqueNonDispatchableHandle&& other) noexcept
  {
    swap(other);
  }

  UniqueNonDispatchableHandle& operator=(
    UniqueNonDispatchableHandle&& other) noexcept
  {
    swap(other);
    return *this;
  }

  const Handle& get() const noexcept { return _handle; }

  operator Handle() const noexcept { return _handle; }

private:
  void swap(UniqueNonDispatchableHandle& other) noexcept
  {
    std::swap(_handle, other._handle);
    std::swap(_deleter, other._deleter);
  }

  T       _handle{};
  Deleter _deleter{};
};

#endif

template<class Vector, class Deleter>
class UniqueArrayHandle
{
public:
  using T = typename Vector::value_type;

  UniqueArrayHandle() = default;

  UniqueArrayHandle(uint32_t size, Vector&& array, Deleter deleter) noexcept
    : _array{std::move(array)}
    , _deleter{std::move(deleter)}
    , _size{size}
  {
    assert(!_array.empty());
  }

  ~UniqueArrayHandle() noexcept
  {
    if (!_array.empty()) {
      _deleter(_array.data());
    }
  }

  UniqueArrayHandle(const UniqueArrayHandle&) noexcept            = delete;
  UniqueArrayHandle& operator=(const UniqueArrayHandle&) noexcept = delete;

  UniqueArrayHandle(UniqueArrayHandle&& other) noexcept { swap(other); }

  UniqueArrayHandle& operator=(UniqueArrayHandle&& other) noexcept
  {
    swap(other);
    return *this;
  }

  const T& operator[](const size_t index) const noexcept
  {
    return _array[index];
  }

  T& operator[](const size_t index) noexcept { return _array[index]; }

  const T* get() const noexcept { return _array.data(); }
  T*       get() noexcept { return _array.data(); }

private:
  void swap(UniqueArrayHandle& other) noexcept
  {
    std::swap(_array, other._array);
    std::swap(_deleter, other._deleter);
    std::swap(_size, other._size);
  }

  Vector   _array{};
  Deleter  _deleter{};
  uint32_t _size{};
};

template<typename T>
class OptionalParameter
{
public:
  using Handle = typename T::Handle;

  // NOLINTNEXTLINE(hicpp-explicit-conversions, google-explicit-constructor)
  OptionalParameter(const T& value) noexcept
    : _handle{value.get()}
  {}

  OptionalParameter() noexcept  = default;
  ~OptionalParameter() noexcept = default;

  OptionalParameter(const OptionalParameter&)            = delete;
  OptionalParameter& operator=(const OptionalParameter&) = delete;

  OptionalParameter(OptionalParameter&&)            = delete;
  OptionalParameter& operator=(OptionalParameter&&) = delete;

  Handle get() const noexcept { return _handle; }

private:
  Handle _handle{};
};

template<typename T>
using GlobalObject = UniqueDispatchableHandle<T, GlobalDeleter<T>>;

template<typename T>
using InstanceChild =
  UniqueNonDispatchableHandle<T, DependantDeleter<T, VkInstance>>;

template<typename T>
using DispatchableDeviceChild =
  UniqueDispatchableHandle<T, DependantDeleter<T, VkDevice>>;

template<typename T>
using NonDispatchableDeviceChild =
  UniqueNonDispatchableHandle<T, DependantDeleter<T, VkDevice>>;

template<typename Vector, typename Pool, typename FreeFuncResult>
using PoolChild = UniqueArrayHandle<
  Vector,
  PoolDeleter<typename Vector::value_type, Pool, FreeFuncResult>>;

using Device   = GlobalObject<VkDevice>;
using Instance = GlobalObject<VkInstance>;

using PhysicalDevice = VkPhysicalDevice; // Weak handle, no destroy function
using Queue          = VkQueue;          // Weak handle, no destroy function

using Buffer              = NonDispatchableDeviceChild<VkBuffer>;
using BufferView          = NonDispatchableDeviceChild<VkBufferView>;
using CommandBuffer       = DispatchableDeviceChild<VkCommandBuffer>;
using CommandPool         = NonDispatchableDeviceChild<VkCommandPool>;
using DescriptorPool      = NonDispatchableDeviceChild<VkDescriptorPool>;
using DescriptorSetLayout = NonDispatchableDeviceChild<VkDescriptorSetLayout>;
using DeviceMemory        = NonDispatchableDeviceChild<VkDeviceMemory>;
using Event               = NonDispatchableDeviceChild<VkEvent>;
using Fence               = NonDispatchableDeviceChild<VkFence>;
using Framebuffer         = NonDispatchableDeviceChild<VkFramebuffer>;
using Image               = NonDispatchableDeviceChild<VkImage>;
using ImageView           = NonDispatchableDeviceChild<VkImageView>;
using Pipeline            = NonDispatchableDeviceChild<VkPipeline>;
using PipelineCache       = NonDispatchableDeviceChild<VkPipelineCache>;
using PipelineLayout      = NonDispatchableDeviceChild<VkPipelineLayout>;
using QueryPool           = NonDispatchableDeviceChild<VkQueryPool>;
using RenderPass          = NonDispatchableDeviceChild<VkRenderPass>;
using Sampler             = NonDispatchableDeviceChild<VkSampler>;
using Semaphore           = NonDispatchableDeviceChild<VkSemaphore>;
using ShaderModule        = NonDispatchableDeviceChild<VkShaderModule>;

template<class VkCommandBufferVector>
using CommandBuffers = PoolChild<VkCommandBufferVector, VkCommandPool, void>;

template<class VkDescriptorSetVector>
using DescriptorSets =
  PoolChild<VkDescriptorSetVector, VkDescriptorPool, VkResult>;

// VK_KHR_swapchain
using SwapchainKHR = NonDispatchableDeviceChild<VkSwapchainKHR>;

// VK_KHR_surface
using SurfaceKHR = InstanceChild<VkSurfaceKHR>;

// VK_EXT_debug_report
using DebugReportCallbackEXT = InstanceChild<VkDebugReportCallbackEXT>;

template<size_t...>
struct IndexSequence {};

template<size_t N, size_t... Next>
struct IndexSequenceHelper
  : public IndexSequenceHelper<N - 1U, N - 1U, Next...> {};

template<size_t... Next>
struct IndexSequenceHelper<0U, Next...> {
  using type = IndexSequence<Next...>;
};

template<size_t N>
using makeIndexSequence = typename IndexSequenceHelper<N>::type;

template<class T, class Parent, class DestroyFunc, size_t count, size_t... Is>
std::array<T, count>
make_handle_array_h(Parent                                parent,
                    DestroyFunc                           destroyFunc,
                    std::array<typename T::Handle, count> handles,
                    IndexSequence<Is...>) noexcept
{
  return {T{handles[Is], {parent, destroyFunc}}...};
}

template<class T, class Parent, class DestroyFunc, size_t count>
std::array<T, count>
make_handle_array(Parent                                parent,
                  DestroyFunc                           destroyFunc,
                  std::array<typename T::Handle, count> handles) noexcept
{
  return make_handle_array_h<T, Parent, DestroyFunc, count>(
    parent, destroyFunc, handles, makeIndexSequence<count>());
}

namespace detail {

template<class Value, class Vector, class Func, class... Args>
inline VkResult
wrapVectorAccessor(Vector& vector, Func func, Args... args) noexcept
{
  uint32_t count = 0U;
  VkResult r     = func(args..., &count, nullptr);
  if (r > VK_INCOMPLETE) {
    vector.clear();
    return r;
  }

  vector = Vector(count);
  if ((r = func(args..., &count, vector.data()))) {
    vector.clear();
    return r;
  }

  return VK_SUCCESS;
}

} // namespace detail

class VulkanApi;

struct MappedMemory {
  MappedMemory() noexcept = default;

  MappedMemory(const VulkanApi& api,
               VkDevice         device,
               VkDeviceMemory   memory,
               void*            data) noexcept
    : _api{&api}
    , _device{device}
    , _memory{memory}
    , _data{data}
  {}

  MappedMemory(const MappedMemory&)            = delete;
  MappedMemory& operator=(const MappedMemory&) = delete;

  MappedMemory(MappedMemory&& mappedMemory) noexcept
    : _api{mappedMemory._api}
    , _device{mappedMemory._device}
    , _memory{mappedMemory._memory}
    , _data{mappedMemory._data}
  {
    mappedMemory._device = {};
    mappedMemory._memory = {};
    mappedMemory._data   = {};
  }

  MappedMemory& operator=(MappedMemory&& mappedMemory) noexcept
  {
    std::swap(_api, mappedMemory._api);
    std::swap(_device, mappedMemory._device);
    std::swap(_memory, mappedMemory._memory);
    std::swap(_data, mappedMemory._data);
    return *this;
  }

  ~MappedMemory() noexcept;

  const void* get() const noexcept { return _data; }
  void*       get() noexcept { return _data; }

private:
  const VulkanApi* _api{};
  VkDevice         _device{};
  VkDeviceMemory   _memory{};
  void*            _data{};
};

class VulkanInitApi
{
public:
  template<typename NotFoundFunc>
  VkResult init(PFN_vkGetInstanceProcAddr pGetInstanceProcAddr,
                NotFoundFunc              notFound) noexcept
  {
#define SK_INIT(name)                                             \
  do {                                                            \
    if (!(name = PFN_##name(getInstanceProcAddr(NULL, #name)))) { \
      notFound(#name);                                            \
    }                                                             \
  } while (0)

    vkGetInstanceProcAddr = pGetInstanceProcAddr;
    SK_INIT(vkCreateInstance);
    vkDestroyInstance = {}; // Loaded after we create an instance
    SK_INIT(vkEnumerateInstanceExtensionProperties);
    SK_INIT(vkEnumerateInstanceLayerProperties);

    if (!vkCreateInstance || !vkEnumerateInstanceExtensionProperties ||
        !vkEnumerateInstanceLayerProperties) {
      return VK_ERROR_INITIALIZATION_FAILED;
    }

    return VK_SUCCESS;
#undef SK_INIT
  }

  VkResult init(PFN_vkGetInstanceProcAddr pGetInstanceProcAddr) noexcept
  {
    return init(pGetInstanceProcAddr, [](const char*) {});
  }

  PFN_vkVoidFunction getInstanceProcAddr(VkInstance        instance,
                                         const char* const name) const noexcept
  {
    return vkGetInstanceProcAddr(instance, name);
  }

  VkResult createInstance(const VkInstanceCreateInfo& createInfo,
                          Instance&                   instance) noexcept
  {
    VkInstance h = {};
    if (const VkResult r = vkCreateInstance(&createInfo, nullptr, &h)) {
      return r;
    }

    if (!h) {
      // Shouldn't actually happen, but this lets the compiler know that
      return VK_ERROR_INITIALIZATION_FAILED;
    }

    if (!vkDestroyInstance) {
      vkDestroyInstance = reinterpret_cast<PFN_vkDestroyInstance>(
        getInstanceProcAddr(instance, "vkDestroyInstance"));
    }

    instance = {h, {vkDestroyInstance}};
    return VK_SUCCESS;
  }

  template<class Vector>
  VkResult enumerateInstanceExtensionProperties(
    Vector& properties) const noexcept
  {
    return detail::wrapVectorAccessor<VkExtensionProperties>(
      properties, vkEnumerateInstanceExtensionProperties, nullptr);
  }

  template<class Vector>
  VkResult enumerateInstanceExtensionProperties(
    const char* const layerName,
    Vector&           properties) const noexcept
  {
    return detail::wrapVectorAccessor<VkExtensionProperties>(
      properties, vkEnumerateInstanceExtensionProperties, layerName);
  }

  template<class Vector>
  VkResult enumerateInstanceLayerProperties(Vector& properties) const noexcept
  {
    return detail::wrapVectorAccessor<VkLayerProperties>(
      properties, vkEnumerateInstanceLayerProperties);
  }

private:
  PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr{};

#define SK_FUNC(name) \
  PFN_##name name {}

  SK_FUNC(vkCreateInstance);
  SK_FUNC(vkDestroyInstance);
  SK_FUNC(vkEnumerateInstanceExtensionProperties);
  SK_FUNC(vkEnumerateInstanceLayerProperties);

#undef SK_FUNC
};

class VulkanApi
{
public:
  template<typename NotFoundFunc>
  VkResult init(const VulkanInitApi& initApi,
                const Instance&      instance,
                NotFoundFunc         notFound) noexcept
  {
    VkResult r = VK_SUCCESS;

    const auto notFoundWrapper = [&r, notFound](const char* name) {
      r = VK_INCOMPLETE;
      notFound(name);
    };

#define SK_INIT(name)                                                         \
  do {                                                                        \
    if (!(name = PFN_##name(initApi.getInstanceProcAddr(instance, #name)))) { \
      notFoundWrapper(#name);                                                 \
    }                                                                         \
  } while (0)

    SK_INIT(vkAllocateCommandBuffers);
    SK_INIT(vkAllocateDescriptorSets);
    SK_INIT(vkAllocateMemory);
    SK_INIT(vkBeginCommandBuffer);
    SK_INIT(vkBindBufferMemory);
    SK_INIT(vkBindImageMemory);
    SK_INIT(vkCmdBeginQuery);
    SK_INIT(vkCmdBeginRenderPass);
    SK_INIT(vkCmdBindDescriptorSets);
    SK_INIT(vkCmdBindIndexBuffer);
    SK_INIT(vkCmdBindPipeline);
    SK_INIT(vkCmdBindVertexBuffers);
    SK_INIT(vkCmdBlitImage);
    SK_INIT(vkCmdClearAttachments);
    SK_INIT(vkCmdClearColorImage);
    SK_INIT(vkCmdClearDepthStencilImage);
    SK_INIT(vkCmdCopyBuffer);
    SK_INIT(vkCmdCopyBufferToImage);
    SK_INIT(vkCmdCopyImage);
    SK_INIT(vkCmdCopyImageToBuffer);
    SK_INIT(vkCmdCopyQueryPoolResults);
    SK_INIT(vkCmdDispatch);
    SK_INIT(vkCmdDispatchIndirect);
    SK_INIT(vkCmdDraw);
    SK_INIT(vkCmdDrawIndexed);
    SK_INIT(vkCmdDrawIndexedIndirect);
    SK_INIT(vkCmdDrawIndirect);
    SK_INIT(vkCmdEndQuery);
    SK_INIT(vkCmdEndRenderPass);
    SK_INIT(vkCmdExecuteCommands);
    SK_INIT(vkCmdFillBuffer);
    SK_INIT(vkCmdNextSubpass);
    SK_INIT(vkCmdPipelineBarrier);
    SK_INIT(vkCmdPushConstants);
    SK_INIT(vkCmdResetEvent);
    SK_INIT(vkCmdResetQueryPool);
    SK_INIT(vkCmdResolveImage);
    SK_INIT(vkCmdSetBlendConstants);
    SK_INIT(vkCmdSetDepthBias);
    SK_INIT(vkCmdSetDepthBounds);
    SK_INIT(vkCmdSetEvent);
    SK_INIT(vkCmdSetLineWidth);
    SK_INIT(vkCmdSetScissor);
    SK_INIT(vkCmdSetStencilCompareMask);
    SK_INIT(vkCmdSetStencilReference);
    SK_INIT(vkCmdSetStencilWriteMask);
    SK_INIT(vkCmdSetViewport);
    SK_INIT(vkCmdUpdateBuffer);
    SK_INIT(vkCmdWaitEvents);
    SK_INIT(vkCmdWriteTimestamp);
    SK_INIT(vkCreateBuffer);
    SK_INIT(vkCreateBufferView);
    SK_INIT(vkCreateCommandPool);
    SK_INIT(vkCreateComputePipelines);
    SK_INIT(vkCreateDescriptorPool);
    SK_INIT(vkCreateDescriptorSetLayout);
    SK_INIT(vkCreateDevice);
    SK_INIT(vkCreateEvent);
    SK_INIT(vkCreateFence);
    SK_INIT(vkCreateFramebuffer);
    SK_INIT(vkCreateGraphicsPipelines);
    SK_INIT(vkCreateImage);
    SK_INIT(vkCreateImageView);
    SK_INIT(vkCreateInstance);
    SK_INIT(vkCreatePipelineCache);
    SK_INIT(vkCreatePipelineLayout);
    SK_INIT(vkCreateQueryPool);
    SK_INIT(vkCreateRenderPass);
    SK_INIT(vkCreateSampler);
    SK_INIT(vkCreateSemaphore);
    SK_INIT(vkCreateShaderModule);
    SK_INIT(vkDestroyBuffer);
    SK_INIT(vkDestroyBufferView);
    SK_INIT(vkDestroyCommandPool);
    SK_INIT(vkDestroyDescriptorPool);
    SK_INIT(vkDestroyDescriptorSetLayout);
    SK_INIT(vkDestroyDevice);
    SK_INIT(vkDestroyEvent);
    SK_INIT(vkDestroyFence);
    SK_INIT(vkDestroyFramebuffer);
    SK_INIT(vkDestroyImage);
    SK_INIT(vkDestroyImageView);
    SK_INIT(vkDestroyPipeline);
    SK_INIT(vkDestroyPipelineCache);
    SK_INIT(vkDestroyPipelineLayout);
    SK_INIT(vkDestroyQueryPool);
    SK_INIT(vkDestroyRenderPass);
    SK_INIT(vkDestroySampler);
    SK_INIT(vkDestroySemaphore);
    SK_INIT(vkDestroyShaderModule);
    SK_INIT(vkDeviceWaitIdle);
    SK_INIT(vkEndCommandBuffer);
    SK_INIT(vkEnumerateDeviceExtensionProperties);
    SK_INIT(vkEnumerateDeviceLayerProperties);
    SK_INIT(vkEnumeratePhysicalDevices);
    SK_INIT(vkFlushMappedMemoryRanges);
    SK_INIT(vkFreeCommandBuffers);
    SK_INIT(vkFreeDescriptorSets);
    SK_INIT(vkFreeMemory);
    SK_INIT(vkGetBufferMemoryRequirements);
    SK_INIT(vkGetDeviceMemoryCommitment);
    SK_INIT(vkGetDeviceProcAddr);
    SK_INIT(vkGetDeviceQueue);
    SK_INIT(vkGetEventStatus);
    SK_INIT(vkGetFenceStatus);
    SK_INIT(vkGetImageMemoryRequirements);
    SK_INIT(vkGetImageSparseMemoryRequirements);
    SK_INIT(vkGetImageSubresourceLayout);
    SK_INIT(vkGetInstanceProcAddr);
    SK_INIT(vkGetPhysicalDeviceFeatures);
    SK_INIT(vkGetPhysicalDeviceFormatProperties);
    SK_INIT(vkGetPhysicalDeviceImageFormatProperties);
    SK_INIT(vkGetPhysicalDeviceMemoryProperties);
    SK_INIT(vkGetPhysicalDeviceProperties);
    SK_INIT(vkGetPhysicalDeviceQueueFamilyProperties);
    SK_INIT(vkGetPhysicalDeviceSparseImageFormatProperties);
    SK_INIT(vkGetPipelineCacheData);
    SK_INIT(vkGetQueryPoolResults);
    SK_INIT(vkGetRenderAreaGranularity);
    SK_INIT(vkInvalidateMappedMemoryRanges);
    SK_INIT(vkMapMemory);
    SK_INIT(vkMergePipelineCaches);
    SK_INIT(vkQueueBindSparse);
    SK_INIT(vkQueueSubmit);
    SK_INIT(vkQueueWaitIdle);
    SK_INIT(vkResetCommandBuffer);
    SK_INIT(vkResetCommandPool);
    SK_INIT(vkResetDescriptorPool);
    SK_INIT(vkResetEvent);
    SK_INIT(vkResetFences);
    SK_INIT(vkSetEvent);
    SK_INIT(vkUnmapMemory);
    SK_INIT(vkUpdateDescriptorSets);
    SK_INIT(vkWaitForFences);

    // VK_EXT_debug_report
    SK_INIT(vkCreateDebugReportCallbackEXT);
    SK_INIT(vkDebugReportMessageEXT);
    SK_INIT(vkDestroyDebugReportCallbackEXT);

    // VK_KHR_surface
    SK_INIT(vkDestroySurfaceKHR);
    SK_INIT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    SK_INIT(vkGetPhysicalDeviceSurfaceFormatsKHR);
    SK_INIT(vkGetPhysicalDeviceSurfacePresentModesKHR);
    SK_INIT(vkGetPhysicalDeviceSurfaceSupportKHR);

    // VK_KHR_swapchain
    SK_INIT(vkAcquireNextImageKHR);
    SK_INIT(vkCreateSwapchainKHR);
    SK_INIT(vkDestroySwapchainKHR);
    SK_INIT(vkGetDeviceGroupPresentCapabilitiesKHR);
    SK_INIT(vkGetDeviceGroupSurfacePresentModesKHR);
    SK_INIT(vkGetPhysicalDevicePresentRectanglesKHR);
    SK_INIT(vkGetSwapchainImagesKHR);
    SK_INIT(vkQueuePresentKHR);

#undef SK_INIT

    return r;
  }

  VkResult init(const VulkanInitApi& initApi, const Instance& instance) noexcept
  {
    return init(initApi, instance, [](const char*) {});
  }

  template<class VkCommandBufferVector>
  VkResult allocateCommandBuffers(
    const Device&                          device,
    const VkCommandBufferAllocateInfo&     allocateInfo,
    CommandBuffers<VkCommandBufferVector>& commandBuffers) const noexcept
  {
    VkCommandBufferVector rawCommandBuffers =
      VkCommandBufferVector(allocateInfo.commandBufferCount);

    if (const VkResult r = vkAllocateCommandBuffers(
          device, &allocateInfo, rawCommandBuffers.data())) {
      return r;
    }

    commandBuffers = CommandBuffers<VkCommandBufferVector>{
      allocateInfo.commandBufferCount,
      std::move(rawCommandBuffers),
      PoolDeleter<VkCommandBuffer, VkCommandPool, void>{
        device,
        allocateInfo.commandPool,
        allocateInfo.commandBufferCount,
        vkFreeCommandBuffers}};
    return VK_SUCCESS;
  }

  template<class VkDescriptorSetVector>
  VkResult allocateDescriptorSets(
    const Device&                          device,
    const VkDescriptorSetAllocateInfo&     allocateInfo,
    DescriptorSets<VkDescriptorSetVector>& descriptorSets) const noexcept
  {
    auto descriptorSetVector =
      VkDescriptorSetVector(allocateInfo.descriptorSetCount);

    if (const VkResult r = vkAllocateDescriptorSets(
          device, &allocateInfo, descriptorSetVector.data())) {
      return r;
    }

    descriptorSets = DescriptorSets<VkDescriptorSetVector>{
      allocateInfo.descriptorSetCount,
      std::move(descriptorSetVector),
      PoolDeleter<VkDescriptorSet, VkDescriptorPool, VkResult>{
        device,
        allocateInfo.descriptorPool,
        allocateInfo.descriptorSetCount,
        vkFreeDescriptorSets}};
    return VK_SUCCESS;
  }

  VkResult bindBufferMemory(const Device&       device,
                            const Buffer&       buffer,
                            const DeviceMemory& memory,
                            VkDeviceSize        memoryOffset) const noexcept
  {
    return vkBindBufferMemory
             ? vkBindBufferMemory(device, buffer, memory, memoryOffset)
             : VK_ERROR_FEATURE_NOT_PRESENT;
  }

  VkResult createBuffer(const Device&             device,
                        const VkBufferCreateInfo& createInfo,
                        Buffer&                   buffer) const noexcept
  {
    VkBuffer       h = {};
    const VkResult r = vkCreateBuffer(device, &createInfo, nullptr, &h);
    return wrapResult(r, h, {device, vkDestroyBuffer}, buffer);
  }

  VkResult createBufferView(const Device&                 device,
                            const VkBufferViewCreateInfo& createInfo,
                            BufferView& bufferView) const noexcept
  {
    VkBufferView   h = {};
    const VkResult r = vkCreateBufferView(device, &createInfo, nullptr, &h);
    return wrapResult(r, h, {device, vkDestroyBufferView}, bufferView);
  }

  VkResult createCommandPool(const Device&                  device,
                             const VkCommandPoolCreateInfo& createInfo,
                             CommandPool& commandPool) const noexcept
  {
    VkCommandPool  h = {};
    const VkResult r = vkCreateCommandPool(device, &createInfo, nullptr, &h);
    return wrapResult(r, h, {device, vkDestroyCommandPool}, commandPool);
  }

  VkResult createDescriptorPool(const Device&                     device,
                                const VkDescriptorPoolCreateInfo& createInfo,
                                DescriptorPool& descriptorPool) const noexcept
  {
    VkDescriptorPool h = {};
    const VkResult r = vkCreateDescriptorPool(device, &createInfo, nullptr, &h);

    return wrapResult(r, h, {device, vkDestroyDescriptorPool}, descriptorPool);
  }

  VkResult createDescriptorSetLayout(
    const Device&                          device,
    const VkDescriptorSetLayoutCreateInfo& createInfo,
    DescriptorSetLayout&                   descriptorSetLayout) const noexcept
  {
    VkDescriptorSetLayout h = {};
    const VkResult        r =
      vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &h);

    return wrapResult(
      r, h, {device, vkDestroyDescriptorSetLayout}, descriptorSetLayout);
  }

  VkResult createDevice(const PhysicalDevice&     physicalDevice,
                        const VkDeviceCreateInfo& createInfo,
                        Device&                   result) const noexcept
  {
    VkDevice       h = {};
    const VkResult r = vkCreateDevice(physicalDevice, &createInfo, nullptr, &h);

    return wrapResult(r, h, {vkDestroyDevice}, result);
  }

  VkResult createEvent(const Device&            device,
                       const VkEventCreateInfo& createInfo,
                       Event&                   event) const noexcept
  {
    VkEvent        h = {};
    const VkResult r = vkCreateEvent(device, &createInfo, nullptr, &h);

    return wrapResult(r, h, {device, vkDestroyEvent}, event);
  }

  VkResult createFence(const Device&            device,
                       const VkFenceCreateInfo& createInfo,
                       Fence&                   fence) const noexcept
  {
    VkFence        h = {};
    const VkResult r = vkCreateFence(device, &createInfo, nullptr, &h);

    return wrapResult(r, h, {device, vkDestroyFence}, fence);
  }

  VkResult createFramebuffer(const Device&                  device,
                             const VkFramebufferCreateInfo& createInfo,
                             Framebuffer& framebuffer) const noexcept
  {
    VkFramebuffer  h = {};
    const VkResult r = vkCreateFramebuffer(device, &createInfo, nullptr, &h);

    return wrapResult(r, h, {device, vkDestroyFramebuffer}, framebuffer);
  }

  VkResult createImage(const Device&            device,
                       const VkImageCreateInfo& createInfo,
                       Image&                   image) const noexcept
  {
    VkImage        h = {};
    const VkResult r = vkCreateImage(device, &createInfo, nullptr, &h);

    return wrapResult(r, h, {device, vkDestroyImage}, image);
  }

  VkResult createImageView(const Device&                device,
                           const VkImageViewCreateInfo& createInfo,
                           ImageView& imageView) const noexcept
  {
    VkImageView    h = {};
    const VkResult r = vkCreateImageView(device, &createInfo, nullptr, &h);

    return wrapResult(r, h, {device, vkDestroyImageView}, imageView);
  }

  template<size_t count>
  VkResult createGraphicsPipelines(
    const Device&                                          device,
    const OptionalParameter<PipelineCache>&                pipelineCache,
    const std::array<VkGraphicsPipelineCreateInfo, count>& createInfos,
    std::array<Pipeline, count>& pipelines) const noexcept
  {
    std::array<VkPipeline, count> pipelineHandles{};

    if (const VkResult r =
          vkCreateGraphicsPipelines(device,
                                    pipelineCache.get(),
                                    static_cast<uint32_t>(createInfos.size()),
                                    createInfos.data(),
                                    nullptr,
                                    pipelineHandles.data())) {
      return r;
    }

    pipelines = make_handle_array<Pipeline>(
      device.get(), vkDestroyPipeline, pipelineHandles);
    return VK_SUCCESS;
  }

  VkResult createPipelineCache(const Device&                    device,
                               const VkPipelineCacheCreateInfo& createInfo,
                               PipelineCache& pipelineCache) const noexcept
  {
    VkPipelineCache h = {};
    const VkResult  r = vkCreatePipelineCache(device, &createInfo, nullptr, &h);

    return wrapResult(r, h, {device, vkDestroyPipelineCache}, pipelineCache);
  }

  VkResult createPipelineLayout(const Device&                     device,
                                const VkPipelineLayoutCreateInfo& createInfo,
                                PipelineLayout& pipelineLayout) const noexcept
  {
    VkPipelineLayout h = {};
    const VkResult r = vkCreatePipelineLayout(device, &createInfo, nullptr, &h);

    return wrapResult(r, h, {device, vkDestroyPipelineLayout}, pipelineLayout);
  }

  VkResult createQueryPool(const Device&                device,
                           const VkQueryPoolCreateInfo& createInfo,
                           QueryPool& queryPool) const noexcept
  {
    VkQueryPool    h = {};
    const VkResult r = vkCreateQueryPool(device, &createInfo, nullptr, &h);

    return wrapResult(r, h, {device, vkDestroyQueryPool}, queryPool);
  }

  VkResult createRenderPass(const Device&                 device,
                            const VkRenderPassCreateInfo& createInfo,
                            RenderPass& renderPass) const noexcept
  {
    VkRenderPass   h = {};
    const VkResult r = vkCreateRenderPass(device, &createInfo, nullptr, &h);

    return wrapResult(r, h, {device, vkDestroyRenderPass}, renderPass);
  }

  VkResult createSampler(const Device&              device,
                         const VkSamplerCreateInfo& createInfo,
                         Sampler&                   sampler) const noexcept
  {
    VkSampler      h = {};
    const VkResult r = vkCreateSampler(device, &createInfo, nullptr, &h);

    return wrapResult(r, h, {device, vkDestroySampler}, sampler);
  }

  VkResult createSemaphore(const Device&                device,
                           const VkSemaphoreCreateInfo& createInfo,
                           Semaphore& semaphore) const noexcept
  {
    VkSemaphore    h = {};
    const VkResult r = vkCreateSemaphore(device, &createInfo, nullptr, &h);

    return wrapResult(r, h, {device, vkDestroySemaphore}, semaphore);
  }

  VkResult createShaderModule(const Device&                   device,
                              const VkShaderModuleCreateInfo& createInfo,
                              ShaderModule& shaderModule) const noexcept
  {
    VkShaderModule h = {};
    const VkResult r = vkCreateShaderModule(device, &createInfo, nullptr, &h);

    return wrapResult(r, h, {device, vkDestroyShaderModule}, shaderModule);
  }

  VkResult deviceWaitIdle(const Device& device) const noexcept
  {
    return vkDeviceWaitIdle(device);
  }

  template<class Vector>
  VkResult enumerateDeviceExtensionProperties(
    const PhysicalDevice& physicalDevice,
    const char* const     layerName,
    Vector&               properties) const noexcept
  {
    return detail::wrapVectorAccessor<VkExtensionProperties>(
      properties,
      vkEnumerateDeviceExtensionProperties,
      physicalDevice,
      layerName);
  }

  template<class Vector>
  VkResult enumerateDeviceExtensionProperties(
    const PhysicalDevice& physicalDevice,
    Vector&               properties) const noexcept
  {
    return detail::wrapVectorAccessor<VkExtensionProperties>(
      properties,
      vkEnumerateDeviceExtensionProperties,
      physicalDevice,
      nullptr);
  }

  template<class Vector>
  VkResult enumeratePhysicalDevices(const Instance& instance,
                                    Vector& physicalDevices) const noexcept
  {
    uint32_t count = 0U;
    VkResult r     = vkEnumeratePhysicalDevices(instance, &count, nullptr);
    if (r > VK_INCOMPLETE) {
      return r;
    }

    physicalDevices = Vector(count);
    if ((r = vkEnumeratePhysicalDevices(
           instance, &count, physicalDevices.data()))) {
      return r;
    }

    return VK_SUCCESS;
  }

  sk::Queue getDeviceQueue(const Device&  device,
                           const uint32_t queueFamilyIndex,
                           const uint32_t queueIndex) const noexcept
  {
    VkQueue queue{};
    vkGetDeviceQueue(device, queueFamilyIndex, queueIndex, &queue);
    return sk::Queue{queue};
  }

  VkPhysicalDeviceMemoryProperties getPhysicalDeviceMemoryProperties(
    VkPhysicalDevice physicalDevice) const noexcept
  {
    VkPhysicalDeviceMemoryProperties properties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &properties);
    return properties;
  }

  VkPhysicalDeviceProperties getPhysicalDeviceProperties(
    const PhysicalDevice& physicalDevice) const noexcept
  {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    return properties;
  }

  template<class Vector>
  VkResult getPhysicalDeviceQueueFamilyProperties(
    const PhysicalDevice& physicalDevice,
    Vector&               queueFamilyProperties) const noexcept
  {
    uint32_t count = 0U;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);

    queueFamilyProperties = Vector(count);
    vkGetPhysicalDeviceQueueFamilyProperties(
      physicalDevice, &count, queueFamilyProperties.data());

    return VK_SUCCESS;
  }

  VkMemoryRequirements getBufferMemoryRequirements(
    const Device& device,
    const Buffer& buffer) const noexcept
  {
    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(device, buffer, &requirements);
    return requirements;
  }

  VkResult allocateMemory(const Device&               device,
                          const VkMemoryAllocateInfo& info,
                          DeviceMemory&               memory) const noexcept
  {
    VkDeviceMemory h = {};
    if (const VkResult r = vkAllocateMemory(device, &info, nullptr, &h)) {
      return r;
    }

    if (!h) {
      return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    }

    memory = DeviceMemory{h, {device, vkFreeMemory}};
    return VK_SUCCESS;
  }

  VkResult mapMemory(const Device&       device,
                     const DeviceMemory& memory,
                     VkDeviceSize        offset,
                     VkDeviceSize        size,
                     VkMemoryMapFlags    flags,
                     MappedMemory&       mappedMemory) const noexcept
  {
    void* data = nullptr;
    if (const VkResult r =
          vkMapMemory(device, memory, offset, size, flags, &data)) {
      return r;
    }

    mappedMemory = MappedMemory{*this, device, memory, data};
    return VK_SUCCESS;
  }

  VkResult queueSubmit(const Queue&        queue,
                       uint32_t            submitCount,
                       const VkSubmitInfo& submits,
                       const Fence&        fence) const noexcept
  {
    return vkQueueSubmit(queue, submitCount, &submits, fence);
  }

  VkResult queueSubmit(const Queue&        queue,
                       const VkSubmitInfo& submit,
                       const Fence&        fence) const noexcept
  {
    return vkQueueSubmit(queue, 1U, &submit, fence);
  }

  template<size_t descriptorWriteCount, size_t descriptorCopyCount>
  void updateDescriptorSets(
    const Device&                                          device,
    std::array<VkWriteDescriptorSet, descriptorWriteCount> descriptorWrites,
    std::array<VkCopyDescriptorSet, descriptorCopyCount>   descriptorCopies)
    const noexcept
  {
    vkUpdateDescriptorSets(device,
                           static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(),
                           static_cast<uint32_t>(descriptorCopies.size()),
                           descriptorCopies.data());
  }

  VkResult resetFence(const Device& device, const Fence& fence) const noexcept
  {
    VkFence h = fence;
    return vkResetFences(device, 1U, &h);
  }

  VkResult waitForFence(const Device& device,
                        const Fence&  fence,
                        uint64_t      timeout) const noexcept
  {
    VkFence h = fence;
    return vkWaitForFences(device, 1U, &h, VK_TRUE, timeout);
  }

  VkResult waitForFence(const Device& device, const Fence& fence) const noexcept
  {
    VkFence h = fence;
    return vkWaitForFences(device, 1U, &h, VK_TRUE, UINT64_MAX);
  }

  // Scoped command buffer interface
  SYBOK_NODISCARD
  CommandScope beginCommandBuffer(
    VkCommandBuffer          commandBuffer,
    VkCommandBufferBeginInfo beginInfo) const noexcept;

  // VK_EXT_debug_report

  VkResult createDebugReportCallbackEXT(
    const Instance&                           instance,
    const VkDebugReportCallbackCreateInfoEXT& createInfo,
    DebugReportCallbackEXT&                   callback) const noexcept
  {
    VkDebugReportCallbackEXT h = {};

    if (const VkResult r =
          vkCreateDebugReportCallbackEXT(instance, &createInfo, nullptr, &h)) {
      return r;
    }

    if (!h) {
      return VK_ERROR_FEATURE_NOT_PRESENT;
    }

    callback = {h, {instance, vkDestroyDebugReportCallbackEXT}};
    return VK_SUCCESS;
  }

  // VK_KHR_surface

  VkResult getPhysicalDeviceSurfaceCapabilitiesKHR(
    const PhysicalDevice&     physicalDevice,
    const SurfaceKHR&         surface,
    VkSurfaceCapabilitiesKHR& capabilities) const noexcept
  {
    return vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      physicalDevice, surface, &capabilities);
  }

  template<typename Vector>
  VkResult getPhysicalDeviceSurfaceFormatsKHR(
    const PhysicalDevice& physicalDevice,
    const SurfaceKHR&     surface,
    Vector&               surfaceFormats) const noexcept
  {
    return detail::wrapVectorAccessor<VkSurfaceFormatKHR>(
      surfaceFormats,
      vkGetPhysicalDeviceSurfaceFormatsKHR,
      physicalDevice,
      surface.get());
  }

  template<typename Vector>
  VkResult getPhysicalDeviceSurfacePresentModesKHR(
    const PhysicalDevice& physicalDevice,
    const SurfaceKHR&     surface,
    Vector&               presentModes) const noexcept
  {
    return detail::wrapVectorAccessor<VkPresentModeKHR>(
      presentModes,
      vkGetPhysicalDeviceSurfacePresentModesKHR,
      physicalDevice,
      surface.get());
  }

  VkResult getPhysicalDeviceSurfaceSupportKHR(
    const PhysicalDevice& physicalDevice,
    uint32_t              queueFamilyIndex,
    const SurfaceKHR&     surface,
    bool&                 supported) const noexcept
  {
    VkBool32 s = {};

    if (VkResult r = vkGetPhysicalDeviceSurfaceSupportKHR(
          physicalDevice, queueFamilyIndex, surface, &s)) {
      return r;
    }

    supported = s;
    return VK_SUCCESS;
  }

  // VK_KHR_swapchain

  VkResult acquireNextImageKHR(const Device&                   device,
                               const SwapchainKHR&             swapchain,
                               uint64_t                        timeout,
                               const Semaphore&                semaphore,
                               const OptionalParameter<Fence>& fence,
                               uint32_t* pImageIndex) const noexcept
  {
    return vkAcquireNextImageKHR(
      device, swapchain, timeout, semaphore, fence.get(), pImageIndex);
  }

  template<class Vector>
  VkResult getSwapchainImagesKHR(const Device&       device,
                                 const SwapchainKHR& swapchain,
                                 Vector&             images) const noexcept
  {
    return detail::wrapVectorAccessor<VkImage>(
      images, vkGetSwapchainImagesKHR, device.get(), swapchain.get());
  }

  VkResult createSwapchainKHR(const Device&                   device,
                              const VkSwapchainCreateInfoKHR& createInfo,
                              SwapchainKHR& swapchain) const noexcept
  {
    VkSwapchainKHR h = {};
    const VkResult r = vkCreateSwapchainKHR(device, &createInfo, nullptr, &h);

    if (r) {
      return r;
    }

    if (!h) {
      return VK_ERROR_INCOMPATIBLE_DRIVER;
    }

    swapchain = {h, {device, vkDestroySwapchainKHR}};
    return VK_SUCCESS;
  }

  VkResult queuePresentKHR(const Queue&            queue,
                           const VkPresentInfoKHR& presentInfo) const noexcept
  {
    return vkQueuePresentKHR(queue, &presentInfo);
  }

#define SK_FUNC(name) \
  PFN_##name name {} // NOLINT

  // Vulkan 1.0 Core
  SK_FUNC(vkAllocateCommandBuffers);
  SK_FUNC(vkAllocateDescriptorSets);
  SK_FUNC(vkAllocateMemory);
  SK_FUNC(vkBeginCommandBuffer);
  SK_FUNC(vkBindBufferMemory);
  SK_FUNC(vkBindImageMemory);
  SK_FUNC(vkCmdBeginQuery);
  SK_FUNC(vkCmdBeginRenderPass);
  SK_FUNC(vkCmdBindDescriptorSets);
  SK_FUNC(vkCmdBindIndexBuffer);
  SK_FUNC(vkCmdBindPipeline);
  SK_FUNC(vkCmdBindVertexBuffers);
  SK_FUNC(vkCmdBlitImage);
  SK_FUNC(vkCmdClearAttachments);
  SK_FUNC(vkCmdClearColorImage);
  SK_FUNC(vkCmdClearDepthStencilImage);
  SK_FUNC(vkCmdCopyBuffer);
  SK_FUNC(vkCmdCopyBufferToImage);
  SK_FUNC(vkCmdCopyImage);
  SK_FUNC(vkCmdCopyImageToBuffer);
  SK_FUNC(vkCmdCopyQueryPoolResults);
  SK_FUNC(vkCmdDispatch);
  SK_FUNC(vkCmdDispatchIndirect);
  SK_FUNC(vkCmdDraw);
  SK_FUNC(vkCmdDrawIndexed);
  SK_FUNC(vkCmdDrawIndexedIndirect);
  SK_FUNC(vkCmdDrawIndirect);
  SK_FUNC(vkCmdEndQuery);
  SK_FUNC(vkCmdEndRenderPass);
  SK_FUNC(vkCmdExecuteCommands);
  SK_FUNC(vkCmdFillBuffer);
  SK_FUNC(vkCmdNextSubpass);
  SK_FUNC(vkCmdPipelineBarrier);
  SK_FUNC(vkCmdPushConstants);
  SK_FUNC(vkCmdResetEvent);
  SK_FUNC(vkCmdResetQueryPool);
  SK_FUNC(vkCmdResolveImage);
  SK_FUNC(vkCmdSetBlendConstants);
  SK_FUNC(vkCmdSetDepthBias);
  SK_FUNC(vkCmdSetDepthBounds);
  SK_FUNC(vkCmdSetEvent);
  SK_FUNC(vkCmdSetLineWidth);
  SK_FUNC(vkCmdSetScissor);
  SK_FUNC(vkCmdSetStencilCompareMask);
  SK_FUNC(vkCmdSetStencilReference);
  SK_FUNC(vkCmdSetStencilWriteMask);
  SK_FUNC(vkCmdSetViewport);
  SK_FUNC(vkCmdUpdateBuffer);
  SK_FUNC(vkCmdWaitEvents);
  SK_FUNC(vkCmdWriteTimestamp);
  SK_FUNC(vkCreateBuffer);
  SK_FUNC(vkCreateBufferView);
  SK_FUNC(vkCreateCommandPool);
  SK_FUNC(vkCreateComputePipelines);
  SK_FUNC(vkCreateDescriptorPool);
  SK_FUNC(vkCreateDescriptorSetLayout);
  SK_FUNC(vkCreateDevice);
  SK_FUNC(vkCreateEvent);
  SK_FUNC(vkCreateFence);
  SK_FUNC(vkCreateFramebuffer);
  SK_FUNC(vkCreateGraphicsPipelines);
  SK_FUNC(vkCreateImage);
  SK_FUNC(vkCreateImageView);
  SK_FUNC(vkCreateInstance);
  SK_FUNC(vkCreatePipelineCache);
  SK_FUNC(vkCreatePipelineLayout);
  SK_FUNC(vkCreateQueryPool);
  SK_FUNC(vkCreateRenderPass);
  SK_FUNC(vkCreateSampler);
  SK_FUNC(vkCreateSemaphore);
  SK_FUNC(vkCreateShaderModule);
  SK_FUNC(vkDestroyBuffer);
  SK_FUNC(vkDestroyBufferView);
  SK_FUNC(vkDestroyCommandPool);
  SK_FUNC(vkDestroyDescriptorPool);
  SK_FUNC(vkDestroyDescriptorSetLayout);
  SK_FUNC(vkDestroyDevice);
  SK_FUNC(vkDestroyEvent);
  SK_FUNC(vkDestroyFence);
  SK_FUNC(vkDestroyFramebuffer);
  SK_FUNC(vkDestroyImage);
  SK_FUNC(vkDestroyImageView);
  SK_FUNC(vkDestroyPipeline);
  SK_FUNC(vkDestroyPipelineCache);
  SK_FUNC(vkDestroyPipelineLayout);
  SK_FUNC(vkDestroyQueryPool);
  SK_FUNC(vkDestroyRenderPass);
  SK_FUNC(vkDestroySampler);
  SK_FUNC(vkDestroySemaphore);
  SK_FUNC(vkDestroyShaderModule);
  SK_FUNC(vkDeviceWaitIdle);
  SK_FUNC(vkEndCommandBuffer);
  SK_FUNC(vkEnumerateDeviceExtensionProperties);
  SK_FUNC(vkEnumerateDeviceLayerProperties);
  SK_FUNC(vkEnumeratePhysicalDevices);
  SK_FUNC(vkFlushMappedMemoryRanges);
  SK_FUNC(vkFreeCommandBuffers);
  SK_FUNC(vkFreeDescriptorSets);
  SK_FUNC(vkFreeMemory);
  SK_FUNC(vkGetBufferMemoryRequirements);
  SK_FUNC(vkGetDeviceMemoryCommitment);
  SK_FUNC(vkGetDeviceProcAddr);
  SK_FUNC(vkGetDeviceQueue);
  SK_FUNC(vkGetEventStatus);
  SK_FUNC(vkGetFenceStatus);
  SK_FUNC(vkGetImageMemoryRequirements);
  SK_FUNC(vkGetImageSparseMemoryRequirements);
  SK_FUNC(vkGetImageSubresourceLayout);
  SK_FUNC(vkGetInstanceProcAddr);
  SK_FUNC(vkGetPhysicalDeviceFeatures);
  SK_FUNC(vkGetPhysicalDeviceFormatProperties);
  SK_FUNC(vkGetPhysicalDeviceImageFormatProperties);
  SK_FUNC(vkGetPhysicalDeviceMemoryProperties);
  SK_FUNC(vkGetPhysicalDeviceProperties);
  SK_FUNC(vkGetPhysicalDeviceQueueFamilyProperties);
  SK_FUNC(vkGetPhysicalDeviceSparseImageFormatProperties);
  SK_FUNC(vkGetPipelineCacheData);
  SK_FUNC(vkGetQueryPoolResults);
  SK_FUNC(vkGetRenderAreaGranularity);
  SK_FUNC(vkInvalidateMappedMemoryRanges);
  SK_FUNC(vkMapMemory);
  SK_FUNC(vkMergePipelineCaches);
  SK_FUNC(vkQueueBindSparse);
  SK_FUNC(vkQueueSubmit);
  SK_FUNC(vkQueueWaitIdle);
  SK_FUNC(vkResetCommandBuffer);
  SK_FUNC(vkResetCommandPool);
  SK_FUNC(vkResetDescriptorPool);
  SK_FUNC(vkResetEvent);
  SK_FUNC(vkResetFences);
  SK_FUNC(vkSetEvent);
  SK_FUNC(vkUnmapMemory);
  SK_FUNC(vkUpdateDescriptorSets);
  SK_FUNC(vkWaitForFences);

  // VK_EXT_debug_report
  SK_FUNC(vkCreateDebugReportCallbackEXT);
  SK_FUNC(vkDebugReportMessageEXT);
  SK_FUNC(vkDestroyDebugReportCallbackEXT);

  // VK_KHR_surface
  SK_FUNC(vkDestroySurfaceKHR);
  SK_FUNC(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
  SK_FUNC(vkGetPhysicalDeviceSurfaceFormatsKHR);
  SK_FUNC(vkGetPhysicalDeviceSurfacePresentModesKHR);
  SK_FUNC(vkGetPhysicalDeviceSurfaceSupportKHR);

  // VK_KHR_swapchain
  SK_FUNC(vkAcquireNextImageKHR);
  SK_FUNC(vkCreateSwapchainKHR);
  SK_FUNC(vkDestroySwapchainKHR);
  SK_FUNC(vkGetDeviceGroupPresentCapabilitiesKHR);
  SK_FUNC(vkGetDeviceGroupSurfacePresentModesKHR);
  SK_FUNC(vkGetPhysicalDevicePresentRectanglesKHR);
  SK_FUNC(vkGetSwapchainImagesKHR);
  SK_FUNC(vkQueuePresentKHR);

#undef SK_FUNC

private:
  template<class T>
  static inline VkResult wrapResult(const VkResult           r,
                                    const typename T::Handle handle,
                                    typename T::Deleter&&    deleter,
                                    T&                       result) noexcept
  {
    if (r) {
      return r;
    }

    if (!handle) {
      return VK_ERROR_INITIALIZATION_FAILED;
    }

    result = T{handle, std::move(deleter)};
    return VK_SUCCESS;
  }
};

/// Scope for commands that work both inside and outside a render pass
class CommonCommandScope
{
public:
  CommonCommandScope(const VulkanApi& api,
                     VkCommandBuffer  commandBuffer,
                     VkResult         result) noexcept
    : _api{api}
    , _commandBuffer{commandBuffer}
    , _result{result}
  {}

  CommonCommandScope(const CommonCommandScope&) noexcept            = delete;
  CommonCommandScope& operator=(const CommonCommandScope&) noexcept = delete;

  CommonCommandScope(CommonCommandScope&& scope) noexcept
    : _api{scope._api}
    , _commandBuffer{scope._commandBuffer}
    , _result{scope._result}
  {
    scope._commandBuffer = {};
  }

  CommonCommandScope& operator=(CommonCommandScope&&) = delete;

  ~CommonCommandScope() noexcept = default;

  explicit operator bool() const noexcept { return _result == VK_SUCCESS; }

  VkResult error() const noexcept { return _result; }

  void bindPipeline(VkPipelineBindPoint pipelineBindPoint,
                    VkPipeline          pipeline) const noexcept
  {
    _api.vkCmdBindPipeline(_commandBuffer, pipelineBindPoint, pipeline);
  }

  void setViewport(uint32_t          firstViewport,
                   uint32_t          viewportCount,
                   const VkViewport* pViewports) const noexcept
  {
    _api.vkCmdSetViewport(
      _commandBuffer, firstViewport, viewportCount, pViewports);
  }

  void setScissor(uint32_t        firstScissor,
                  uint32_t        scissorCount,
                  const VkRect2D* pScissors) const noexcept
  {
    _api.vkCmdSetScissor(_commandBuffer, firstScissor, scissorCount, pScissors);
  }

  void setLineWidth(float lineWidth) const noexcept
  {
    _api.vkCmdSetLineWidth(_commandBuffer, lineWidth);
  }

  void setDepthBias(float depthBiasConstantFactor,
                    float depthBiasClamp,
                    float depthBiasSlopeFactor) const noexcept
  {
    _api.vkCmdSetDepthBias(_commandBuffer,
                           depthBiasConstantFactor,
                           depthBiasClamp,
                           depthBiasSlopeFactor);
  }

  void setBlendConstants(const float blendConstants[4]) const noexcept
  {
    _api.vkCmdSetBlendConstants(_commandBuffer, blendConstants);
  }

  void setDepthBounds(float minDepthBounds, float maxDepthBounds) const noexcept
  {
    _api.vkCmdSetDepthBounds(_commandBuffer, minDepthBounds, maxDepthBounds);
  }

  void setStencilCompareMask(VkStencilFaceFlags faceMask,
                             uint32_t           compareMask) const noexcept
  {
    _api.vkCmdSetStencilCompareMask(_commandBuffer, faceMask, compareMask);
  }

  void setStencilWriteMask(VkStencilFaceFlags faceMask,
                           uint32_t           writeMask) const noexcept
  {
    _api.vkCmdSetStencilWriteMask(_commandBuffer, faceMask, writeMask);
  }

  void setStencilReference(VkStencilFaceFlags faceMask,
                           uint32_t           reference) const noexcept
  {
    _api.vkCmdSetStencilReference(_commandBuffer, faceMask, reference);
  }

  void bindDescriptorSets(VkPipelineBindPoint    pipelineBindPoint,
                          VkPipelineLayout       layout,
                          uint32_t               firstSet,
                          uint32_t               descriptorSetCount,
                          const VkDescriptorSet* pDescriptorSets,
                          uint32_t               dynamicOffsetCount,
                          const uint32_t*        pDynamicOffsets) const noexcept
  {
    _api.vkCmdBindDescriptorSets(_commandBuffer,
                                 pipelineBindPoint,
                                 layout,
                                 firstSet,
                                 descriptorSetCount,
                                 pDescriptorSets,
                                 dynamicOffsetCount,
                                 pDynamicOffsets);
  }

  void bindIndexBuffer(VkBuffer     buffer,
                       VkDeviceSize offset,
                       VkIndexType  indexType) const noexcept
  {
    _api.vkCmdBindIndexBuffer(_commandBuffer, buffer, offset, indexType);
  }

  void bindVertexBuffers(uint32_t            firstBinding,
                         uint32_t            bindingCount,
                         const VkBuffer*     pBuffers,
                         const VkDeviceSize* pOffsets) const noexcept
  {
    _api.vkCmdBindVertexBuffers(
      _commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets);
  }

  void waitEvents(
    uint32_t                     eventCount,
    const VkEvent*               pEvents,
    VkPipelineStageFlags         srcStageMask,
    VkPipelineStageFlags         dstStageMask,
    uint32_t                     memoryBarrierCount,
    const VkMemoryBarrier*       pMemoryBarriers,
    uint32_t                     bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier* pBufferMemoryBarriers,
    uint32_t                     imageMemoryBarrierCount,
    const VkImageMemoryBarrier*  pImageMemoryBarriers) const noexcept
  {
    _api.vkCmdWaitEvents(_commandBuffer,
                         eventCount,
                         pEvents,
                         srcStageMask,
                         dstStageMask,
                         memoryBarrierCount,
                         pMemoryBarriers,
                         bufferMemoryBarrierCount,
                         pBufferMemoryBarriers,
                         imageMemoryBarrierCount,
                         pImageMemoryBarriers);
  }

  void pipelineBarrier(
    VkPipelineStageFlags         srcStageMask,
    VkPipelineStageFlags         dstStageMask,
    VkDependencyFlags            dependencyFlags,
    uint32_t                     memoryBarrierCount,
    const VkMemoryBarrier*       pMemoryBarriers,
    uint32_t                     bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier* pBufferMemoryBarriers,
    uint32_t                     imageMemoryBarrierCount,
    const VkImageMemoryBarrier*  pImageMemoryBarriers) const noexcept
  {
    _api.vkCmdPipelineBarrier(_commandBuffer,
                              srcStageMask,
                              dstStageMask,
                              dependencyFlags,
                              memoryBarrierCount,
                              pMemoryBarriers,
                              bufferMemoryBarrierCount,
                              pBufferMemoryBarriers,
                              imageMemoryBarrierCount,
                              pImageMemoryBarriers);
  }

  void beginQuery(VkQueryPool         queryPool,
                  uint32_t            query,
                  VkQueryControlFlags flags) const noexcept
  {
    _api.vkCmdBeginQuery(_commandBuffer, queryPool, query, flags);
  }

  void endQuery(VkQueryPool queryPool, uint32_t query) const noexcept
  {
    _api.vkCmdEndQuery(_commandBuffer, queryPool, query);
  }

  void writeTimestamp(VkPipelineStageFlagBits pipelineStage,
                      VkQueryPool             queryPool,
                      uint32_t                query) const noexcept
  {
    _api.vkCmdWriteTimestamp(_commandBuffer, pipelineStage, queryPool, query);
  }

  void pushConstants(VkPipelineLayout   layout,
                     VkShaderStageFlags stageFlags,
                     uint32_t           offset,
                     uint32_t           size,
                     const void*        pValues) const noexcept
  {
    _api.vkCmdPushConstants(
      _commandBuffer, layout, stageFlags, offset, size, pValues);
  }

  void executeCommands(uint32_t               commandBufferCount,
                       const VkCommandBuffer* pCommandBuffers) const noexcept
  {
    _api.vkCmdExecuteCommands(
      _commandBuffer, commandBufferCount, pCommandBuffers);
  }

protected:
  const VulkanApi& _api;
  VkCommandBuffer  _commandBuffer;
  VkResult         _result;
};

// Top level command scope outside a render pass
class CommandScope : public CommonCommandScope
{
public:
  CommandScope(const VulkanApi& api,
               VkCommandBuffer  commandBuffer,
               VkResult         result) noexcept
    : CommonCommandScope{api, commandBuffer, result}
  {}

  CommandScope(const CommandScope&)            = delete;
  CommandScope& operator=(const CommandScope&) = delete;

  CommandScope(CommandScope&& scope) noexcept
    : CommonCommandScope{std::forward<CommandScope>(scope)}
  {}

  CommandScope& operator=(CommandScope&&) = delete;

  ~CommandScope() noexcept
  {
    assert(!_commandBuffer); // Buffer must be finished with end()
  }

  VkResult end() noexcept
  {
    if (_commandBuffer) {
      VkResult r     = _api.vkEndCommandBuffer(_commandBuffer);
      _commandBuffer = {};
      return r;
    }

    return VK_NOT_READY;
  }

  void dispatch(uint32_t groupCountX,
                uint32_t groupCountY,
                uint32_t groupCountZ) const noexcept
  {
    _api.vkCmdDispatch(_commandBuffer, groupCountX, groupCountY, groupCountZ);
  }

  void dispatchIndirect(VkBuffer buffer, VkDeviceSize offset) const noexcept
  {
    _api.vkCmdDispatchIndirect(_commandBuffer, buffer, offset);
  }

  void copyBuffer(VkBuffer            srcBuffer,
                  VkBuffer            dstBuffer,
                  uint32_t            regionCount,
                  const VkBufferCopy* pRegions) const noexcept
  {
    _api.vkCmdCopyBuffer(
      _commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions);
  }

  void copyImage(VkImage            srcImage,
                 VkImageLayout      srcImageLayout,
                 VkImage            dstImage,
                 VkImageLayout      dstImageLayout,
                 uint32_t           regionCount,
                 const VkImageCopy* pRegions) const noexcept
  {
    _api.vkCmdCopyImage(_commandBuffer,
                        srcImage,
                        srcImageLayout,
                        dstImage,
                        dstImageLayout,
                        regionCount,
                        pRegions);
  }

  void blitImage(VkImage            srcImage,
                 VkImageLayout      srcImageLayout,
                 VkImage            dstImage,
                 VkImageLayout      dstImageLayout,
                 uint32_t           regionCount,
                 const VkImageBlit* pRegions,
                 VkFilter           filter) const noexcept
  {
    _api.vkCmdBlitImage(_commandBuffer,
                        srcImage,
                        srcImageLayout,
                        dstImage,
                        dstImageLayout,
                        regionCount,
                        pRegions,
                        filter);
  }

  void copyBufferToImage(VkBuffer                 srcBuffer,
                         VkImage                  dstImage,
                         VkImageLayout            dstImageLayout,
                         uint32_t                 regionCount,
                         const VkBufferImageCopy* pRegions) const noexcept
  {
    _api.vkCmdCopyBufferToImage(_commandBuffer,
                                srcBuffer,
                                dstImage,
                                dstImageLayout,
                                regionCount,
                                pRegions);
  }

  void copyImageToBuffer(VkImage                  srcImage,
                         VkImageLayout            srcImageLayout,
                         VkBuffer                 dstBuffer,
                         uint32_t                 regionCount,
                         const VkBufferImageCopy* pRegions) const noexcept
  {
    _api.vkCmdCopyImageToBuffer(_commandBuffer,
                                srcImage,
                                srcImageLayout,
                                dstBuffer,
                                regionCount,
                                pRegions);
  }

  void updateBuffer(VkBuffer     dstBuffer,
                    VkDeviceSize dstOffset,
                    VkDeviceSize dataSize,
                    const void*  pData) const noexcept
  {
    _api.vkCmdUpdateBuffer(
      _commandBuffer, dstBuffer, dstOffset, dataSize, pData);
  }

  void fillBuffer(VkBuffer     dstBuffer,
                  VkDeviceSize dstOffset,
                  VkDeviceSize size,
                  uint32_t     data) const noexcept
  {
    _api.vkCmdFillBuffer(_commandBuffer, dstBuffer, dstOffset, size, data);
  }

  void clearColorImage(VkImage                        image,
                       VkImageLayout                  imageLayout,
                       const VkClearColorValue&       color,
                       uint32_t                       rangeCount,
                       const VkImageSubresourceRange* pRanges) const noexcept
  {
    _api.vkCmdClearColorImage(
      _commandBuffer, image, imageLayout, &color, rangeCount, pRanges);
  }

  void clearDepthStencilImage(
    VkImage                         image,
    VkImageLayout                   imageLayout,
    const VkClearDepthStencilValue& depthStencil,
    uint32_t                        rangeCount,
    const VkImageSubresourceRange*  pRanges) const noexcept
  {
    _api.vkCmdClearDepthStencilImage(
      _commandBuffer, image, imageLayout, &depthStencil, rangeCount, pRanges);
  }

  void resolveImage(VkImage               srcImage,
                    VkImageLayout         srcImageLayout,
                    VkImage               dstImage,
                    VkImageLayout         dstImageLayout,
                    uint32_t              regionCount,
                    const VkImageResolve* pRegions) const noexcept
  {
    _api.vkCmdResolveImage(_commandBuffer,
                           srcImage,
                           srcImageLayout,
                           dstImage,
                           dstImageLayout,
                           regionCount,
                           pRegions);
  }

  void setEvent(VkEvent event, VkPipelineStageFlags stageMask) const noexcept
  {
    _api.vkCmdSetEvent(_commandBuffer, event, stageMask);
  }

  void resetEvent(VkEvent event, VkPipelineStageFlags stageMask) const noexcept
  {
    _api.vkCmdResetEvent(_commandBuffer, event, stageMask);
  }

  void resetQueryPool(VkQueryPool queryPool,
                      uint32_t    firstQuery,
                      uint32_t    queryCount) const noexcept
  {
    _api.vkCmdResetQueryPool(_commandBuffer, queryPool, firstQuery, queryCount);
  }

  void copyQueryPoolResults(VkQueryPool        queryPool,
                            uint32_t           firstQuery,
                            uint32_t           queryCount,
                            VkBuffer           dstBuffer,
                            VkDeviceSize       dstOffset,
                            VkDeviceSize       stride,
                            VkQueryResultFlags flags) const noexcept
  {
    _api.vkCmdCopyQueryPoolResults(_commandBuffer,
                                   queryPool,
                                   firstQuery,
                                   queryCount,
                                   dstBuffer,
                                   dstOffset,
                                   stride,
                                   flags);
  }

  SYBOK_NODISCARD
  RenderCommandScope beginRenderPass(
    const VkRenderPassBeginInfo& renderPassBegin,
    VkSubpassContents            contents) const noexcept;
};

class RenderCommandScope : public CommonCommandScope
{
public:
  RenderCommandScope(const VulkanApi& api,
                     VkCommandBuffer  commandBuffer) noexcept
    : CommonCommandScope{api, commandBuffer, VK_SUCCESS}
  {}

  RenderCommandScope(const RenderCommandScope&)            = delete;
  RenderCommandScope& operator=(const RenderCommandScope&) = delete;

  RenderCommandScope(RenderCommandScope&& scope) noexcept
    : CommonCommandScope{std::forward<RenderCommandScope>(scope)}
  {}

  RenderCommandScope& operator=(RenderCommandScope&&) = delete;

  ~RenderCommandScope() noexcept { _api.vkCmdEndRenderPass(_commandBuffer); }

  void draw(uint32_t vertexCount,
            uint32_t instanceCount,
            uint32_t firstVertex,
            uint32_t firstInstance) const noexcept
  {
    _api.vkCmdDraw(
      _commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
  }

  void drawIndexed(uint32_t indexCount,
                   uint32_t instanceCount,
                   uint32_t firstIndex,
                   int32_t  vertexOffset,
                   uint32_t firstInstance) const noexcept
  {
    _api.vkCmdDrawIndexed(_commandBuffer,
                          indexCount,
                          instanceCount,
                          firstIndex,
                          vertexOffset,
                          firstInstance);
  }

  void drawIndirect(VkBuffer     buffer,
                    VkDeviceSize offset,
                    uint32_t     drawCount,
                    uint32_t     stride) const noexcept
  {
    _api.vkCmdDrawIndirect(_commandBuffer, buffer, offset, drawCount, stride);
  }

  void drawIndexedIndirect(VkBuffer     buffer,
                           VkDeviceSize offset,
                           uint32_t     drawCount,
                           uint32_t     stride) const noexcept
  {
    _api.vkCmdDrawIndexedIndirect(
      _commandBuffer, buffer, offset, drawCount, stride);
  }

  void clearAttachments(uint32_t                 attachmentCount,
                        const VkClearAttachment& attachments,
                        uint32_t                 rectCount,
                        const VkClearRect*       pRects) const noexcept
  {
    _api.vkCmdClearAttachments(
      _commandBuffer, attachmentCount, &attachments, rectCount, pRects);
  }

  void nextSubpass(VkSubpassContents contents) const noexcept
  {
    _api.vkCmdNextSubpass(_commandBuffer, contents);
  }
};

inline CommandScope
VulkanApi::beginCommandBuffer(
  VkCommandBuffer                commandBuffer,
  const VkCommandBufferBeginInfo beginInfo) const noexcept
{
  if (const VkResult r = vkBeginCommandBuffer(commandBuffer, &beginInfo)) {
    return {*this, nullptr, r};
  }

  return {*this, commandBuffer, VK_SUCCESS};
}

inline RenderCommandScope
CommandScope::beginRenderPass(const VkRenderPassBeginInfo& renderPassBegin,
                              VkSubpassContents contents) const noexcept
{
  _api.vkCmdBeginRenderPass(_commandBuffer, &renderPassBegin, contents);

  return {_api, _commandBuffer};
}

inline MappedMemory::~MappedMemory() noexcept
{
  if (_api && _memory) {
    _api->vkUnmapMemory(_device, _memory);
  }
}

} // namespace sk

#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif

#endif // SYBOK_HPP
