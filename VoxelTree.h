#pragma once
#include <vector>
#include <random>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"

namespace vv {
	struct VoxelNode {
		//VoxelNode() {};
		//VoxelNode(uint32_t c, bool l) : color(c), isLeaf(l) {};
		uint32_t color = 0;
		uint32_t isLeaf = 1;
		VoxelNode* children[8];
	};

	class VoxelTree
	{
	public:
		static std::vector<uint32_t> genRandomGrid();
		static uint32_t subToIndex(std::vector<uint8_t> sub);
		static glm::ivec3 getOctantOffset(uint32_t octant, uint32_t dim);
		static bool areChildrenSame(VoxelNode* node);
		//static std::vector<uint32_t> compressGrid(std::vector<uint32_t>);
		static VoxelNode* compressGrid(const uint32_t* data, glm::ivec3 pos, uint32_t treeDim, uint32_t curDim);
		static void serialize(VoxelNode* node, std::vector<uint32_t>& serializedData);
	};
}


