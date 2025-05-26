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
		SimpleRenderSystem(VvDevice &device, VkRenderPass renderPass, VkRenderPass offScreenRenderPass);
		~SimpleRenderSystem();

		SimpleRenderSystem(const SimpleRenderSystem&) = delete;
		SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;

		void createBuffers(std::vector<VoxelData>& voxelData, std::vector<std::vector<uint32_t>> models, VkDescriptorImageInfo offscreenDescriptor);
		void renderGameObjects(VkCommandBuffer commandBuffer, std::vector<VvGameObject> &gameObjects, const VvCamera &camera, uint32_t pass);

		void* ssboMappedData1;
		void* ssboMappedData2;

		uint32_t instanceCount = 0;
	private:
		void createPipelineLayout();
		void createPipeline();
		VkRenderPass renderPass;
		VkRenderPass offScreenRenderPass;
		VkBuffer uniformBuffer;
		VkDeviceMemory uniformBufferMemory;
		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		VvDevice& vvDevice;

		std::unique_ptr<VvPipeline> vvPipeline;
		std::unique_ptr<VvPipeline> vvOffscreenPipeline;
		std::unique_ptr<VvPipeline> vvRenderFixPipeline;

		VkPipelineLayout pipelineLayout;
		VkDescriptorSet descriptorSet;
	};
};