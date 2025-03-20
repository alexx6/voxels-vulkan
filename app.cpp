#include "app.h"
#include <iostream>
#include <stdexcept>
#include <array>
#include "simple_render_system.h"
#include "vv_camera.h"
#include "keyboard_movement_controller.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"

#include <chrono>

namespace vv {
	App::App() {
		loadGameObjects();
	}

	App::~App() {
	}

	void App::run() {
		SimpleRenderSystem simpleRenderSystem{ vvDevice, vvRenderer.getSwapChainRenderPass() };
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
      camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 1000.f);

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

	void App::loadGameObjects() {
        std::shared_ptr<VvModel> vvModel = createCubeModel(vvDevice, { .0f, .0f, .0f });

        //for (int i = 0; i < 1000; ++i)
        //{
        //  auto cube1 = VvGameObject::createGameObject();

        //  cube1.model = vvModel;
        //  cube1.transform.translation = { 100.f * (i % 10 - 5), 100.f * ((i % 100) / 10 - 5), 100.f * (i / 100 - 5) };
        //  cube1.transform.scale = { 100.f, 100.f, 100.f };
        //  gameObjects.push_back(std::move(cube1));
        //}

          auto cube1 = VvGameObject::createGameObject();

          cube1.model = vvModel;
          cube1.transform.translation = { 0.f, 0.f, 0.f };
          cube1.transform.scale = { 100.f, 100.f, 100.f };
          gameObjects.push_back(std::move(cube1));
	}
}