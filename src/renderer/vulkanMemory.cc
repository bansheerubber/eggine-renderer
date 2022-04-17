#include "vulkanMemory.h"

#include "window.h"

render::VulkanBuffer render::Window::allocateBuffer(vk::BufferCreateInfo &bufferInfo, vk::MemoryPropertyFlags propertyFlags) {
	VulkanBuffer buffer(this);

	buffer.size = bufferInfo.size;

	buffer.buffer = this->device.device.createBuffer(bufferInfo);

	vk::MemoryRequirements requirements = this->device.device.getBufferMemoryRequirements(buffer.buffer);

	vk::PhysicalDeviceMemoryProperties properties = this->device.physicalDevice.getMemoryProperties();
	uint32_t memoryType = (uint32_t)-1;
	for(uint32_t i = 0; i < properties.memoryTypeCount; i++) {
		if((requirements.memoryTypeBits & (1 << i)) && (properties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags) {
			memoryType = i;
			break;
		}
	}

	vk::MemoryAllocateInfo allocationInfo(requirements.size, memoryType);
	buffer.memory = this->device.device.allocateMemory(allocationInfo);

	this->device.device.bindBufferMemory(buffer.buffer, buffer.memory, 0);

	return buffer;
}

void render::VulkanBuffer::destroy() {
	if(this->mappedMemory != nullptr) {
		this->unmap();
	}

	this->valid = false;
	
	this->window->device.device.destroyBuffer(this->buffer);
	this->window->device.device.freeMemory(this->memory);
}

void* render::VulkanBuffer::map() {
	if(this->mappedMemory != nullptr) {
		return this->mappedMemory;	
	}

	this->mappedMemory = this->window->device.device.mapMemory(this->memory, 0, this->size, {});
	return this->mappedMemory;
}

void render::VulkanBuffer::unmap() {
	this->window->device.device.unmapMemory(this->memory);
	this->mappedMemory = nullptr;
}
