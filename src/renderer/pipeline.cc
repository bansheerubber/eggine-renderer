#include "pipeline.h"

#include "program.h"
#include "window.h"
#include "debug.h"

render::VulkanPipelineResult render::VulkanPipeline::newPipeline() {
	VulkanPipelineResult output = {
		new vk::PipelineLayout,
		new vk::RenderPass,
		new vk::Pipeline,
	};
	
	vk::PipelineVertexInputStateCreateInfo vertexInputInfo({}, 0, nullptr, 0, nullptr);
	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo({}, primitiveToVulkanPrimitive(this->topology), false);
	vk::Viewport viewport(0.0f, 0.0f, this->viewportWidth, this->viewportHeight, 0.0f, 1.0f);
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

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo({}, 0, nullptr, 0, nullptr);
	vk::Result result = this->window->device.device.createPipelineLayout(&pipelineLayoutInfo, nullptr, output.layout); // TODO remember to clean up
	if(result != vk::Result::eSuccess) {
		console::error("vulkan: could not create pipeline layout: %s\n", vkResultToString((VkResult)result).c_str());
		exit(1);
	}

	// handle renderpass/subpasses
	vk::AttachmentDescription colorAttachment({}, this->window->swapchainFormat.format, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR);

	vk::AttachmentReference attachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal); // attach to the output color location in the shader
	vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &attachmentReference);

	// create a subpass dependency that waits for color
	vk::SubpassDependency dependency(
		VK_SUBPASS_EXTERNAL, // src
		0, // dest
		vk::PipelineStageFlagBits::eColorAttachmentOutput, // src stage
		vk::PipelineStageFlagBits::eColorAttachmentOutput, // dest stage
		{}, // src access
		vk::AccessFlagBits::eColorAttachmentWrite // dest access
	);

	vk::RenderPassCreateInfo renderPassInfo({}, 1, &colorAttachment, 1, &subpass, 1, &dependency);
	result = this->window->device.device.createRenderPass(&renderPassInfo, nullptr, output.renderPass); // TODO remember to clean up
	if(result != vk::Result::eSuccess) {
		console::error("vulkan: could not create render pass: %s\n", vkResultToString((VkResult)result).c_str());
		exit(1);
	}

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
		*output.renderPass,
		0
	);

	vk::PipelineCache pipelineCache = vk::PipelineCache();
	result = this->window->device.device.createGraphicsPipelines(pipelineCache, 1, &pipelineInfo, nullptr, output.pipeline);
	if(result != vk::Result::eSuccess) {
		console::error("vulkan: could not create pipeline: %s\n", vkResultToString((VkResult)result).c_str());
		exit(1);
	}

	output.framebuffers.resize(this->window->renderImageViews.size());
	for(size_t i = 0; i < this->window->renderImageViews.size(); i++) {
		output.framebuffers[i] = new vk::Framebuffer;
		vk::ImageView attachments[] = { this->window->renderImageViews[i] };
		vk::FramebufferCreateInfo framebufferInfo({}, *output.renderPass, 1, attachments, this->window->swapchainExtent.width, this->window->swapchainExtent.height, 1);

		result = this->window->device.device.createFramebuffer(&framebufferInfo, nullptr, output.framebuffers[i]);
		if(result != vk::Result::eSuccess) {
			console::error("vulkan: could not create framebuffers: %s\n", vkResultToString((VkResult)result).c_str());
			exit(1);
		}
	}

	return output;
}
