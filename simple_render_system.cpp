#include "simple_render_system.h"
#include <iostream>
#include <stdexcept>
#include <array>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"

namespace vv {
	struct Matrices {
		glm::mat4 view{ 1.f };
		alignas(16) glm::mat4 inverseView{ 1.f };
		alignas(16) glm::mat4 inverseProjection{ 1.f };
	};

	uint32_t SimpleRenderSystem::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(vvDevice.getPhysicalDevice(), &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("Failed to find suitable memory type!");
	}

	struct SimplePushConstantData {
		glm::mat4 transform{ 1.f };
		glm::mat4 projectionView{ 1.f };
		alignas(16) glm::uint32_t dataOffset;
		alignas(16) glm::ivec3 vbPos;
		alignas(16) glm::ivec3 vbSize;
	};

	SimpleRenderSystem::SimpleRenderSystem(VvDevice& device, VkRenderPass renderPass) : vvDevice{ device }, renderPass(renderPass) {}

	SimpleRenderSystem::~SimpleRenderSystem() {
		vkDestroyPipelineLayout(vvDevice.device(), pipelineLayout, nullptr);
		//to do: destroy everything
	}

	void SimpleRenderSystem::createPipeline() {
		assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

		PipelineConfigInfo pipelineConfig{};
		VvPipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;
		vvPipeline = std::make_unique<VvPipeline>(
			vvDevice,
			"shaders/simple_shader.vert.spv",
			"shaders/simple_shader.frag.spv",
			pipelineConfig
		);
	}

	void SimpleRenderSystem::renderGameObjects(VkCommandBuffer commandBuffer, std::vector<VvGameObject>& gameObjects, const VvCamera& camera) {
		vvPipeline->bind(commandBuffer);

		bool first = true;

		for (auto& obj : gameObjects) {
			SimplePushConstantData push{};
			push.dataOffset = obj.dataOffset;
			push.projectionView = camera.getProjection() * camera.getView();
			push.transform = obj.transform.mat4();
			push.vbPos = glm::floor(obj.transform.translation);
			push.vbSize = glm::floor(obj.transform.scale);
			//obj.transform.translation.x += 0.0001;
			//obj.transform.translation.y += 0.0002;
			//obj.transform.translation.z += 0.0003;

			vkCmdPushConstants(
				commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(SimplePushConstantData),
				&push);

			Matrices matrices;
			matrices.view = camera.getView();
			matrices.inverseView = glm::inverse(camera.getView());
			matrices.inverseProjection = glm::inverse(camera.getProjection());

			void* data;
			vkMapMemory(vvDevice.device(), uniformBufferMemory, 0, sizeof(matrices), 0, &data);
			memcpy(data, &matrices, sizeof(matrices));
			vkUnmapMemory(vvDevice.device(), uniformBufferMemory);

			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

			if (first) 
			{
				obj.model->bind(commandBuffer);
				first = false;
			}

			obj.model->draw(commandBuffer);
		};
	}

	void SimpleRenderSystem::createBuffers(std::vector<VoxelData>& voxelData)
	{
		//Create descriptor pool

		VkDescriptorPoolSize poolSizes[] = {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 }
		};

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 2;
		poolInfo.pPoolSizes = poolSizes;
		poolInfo.maxSets = 1;
		
