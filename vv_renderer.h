#pragma once

#include "vv_window.h"
#include "vv_device.hpp"
#include "vv_swap_chain.hpp"
#include <memory>
#include <vector>
#include "vv_model.h"
#include <cassert>

namespace vv {
	class VvRenderer {
	public:
		VvRenderer(VvWindow& window, VvDevice& device);
		~VvRenderer();

		VvRenderer(const VvRenderer&) = delete;
		VvRenderer& operator=(const VvRenderer&) = delete;

		VkRenderPass getSwapChainRenderPass() const { return vvSwapChain->getRenderPass(); };
		VkRenderPass getOffScreenRenderPass() const { return vvSwapChain->getOffscreenPass().renderPass; };

		float getAspectRatio() const { return vvSwapChain->extentAspectRatio(); };
		bool isFrameInProgress() const { return isFrameStarted; };

		VkCommandBuffer getCurrentCommandBuffer() const {
			assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
			return commandBuffers[currentFrameIndex];
		}

		int getFrameIndex() const {
			assert(isFrameStarted && "Cannot get frame index when frame not in progress");
			return currentFrameIndex;
		};

		VkCommandBuffer beginFrame();
		void endFrame();
		void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
		void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

		void beginOffscreenRenderPass(VkCommandBuffer commandBuffer);

		void endOffscreenRenderPass(VkCommandBuffer commandBuffer);

		VkDescriptorImageInfo getOffscreenDescriptor() const;

	private:
		void createCommandBuffers();
		void freeCommandBuffers();
		void recreateSwapChain();

		VvWindow& vvWindow;
		VvDevice& vvDevice;
		std::unique_ptr<VvSwapChain> vvSwapChain;
		std::vector<VkCommandBuffer> commandBuffers;

		uint32_t currentImageIndex;
		int currentFrameIndex;
		bool isFrameStarted;
	};
};