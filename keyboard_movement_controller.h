#pragma once

#include "vv_game_object.h"
#include "vv_window.h"

namespace vv {
	class KeyboardMovementController {
	public:
		struct KeyMappings {
			int moveLeft = GLFW_KEY_A;
			int moveRight = GLFW_KEY_D;
			int moveForward = GLFW_KEY_W;
			int moveBackward = GLFW_KEY_S;
			int moveUp = GLFW_KEY_E;
			int moveDown = GLFW_KEY_Q;
			int lookLeft = GLFW_KEY_LEFT;
			int lookRight = GLFW_KEY_RIGHT;
			int lookUp = GLFW_KEY_UP;
			int lookDown = GLFW_KEY_DOWN;
			int speedUp = GLFW_KEY_PAGE_UP;
			int speedDown = GLFW_KEY_PAGE_DOWN;
		};

		void moveInPlaneXZ(GLFWwindow* window, float dt, VvGameObject& gameObject);

		KeyMappings keys{};

		float moveSpeed{ 100.f };
		float lookSpeed{ 5.f };
	};
}