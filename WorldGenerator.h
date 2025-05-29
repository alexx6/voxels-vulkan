#pragma once
#include <vector>
#include "vv_game_object.h"


namespace vv {
	class WorldGenerator
	{
	public:
		//WorldGenerator(std::vector<uint32_t> ms, std::vector<uint32_t> mo);

		VoxelChunk generateChunk(glm::ivec3 chunkPos);

		std::vector<uint32_t> modelSizes;
		std::vector<uint32_t> modelOffsets;
		std::vector<uint32_t> modelDataSizes;

		VoxelData getVoxelInstance(glm::ivec3 pos, uint32_t orientation, uint32_t modelId, bool disableLOD = false);
		void addModelInfo(uint32_t modelSize, uint32_t dataSize);
	};

}

