#pragma once
#include <vector>
#include "vv_game_object.h"


namespace vv {
	class WorldGenerator
	{
	public:
		WorldGenerator(std::vector<uint32_t> ms, std::vector<uint32_t> mo);

		void generateChunk(std::vector<VoxelData>& vd, uint32_t x, uint32_t y);

		std::vector<uint32_t> modelSizes;
		std::vector<uint32_t> modelOffsets;

		VoxelData getVoxelInstance(glm::ivec3 pos, uint32_t orientation, uint32_t modelId);
	};

}

