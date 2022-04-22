#ifndef __switch__
#include <glad/gl.h>
#endif

#include "texture.h"

#include <fstream>
#include <stdlib.h>
#include <string.h>

#include "../engine/console.h"
#include "../util/png.h"
#include "window.h"

render::Texture::Texture(Window* window) {
	this->window = window;
}

void render::Texture::setFilters(TextureFilter minFilter, TextureFilter magFilter) {
	this->minFilter = minFilter;
	this->magFilter = magFilter;
}

void render::Texture::setWrap(TextureWrap uWrap, TextureWrap vWrap) {
	this->uWrap = uWrap;
	this->vWrap = vWrap;
}

void render::Texture::loadPNGFromFile(std::string filename) {
	std::ifstream file(filename, std::ios::binary);

	if(file.bad() || file.fail()) {
		console::error("failed to open file for png %s\n", filename.c_str());
		file.close();
		this->window->addError();
		return;
	}

	file.seekg(0, file.end);
	uint64_t length = (uint64_t)file.tellg();
	file.seekg(0, file.beg);
	char* buffer = new char[length];
	file.read((char*)buffer, length);
	file.close();

	this->loadPNG((unsigned char*)buffer, length);

	delete[] buffer;	
}

void render::Texture::Texture::loadPNG(const unsigned char* buffer, unsigned int size) {
	png file = loadPng(buffer, size);
	if(file.buffer != nullptr) {
		this->load(
			file.buffer,
			file.bufferSize,
			file.width,
			file.height,
			file.bitDepth,
			file.channels
		);

		delete[] file.buffer;
	}
}

