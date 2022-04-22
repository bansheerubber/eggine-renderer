#ifndef __switch__

#include "memory.h"
#include "window.h"
#include "debug.h"

vk::CommandBuffer render::Window::beginTransientCommands() {
	// create temporary command buffer
	vk::CommandBufferAllocateInfo allocationInfo(this->transientCommandPool, vk::CommandBufferLevel::ePrimary, 1); // TODO use transient command pool
	vk::CommandBuffer commandBuffer = this->device.device.allocateCommandBuffers(allocationInfo)[0];
	this->transientCommandBuffers.push_back(commandBuffer);

	vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	commandBuffer.begin(beginInfo);

	return commandBuffer;
}

void render::Window::endTransientCommands(vk::CommandBuffer buffer, vk::Fence fence) {
	buffer.end();

	vk::SubmitInfo submitInfo(0, nullptr, nullptr, 1, &buffer, 0, nullptr);
	vk::Result result = this->graphicsQueue.submit(1, &submitInfo, fence); // submit work to GPU
	if(result != vk::Result::eSuccess) {
		console::print("vulkan: failed to submit memory transfer graphics queue %s\n", vkResultToString((VkResult)result).c_str());
		exit(1);
	}
}

void render::Window::copyVulkanBuffer(Piece* source, Piece* destination) {
	// create a new fence for this operation
	vk::FenceCreateInfo fenceInfo;
	vk::Fence fence = this->device.device.createFence(fenceInfo);
	this->memoryCopyFences.push_back(fence);
	
	vk::CommandBuffer commandBuffer = this->beginTransientCommands();
	vk::BufferCopy copyInfo(0, 0, source->bufferSize);
	commandBuffer.copyBuffer(source->buffer, destination->buffer, 1, &copyInfo);
	this->endTransientCommands(commandBuffer, fence);
}

void render::Window::copyVulkanBufferToImage(Piece* source, Piece* destination, uint32_t width, uint32_t height) {
	// create a new fence for this operation
	vk::FenceCreateInfo fenceInfo;
	vk::Fence fence = this->device.device.createFence(fenceInfo);
	this->memoryCopyFences.push_back(fence);
	
	vk::CommandBuffer commandBuffer = this->beginTransientCommands();
	vk::BufferImageCopy copyInfo(
		0,
		0,
		0,
		vk::ImageSubresourceLayers(
			vk::ImageAspectFlagBits::eColor,
			0,
			0,
			1
		),
		vk::Offset3D(0, 0, 0),
		vk::Extent3D(width, height, 1)
	);
	commandBuffer.copyBufferToImage(source->buffer, destination->image, vk::ImageLayout::eTransferDstOptimal, 1, &copyInfo);
	this->endTransientCommands(commandBuffer, fence);
}
#endif
