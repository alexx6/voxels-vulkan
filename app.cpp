#include "app.h"
#include <iostream>
#include <stdexcept>
#include <array>
#include "vv_camera.h"
#include "keyboard_movement_controller.h"
#include <fstream>
#include "VoxelTree.h"
#include "WorldGenerator.h"
#include <future>
#include <mutex>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"

#include <chrono>

std::mutex writeMutex;

namespace vv {
	App::App() {
	}

	App::~App() {
	}

	void App::run() {
		SimpleRenderSystem simpleRenderSystem{ vvDevice, vvRenderer.getSwapChainRenderPass() };

    loadGameObjects(simpleRenderSystem);

    VvCamera camera{};
    //camera.setViewDirection(glm::vec3(0.f), glm::vec3(0.5f, 0.f, 1.f));
    //camera.setViewTarget(glm::vec3(-1.f, -2.f, 2.f), glm::vec3(0.f, 0.f, 2.5f));

    auto viewerObject = VvGameObject::createGameObject();
    viewerObject.transform.translation = glm::vec3(15000.f, 15000.f, -5000.f);
    KeyboardMovementController cameraController{};

    auto currentTime = std::chrono::high_resolution_clock::now();

    uint32_t framesCounter = 0;
    float framesCounterTime = 0.f;

		while (!vvWindow.shouldClose()) {
			glfwPollEvents();

      auto newTime = std::chrono::high_resolution_clock::now();
      float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
      currentTime = newTime;

      framesCounterTime += frameTime;
      ++framesCounter;

      if (framesCounterTime >= 1.f) 
      {
        std::cout << "CURRENT FPS: " << framesCounter / framesCounterTime << std::endl;
        framesCounter = 0;
        framesCounterTime = 0.f;
      }

      cameraController.moveInPlaneXZ(vvWindow.getGLFWwindow(), frameTime, viewerObject);
      camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

      float aspect = vvRenderer.getAspectRatio();
      //camera.setOrthographicProjection(-aspect, aspect, -1, 1, -1, 1);
      camera.setPerspectiveProjection(glm::radians(60.f), aspect, 0.0f, 2000000.f);

			if (auto commandBuffer = vvRenderer.beginFrame()) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Текущий layout
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;   // Требуемый layout
        barrier.image = simpleRenderSystem.storageImage;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

        vkCmdPipelineBarrier(
          commandBuffer,
          VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,    // Самые ранние возможные стадии
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, // Или FRAGMENT_SHADER_BIT
          0,
          0, nullptr,
          0, nullptr,
          1, &barrier
        );

        VkImageMemoryBarrier barrier1{};
        barrier1.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier1.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Текущий layout
        barrier1.newLayout = VK_IMAGE_LAYOUT_GENERAL;   // Требуемый layout
        barrier1.image = simpleRenderSystem.storageImage1;
        barrier1.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier1.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier1.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier1.subresourceRange.baseMipLevel = 0;
        barrier1.subresourceRange.levelCount = 1;
        barrier1.subresourceRange.baseArrayLayer = 0;
        barrier1.subresourceRange.layerCount = 1;
        barrier1.srcAccessMask = 0;
        barrier1.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

        vkCmdPipelineBarrier(
          commandBuffer,
          VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,    // Самые ранние возможные стадии
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, // Или FRAGMENT_SHADER_BIT
          0,
          0, nullptr,
          0, nullptr,
          1, &barrier1
        );

        VkImageMemoryBarrier barrier2{};
        barrier2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier2.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Текущий layout
        barrier2.newLayout = VK_IMAGE_LAYOUT_GENERAL;   // Требуемый layout
        barrier2.image = simpleRenderSystem.storageImage2;
        barrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier2.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier2.subresourceRange.baseMipLevel = 0;
        barrier2.subresourceRange.levelCount = 1;
        barrier2.subresourceRange.baseArrayLayer = 0;
        barrier2.subresourceRange.layerCount = 1;
        barrier2.srcAccessMask = 0;
        barrier2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

        vkCmdPipelineBarrier(
          commandBuffer,
          VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
          VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
          0,
          0, nullptr,
          0, nullptr,
          1, &barrier2
        );

				vvRenderer.beginSwapChainRenderPass(commandBuffer);
				simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects, camera);
				vvRenderer.endSwapChainRenderPass(commandBuffer);

        simpleRenderSystem.doComputeShader(commandBuffer);

				vvRenderer.endFrame();
			}
		}
		vkDeviceWaitIdle(vvDevice.device());
	}

    std::unique_ptr<VvModel> createCubeModel(VvDevice& device, glm::vec3 offset) {
        VvModel::Builder modelBuilder{};
        modelBuilder.vertices = {
          {{0.f, 0.f, 0.f}, {.9f, .9f, .9f}},
          {{0.f, 1.f, 0.f}, {.9f, .9f, .9f}},
          {{0.f, 1.f, 1.f}, {.9f, .9f, .9f}},
          {{0.f, 0.f, 1.f}, {.9f, .9f, .9f}},

          {{1.f, 0.f, 0.f}, {.9f, .9f, .9f}},
          {{1.f, 1.f, 0.f}, {.9f, .9f, .9f}},
          {{1.f, 1.f, 1.f}, {.9f, .9f, .9f}},
          {{1.f, 0.f, 1.f}, {.9f, .9f, .9f}},
        };

        for (auto& v : modelBuilder.vertices) {
          v.position += offset;
        }

        modelBuilder.indices = {
          0, 1, 2,  0, 2, 3,
          4, 5, 1,  4, 1, 0,
          7, 6, 5,  7, 5, 4,
          3, 2, 6,  3, 6, 7,
          1, 5, 6,  1, 6, 2,
          4, 0, 3,  4, 3, 7
        };

        return std::make_unique<VvModel>(device, modelBuilder);
    }

    VoxelModel loadVoxelModel(std::string modelName)
    {
      std::ifstream f;
      f.open("./" + modelName + ".qb", std::ios::in | std::ios::binary);

      //
      uint32_t sizeX;
      //

      uint32_t version;
      f.read((char*)&version, sizeof(version));

      uint32_t colorFormat;
      f.read((char*)&colorFormat, sizeof(colorFormat));

      uint32_t zAxisOrientation;
      f.read((char*)&zAxisOrientation, sizeof(zAxisOrientation));

      uint32_t compressed;
      f.read((char*)&compressed, sizeof(compressed));

      uint32_t visibilityMaskEncoded;
      f.read((char*)&visibilityMaskEncoded, sizeof(visibilityMaskEncoded));

      uint32_t numMatrices;
      f.read((char*)&numMatrices, sizeof(numMatrices));

      std::vector<std::vector<uint32_t>> resultData(numMatrices);

      for (uint32_t i = 0; i < numMatrices; ++i)
      {
        char matrixNameLength;
        f.read((char*)&matrixNameLength, sizeof(matrixNameLength));

        char* matrixName = new char[matrixNameLength + 1];
        f.read(matrixName, matrixNameLength);

        f.read((char*)&sizeX, sizeof(sizeX));

        uint32_t sizeY;
        f.read((char*)&sizeY, sizeof(sizeY));

        uint32_t sizeZ;
        f.read((char*)&sizeZ, sizeof(sizeZ));

        int32_t posX;
        f.read((char*)&posX, sizeof(posX));

        int32_t posY;
        f.read((char*)&posY, sizeof(posY));

        int32_t posZ;
        f.read((char*)&posZ, sizeof(posZ));

        std::vector<uint32_t> voxels(sizeX * sizeY * sizeZ);

        if (compressed == 0)
        {
          f.read((char*)voxels.data(), sizeof(uint32_t) * sizeX * sizeY * sizeZ);
        }
        else 
        {
          uint32_t x = 0;
          uint32_t y = 0;
          uint32_t z = 0;
          uint32_t data = 0;
          uint32_t index = 0;
          uint32_t count = 0;
          const uint32_t CODEFLAG = 2;
          const uint32_t NEXTSLICEFLAG = 6;

          while (z < sizeZ)
          {
            index = 0;

            while (true) 
            {
              f.read((char*)&data, sizeof(data));

              if (data == NEXTSLICEFLAG)
              {
                break;
              }
              else if (data == CODEFLAG) 
              {
                f.read((char*)&count, sizeof(count));
                f.read((char*)&data, sizeof(data));

                for (uint32_t j = 0; j < count; ++j)
                {
                  x = index % sizeX;
                  y = index / sizeX;

                  index++;
                  voxels[x + y * sizeX + z * sizeX * sizeY] = data;
                }
              }
              else
              {
                x = index % sizeX;
                y = index / sizeX;
                index++;
                voxels[x + y * sizeX + z * sizeX * sizeY] = data;
              }
            }

            ++z;
          }
        }

        //for (uint32_t z = 0; z < sizeZ; ++z)
        //{
        //  for (uint32_t y = 0; y < sizeY; ++y)
        //  {
        //    for (uint32_t x = 0; x < sizeX; ++x)
        //    {
        //    }
        //  }
        //}

        //VoxelData vd
        //{
        //  glm::ivec3{posX, posY, posZ},
        //  glm::ivec3{sizeX, sizeY, sizeZ},
        //  voxels
        //};

        resultData[i] = voxels;
      }

      return { sizeX, resultData[0] };
    }

	void App::loadGameObjects(SimpleRenderSystem &simpleRenderSystem) {
        std::shared_ptr<VvModel> vvModel = createCubeModel(vvDevice, { .0f, .0f, .0f });

        //std::vector<VoxelData> vd = loadVoxelModel();

        std::vector<std::string> modelNames = { "rock_red", "crystal" };

        for (int i = 0; i < modelNames.size(); ++i)
        {
          struct stat buf;

          if (stat(modelNames[i].c_str(), &buf) != -1)
          {
            continue;
          }

          VoxelModel model = VoxelTree::getCompressedData(loadVoxelModel(modelNames[i]));
          
          std::ofstream file(modelNames[i], std::ios::binary);

          uint32_t size = model.modelData.size();
          file.write(reinterpret_cast<const char*>(&size), sizeof(uint32_t));

          uint32_t modelSize = model.size;
          file.write(reinterpret_cast<const char*>(&modelSize), sizeof(uint32_t));

          file.write(reinterpret_cast<const char*>(model.modelData.data()), size * sizeof(uint32_t));
          file.close();
        }

        WorldGenerator generator;
        std::vector<std::vector<uint32_t>> models;

        std::vector<uint32_t> modelColors;
        uint32_t curOffset = 0;

        for (int i = 0; i < modelNames.size(); ++i)
        {
          std::ifstream file(modelNames[i], std::ios::binary);

          if (!file) 
          {
            std::cerr << "File \"" << modelNames[i] << "\" not found." << std::endl;
          }

          uint32_t size;
          file.read(reinterpret_cast<char*>(&size), sizeof(uint32_t));

          uint32_t modelSize;
          file.read(reinterpret_cast<char*>(&modelSize), sizeof(uint32_t));

          std::vector<uint32_t> modelData(size);
          file.read(reinterpret_cast<char*>(modelData.data()), size * sizeof(uint32_t));
          file.close();


          models.push_back(modelData);

          generator.addModelInfo(modelSize, modelData.size());

          modelColors.push_back(modelData[1]);

          curOffset += modelData.size();
        }

        std::vector<VoxelData> vd;

        glm::ivec3 worldSize(1, 1, 1);

        //const int numThreads = std::thread::hardware_concurrency();
        //const int totalChunks = worldSize.x * worldSize.y * worldSize.z;
        //const int groupSize = totalChunks / numThreads;
        //
        //struct GenResult
        //{
        //  VoxelData data;
        //  VoxelModel model;
        //};

        //std::vector<std::future<std::vector<VoxelData>>> futures;
        //
        //VoxelChunk chunk = generator.generateChunk(glm::ivec3(0, 0, 0));

        //for (int t = 0; t < numThreads; ++t)
        //{
        //  int start = t * groupSize;
        //  int end = (t == numThreads - 1) ? totalChunks : start + groupSize;


        //  futures.push_back(std::async(std::launch::async, [this, &generator, &worldSize, &models, &modelColors, &modelNames, &chunk, start, end]() {
        //    std::vector<VoxelData> localVd;

        //    for (int i = start; i < end; ++i)
        //    {
        //      int x = i % worldSize.x;
        //      int y = i / worldSize.x / worldSize.z;
        //      int z = i / worldSize.x % worldSize.z;

        //      std::vector<uint32_t> md = VoxelTree::voxelizeChunk(chunk, modelColors).modelData;

        //      {
        //        std::lock_guard<std::mutex> lock(writeMutex);
        //        models.push_back(md);
        //        generator.addModelInfo(128 * 256, models.back().size());

        //        localVd.push_back(generator.getVoxelInstance(glm::ivec3(x * 64 * 256, y * 64 * 256, z * 64 * 256), 0, modelNames.size(), true));
        //      }

        //      std::cout << "CURRENT GENERATION PROGRESS : " << z + x * worldSize.x + y * worldSize.x * worldSize.y << std::endl;

        //    }

        //    return localVd;
        //  }));
        //}

        //for (auto& future : futures) {
        //  auto result = future.get();
        //  vd.insert(vd.end(), result.begin(), result.end());
        //}

        //for (int i = 0; i < worldSize.x; ++i)
        //{
        //  for (int j = 0; j < worldSize.y; ++j)
        //  {
        //    for (int k = 0; k < worldSize.z; ++k)
        //    {
        //      VoxelChunk chunk = generator.generateChunk(glm::ivec3(i, j, k));

        //      models.push_back(VoxelTree::voxelizeChunk(chunk, modelColors).modelData);

        //      generator.addModelInfo(128 * 256, models.back().size());

        //      //vd.insert(vd.end(), chunk.chunkData.begin(), chunk.chunkData.end());
        //      vd.push_back(generator.getVoxelInstance(glm::ivec3(i * 64 * 256, j * 64 * 256, k * 64 * 256), 0, models.size() - 1, true));

        //      std::cout << "CURRENT GENERATION PROGRESS : " << k + j * worldSize.x + i * worldSize.x * worldSize.y << std::endl;
        //    } 
        //  }
        //}

        VoxelChunk chunk = generator.generateChunk(glm::ivec3(0, 0, 0));
        vd.insert(vd.end(), chunk.chunkData.begin(), chunk.chunkData.end());

        chunk = generator.generateChunk(glm::ivec3(0, 0, 1));
        vd.insert(vd.end(), chunk.chunkData.begin(), chunk.chunkData.end());

        chunk = generator.generateChunk(glm::ivec3(0, 0, 2));
        vd.insert(vd.end(), chunk.chunkData.begin(), chunk.chunkData.end());

        chunk = generator.generateChunk(glm::ivec3(0, 1, 0));
        vd.insert(vd.end(), chunk.chunkData.begin(), chunk.chunkData.end());

        chunk = generator.generateChunk(glm::ivec3(0, 1, 1));
        vd.insert(vd.end(), chunk.chunkData.begin(), chunk.chunkData.end());

        chunk = generator.generateChunk(glm::ivec3(0, 1, 2));
        vd.insert(vd.end(), chunk.chunkData.begin(), chunk.chunkData.end());

        chunk = generator.generateChunk(glm::ivec3(0, 2, 0));
        vd.insert(vd.end(), chunk.chunkData.begin(), chunk.chunkData.end());

        chunk = generator.generateChunk(glm::ivec3(0, 2, 1));
        vd.insert(vd.end(), chunk.chunkData.begin(), chunk.chunkData.end());

        chunk = generator.generateChunk(glm::ivec3(0, 2, 2));
        vd.insert(vd.end(), chunk.chunkData.begin(), chunk.chunkData.end());

        chunk = generator.generateChunk(glm::ivec3(1, 0, 0));
        vd.insert(vd.end(), chunk.chunkData.begin(), chunk.chunkData.end());

        chunk = generator.generateChunk(glm::ivec3(1, 0, 1));
        vd.insert(vd.end(), chunk.chunkData.begin(), chunk.chunkData.end());

        chunk = generator.generateChunk(glm::ivec3(1, 0, 2));
        vd.insert(vd.end(), chunk.chunkData.begin(), chunk.chunkData.end());

        chunk = generator.generateChunk(glm::ivec3(1, 1, 0));
        vd.insert(vd.end(), chunk.chunkData.begin(), chunk.chunkData.end());

        chunk = generator.generateChunk(glm::ivec3(1, 1, 1));
        vd.insert(vd.end(), chunk.chunkData.begin(), chunk.chunkData.end());

        chunk = generator.generateChunk(glm::ivec3(1, 1, 2));
        vd.insert(vd.end(), chunk.chunkData.begin(), chunk.chunkData.end());

        chunk = generator.generateChunk(glm::ivec3(1, 2, 0));
        vd.insert(vd.end(), chunk.chunkData.begin(), chunk.chunkData.end());

        chunk = generator.generateChunk(glm::ivec3(1, 2, 1));
        vd.insert(vd.end(), chunk.chunkData.begin(), chunk.chunkData.end());

        chunk = generator.generateChunk(glm::ivec3(1, 2, 2));
        vd.insert(vd.end(), chunk.chunkData.begin(), chunk.chunkData.end());

        chunk = generator.generateChunk(glm::ivec3(2, 0, 0));
        vd.insert(vd.end(), chunk.chunkData.begin(), chunk.chunkData.end());

        chunk = generator.generateChunk(glm::ivec3(2, 0, 1));
        vd.insert(vd.end(), chunk.chunkData.begin(), chunk.chunkData.end());

        chunk = generator.generateChunk(glm::ivec3(2, 0, 2));
        vd.insert(vd.end(), chunk.chunkData.begin(), chunk.chunkData.end());

        chunk = generator.generateChunk(glm::ivec3(2, 1, 0));
        vd.insert(vd.end(), chunk.chunkData.begin(), chunk.chunkData.end());

        chunk = generator.generateChunk(glm::ivec3(2, 1, 1));
        vd.insert(vd.end(), chunk.chunkData.begin(), chunk.chunkData.end());

        chunk = generator.generateChunk(glm::ivec3(2, 1, 2));
        vd.insert(vd.end(), chunk.chunkData.begin(), chunk.chunkData.end());

        chunk = generator.generateChunk(glm::ivec3(2, 2, 0));
        //vd.insert(vd.end(), chunk.chunkData.begin(), chunk.chunkData.end());
        models.push_back(VoxelTree::voxelizeChunk(chunk, modelColors).modelData);
        generator.addModelInfo(128 * 128, models.back().size());
        //vd.insert(vd.end(), chunk.chunkData.begin(), chunk.chunkData.end());
        vd.push_back(generator.getVoxelInstance(glm::ivec3(2 * 64 * 256, 2 * 64 * 256, 0 * 64 * 256), 0, models.size() - 1, true));
        
        
        chunk = generator.generateChunk(glm::ivec3(2, 2, 1));
        vd.insert(vd.end(), chunk.chunkData.begin(), chunk.chunkData.end());

        chunk = generator.generateChunk(glm::ivec3(2, 2, 2));
        vd.insert(vd.end(), chunk.chunkData.begin(), chunk.chunkData.end());
        //models.push_back(VoxelTree::getCompressedData(generator.generateChunkShellModel()).modelData);
        //vd.push_back(generator.getVoxelInstance(glm::ivec3(0), 0, modelNames.size()));

        //for (int i = 0; i < 1000; ++i) 
        //{
        //  uint32_t modelId = i % modelNames.size();

        //  VoxelData voxel;
        //  voxel.pos = glm::ivec3(((i / 10) % 10) * 690, i / 100 * 690, (i % 10) * 690);
        //  voxel.size = modelSizes[modelId];
        //  voxel.orientation = i % 24;
        //  voxel.modelOffset = modelOffsets[modelId];
        //  //voxel.modelSize = modelSizes[modelId];
        //  vd.push_back(voxel);
        //}



        auto cube1 = VvGameObject::createGameObject();

        cube1.model = vvModel;
        //cube1.transform.translation = vd[i].pos;
        //cube1.transform.scale = vd[i].size;
        //cube1.dataOffset = vd[i].modelOffset;
        gameObjects.push_back(std::move(cube1));

        simpleRenderSystem.instanceCount = vd.size();
        simpleRenderSystem.createBuffers(vd, models);

        //auto cube1 = VvGameObject::createGameObject();

        //cube1.model = vvModel;
        //cube1.transform.translation = { 0.f, 0.f, 0.f };
        //cube1.transform.scale = { 10.f, 10.f, 10.f };
        //gameObjects.push_back(std::move(cube1));

        //auto cube2 = VvGameObject::createGameObject();

        //cube2.model = vvModel;
        //cube2.transform.translation = { 10.f, 0.f, 0.f };
        //cube2.transform.scale = { 100.f, 100.f, 100.f };
        //gameObjects.push_back(std::move(cube2));
	}
}