#ifndef __switch__
#include "pipeline.h"

#include "program.h"
#include "vertexAttributes.h"
#include "window.h"

#include "debug.h"

render::VulkanPipelineResult render::VulkanPipeline::newPipeline() {
	VulkanPipelineResult output = {
		new vk::PipelineLayout,
		new vk::Pipeline,
	};

	// handle pipeline layout
	{
		vk::PipelineLayoutCreateInfo pipelineLayoutInfo({}, 0, nullptr, 0, nullptr);
		vk::Result result = this->window->device.device.createPipelineLayout(&pipelineLayoutInfo, nullptr, output.layout); // TODO remember to clean up
		if(result != vk::Result::eSuccess) {
			console::error("vulkan: could not create pipeline layout: %s\n", vkResultToString((VkResult)result).c_str());
			exit(1);
		}
	}
	
	// handle pipeline
	{
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
		if(this->attributes != nullptr) {
			vertexInputInfo = vk::PipelineVertexInputStateCreateInfo(
				{},
				(uint32_t)this->attributes->inputBindings.size(),
				this->attributes->inputBindings.data(),
				(uint32_t)this->attributes->inputAttributes.size(),
				this->attributes->inputAttributes.data()
			);
		}

		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo({}, primitiveToVulkanPrimitive(this->topology), false);
		vk::Viewport viewport(0.0f, this->viewportHeight, this->viewportWidth, -this->viewportHeight, 0.0f, 1.0f);
		vk::Rect2D scissor({ 0, 0 }, this->window->swapchainExtent);

		vk::PipelineViewportStateCreateInfo viewportStateInfo({}, 1, &viewport, 1, &scissor);
		vk::PipelineRasterizationStateCreateInfo rasterizationInfo(
			{},
			false,
			false,
			vk::PolygonMode::eFill, // TODO change fill based on primitive?
			vk::CullModeFlagBits::eNone,
			vk::FrontFace::eCounterClockwise,
			false,
			0.0f,
			0.0f,
			0.0f,
			1.0f
		);

		vk::PipelineMultisampleStateCreateInfo multisampleInfo({}, vk::SampleCountFlagBits::e1, false, 1.0f, nullptr, false, false);

		vk::PipelineColorBlendAttachmentState colorBlend(
			false,
			vk::BlendFactor::eOne, // color blend
			vk::BlendFactor::eZero,
			vk::BlendOp::eAdd,
			vk::BlendFactor::eOne, // alpha blend
			vk::BlendFactor::eZero,
			vk::BlendOp::eAdd,
			vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA 
		);

		vk::PipelineColorBlendStateCreateInfo colorBlendInfo(
			{},
			false,
			vk::LogicOp::eCopy,
			1,
			&colorBlend,
			{ 0.0f, 0.0f, 0.0f, 0.0f, }
		);

		vk::PipelineDynamicStateCreateInfo dynamicStateInfo({}, 0, nullptr);

		// create the pipeline
		vk::GraphicsPipelineCreateInfo pipelineInfo(
			{},
			this->program->stages.size(),
			this->program->stages.data(),
			&vertexInputInfo,
			&inputAssemblyInfo,
			nullptr,
			&viewportStateInfo,
			&rasterizationInfo,
			&multisampleInfo,
			nullptr,
			&colorBlendInfo,
			&dynamicStateInfo,
			*output.layout,
			this->window->renderPass,
			0
		);

		vk::PipelineCache pipelineCache = vk::PipelineCache();
		vk::Result result = this->window->device.device.createGraphicsPipelines(pipelineCache, 1, &pipelineInfo, nullptr, output.pipeline);
		if(result != vk::Result::eSuccess) {
			console::error("vulkan: could not create pipeline: %s\n", vkResultToString((VkResult)result).c_str());
			exit(1);
		}
	}

	return output;
}
#endif
