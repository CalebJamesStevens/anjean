
#include "Resource.h"
#include "ResourceHandle.h"
#include <string>
#include <vector>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <typeindex>
#include <unordered_map>

namespace Anjean::Runtime
{
// Resource manager
class ResourceManager
{
  private:
	// Two-level storage system: organize by type first, then by unique identifier
	// This approach enables type-safe resource access while maintaining efficient lookup
	std::unordered_map<std::type_index,
	                   std::unordered_map<std::string, std::shared_ptr<Resource>>>
	    resources;

	// Two-level reference counting system for automatic resource lifecycle management
	// First level maps resource type, second level maps resource IDs to their data
	struct ResourceData
	{
		std::shared_ptr<Resource> resource;        // The actual resource
		int                       refCount;        // Reference count for this resource
	};
	std::unordered_map<std::type_index,
	                   std::unordered_map<std::string, ResourceData>>
	    refCounts;

  public:
	template <typename T>
	ResourceHandle<T> Load(const std::string &resourceId)
	{
		static_assert(std::is_base_of<Resource, T>::value, "T must derive from Resource");

		// Step 3a: Check existing resource cache to avoid redundant loading
		auto &typeResources = resources[std::type_index(typeid(T))];
		auto  it            = typeResources.find(resourceId);

		if (it != typeResources.end())
		{
			// Resource exists in cache - increment reference count and return handle
			refCounts[resourceId]++;
			return ResourceHandle<T>(resourceId, this);
		}

		// Step 3b: Create new resource instance and attempt loading
		auto resource = std::make_shared<T>(resourceId);
		if (!resource->Load())
		{
			// Loading failed - return invalid handle rather than corrupting cache
			return ResourceHandle<T>();
		}

		// Step 3c: Cache successful resource and initialize reference tracking
		typeResources[resourceId] = resource;
		refCounts[resourceId]     = 1;

		return ResourceHandle<T>(resourceId, this);
	}
	template <typename T>
	T *GetResource(const std::string &resourceId)
	{
		// Access type-specific resource container using compile-time type information
		auto &typeResources = resources[std::type_index(typeid(T))];
		auto  it            = typeResources.find(resourceId);

		if (it != typeResources.end())
		{
			// Resource found - perform safe downcast and return typed pointer
			return static_cast<T *>(it->second.get());
		}

		// Resource not found - return null for safe handling by caller
		return nullptr;
	}

	template <typename T>
	bool HasResource(const std::string &resourceId)
	{
		// Efficient existence check without resource access overhead
		auto resourceIt = resources.find(std::type_index(typeid(T)));
		return resourceIt != resources.end();
	}

	template <typename T>
	void Release(const std::string &resourceId)
	{
		// Locate reference count entry for this resource
		auto typeIndex = std::type_index(typeid(T));

		auto it = refCounts.find(typeIndex);
		if (it != refCounts.end())
		{
			it->second--;

			// Check if resource has no remaining references
			if (it->second <= 0)
			{
				// Step 5a: Locate and unload the unreferenced resource across all type containers
				for (auto &[type, typeResources] : resources)
				{
					auto resourceIt = typeResources.find(resourceId);
					if (resourceIt != typeResources.end())
					{
						resourceIt->second->Unload();           // Allow resource to clean up its data
						typeResources.erase(resourceIt);        // Remove from cache
						break;
					}
				}

				// Step 5b: Clean up reference counting entry
				refCounts.erase(it);
			}
		}
	}

	void UnloadAll()
	{
		// Emergency cleanup method for system shutdown or major state changes
		for (auto &[type, typeResources] : resources)
		{
			for (auto &[id, resource] : typeResources)
			{
				resource->Unload();        // Ensure all resources clean up properly
			}
			typeResources.clear();        // Clear type-specific containers
		}
		refCounts.clear();        // Reset all reference counts
	}
};

}        // namespace Anjean::Runtime
