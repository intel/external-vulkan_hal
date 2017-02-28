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

#include "vulkan_wrapper.h"

static VkResult GetSwapchainGrallocUsageANDROID(VkDevice /*dev*/,
                                                VkFormat /*fmt*/,
                                                VkImageUsageFlags /*usage*/,
                                                int* grallocUsage) {
  *grallocUsage =
      GRALLOC_USAGE_HW_FB | GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_TEXTURE;
  return VK_SUCCESS;
}

static VkResult AcquireImageANDROID(VkDevice, VkImage /*dev*/,
                                    int nativeFenceFd,
                                    VkSemaphore /*semaphore*/,
                                    VkFence /*fence*/) {
  close(nativeFenceFd);
  return VK_SUCCESS;
}

static VkResult QueueSignalReleaseImageANDROID(VkQueue /*queue*/,
                                               VkImage /*image*/,
                                               int* pNativeFenceFd) {
  *pNativeFenceFd = -1;
  return VK_SUCCESS;
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


static PFN_vkVoidFunction GetDeviceProcAddr(VkDevice device, const char* name)
{
  PFN_vkVoidFunction pfn;

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
