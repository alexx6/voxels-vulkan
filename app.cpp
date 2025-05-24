#include "app.h"
#include <iostream>
#include <stdexcept>
#include <array>
#include "vv_camera.h"
#include "keyboard_movement_controller.h"
#include <fstream>
#include "VoxelTree.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"

#include <chrono>

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
    viewerObject.transform.translation = glm::vec3(0.f, 10.f, -5.f);
    KeyboardMovementController cameraController{};

    auto currentTime = std::chrono::high_resolution_clock::now();

		while (!vvWindow.shouldClose()) {
			glfwPollEvents();

      auto newTime = std::chrono::high_resolution_clock::now();
      float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
      currentTime = newTime;

      cameraController.moveInPlaneXZ(vvWindow.getGLFWwindow(), frameTime, viewerObject);
      camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

      float aspect = vvRenderer.getAspectRatio();
      //camera.setOrthographicProjection(-aspect, aspect, -1, 1, -1, 1);
      camera.setPerspectiveProjection(glm::radians(60.f), aspect, 0.0f, 2000000.f);

			if (auto commandBuffer = vvRenderer.beginFrame()) {
				vvRenderer.beginSwapChainRenderPass(commandBuffer);
				simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects, camera);
				vvRenderer.endSwapChainRenderPass(commandBuffer);
				vvRenderer.endFrame();
			}
		}
		vkDeviceWaitIdle(vvDevice.device());
	}

    std::unique_ptr<VvModel> createCubeModel(VvDevice& device, glm::vec3 offset) {
        VvModel::Builder modelBuilder{};
        modelBuilder.vertices = {
            // left face (white)
            {{0.f, 0.f, 0.f}, {.9f, .9f, .9f}},
            {{0.f, 1.f, 1.f}, {.9f, .9f, .9f}},
            {{0.f, 0.f, 1.f}, {.9f, .9f, .9f}},
            {{0.f, 1.f, 0.f}, {.9f, .9f, .9f}},

            // right face (yellow)
            {{1.f, 0.f, 0.f}, {.8f, .8f, .1f}},
            {{1.f, 1.f, 1.f}, {.8f, .8f, .1f}},
            {{1.f, 0.f, 1.f}, {.8f, .8f, .1f}},
            {{1.f, 1.f, 0.f}, {.8f, .8f, .1f}},

            // top face (orange, remember y axis points down)
            {{0.f, 0.f, 0.f}, {.9f, .6f, .1f}},
            {{1.f, 0.f, 1.f}, {.9f, .6f, .1f}},
            {{0.f, 0.f, 1.f}, {.9f, .6f, .1f}},
            {{1.f, 0.f, 0.f}, {.9f, .6f, .1f}},

            // bottom face (red)
            {{0.f, 1.f, 0.f}, {.8f, .1f, .1f}},
            {{1.f, 1.f, 1.f}, {.8f, .1f, .1f}},
            {{0.f, 1.f, 1.f}, {.8f, .1f, .1f}},
            {{1.f, 1.f, 0.f}, {.8f, .1f, .1f}},

            // nose face (blue)
            {{0.f, 0.f, 1.f}, {.1f, .1f, .8f}},
            {{1.f, 1.f, 1.f}, {.1f, .1f, .8f}},
            {{0.f, 1.f, 1.f}, {.1f, .1f, .8f}},
            {{1.f, 0.f, 1.f}, {.1f, .1f, .8f}},

            // tail face (green)
            {{0.f, 0.f, 0.f}, {.1f, .8f, .1f}},
            {{1.f, 1.f, 0.f}, {.1f, .8f, .1f}},
            {{0.f, 1.f, 0.f}, {.1f, .8f, .1f}},
            {{1.f, 0.f, 0.f}, {.1f, .8f, .1f}},
        };
        for (auto& v : modelBuilder.vertices) {
            v.position += offset;
        }

        modelBuilder.indices = { 0,  1,  2,  0,  3,  1,  4,  5,  6,  4,  7,  5,  8,  9,  10, 8,  11, 9,
                                12, 13, 14, 12, 15, 13, 16, 17, 18, 16, 19, 17, 20, 21, 22, 20, 23, 21 };

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

        std::vector<std::string> modelNames = { "crystal", "rock_big" };

        std::vector<uint32_t> modelSizes;

        std::vector<std::vector<uint32_t>> models;

        std::vector<uint32_t> modelOffsets;
        uint32_t curOffset = 0;

        for (int i = 0; i < modelNames.size(); ++i)
        {
          VoxelModel model = VoxelTree::getCompressedData(loadVoxelModel(modelNames[i]));
          models.push_back(model.modelData);
          modelSizes.push_back(model.size);

          modelOffsets.push_back(curOffset);
          curOffset += models[i].size();
        }

        std::vector<VoxelData> vd;
        for (int i = 0; i < 1000; ++i) 
        {
          uint32_t modelId = i % 2;

          VoxelData voxel;
          voxel.pos = glm::ivec3(((i / 10) % 10) * 690, i / 100 * 690, (i % 10) * 690);
          voxel.size = glm::ivec3(modelSizes[modelId]);
          voxel.modelOffset = modelOffsets[modelId];
          //voxel.modelSize = modelSizes[modelId];
          vd.push_back(voxel);
        }


        uint32_t dataOffset = 0;
        for (int i = 0; i < vd.size(); ++i)
        {
          auto cube1 = VvGameObject::createGameObject();

          cube1.model = vvModel;
          cube1.transform.translation = vd[i].pos;
          //cube1.transform.scale = vd[i].size;
          cube1.dataOffset = vd[i].modelOffset;
          gameObjects.push_back(std::move(cube1));
        }

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