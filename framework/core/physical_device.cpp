/* Copyright (c) 2020-2025, Arm Limited and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "physical_device.h"

namespace vkb
{
PhysicalDevice::PhysicalDevice(Instance &instance, VkPhysicalDevice physical_device) :
    instance{instance},
    handle{physical_device}
{
	vkGetPhysicalDeviceFeatures(physical_device, &features);
	vkGetPhysicalDeviceProperties(physical_device, &properties);
	vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

	LOGI("Found GPU: {}", properties.deviceName);

	uint32_t queue_family_properties_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_properties_count, nullptr);
	queue_family_properties = std::vector<VkQueueFamilyProperties>(queue_family_properties_count);
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_properties_count, queue_family_properties.data());

	uint32_t device_extension_count;
	VK_CHECK(vkEnumerateDeviceExtensionProperties(get_handle(), nullptr, &device_extension_count, nullptr));
	device_extensions = std::vector<VkExtensionProperties>(device_extension_count);
	VK_CHECK(vkEnumerateDeviceExtensionProperties(get_handle(), nullptr, &device_extension_count, device_extensions.data()));

	// Display supported extensions
	if (device_extensions.size() > 0)
	{
		LOGD("Device supports the following extensions:");
		for (auto &extension : device_extensions)
		{
			LOGD("  \t{}", extension.extensionName);
		}
	}
}

DriverVersion PhysicalDevice::get_driver_version() const
{
	DriverVersion version;

	VkPhysicalDeviceProperties const &properties = get_properties();
	switch (properties.vendorID)
	{
		case 0x10DE:
			// Nvidia
			version.major = (properties.driverVersion >> 22) & 0x3ff;
			version.minor = (properties.driverVersion >> 14) & 0x0ff;
			version.patch = (properties.driverVersion >> 6) & 0x0ff;
			// Ignoring optional tertiary info in lower 6 bits
			break;
		case 0x8086:
			version.major = (properties.driverVersion >> 14) & 0x3ffff;
			version.minor = properties.driverVersion & 0x3ffff;
			break;
		default:
			version.major = VK_VERSION_MAJOR(properties.driverVersion);
			version.minor = VK_VERSION_MINOR(properties.driverVersion);
			version.patch = VK_VERSION_PATCH(properties.driverVersion);
			break;
	}

	return version;
}

Instance &PhysicalDevice::get_instance() const
{
	return instance;
}

VkBool32 PhysicalDevice::is_present_supported(VkSurfaceKHR surface, uint32_t queue_family_index) const
{
	VkBool32 present_supported{VK_FALSE};

	if (surface != VK_NULL_HANDLE)
	{
		VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(handle, queue_family_index, surface, &present_supported));
	}

	return present_supported;
}

bool PhysicalDevice::is_extension_supported(const std::string &requested_extension) const
{
	return std::ranges::find_if(device_extensions,
	                            [requested_extension](auto &device_extension) {
		                            return std::strcmp(device_extension.extensionName, requested_extension.c_str()) == 0;
	                            }) != device_extensions.end();
}

const VkFormatProperties PhysicalDevice::get_format_properties(VkFormat format) const
{
	VkFormatProperties format_properties;

	vkGetPhysicalDeviceFormatProperties(handle, format, &format_properties);

	return format_properties;
}

VkPhysicalDevice PhysicalDevice::get_handle() const
{
	return handle;
}

const VkPhysicalDeviceFeatures &PhysicalDevice::get_features() const
{
	return features;
}

const VkPhysicalDeviceProperties &PhysicalDevice::get_properties() const
{
	return properties;
}

const VkPhysicalDeviceMemoryProperties &PhysicalDevice::get_memory_properties() const
{
	return memory_properties;
}

uint32_t PhysicalDevice::get_memory_type(uint32_t bits, VkMemoryPropertyFlags properties, VkBool32 *memory_type_found) const
{
	for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
	{
		if ((bits & 1) == 1)
		{
			if ((memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				if (memory_type_found)
				{
					*memory_type_found = true;
				}
				return i;
			}
		}
		bits >>= 1;
	}

	if (memory_type_found)
	{
		*memory_type_found = false;
		return ~0;
	}
	else
	{
		throw std::runtime_error("Could not find a matching memory type");
	}
}

const std::vector<VkQueueFamilyProperties> &PhysicalDevice::get_queue_family_properties() const
{
	return queue_family_properties;
}

uint32_t PhysicalDevice::get_queue_family_performance_query_passes(
    const VkQueryPoolPerformanceCreateInfoKHR *perf_query_create_info) const
{
	uint32_t passes_needed;
	vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR(get_handle(), perf_query_create_info,
	                                                        &passes_needed);
	return passes_needed;
}

void PhysicalDevice::enumerate_queue_family_performance_query_counters(
    uint32_t                            queue_family_index,
    uint32_t                           *count,
    VkPerformanceCounterKHR            *counters,
    VkPerformanceCounterDescriptionKHR *descriptions) const
{
	VK_CHECK(vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR(
	    get_handle(), queue_family_index, count, counters, descriptions));
}

const VkPhysicalDeviceFeatures PhysicalDevice::get_requested_features() const
{
	return requested_features;
}

VkPhysicalDeviceFeatures &PhysicalDevice::get_mutable_requested_features()
{
	return requested_features;
}

void *PhysicalDevice::get_extension_feature_chain() const
{
	return last_requested_extension_feature;
}

}        // namespace vkb
