#pragma once

#ifndef __switch__
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

namespace render {
	struct VulkanBuffer {
		friend class Window;
		
		public:
			VulkanBuffer() {}
			VulkanBuffer(class Window* window) {
				this->window = window;
			}
			
			void destroy();

			void* map();
			void unmap();

			inline bool isValid() {
				return this->valid;
			}

			inline vk::Buffer getBuffer() {
				return this->buffer;
			}

		private:
			class Window* window = nullptr;
			
			vk::Buffer buffer;
			vk::DeviceMemory memory;

			vk::DeviceSize size = 0;

			bool valid = true;

			void* mappedMemory = nullptr;
	};
};
#endif
