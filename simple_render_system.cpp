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
		alignas(16) glm::vec3 color;
		alignas(16) glm::ivec3 vbPos;
		alignas(16) glm::ivec3 vbSize;
	};

	SimpleRenderSystem::SimpleRenderSystem(VvDevice& device, VkRenderPass renderPass) : vvDevice{ device } {
		createPipelineLayout();
		createPipeline(renderPass);
	}

	SimpleRenderSystem::~SimpleRenderSystem() {
		vkDestroyPipelineLayout(vvDevice.device(), pipelineLayout, nullptr);
		//to do: destroy everything
	}

	void SimpleRenderSystem::createPipelineLayout() {
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &uboLayoutBinding;

		VkDescriptorSetLayout descriptorSetLayout;
		if (vkCreateDescriptorSetLayout(vvDevice.device(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor set layout!");
		}

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

		//VkBuffer uniformBuffer;
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(Matrices);  // Размер данных
		bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;  // Использование как uniform-буфера
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;  // Режим доступа (обычно EXCLUSIVE)

		if (vkCreateBuffer(vvDevice.device(), &bufferInfo, nullptr, &uniformBuffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create uniform buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(vvDevice.device(), uniformBuffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		uniformBufferMemory = VkDeviceMemory();
		if (vkAllocateMemory(vvDevice.device(), &allocInfo, nullptr, &uniformBufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate uniform buffer memory!");
		}

		vkBindBufferMemory(vvDevice.device(), uniformBuffer, uniformBufferMemory, 0);

		VkDescriptorPoolSize poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = 1;  // Количество дескрипторов

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = 1;  // Максимальное количество дескрипторных наборов

		VkDescriptorPool descriptorPool;
		if (vkCreateDescriptorPool(vvDevice.device(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor pool!");
		}

		VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
		descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocInfo.descriptorPool = descriptorPool;
		descriptorSetAllocInfo.descriptorSetCount = 1;
		descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayout;

		if (vkAllocateDescriptorSets(vvDevice.device(), &descriptorSetAllocInfo, &descriptorSet) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate descriptor set!");
		}

		VkDescriptorBufferInfo descriptorBufferInfo{};
		descriptorBufferInfo.buffer = uniformBuffer;
		descriptorBufferInfo.offset = 0;
		descriptorBufferInfo.range = sizeof(Matrices);  // Размер данных

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSet;
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &descriptorBufferInfo;

		vkUpdateDescriptorSets(vvDevice.device(), 1, &descriptorWrite, 0, nullptr);
	}

	void SimpleRenderSystem::createPipeline(VkRenderPass renderPass) {
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
		

		for (auto& obj : gameObjects) {
			SimplePushConstantData push{};
			push.color = { obj.color };
			push.projectionView = camera.getProjection() * camera.getView();
			push.transform = obj.transform.mat4();
			push.vbPos = obj.transform.translation;
			push.vbSize = obj.transform.scale * glm::float32(10);

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

			obj.model->bind(commandBuffer);
			obj.model->draw(commandBuffer);

		};
	}
}