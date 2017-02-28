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

#include <dlfcn.h>
#include <cutils/log.h>

#include "vulkan_wrapper.h"

namespace mesa_vulkan {

static void* LibraryHandle = NULL;

bool InitializeVulkan() {
  if (LibraryHandle)
    return true;

  LibraryHandle = dlopen("vulkan.mesa_intel.so", RTLD_NOW);

  if (LibraryHandle == NULL) {
    ALOGE("Failed to load Vulkan library. %s", dlerror());
    return false;
  }

  vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
      dlsym(LibraryHandle, "vk_icdGetInstanceProcAddr"));

  if (vkGetInstanceProcAddr == NULL) {
    ALOGE("Could not find vk_icdGetInstanceProcAddr symbol. %s", dlerror());
    return false;
  }

  vkCreateInstance = reinterpret_cast<PFN_vkCreateInstance>(
      vkGetInstanceProcAddr(NULL, "vkCreateInstance"));

  if (vkCreateInstance == NULL) {
    ALOGE("Could not find vkCreateInstance symbol. %s", dlerror());
    return false;
  }

  vkEnumerateInstanceExtensionProperties =
      reinterpret_cast<PFN_vkEnumerateInstanceExtensionProperties>(
          vkGetInstanceProcAddr(NULL, "vkEnumerateInstanceExtensionProperties"));

  if (vkEnumerateInstanceExtensionProperties == NULL) {
    ALOGE("Could not find vkEnumerateInstanceExtensionProperties symbol. %s", dlerror());
    return false;
  }

  vkGetDeviceProcAddr = reinterpret_cast<PFN_vkGetDeviceProcAddr>(
      vkGetInstanceProcAddr(NULL, "vkGetDeviceProcAddr"));

  if (vkGetDeviceProcAddr == NULL) {
    ALOGE("Could not find vkGetDeviceProcAddr symbol. %s", dlerror());
    return false;
  }

  return true;
}

void Close() {
  if (LibraryHandle) {
    dlclose(LibraryHandle);
    LibraryHandle = NULL;
    vkEnumerateInstanceExtensionProperties = NULL;
    vkCreateInstance = NULL;
    vkGetInstanceProcAddr = NULL;
    vkGetDeviceProcAddr = NULL;
  }
}

PFN_vkCreateInstance vkCreateInstance;
PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
PFN_vkEnumerateInstanceExtensionProperties
    vkEnumerateInstanceExtensionProperties;
}
