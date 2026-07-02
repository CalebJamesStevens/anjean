#pragma once

#include "../Core/Core.h"

#include <cstdint>
#include <filesystem>
#include <limits>
#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace Anjean::Runtime
{
class MeshLoader
{
  public:
	Core::ModelAssetId loadModelAsset(const std::filesystem::path &path);

	const Core::ImportedModel *getModel(Core::ModelAssetId id) const;
	const Core::ImportedModel *getModel(const std::filesystem::path &path) const;

	bool hasModel(Core::ModelAssetId id) const;

  private:
	Core::ImportedModel importModelFile(const std::filesystem::path &path) const;

	Core::ModelAssetId nextModelAssetId = 1;

	std::unordered_map<std::filesystem::path, Core::ModelAssetId> pathToModelId;
	std::unordered_map<Core::ModelAssetId, Core::ImportedModel>   models;
};
}        // namespace Anjean::Runtime