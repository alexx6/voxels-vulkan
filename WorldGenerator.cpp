#include "WorldGenerator.h"
#include "FastNoiseLite.h"
#include <iostream>
#include <future>

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

		const int numThreads = std::thread::hardware_concurrency();
		const int groupSize = 7077888 / numThreads;

		std::vector<std::future<std::vector<VoxelData>>> futures;

		for (int t = 0; t < numThreads; ++t) 
		{
			int start = t * groupSize;
			int end = (t == numThreads - 1) ? 7077888 : start + groupSize;

			futures.push_back(std::async(std::launch::async, [this, &noise, start, end]() {
				std::vector<VoxelData> localVd;

				for (int i = start; i < end; ++i)
				{

					glm::ivec3 pos(i % 192 * 300, i / 36864 * 300, i / 192 % 192 * 300);
					uint32_t modelId = int(noise.GetNoise((float)pos.x, (float)pos.y, (float)pos.z) * 100 + 100) % (modelSizes.size());

					//pos += glm::ivec3(rand() / (float)RAND_MAX * 200, rand() / (float)RAND_MAX * 200, rand() / (float)RAND_MAX * 200);

					//std::cout << noise.GetNoise((float)pos.x, (float)pos.y, (float)pos.z) << std::endl;
					if (noise.GetNoise((float)pos.x, (float)pos.y, (float)pos.z) > 0.3) {
						localVd.push_back(getVoxelInstance(pos, i % 4, modelId));
					}
				}

				return localVd;
			}));

		}

		for (auto& future : futures) {
			auto result = future.get();
			vd.insert(vd.end(), result.begin(), result.end());
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