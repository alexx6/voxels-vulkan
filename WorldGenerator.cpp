#include "WorldGenerator.h"
#include "FastNoiseLite.h"
#include <iostream>
#include <future>

namespace vv {
	//WorldGenerator::WorldGenerator(std::vector<uint32_t> ms, std::vector<uint32_t> mo)
	//{
	//	modelSizes = ms;
	//	modelOffsets = mo;
	//}

	//chunks are 10K x 10K x 10K
	VoxelChunk WorldGenerator::generateChunk(glm::ivec3 chunkPos)
	{
		VoxelChunk chunk;
		chunk.pos = chunkPos;

		FastNoiseLite noise;
		noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
		noise.SetSeed(777);
		noise.SetFrequency(0.00025f);

		const int numThreads = std::thread::hardware_concurrency();
		const int groupSize = 262144 / numThreads;

		std::vector<std::future<std::vector<VoxelData>>> futures;

		for (int t = 0; t < numThreads; ++t) 
		{
			int start = t * groupSize;
			int end = (t == numThreads - 1) ? 262144 : start + groupSize;

			futures.push_back(std::async(std::launch::async, [this, &noise, start, end, &chunkPos]() {
				std::vector<VoxelData> localVd;

				for (int i = start; i < end; ++i)
				{

					glm::ivec3 pos(i % 64 * 256, i / 4096 * 256, i / 64 % 64 * 256);
					pos += chunkPos * 64 * 256;

					float r = (float)rand() / RAND_MAX;
					uint32_t modelId = 0;
					if (r > 0.9)
						modelId = 1;

					//uint32_t modelId = int(noise.GetNoise((float)pos.x, (float)pos.y, (float)pos.z) * 100 + 123) % (2);

					//pos += glm::ivec3(rand() / (float)RAND_MAX * 200, rand() / (float)RAND_MAX * 200, rand() / (float)RAND_MAX * 200);

					//std::cout << noise.GetNoise((float)pos.x, (float)pos.y, (float)pos.z) << std::endl;
					if (noise.GetNoise((float)pos.x, (float)pos.y, (float)pos.z) > 0.3) {
						localVd.push_back(getVoxelInstance(pos, i % 24, modelId));
					}
				}

				return localVd;
			}));

		}

		for (auto& future : futures) {
			auto result = future.get();
			chunk.chunkData.insert(chunk.chunkData.end(), result.begin(), result.end());
		}

		//fallback
		if (chunk.chunkData.empty())
			chunk.chunkData.push_back(getVoxelInstance(glm::ivec3(0), 0, 0));

		return chunk;
	}

	VoxelData WorldGenerator::getVoxelInstance(glm::ivec3 pos, uint32_t orientation, uint32_t modelId, bool disableLOD)
	{
		VoxelData voxel;
		voxel.pos = pos;
		voxel.orientation = orientation;
		voxel.size = modelSizes[modelId];
		voxel.modelId = modelId;
		voxel.modelOffset = modelOffsets[modelId];
		voxel.disableLOD = disableLOD;

		return voxel;
	}

	void WorldGenerator::addModelInfo(uint32_t modelSize, uint32_t dataSize)
	{
		modelOffsets.push_back(modelOffsets.empty() ? 0 : modelOffsets.back() + modelDataSizes.back());
		modelDataSizes.push_back(dataSize);
		modelSizes.push_back(modelSize);
	}
}