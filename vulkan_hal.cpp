/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cutils/log.h>
#include <hardware/hardware.h>
#include <hardware/hwvulkan.h>
#include <hardware/gralloc.h>
#include <vulkan/vk_android_native_buffer.h>
#include <sync/sync.h>

#include "vulkan_wrapper.h"
#include "vulkan/vulkan_intel.h"

static VkResult GetSwapchainGrallocUsageANDROID(VkDevice /*dev*/,
                                                VkFormat /*fmt*/,
                                                VkImageUsageFlags usage,
                                                int* grallocUsage) {
  VkImageUsageFlags usageSrc =
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;

  VkImageUsageFlags usageDst =
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  // used for texturing
  if (usage & usageSrc)
    *grallocUsage |= GRALLOC_USAGE_HW_TEXTURE;

  // used for rendering
  if (usage & usageDst)
    *grallocUsage |= GRALLOC_USAGE_HW_RENDER;

  return VK_SUCCESS;
}

static VkResult AcquireImageANDROID(VkDevice, VkImage /*dev*/,
                                    int nativeFenceFd,
                                    VkSemaphore /*semaphore*/,
                                    VkFence /*fence*/) {
  // wait for fence to signal before acquiring image
  sync_wait(nativeFenceFd, -1);

  close(nativeFenceFd);
  return VK_SUCCESS;
}

static VkResult QueueSignalReleaseImageANDROID(VkQueue /*queue*/,
                                               VkImage /*image*/,
                                               int* pNativeFenceFd) {
  if (pNativeFenceFd)
    *pNativeFenceFd = -1;
  return VK_SUCCESS;
}

static VkResult CreateImage(VkDevice device,
                            const VkImageCreateInfo* pCreateInfo,
                            const VkAllocationCallbacks* pAllocator,
                            VkImage* pImage) {
  static PFN_vkCreateDmaBufImageINTEL dmabufFunc =
      reinterpret_cast<PFN_vkCreateDmaBufImageINTEL>(
          mesa_vulkan::vkGetDeviceProcAddr(device, "vkCreateDmaBufImageINTEL"));

  if (!dmabufFunc || !pCreateInfo->pNext) {
    ALOGE("ANDROID extension structure not found");
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }

  const VkImageCreateInfo* p =
      reinterpret_cast<const VkImageCreateInfo*>(pCreateInfo->pNext);

  // we hardcode VK_STRUCTURE_TYPE_NATIVE_BUFFER_ANDROID for now as
  // vk_android_native_buffer.h is using old style cast
  while (p && p->sType != 1000010000)
    p = reinterpret_cast<const VkImageCreateInfo*>(p->pNext);

  if (!p) {
    ALOGE("VK_ANDROID_native_buffer extension structure not found");
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }

  const VkNativeBufferANDROID* buffer =
      reinterpret_cast<const VkNativeBufferANDROID*>(pCreateInfo->pNext);

  const native_handle_t* handle =
      reinterpret_cast<const native_handle_t*>(buffer->handle);

  VkDeviceMemory pMem;
  VkDmaBufImageCreateInfo dmabufInfo = {
      .sType = static_cast<VkStructureType>(
          VK_STRUCTURE_TYPE_DMA_BUF_IMAGE_CREATE_INFO_INTEL),
      .pNext = NULL,
      .fd = handle->data[0],
      .format = pCreateInfo->format,
      .extent = {
          .width = pCreateInfo->extent.width,
          .height = pCreateInfo->extent.height,
          .depth = pCreateInfo->extent.depth,
      },
      // FIXME magic, we know this surface tiling to be I915_TILING_X and
      // take this in to account when giving stride, Mesa will internally
      // use this exact value as row pitch validation for the surface.
      .strideInBytes = static_cast<uint32_t>(buffer->stride * 4),
  };

  return dmabufFunc(device, &dmabufInfo, pAllocator, &pMem, pImage);
}

static int CloseDevice(struct hw_device_t* dev) {
  mesa_vulkan::Close();
  delete dev;
  return 0;
}

