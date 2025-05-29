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

		void createBuffers(std::vector<VoxelData>& voxelData, std::vector<std::vector<uint32_t>> models);
		void renderGameObjects(VkCommandBuffer commandBuffer, std::vector<VvGameObject> &gameObjects, const VvCamera &camera);

		void doComputeShader(VkCommandBuffer commandBuffer);

		void* ssboMappedData1;
		void* ssboMappedData2;

		uint32_t instanceCount = 0;
		VkImage storageImage;
		VkImage storageImage1;
		VkImage storageImage2;
		VkBuffer uniformBuffer;
	private:
		void createPipelineLayout();
		void createPipeline();
		VkRenderPass renderPass;
		VkDeviceMemory uniformBufferMemory;
		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		VkWriteDescriptorSet getImageDescriptorWrite();

		VvDevice& vvDevice;

		std::unique_ptr<VvPipeline> vvPipeline;
		VkPipelineLayout pipelineLayout;
		VkDescriptorSet descriptorSet;
	};
};