		VkDescriptorPool descriptorPool;
		if (vkCreateDescriptorPool(vvDevice.device(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) 
			throw std::runtime_error("Failed to create descriptor pool!");

		//Create descriptor set layout with uniform buffer and storage buffer

		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding ssboLayoutBinding{};
		ssboLayoutBinding.binding = 1;
		ssboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		ssboLayoutBinding.descriptorCount = 1;
		ssboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		ssboLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding layoutBindings[] = { uboLayoutBinding, ssboLayoutBinding };

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 2;
		layoutInfo.pBindings = layoutBindings;

		VkDescriptorSetLayout descriptorSetLayout;
		if (vkCreateDescriptorSetLayout(vvDevice.device(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor set layout!");
		}

		//Create pipeline layout & PushConstant
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(SimplePushConstantData);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		if (vkCreatePipelineLayout(vvDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create pipeline layout");
		}

		//Allocate DescriptorSets
		
		VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
		descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocInfo.descriptorPool = descriptorPool;
		descriptorSetAllocInfo.descriptorSetCount = 1;
		descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayout;
		
		if (vkAllocateDescriptorSets(vvDevice.device(), &descriptorSetAllocInfo, &descriptorSet) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate descriptor set!");
		}

		//Allocate memory for uniform buffer (matrices)

		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(Matrices);
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(vvDevice.device(), &bufferInfo, nullptr, &uniformBuffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create uniform buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(vvDevice.device(), uniformBuffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		if (vkAllocateMemory(vvDevice.device(), &allocInfo, nullptr, &uniformBufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate uniform buffer memory!");
		}

		vkBindBufferMemory(vvDevice.device(), uniformBuffer, uniformBufferMemory, 0);

		//Write all buffers to descriptor sets

		std::vector<VkWriteDescriptorSet> descriptorWrites;

		//Write uniform buffer
		VkDescriptorBufferInfo uniformDescriptorBufferInfo{};
		uniformDescriptorBufferInfo.buffer = uniformBuffer;
		uniformDescriptorBufferInfo.offset = 0;
		uniformDescriptorBufferInfo.range = sizeof(Matrices);

		VkWriteDescriptorSet uniformDescriptorWrite{};
		uniformDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		uniformDescriptorWrite.dstSet = descriptorSet;
		uniformDescriptorWrite.dstBinding = 0;
		uniformDescriptorWrite.dstArrayElement = 0;
		uniformDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformDescriptorWrite.descriptorCount = 1;
		uniformDescriptorWrite.pBufferInfo = &uniformDescriptorBufferInfo;

		//Need to write memory manager later
		uint32_t totalSize = 0;
		for (VoxelData& vd : voxelData)
			totalSize += vd.data.size();

		VkBufferCreateInfo ssboInfo{};
		ssboInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		ssboInfo.size = (uint64_t)sizeof(uint32_t) * totalSize;
		ssboInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		ssboInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		//We don't need to modify it at the moment
		VkBuffer ssboBuffer;
		if (vkCreateBuffer(vvDevice.device(), &ssboInfo, nullptr, &ssboBuffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create uniform buffer!");
		}

		VkMemoryRequirements ssboMemRequirements;
		vkGetBufferMemoryRequirements(vvDevice.device(), ssboBuffer, &ssboMemRequirements);

		VkMemoryAllocateInfo ssboAllocInfo{};
		ssboAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		ssboAllocInfo.allocationSize = ssboMemRequirements.size;
		ssboAllocInfo.memoryTypeIndex = findMemoryType(ssboMemRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		VkDeviceMemory ssboBufferMemory;
		if (vkAllocateMemory(vvDevice.device(), &ssboAllocInfo, nullptr, &ssboBufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate uniform buffer memory!");
		}

		vkBindBufferMemory(vvDevice.device(), ssboBuffer, ssboBufferMemory, 0);

		VkDescriptorBufferInfo ssboDescriptorBufferInfo{};
		ssboDescriptorBufferInfo.buffer = ssboBuffer;
		ssboDescriptorBufferInfo.offset = 0;
		ssboDescriptorBufferInfo.range = uint64_t(sizeof(uint32_t)) * totalSize;

		VkWriteDescriptorSet ssboDescriptorWrite{};
		ssboDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		ssboDescriptorWrite.dstSet = descriptorSet;
		ssboDescriptorWrite.dstBinding = 1;
		ssboDescriptorWrite.dstArrayElement = 0;
		ssboDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		ssboDescriptorWrite.descriptorCount = 1;
		ssboDescriptorWrite.pBufferInfo = &ssboDescriptorBufferInfo;

		uint32_t dataOffset = 0;
		for (uint32_t i = 0; i < voxelData.size(); ++i)
		{
			void* ssboMappedData;
			vkMapMemory(vvDevice.device(), ssboBufferMemory, (uint64_t)sizeof(uint32_t) * dataOffset, (uint64_t)sizeof(uint32_t) * voxelData[i].data.size(), 0, &ssboMappedData);
			memcpy(ssboMappedData, voxelData[i].data.data(), (uint64_t)sizeof(uint32_t) * voxelData[i].data.size());
			vkUnmapMemory(vvDevice.device(), ssboBufferMemory);

			dataOffset += voxelData[i].data.size();
		}

		//Push writes
		descriptorWrites.push_back(uniformDescriptorWrite);
		descriptorWrites.push_back(ssboDescriptorWrite);

		vkUpdateDescriptorSets(vvDevice.device(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);

		createPipeline();
	}
}