
#include <string>
#include <unordered_map>
#include <vector>

namespace Anjean::Runtime
{
// Component type ID system
class ComponentTypeIDSystem
{
  private:
	static size_t nextTypeID;

  public:
	template <typename T>
	static size_t GetTypeID()
	{
		static size_t typeID = nextTypeID++;
		return typeID;
	}
};

size_t ComponentTypeIDSystem::nextTypeID = 0;

// Component base class with type ID
class Component
{
  public:
	virtual ~Component() = default;

	template <typename T>
	static size_t GetTypeID()
	{
		return ComponentTypeIDSystem::GetTypeID<T>();
	}
};

// Entity with optimized component access
class Entity
{
  private:
	std::vector<std::unique_ptr<Component>> components;
	std::unordered_map<size_t, Component *> componentMap;

  public:
	template <typename T, typename... Args>
	T *AddComponent(Args &&...args)
	{
		static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");

		size_t typeID = Component::GetTypeID<T>();

		// Check if component of this type already exists
		auto it = componentMap.find(typeID);
		if (it != componentMap.end())
		{
			return static_cast<T *>(it->second);
		}

		// Create new component
		auto component       = std::make_unique<T>(std::forward<Args>(args)...);
		T   *componentPtr    = component.get();
		componentMap[typeID] = componentPtr;
		components.push_back(std::move(component));
		return componentPtr;
	}

	template <typename T>
	T *GetComponent()
	{
		size_t typeID = Component::GetTypeID<T>();
		auto   it     = componentMap.find(typeID);
		if (it != componentMap.end())
		{
			return static_cast<T *>(it->second);
		}
		return nullptr;
	}

	template <typename T>
	bool RemoveComponent()
	{
		size_t typeID = Component::GetTypeID<T>();
		auto   it     = componentMap.find(typeID);
		if (it != componentMap.end())
		{
			Component *componentPtr = it->second;
			componentMap.erase(it);

			for (auto compIt = components.begin(); compIt != components.end(); ++compIt)
			{
				if (compIt->get() == componentPtr)
				{
					components.erase(compIt);
					return true;
				}
			}
		}
		return false;
	}
};

}        // namespace Anjean::Runtime
