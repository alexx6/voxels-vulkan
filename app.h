#pragma once

#include "vv_window.h"
#include "vv_device.hpp"
#include "vv_renderer.h"
#include <memory>
#include <vector>
#include "vv_model.h"
#include "vv_game_object.h"

namespace vv {
	class App {
	public:
		static constexpr int WIDTH = 1920;
		static constexpr int HEIGHT = 1080;

		App();
		~App();

		App(const App&) = delete;
		App& operator=(const App&) = delete;

		void run();
	private:
		void loadGameObjects();

		VvWindow vvWindow{ WIDTH, HEIGHT, "Voxel Engine" };
		VvDevice vvDevice{ vvWindow };
		VvRenderer vvRenderer{ vvWindow, vvDevice };

		std::vector<VvGameObject> gameObjects;
	};
};