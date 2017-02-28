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

#ifndef VULKAN_WRAPPER_H
#define VULKAN_WRAPPER_H

#define VK_NO_PROTOTYPES 1
#include <vulkan/vulkan.h>

namespace mesa_vulkan {
bool InitializeVulkan();
void Close();

extern PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
extern PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
extern PFN_vkCreateInstance vkCreateInstance;
extern PFN_vkEnumerateInstanceExtensionProperties
    vkEnumerateInstanceExtensionProperties;
}

#endif
