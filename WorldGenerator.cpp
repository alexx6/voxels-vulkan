#include "WorldGenerator.h"
#include "FastNoiseLite.h"
#include <iostream>
namespace vv {
	WorldGenerator::WorldGenerator(std::vector<uint32_t> ms, std::vector<uint32_t> mo)
	{
		modelSizes = ms;
		modelOffsets = mo;
	}

	//chunks are 10K x 10K x 10K
	void WorldGenerator::generateChunk(std::vector<VoxelData>& vd, uint32_t x, uint32_t y)
	{
		FastNoiseLite noise;
		noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
		noise.SetSeed(777);
		noise.SetFrequency(0.0005f);

		for (int i = 0; i < 1000000; ++i)
		{
			uint32_t modelId = i % (modelSizes.size() - 1);

			glm::ivec3 pos(i % 100 * 300, i / 10000 * 300, i / 100 % 100 * 300);
			//pos += glm::ivec3(rand() / (float)RAND_MAX * 200, rand() / (float)RAND_MAX * 200, rand() / (float)RAND_MAX * 200);

			//std::cout << noise.GetNoise((float)pos.x, (float)pos.y, (float)pos.z) << std::endl;
			if (noise.GetNoise((float)pos.x, (float)pos.y, (float)pos.z) > 0.3) {
				vd.push_back(getVoxelInstance(pos, i % 4, modelId));
			}
		}

		//fallback
		vd.push_back(getVoxelInstance(glm::ivec3(0), 0, 0));
	}

	VoxelData WorldGenerator::getVoxelInstance(glm::ivec3 pos, uint32_t orientation, uint32_t modelId)
	{
		VoxelData voxel;
		voxel.pos = pos;
		voxel.orientation = orientation;
		voxel.size = modelSizes[modelId];
		voxel.modelOffset = modelOffsets[modelId];

		return voxel;
	}
}