#pragma once

#include "vv_pipeline.h"
#include "vv_device.hpp"
#include <memory>
#include <vector>
#include "vv_model.h"
#include "vv_game_object.h"
#include "vv_camera.h"

namespace vv {
	class SimpleRenderSystem {
	public:
		SimpleRenderSystem(VvDevice &device, VkRenderPass renderPass);
		~SimpleRenderSystem();

		SimpleRenderSystem(const SimpleRenderSystem&) = delete;
		SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;

		void renderGameObjects(VkCommandBuffer commandBuffer, std::vector<VvGameObject> &gameObjects, const VvCamera &camera);

	private:
		void createPipelineLayout();
		void createPipeline(VkRenderPass renderPass);
		VkBuffer uniformBuffer;
		VkDeviceMemory uniformBufferMemory;
		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		VvDevice& vvDevice;

		std::unique_ptr<VvPipeline> vvPipeline;
		VkPipelineLayout pipelineLayout;
		VkDescriptorSet descriptorSet;
	};
};