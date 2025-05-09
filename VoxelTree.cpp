#include "VoxelTree.h"
//#include <thread>

namespace vv {
	std::vector<uint32_t> VoxelTree::genRandomGrid()
	{
		std::vector<uint32_t> data;
		for (int i = 0; i < 256; ++i)
		{
			for (int j = 0; j < 256; ++j)
			{
				for (int k = 0; k < 256; ++k)
				{
					float a = float(rand()) / RAND_MAX;
					if (i + j + k < 256) {
						if (i + j + k == 255)
						{
							data.push_back((i << 0) + (j << 8) + (k << 16));
							continue;
						}

						data.push_back(0xffffffff);
						continue;
					}

					data.push_back(0);
					//data.push_back(abs(rand()));
					//data.push_back((i >= 128 ? 255 : 0) + ((j >= 128 ? 255 : 0) << 8) + ((j >= 128 ? 255 : 0) << 16));
				}
			}
		}


		std::vector<uint32_t> compressedData;

		serialize(compressGrid(data.data(), glm::ivec3(0), 256, 256), compressedData);

  	return compressedData;
	}
	
	//stupid implementaions for now

	uint32_t VoxelTree::subToIndex(std::vector<uint8_t> sub)
	{
		glm::ivec3 pos(0);
		uint32_t size = pow(2, sub.size());

		for (int i = 0; i < sub.size(); ++i)
		{
			uint32_t curSize = (size >> (i + 1));
			pos.x += curSize * (sub[i] % 2);
			pos.y += curSize * (sub[i] / 2 % 2);
			pos.z += curSize * (sub[i] / 4);
		}

		return pos.x + pos.y * 256 + pos.z * 256 * 256;
	}

	glm::ivec3 VoxelTree::getOctantOffset(uint32_t octant, uint32_t dim)
	{
		dim /= 2;
		return glm::ivec3(dim * (octant % 2), dim * (octant / 2 % 2), dim * (octant / 4));
	}

	////compress to octree without recursion
	//std::vector<uint32_t> VoxelTree::compressGrid(std::vector<uint32_t> data)
	//{
	//	//std::vector<uint32_t> data;
	//	std::vector<uint32_t> compressedData;
	//	std::vector<uint8_t> sub = { 0, 0 };

	//	uint32_t currSize;

	//	bool isDone = false;

	//	uint8_t ilevel = sub.size() - 1;
	//	uint8_t olevel = sub.size() - 1;

	//	uint32_t lastVoxel = 0;
	//	while (!isDone)
	//	{
	//		uint32_t voxel = data[subToIndex(sub)];

	//		if (voxel == lastVoxel)
	//		{
	//			sub[ilevel] += 1;
	//			
	//			for (int i = sub.size() - 1)
	//		}
	//		else
	//		{

	//		}
	//	}

	//	return data;
	//}
	bool VoxelTree::areChildrenSame(VoxelNode* node)
	{
		if (!node->children[0]->isLeaf)
			return false;

		for (int i = 1; i < 8; ++i)
		{
			if ((node->children[i]->color != node->children[i - 1]->color) || (node->children[i]->isLeaf != node->children[i - 1]->isLeaf))
			{
				return false;
			}
		}

		return true;
	}

	//Basic octree compression
	VoxelNode* VoxelTree::compressGrid(const uint32_t* data, glm::ivec3 pos, uint32_t treeDim, uint32_t curDim)
	{
		VoxelNode* node = new VoxelNode();
		if (curDim == 1) {
			node->color = data[pos.x + pos.y * treeDim + pos.z * treeDim * treeDim];
			return node;
		}
		
		for (int i = 0; i < 8; ++i)
		{
			node->children[i] = compressGrid(data, pos + getOctantOffset(i, curDim), treeDim, curDim / 2);
		}

		if (areChildrenSame(node))
		{
			node->color = node->children[0]->color;

			for (int i = 0; i < 8; ++i)
			{
				delete node->children[i];
			}

			return node;
		}

		node->isLeaf = 0;

		return node;
		//uint64_t totalColor;
		//uint32_t totalColoredVoxels;

		//for (int i = 0; i < 8; ++i)
		//{
		//	uint32_t color = node->children[i]->color;
		//	if (color > 0)
		//	{
		//		totalColor += color;
		//	}
		//}

	}

	void VoxelTree::serialize(VoxelNode* node, std::vector<uint32_t> &serializedData)
	{
		uint32_t offset = serializedData.size();

		//serializedData.push_back(offset);
		serializedData.push_back(node->isLeaf);

		if (node->isLeaf)
		{
			if (node->color == 0)
			{
				int a = 0;
				a++;
			}
			serializedData.push_back(node->color);
			return;
		}

		uint32_t mask = 0;
		uint32_t visibleNodes = 0;

		for (int i = 0; i < 8; ++i)
		{
			if (node->children[i]->isLeaf && (node->children[i]->color == 0))
			{
				mask |= (8 << (i * 4));
				continue;
			}

			mask |= (visibleNodes << (i * 4));
			++visibleNodes;
		}

		serializedData.push_back(mask);

		serializedData.resize(offset + 2 + visibleNodes);

		uint32_t curChild = 0;
		for (int i = 0; i < 8; ++i)
		{
			if (node->children[i]->isLeaf && (node->children[i]->color == 0))
			{
				continue;
			}

			serializedData[offset + 2 + curChild] = serializedData.size();
			serialize(node->children[i], serializedData);
			++curChild;
		}
	}
}