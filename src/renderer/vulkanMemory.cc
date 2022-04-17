#ifndef __switch__
#include "vulkanMemory.h"

#include "window.h"
#include "debug.h"

render::VulkanBuffer render::Window::allocateVulkanBuffer(vk::BufferCreateInfo bufferInfo, vk::MemoryPropertyFlags propertyFlags) {
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

void render::Window::copyVulkanBuffer(render::VulkanBuffer source, render::VulkanBuffer destination) {
	// create a new fence for this operation
	vk::FenceCreateInfo fenceInfo;
	vk::Fence fence = this->device.device.createFence(fenceInfo);
	this->memoryCopyFences.push_back(fence);
	
	// create temporary command buffer
	vk::CommandBufferAllocateInfo allocationInfo(this->commandPool, vk::CommandBufferLevel::ePrimary, 1); // TODO use transient command pool
	vk::CommandBuffer commandBuffer = this->device.device.allocateCommandBuffers(allocationInfo)[0];
	this->memoryCopyCommandBuffers.push_back(commandBuffer);

	vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	commandBuffer.begin(beginInfo);

	vk::BufferCopy copyInfo(0, 0, source.size);
	commandBuffer.copyBuffer(source.buffer, destination.buffer, 1, &copyInfo);

	commandBuffer.end();

	vk::SubmitInfo submitInfo(0, nullptr, nullptr, 1, &commandBuffer, 0, nullptr);
	vk::Result result = this->graphicsQueue.submit(1, &submitInfo, fence); // submit work to GPU
	if(result != vk::Result::eSuccess) {
		console::print("vulkan: failed to submit memory transfer graphics queue %s\n", vkResultToString((VkResult)result).c_str());
		exit(1);
	}
}

void render::VulkanBuffer::destroy() {
	if(this->mappedMemory != nullptr) {
		this->unmap();
	}
	
	if(!this->valid) {
		this->window->device.device.destroyBuffer(this->buffer);
		this->window->device.device.freeMemory(this->memory);
	}

	this->valid = false;
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
#endif
