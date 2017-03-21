# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	vulkan_hal.cpp \
	vulkan_wrapper.cpp

LOCAL_CLANG := true
LOCAL_CFLAGS := -std=c99 -fvisibility=hidden -fstrict-aliasing \
	-Weverything -Werror \
	-Wno-padded \
	-Wno-undef \
	-Wno-zero-length-array \
	-DVK_USE_PLATFORM_ANDROID_KHR \
	-DLOG_TAG=\"VulkanHAL\" \
#LOCAL_CFLAGS += -DLOG_NDEBUG=0
LOCAL_CPPFLAGS := -std=c++1y \
	-Wno-c++98-compat-pedantic \
	-Wno-c99-extensions

LOCAL_C_INCLUDES := \
	frameworks/native/vulkan/include \
	$(LOCAL_PATH)/../mesa/include

LOCAL_SHARED_LIBRARIES := libvulkan liblog libdl libcutils libsync

LOCAL_MODULE := vulkan.$(TARGET_BOARD_PLATFORM)
#Prefered path for Vulkan is /vendor/lib/hw
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