void render::Texture::load(
	const unsigned char* buffer,
	unsigned int bufferSize,
	unsigned int width,
	unsigned int height,
	unsigned int bitDepth,
	unsigned int channels
) {
	this->width = width;
	this->height = height;
	this->bitDepth = bitDepth;
	this->channels = channels;
	
	if(this->minFilter == TEXTURE_FILTER_INVALID || this->magFilter == TEXTURE_FILTER_INVALID) {
		console::error("invalid texture filters\n");
		return;
	}

	if(this->uWrap == TEXTURE_WRAP_INVALID || this->vWrap == TEXTURE_WRAP_INVALID) {
		console::error("invalid texture wraps\n");
		return;
	}

	#ifdef __switch__
	this->imageDescriptorMemory = this->window->memory.allocate(DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached, sizeof(DkImageDescriptor), DK_IMAGE_DESCRIPTOR_ALIGNMENT);

	this->samplerDescriptorMemory = this->window->memory.allocate(DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached, sizeof(DkSamplerDescriptor), DK_SAMPLER_DESCRIPTOR_ALIGNMENT);
	
	// allocate memory for the image, we will be deallocating this later though
	Piece* memory = this->window->memory.allocate(
		DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached,
		bufferSize,
		DK_IMAGE_LINEAR_STRIDE_ALIGNMENT
	);
	memcpy(memory->cpuAddr(), buffer, bufferSize);

	dk::ImageLayout layout;
	dk::ImageLayoutMaker{this->window->device}
		.setFlags(DkImageFlags_BlockLinear)
		.setFormat(channelsAndBitDepthToDkFormat(this->channels, this->bitDepth))
		.setDimensions(this->width, this->height)
		.initialize(layout);
	
	// // allocate the actual image that we won't get rid of
	this->memory = this->window->memory.allocate(DkMemBlockFlags_GpuCached | DkMemBlockFlags_Image, layout.getSize(), layout.getAlignment());
	this->image.initialize(layout, this->memory->parent->block, this->memory->start);
	this->imageDescriptor.initialize(this->image);

	this->sampler.setFilter(textureFilterToDkFilter(this->minFilter), textureFilterToDkFilter(this->magFilter));
	this->sampler.setWrapMode(textureWrapToDkWrap(this->uWrap), textureWrapToDkWrap(this->vWrap));
	this->samplerDescriptor.initialize(this->sampler);

	dk::ImageView view{this->image};
	this->window->addTexture(memory, view, this->width, this->height);

	memory->deallocate();
	#else
	if(this->window->backend == OPENGL_BACKEND) {
		// load the GL texture
		glGenTextures(1, &this->texture);
		glBindTexture(GL_TEXTURE_2D, this->texture);

		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			channelsToGLFormat(this->channels),
			this->width,
			this->height,
			0,
			channelsToGLFormat(this->channels),
			bitDepthToGLFormat(this->bitDepth),
			buffer
		);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, textureWrapToGLWrap(this->uWrap));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, textureWrapToGLWrap(this->vWrap));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, textureFilterToGLFilter(this->minFilter));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, textureFilterToGLFilter(this->magFilter));
	}
	else {
		this->stagingBuffer = this->window->memory.allocateBuffer(
			vk::BufferCreateInfo(
				{},
				bufferSize,
				vk::BufferUsageFlagBits::eTransferSrc,
				vk::SharingMode::eExclusive
			),
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
		);

		this->image = this->window->memory.allocateImage(
			vk::ImageCreateInfo(
				{},
				vk::ImageType::e2D,
				channelsAndBitDepthToVulkanFormat(this->channels, this->bitDepth),
				{ this->width, this->height, 1, },
				1,
				1,
				vk::SampleCountFlagBits::e1,
				vk::ImageTiling::eOptimal,
				vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
				vk::SharingMode::eExclusive,
				0,
				nullptr,
				vk::ImageLayout::eUndefined
			),
			vk::MemoryPropertyFlagBits::eDeviceLocal
		);

		memcpy(this->stagingBuffer->map(), buffer, bufferSize);

		// handle layout transitions
		{
			vk::CommandBuffer buffer = this->window->beginTransientCommands();

			vk::ImageMemoryBarrier barrier(
				{},
				vk::AccessFlagBits::eTransferWrite,
				vk::ImageLayout::eUndefined,
				vk::ImageLayout::eTransferDstOptimal,
				VK_QUEUE_FAMILY_IGNORED,
				VK_QUEUE_FAMILY_IGNORED,
				this->image->getImage(),
				vk::ImageSubresourceRange(
					vk::ImageAspectFlagBits::eColor, // aspect mask
					0, // base mip level
					1, // level count
					0, // base array layer
					1 // layer count
				)
			);

			buffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, 0, nullptr, 0, nullptr, 1, &barrier
			);

			this->window->endTransientCommands(buffer, {});
		}

		// copy buffer into valid image memory
		this->window->copyVulkanBufferToImage(this->stagingBuffer, this->image, this->width, this->height);

		// transfer the layout to shader mode
		{
			vk::CommandBuffer buffer = this->window->beginTransientCommands();

			vk::ImageMemoryBarrier barrier(
				vk::AccessFlagBits::eTransferWrite,
				vk::AccessFlagBits::eShaderRead,
				vk::ImageLayout::eTransferDstOptimal,
				vk::ImageLayout::eShaderReadOnlyOptimal,
				VK_QUEUE_FAMILY_IGNORED,
				VK_QUEUE_FAMILY_IGNORED,
				this->image->getImage(),
				vk::ImageSubresourceRange(
					vk::ImageAspectFlagBits::eColor, // aspect mask
					0, // base mip level
					1, // level count
					0, // base array layer
					1 // layer count
				)
			);

			buffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, 0, nullptr, 0, nullptr, 1, &barrier
			);

			this->window->endTransientCommands(buffer, {});
		}

		// create image view
		vk::ImageViewCreateInfo viewInfo(
			{},
			this->image->getImage(),
			vk::ImageViewType::e2D,
			channelsAndBitDepthToVulkanFormat(this->channels, this->bitDepth),
			{ vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, },
			vk::ImageSubresourceRange(
				vk::ImageAspectFlagBits::eColor, // aspect mask
				0, // base mip level
				1, // level count
				0, // base array layer
				1 // layer count
			)
		);

		this->imageView = this->window->device.device.createImageView(viewInfo);

		// create sampler
		vk::SamplerCreateInfo samplerInfo(
			{},
			textureFilterToVulkanFilter(this->magFilter),
			textureFilterToVulkanFilter(this->minFilter),
			vk::SamplerMipmapMode::eNearest,
			vk::SamplerAddressMode::eRepeat,
			vk::SamplerAddressMode::eRepeat,
			vk::SamplerAddressMode::eRepeat,
			0,
			false,
			0,
			false,
			vk::CompareOp::eNever,
			0,
			0,
			vk::BorderColor::eFloatTransparentBlack,
			false
		);

		this->sampler = this->window->device.device.createSampler(samplerInfo);
	}
	#endif
}

unsigned int render::Texture::getWidth() {
	return this->width;
}

unsigned int render::Texture::getHeight() {
	return this->height;
}

void render::Texture::bind(unsigned int location) {
	#ifdef __switch__
	// this->window->bindTexture(location, this);

	this->window->commandBuffer.pushData(this->imageDescriptorMemory->gpuAddr() + location * sizeof(DkImageDescriptor), &this->imageDescriptor, sizeof(DkImageDescriptor));
	this->window->commandBuffer.pushData(this->samplerDescriptorMemory->gpuAddr() + location * sizeof(DkSamplerDescriptor), &this->samplerDescriptor, sizeof(DkSamplerDescriptor));

	this->window->commandBuffer.bindImageDescriptorSet(this->imageDescriptorMemory->gpuAddr(), 1);
	this->window->commandBuffer.bindSamplerDescriptorSet(this->samplerDescriptorMemory->gpuAddr(), 1);
	#else
	if(this->window->backend == OPENGL_BACKEND) {
		glActiveTexture(GL_TEXTURE0 + location);
		glBindTexture(GL_TEXTURE_2D, this->texture);
	}
	#endif
}
