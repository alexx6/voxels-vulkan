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
			"shaders/simple_shader.comp.spv",
			pipelineConfig
		);
	}

	void SimpleRenderSystem::renderGameObjects(VkCommandBuffer commandBuffer, std::vector<VvGameObject>& gameObjects, const VvCamera& camera) {		
		vvPipeline->bind(commandBuffer);

		bool first = true;

		auto& obj = gameObjects[0];
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

		//VkBufferMemoryBarrier barrier{};
		//barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		//barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT; // Запись с CPU
		//barrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT; // Чтение в шейдере
		//barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		//barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		//barrier.buffer = uniformBuffer; // Ваш uniform buffer
		//barrier.offset = 0;
		//barrier.size = VK_WHOLE_SIZE;

		//vkCmdPipelineBarrier(
		//	commandBuffer,
		//	VK_PIPELINE_STAGE_HOST_BIT, // Стадия хоста
		//	VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, // Стадии шейдеров
		//	0,
		//	0, nullptr,
		//	1, &barrier,
		//	0, nullptr
		//);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

		obj.model->bind(commandBuffer);
		obj.model->draw(commandBuffer, instanceCount);
	}

	void SimpleRenderSystem::doComputeShader(VkCommandBuffer commandBuffer)
	{
		vvPipeline->bindCompute(commandBuffer);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
		vkCmdDispatch(commandBuffer, 1920 / 16, 1080 / 16, 1);
	}

	void SimpleRenderSystem::createBuffers(std::vector<VoxelData>& voxelData, std::vector<std::vector<uint32_t>> models)
	{
		//Create descriptor pool

		VkDescriptorPoolSize poolSizes[] = {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  3 }
		};

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 3;
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
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding ssboLayoutBinding1{};
		ssboLayoutBinding1.binding = 1;
		ssboLayoutBinding1.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		ssboLayoutBinding1.descriptorCount = 1;
		ssboLayoutBinding1.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		ssboLayoutBinding1.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding ssboLayoutBinding2{};
		ssboLayoutBinding2.binding = 2;
		ssboLayoutBinding2.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		ssboLayoutBinding2.descriptorCount = 1;
		ssboLayoutBinding2.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		ssboLayoutBinding2.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding storageImageBinding{};
		storageImageBinding.binding = 3;
		storageImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		storageImageBinding.descriptorCount = 1;
		storageImageBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

		VkDescriptorSetLayoutBinding storageImageBinding1{};
		storageImageBinding1.binding = 4;
		storageImageBinding1.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		storageImageBinding1.descriptorCount = 1;
		storageImageBinding1.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

		VkDescriptorSetLayoutBinding storageImageBinding2{};
		storageImageBinding2.binding = 5;
		storageImageBinding2.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		storageImageBinding2.descriptorCount = 1;
		storageImageBinding2.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
		//VkDescriptorSetLayoutBinding storageImageBinding2{};
		//storageImageBinding2.binding = 4;
		//storageImageBinding2.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		//storageImageBinding2.descriptorCount = 1;
		//storageImageBinding2.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding layoutBindings[] = { uboLayoutBinding, 
			ssboLayoutBinding1, ssboLayoutBinding2, storageImageBinding, storageImageBinding1, storageImageBinding2 };

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 6;
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

		//Load models to gpu
		uint32_t totalSize1 = 0;
		for (uint32_t i = 0; i < models.size(); ++i)
			totalSize1 += models[i].size();

		VkBufferCreateInfo ssboInfo1{};
		ssboInfo1.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		ssboInfo1.size = (uint64_t)sizeof(uint32_t) * totalSize1;
		ssboInfo1.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		ssboInfo1.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkBuffer ssboBuffer1;
		if (vkCreateBuffer(vvDevice.device(), &ssboInfo1, nullptr, &ssboBuffer1) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create uniform buffer!");
		}

		VkMemoryRequirements ssboMemRequirements1;
		vkGetBufferMemoryRequirements(vvDevice.device(), ssboBuffer1, &ssboMemRequirements1);

		VkMemoryAllocateInfo ssboAllocInfo1{};
		ssboAllocInfo1.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		ssboAllocInfo1.allocationSize = ssboMemRequirements1.size;
		ssboAllocInfo1.memoryTypeIndex = findMemoryType(ssboMemRequirements1.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		VkDeviceMemory ssboBufferMemory1;
		if (vkAllocateMemory(vvDevice.device(), &ssboAllocInfo1, nullptr, &ssboBufferMemory1) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate uniform buffer memory!");
		}

		vkBindBufferMemory(vvDevice.device(), ssboBuffer1, ssboBufferMemory1, 0);

		VkDescriptorBufferInfo ssboDescriptorBufferInfo1{};
		ssboDescriptorBufferInfo1.buffer = ssboBuffer1;
		ssboDescriptorBufferInfo1.offset = 0;
		ssboDescriptorBufferInfo1.range = uint64_t(sizeof(uint32_t)) * totalSize1;

		VkWriteDescriptorSet ssboDescriptorWrite1{};
		ssboDescriptorWrite1.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		ssboDescriptorWrite1.dstSet = descriptorSet;
		ssboDescriptorWrite1.dstBinding = 1;
		ssboDescriptorWrite1.dstArrayElement = 0;
		ssboDescriptorWrite1.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		ssboDescriptorWrite1.descriptorCount = 1;
		ssboDescriptorWrite1.pBufferInfo = &ssboDescriptorBufferInfo1;

		uint32_t dataOffset = 0;
		for (uint32_t i = 0; i < models.size(); ++i)
		{
			vkMapMemory(vvDevice.device(), ssboBufferMemory1, (uint64_t)sizeof(uint32_t) * dataOffset, (uint64_t)sizeof(uint32_t) * models[i].size(), 0, &ssboMappedData1);
			memcpy(ssboMappedData1, models[i].data(), (uint64_t)sizeof(uint32_t) * models[i].size());
			vkUnmapMemory(vvDevice.device(), ssboBufferMemory1);

			dataOffset += models[i].size();
		}

		//Load voxel data to gpu
		VkBufferCreateInfo ssboInfo2{};
		ssboInfo2.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		ssboInfo2.size = (uint64_t)sizeof(VoxelData) * voxelData.size();
		ssboInfo2.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		ssboInfo2.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkBuffer ssboBuffer2;
		if (vkCreateBuffer(vvDevice.device(), &ssboInfo2, nullptr, &ssboBuffer2) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create uniform buffer!");
		}

		VkMemoryRequirements ssboMemRequirements2;
		vkGetBufferMemoryRequirements(vvDevice.device(), ssboBuffer2, &ssboMemRequirements2);

		VkMemoryAllocateInfo ssboAllocInfo2{};
		ssboAllocInfo2.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		ssboAllocInfo2.allocationSize = ssboMemRequirements2.size;
		ssboAllocInfo2.memoryTypeIndex = findMemoryType(ssboMemRequirements1.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		VkDeviceMemory ssboBufferMemory2;
		if (vkAllocateMemory(vvDevice.device(), &ssboAllocInfo2, nullptr, &ssboBufferMemory2) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate uniform buffer memory!");
		}

		vkBindBufferMemory(vvDevice.device(), ssboBuffer2, ssboBufferMemory2, 0);

		VkDescriptorBufferInfo ssboDescriptorBufferInfo2{};
		ssboDescriptorBufferInfo2.buffer = ssboBuffer2;
		ssboDescriptorBufferInfo2.offset = 0;
		ssboDescriptorBufferInfo2.range = (uint64_t)sizeof(VoxelData) * voxelData.size();

		VkWriteDescriptorSet ssboDescriptorWrite2{};
		ssboDescriptorWrite2.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		ssboDescriptorWrite2.dstSet = descriptorSet;
		ssboDescriptorWrite2.dstBinding = 2;
		ssboDescriptorWrite2.dstArrayElement = 0;
		ssboDescriptorWrite2.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		ssboDescriptorWrite2.descriptorCount = 1;
		ssboDescriptorWrite2.pBufferInfo = &ssboDescriptorBufferInfo2;

		vkMapMemory(vvDevice.device(), ssboBufferMemory2, 0, (uint64_t)sizeof(VoxelData) * voxelData.size(), 0, &ssboMappedData2);
		memcpy(ssboMappedData2, voxelData.data(), (uint64_t)sizeof(VoxelData) * voxelData.size());
		vkUnmapMemory(vvDevice.device(), ssboBufferMemory2);

		VkFormat storageFormat = VK_FORMAT_R32_UINT;

		// Создаем изображение
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = 1920;
		imageInfo.extent.height = 1080;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = storageFormat;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		vkCreateImage(vvDevice.device(), &imageInfo, nullptr, &storageImage);

		// Выделяем память
		VkMemoryRequirements imageMemRequirements;
		vkGetImageMemoryRequirements(vvDevice.device(), storageImage, &imageMemRequirements);

		VkMemoryAllocateInfo imageAllocInfo{};
		imageAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		imageAllocInfo.allocationSize = imageMemRequirements.size;
		imageAllocInfo.memoryTypeIndex = findMemoryType(imageMemRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		VkDeviceMemory storageImageMemory;
		vkAllocateMemory(vvDevice.device(), &imageAllocInfo, nullptr, &storageImageMemory);
		vkBindImageMemory(vvDevice.device(), storageImage, storageImageMemory, 0);

		// Создаем image view
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = storageImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = storageFormat;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView storageImageView;
		vkCreateImageView(vvDevice.device(), &viewInfo, nullptr, &storageImageView);

		VkDescriptorImageInfo descriptorImageInfo{};
		descriptorImageInfo.imageView = storageImageView;
		descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkWriteDescriptorSet descriptorImageWrite{};
		descriptorImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorImageWrite.dstSet = descriptorSet;
		descriptorImageWrite.dstBinding = 3;
		descriptorImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		descriptorImageWrite.descriptorCount = 1;
		descriptorImageWrite.pImageInfo = &descriptorImageInfo;





		// Создаем изображение
		VkImageCreateInfo imageInfo1{};
		imageInfo1.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo1.imageType = VK_IMAGE_TYPE_2D;
		imageInfo1.extent.width = 1920;
		imageInfo1.extent.height = 1080;
		imageInfo1.extent.depth = 1;
		imageInfo1.mipLevels = 1;
		imageInfo1.arrayLayers = 1;
		imageInfo1.format = storageFormat;
		imageInfo1.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo1.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo1.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo1.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo1.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		vkCreateImage(vvDevice.device(), &imageInfo1, nullptr, &storageImage1);

		// Выделяем память
		VkMemoryRequirements imageMemRequirements1;
		vkGetImageMemoryRequirements(vvDevice.device(), storageImage1, &imageMemRequirements1);

		VkMemoryAllocateInfo imageAllocInfo1{};
		imageAllocInfo1.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		imageAllocInfo1.allocationSize = imageMemRequirements1.size;
		imageAllocInfo1.memoryTypeIndex = findMemoryType(imageMemRequirements1.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		VkDeviceMemory storageImageMemory1;
		vkAllocateMemory(vvDevice.device(), &imageAllocInfo1, nullptr, &storageImageMemory1);
		vkBindImageMemory(vvDevice.device(), storageImage1, storageImageMemory1, 0);

		// Создаем image view
		VkImageViewCreateInfo viewInfo1{};
		viewInfo1.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo1.image = storageImage1;
		viewInfo1.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo1.format = storageFormat;
		viewInfo1.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo1.subresourceRange.baseMipLevel = 0;
		viewInfo1.subresourceRange.levelCount = 1;
		viewInfo1.subresourceRange.baseArrayLayer = 0;
		viewInfo1.subresourceRange.layerCount = 1;

		VkImageView storageImageView1;
		vkCreateImageView(vvDevice.device(), &viewInfo1, nullptr, &storageImageView1);

		VkDescriptorImageInfo descriptorImageInfo1{};
		descriptorImageInfo1.imageView = storageImageView1;
		descriptorImageInfo1.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkWriteDescriptorSet descriptorImageWrite1{};
		descriptorImageWrite1.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorImageWrite1.dstSet = descriptorSet;
		descriptorImageWrite1.dstBinding = 4;
		descriptorImageWrite1.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		descriptorImageWrite1.descriptorCount = 1;
		descriptorImageWrite1.pImageInfo = &descriptorImageInfo1;




		// Создаем изображение
		VkImageCreateInfo imageInfo2{};
		imageInfo2.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo2.imageType = VK_IMAGE_TYPE_2D;
		imageInfo2.extent.width = 1920;
		imageInfo2.extent.height = 1080;
		imageInfo2.extent.depth = 1;
		imageInfo2.mipLevels = 1;
		imageInfo2.arrayLayers = 1;
		imageInfo2.format = storageFormat;
		imageInfo2.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo2.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo2.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo2.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo2.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		vkCreateImage(vvDevice.device(), &imageInfo2, nullptr, &storageImage2);

		// Выделяем память
		VkMemoryRequirements imageMemRequirements2;
		vkGetImageMemoryRequirements(vvDevice.device(), storageImage2, &imageMemRequirements2);

		VkMemoryAllocateInfo imageAllocInfo2{};
		imageAllocInfo2.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		imageAllocInfo2.allocationSize = imageMemRequirements2.size;
		imageAllocInfo2.memoryTypeIndex = findMemoryType(imageMemRequirements2.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		VkDeviceMemory storageImageMemory2;
		vkAllocateMemory(vvDevice.device(), &imageAllocInfo2, nullptr, &storageImageMemory2);
		vkBindImageMemory(vvDevice.device(), storageImage2, storageImageMemory2, 0);

		// Создаем image view
		VkImageViewCreateInfo viewInfo2{};
		viewInfo2.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo2.image = storageImage2;
		viewInfo2.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo2.format = storageFormat;
		viewInfo2.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo2.subresourceRange.baseMipLevel = 0;
		viewInfo2.subresourceRange.levelCount = 1;
		viewInfo2.subresourceRange.baseArrayLayer = 0;
		viewInfo2.subresourceRange.layerCount = 1;

		VkImageView storageImageView2;
		vkCreateImageView(vvDevice.device(), &viewInfo2, nullptr, &storageImageView2);

		VkDescriptorImageInfo descriptorImageInfo2{};
		descriptorImageInfo2.imageView = storageImageView2;
		descriptorImageInfo2.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkWriteDescriptorSet descriptorImageWrite2{};
		descriptorImageWrite2.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorImageWrite2.dstSet = descriptorSet;
		descriptorImageWrite2.dstBinding = 5;
		descriptorImageWrite2.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		descriptorImageWrite2.descriptorCount = 1;
		descriptorImageWrite2.pImageInfo = &descriptorImageInfo2;

		//Push writes
		descriptorWrites.push_back(uniformDescriptorWrite);
		descriptorWrites.push_back(ssboDescriptorWrite1);
		descriptorWrites.push_back(ssboDescriptorWrite2);
		descriptorWrites.push_back(descriptorImageWrite);
		descriptorWrites.push_back(descriptorImageWrite1);
		descriptorWrites.push_back(descriptorImageWrite2);

		vkUpdateDescriptorSets(vvDevice.device(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);

		createPipeline();
	}

	//VkWriteDescriptorSet SimpleRenderSystem::getImageDescriptorWrite()
	//{
	//	// Выбираем формат (например, RGBA32F для хранения vec4)
	//	
	//}
}