static VkResult EnumerateInstanceExtensionProperties(
    const char* layer_name, uint32_t* count,
    VkExtensionProperties* properties) {
  return mesa_vulkan::vkEnumerateInstanceExtensionProperties(layer_name, count,
                                                             properties);
}

static PFN_vkVoidFunction GetDeviceProcAddr(VkDevice device, const char* name) {
  PFN_vkVoidFunction pfn;

  /* wrap vkCreateImage to use vkCreateDmaBufImageINTEL */
  if (strcmp(name, "vkCreateImage") == 0) {
    return reinterpret_cast<PFN_vkVoidFunction>(CreateImage);
  }

  if ((pfn = reinterpret_cast<PFN_vkVoidFunction>(
           mesa_vulkan::vkGetDeviceProcAddr(device, name)))) {
    return pfn;
  }

  if (strcmp(name, "vkGetSwapchainGrallocUsageANDROID") == 0)
    return reinterpret_cast<PFN_vkVoidFunction>(GetSwapchainGrallocUsageANDROID);

  if (strcmp(name, "vkAcquireImageANDROID") == 0)
    return reinterpret_cast<PFN_vkVoidFunction>(AcquireImageANDROID);

  if (strcmp(name, "vkQueueSignalReleaseImageANDROID") == 0)
    return reinterpret_cast<PFN_vkVoidFunction>(QueueSignalReleaseImageANDROID);

  return nullptr;
}

static PFN_vkVoidFunction GetInstanceProcAddr(VkInstance instance,
                                              const char* name) {
  PFN_vkVoidFunction pfn;

  if (strcmp(name, "vkGetDeviceProcAddr") == 0)
    return reinterpret_cast<PFN_vkVoidFunction>(GetDeviceProcAddr);

  if ((pfn = reinterpret_cast<PFN_vkVoidFunction>(
           mesa_vulkan::vkGetInstanceProcAddr(instance, name)))) {
    return pfn;
  }

  return nullptr;
}

static VkResult CreateInstance(const VkInstanceCreateInfo* create_info,
                               const VkAllocationCallbacks* allocator,
                               VkInstance* instance) {
  return mesa_vulkan::vkCreateInstance(create_info, allocator, instance);
}

// Declare HAL_MODULE_INFO_SYM here so it can be referenced by
// mesa_vulkan_device
// later.
namespace {
int OpenDevice(const hw_module_t* module, const char* id, hw_device_t** device);
hw_module_methods_t vk_mod_methods = {.open = OpenDevice};
}  // namespace

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-variable-declarations"
__attribute__((visibility("default"))) hwvulkan_module_t HAL_MODULE_INFO_SYM = {
    .common = {.tag = HARDWARE_MODULE_TAG,
               .module_api_version = HWVULKAN_MODULE_API_VERSION_0_1,
               .hal_api_version = HARDWARE_HAL_API_VERSION,
               .id = HWVULKAN_HARDWARE_MODULE_ID,
               .name = "Mesa Vulkan",
               .author = "Intel",
               .methods = &vk_mod_methods},
};
#pragma clang diagnostic pop

namespace {
hwvulkan_device_t mesa_vulkan_device = {
    .common = {
        .tag = HARDWARE_DEVICE_TAG,
        .version = HWVULKAN_DEVICE_API_VERSION_0_1,
        .module = &HAL_MODULE_INFO_SYM.common,
        .close = CloseDevice,
    },
    .EnumerateInstanceExtensionProperties =
        EnumerateInstanceExtensionProperties,
    .CreateInstance = CreateInstance,
    .GetInstanceProcAddr = GetInstanceProcAddr};

int OpenDevice(const hw_module_t* /*module*/, const char* id,
               hw_device_t** device) {
  if (strcmp(id, HWVULKAN_DEVICE_0) == 0) {
    if (!mesa_vulkan::InitializeVulkan()) {
      ALOGE("%s: Failed to initialize Vulkan.", __func__);
      return -ENOENT;
    }

    *device = &mesa_vulkan_device.common;
    return 0;
  }
  return -ENOENT;
}
}
