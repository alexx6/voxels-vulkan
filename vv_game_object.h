#pragma once

#include "vv_model.h"

#include <memory>

#include <glm/gtc/matrix_transform.hpp>

#include <vector>

namespace vv {
	struct TransformComponent {
		glm::vec3 translation{};
		glm::vec3 scale{ 1.f, 1.f, 1.f };
		glm::vec3 rotation{};

		glm::mat4 mat4() {

			auto transform = glm::translate(glm::mat4{ 1.f }, glm::floor(translation));
			transform = glm::rotate(transform, rotation.y, {0.f, 1.f, 0.f});
			transform = glm::rotate(transform, rotation.x, {1.f, 0.f, 0.f});
			transform = glm::rotate(transform, rotation.z, {0.f, 0.f, 1.f});
			transform = glm::scale(transform, glm::floor(scale));
			
			return transform;
		}
 	};

	struct VoxelData {
		alignas(16) glm::ivec3 pos{};
		alignas(4) uint32_t size = 0;
		alignas(4) uint32_t orientation = 0;
		alignas(4) uint32_t modelId = 0;
		alignas(4) uint32_t modelOffset = 0;
		alignas(4) uint32_t disableLOD = 0;
	};

	struct VoxelModel {
		uint32_t size;
		std::vector<uint32_t> modelData;
	};

	struct VoxelChunk {
		glm::ivec3 pos{};
		std::vector<VoxelData> chunkData;
	};

	class VvGameObject {
	public:
		using id_t = unsigned int;

		static VvGameObject createGameObject() {
			static id_t currentId = 0;
			return VvGameObject{ currentId++ };
		}

		VvGameObject(const VvGameObject&) = delete;
		VvGameObject &operator=(const VvGameObject&) = delete;		
		VvGameObject(VvGameObject &&) = default;
		VvGameObject &operator=(VvGameObject&&) = default;

		id_t getId() { return id; };

		std::shared_ptr<VvModel> model{};
		glm::vec3 color{};
		TransformComponent transform{};
		uint32_t dataOffset;
	private:
		VvGameObject(id_t objId) : id{ objId } {};

		id_t id;
		
	};
}