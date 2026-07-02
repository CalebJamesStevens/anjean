#pragma once
#include <cstdint>
#include <iostream>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <algorithm>
#include <ranges>
#include <string>
#include <vector>

namespace Anjean::Core
{
struct Vector3
{
	float x = 0;
	float y = 0;
	float z = 0;

	Vector3 &operator+=(const Vector3 &other)
	{
		x += other.x;
		y += other.y;
		z += other.z;
		return *this;
	}

	Vector3 &operator-=(const Vector3 &other)
	{
		x -= other.x;
		y -= other.y;
		z -= other.z;
		return *this;
	}

	Vector3 &operator*=(float scalar)
	{
		x *= scalar;
		y *= scalar;
		z *= scalar;
		return *this;
	}

	Vector3 &operator/=(float scalar)
	{
		x /= scalar;
		y /= scalar;
		z /= scalar;
		return *this;
	}
};

inline Vector3 operator+(Vector3 lhs, const Vector3 &rhs)
{
	lhs += rhs;
	return lhs;
}

inline Vector3 operator-(Vector3 lhs, const Vector3 &rhs)
{
	lhs -= rhs;
	return lhs;
}

inline Vector3 operator*(Vector3 v, float scalar)
{
	v *= scalar;
	return v;
}

inline Vector3 operator*(float scalar, Vector3 v)
{
	v *= scalar;
	return v;
}

inline Vector3 operator/(Vector3 v, float scalar)
{
	v /= scalar;
	return v;
}
struct Position3D
{
	float x;
	float y;
	float z;
};

struct Position2D
{
	float x;
	float y;
};

struct MeshVertex
{
	Position3D position;
	Position2D uv;
};

struct MeshData
{
	std::uint32_t              id = 0;
	std::vector<MeshVertex>    vertices;
	std::vector<std::uint32_t> indices;
};

struct Quaternion
{
	float w, x, y, z;

	// Construct from axis-angle: q = [cos(θ/2), sin(θ/2) * axis]
	static Quaternion fromAxisAngle(float angleRadians, const Vector3 &axis)
	{
		float halfAngle = angleRadians * 0.5f;
		float sinHalf   = std::sin(halfAngle);
		return {std::cos(halfAngle), axis.x * sinHalf, axis.y * sinHalf, axis.z * sinHalf};
	}

	// Hamilton Product (Quaternion * Quaternion)
	Quaternion multiply(const Quaternion &q) const
	{
		return {w * q.w - x * q.x - y * q.y - z * q.z, w * q.x + x * q.w + y * q.z - z * q.y,
		        w * q.y - x * q.z + y * q.w + z * q.x, w * q.z + x * q.y - y * q.x + z * q.w};
	}
};

enum class ColliderType : std::uint32_t
{
	Capsule          = 0,
	RectangularPrism = 1,
	Sphere           = 2,
	Plane            = 3
};

enum class PhysicsBodyType : std::uint32_t
{
	Static    = 0,
	Kinematic = 1,
	Dynamic   = 2
};

using ObjectId = std::uint32_t;
using MeshId   = std::uint32_t;

struct ColliderState
{
	ObjectId id = 0;
	Vector3  localCenter{};

	// Sphere
	float radius = 1.0f;

	// Rectangular prism
	Vector3      halfExtents{0.5f, 0.5f, 0.5f};
	ColliderType colliderType;
};

struct PhysicsBodyState
{
	ObjectId id = 0;

	Vector3                    position;
	Vector3                    velocity;
	Vector3                    force;
	Vector3                    kinematicMove;
	bool                       hasKinematicMove;
	PhysicsBodyType            physicsBodyType;
	std::vector<ColliderState> colliders;

	float mass = 1.0f;
};

struct PhysicsBodyResult
{
	ObjectId id = 0;

	Vector3 position;
	Vector3 velocity;
};

struct CameraPacket
{
	ObjectId objectId = 0;

	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 rotation = glm::vec3(0.0f);

	float fieldOfView = 45.0f;
	float aspectRatio = 16.0f / 9.0f;
	float orthoWidth  = 10.0f;
	float orthoHeight = 10.0f;
	float nearPlane   = 0.1f;
	float farPlane    = 100.0f;

	glm::vec3 target = {0.0f, 0.0f, 0.0f};
	glm::vec3 up     = {0.0f, 1.0f, 0.0f};
};

struct RenderPacket
{
	ObjectId objectId = 0;
	MeshId   meshId   = 0;

	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 rotation = glm::vec3(0.0f);
	glm::vec3 scale    = glm::vec3(1.0f);
};

struct TransformComponent
{
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 rotation = glm::vec3(0.0f);
	glm::vec3 scale    = glm::vec3(1.0f);
};

struct RenderComponent
{
	MeshId meshId = 0;
};

using MaterialId = std::uint32_t;

struct Primitive
{
	MeshId     meshId     = 0;
	MaterialId materialId = 0;
};

struct Mesh
{
	std::vector<Primitive> primitives;
};

// Structure for a node in the scene graph
struct Node
{
	uint32_t    index = 0;
	std::string name;

	Node               *parent = nullptr;
	std::vector<Node *> children;
	Mesh                mesh;
	glm::mat4           matrix = glm::mat4(1.0f);

	// For animation
	glm::vec3 translation = glm::vec3(0.0f);
	glm::quat rotation    = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	glm::vec3 scale       = glm::vec3(1.0f);

	glm::mat4 getLocalMatrix()
	{
		return glm::translate(glm::mat4(1.0f), translation) *
		       glm::mat4_cast(rotation) *
		       glm::scale(glm::mat4(1.0f), scale) *
		       matrix;
	}

	glm::mat4 getGlobalMatrix()
	{
		glm::mat4 m = getLocalMatrix();
		Node     *p = parent;
		while (p)
		{
			m = p->getLocalMatrix() * m;
			p = p->parent;
		}
		return m;
	}
};

// Structure for animation keyframes
struct AnimationChannel
{
	enum PathType
	{
		TRANSLATION,
		ROTATION,
		SCALE
	};
	PathType path;
	Node    *node = nullptr;
	uint32_t samplerIndex;
};

// Structure for animation interpolation
struct AnimationSampler
{
	enum InterpolationType
	{
		LINEAR,
		STEP,
		CUBICSPLINE
	};
	InterpolationType      interpolation;
	std::vector<float>     inputs;             // Key frame timestamps
	std::vector<glm::vec4> outputsVec4;        // Key frame values (for rotations)
	std::vector<glm::vec3> outputsVec3;        // Key frame values (for translations and scales)
};

// Structure for animation
struct Animation
{
	std::string                   name;
	std::vector<AnimationSampler> samplers;
	std::vector<AnimationChannel> channels;
	float                         start       = std::numeric_limits<float>::max();
	float                         end         = std::numeric_limits<float>::min();
	float                         currentTime = 0.0f;
};

// Structure for a model with nodes, meshes, materials, textures, and animations
struct Model
{
	std::vector<Node *>     nodes;
	std::vector<Node *>     linearNodes;
	std::vector<MaterialId> materials;
	std::vector<Animation>  animations;

	~Model()
	{
		for (auto node : linearNodes)
		{
			delete node;
		}
	}

	Node *findNode(const std::string &name)
	{
		auto nodeIt = std::ranges::find_if(linearNodes, [&name](auto const &node) {
			return node->name == name;
		});
		return (nodeIt != linearNodes.end()) ? *nodeIt : nullptr;
	}

	void updateAnimation(uint32_t index, float deltaTime)
	{
		assert(!animations.empty() && index < animations.size());

		Animation &animation = animations[index];
		animation.currentTime += deltaTime;
		if (animation.currentTime > animation.end)
		{
			animation.currentTime = animation.start;
		}

		for (auto &channel : animation.channels)
		{
			AnimationSampler &sampler = animation.samplers[channel.samplerIndex];

			// Find the current key frame using binary search
			auto keyFrameIt = std::ranges::lower_bound(sampler.inputs, animation.currentTime);
			if (keyFrameIt != sampler.inputs.end() && keyFrameIt != sampler.inputs.begin())
			{
				size_t i = std::distance(sampler.inputs.begin(), keyFrameIt) - 1;
				float  t = (animation.currentTime - sampler.inputs[i]) / (sampler.inputs[i + 1] - sampler.inputs[i]);

				switch (channel.path)
				{
					case AnimationChannel::TRANSLATION:
					{
						glm::vec3 start           = sampler.outputsVec3[i];
						glm::vec3 end             = sampler.outputsVec3[i + 1];
						channel.node->translation = glm::mix(start, end, t);
						break;
					}
					case AnimationChannel::ROTATION:
					{
						glm::quat start        = glm::quat(sampler.outputsVec4[i].w, sampler.outputsVec4[i].x, sampler.outputsVec4[i].y, sampler.outputsVec4[i].z);
						glm::quat end          = glm::quat(sampler.outputsVec4[i + 1].w, sampler.outputsVec4[i + 1].x, sampler.outputsVec4[i + 1].y, sampler.outputsVec4[i + 1].z);
						channel.node->rotation = glm::slerp(start, end, t);
						break;
					}
					case AnimationChannel::SCALE:
					{
						glm::vec3 start     = sampler.outputsVec3[i];
						glm::vec3 end       = sampler.outputsVec3[i + 1];
						channel.node->scale = glm::mix(start, end, t);
						break;
					}
				}
				break;
			}
		}
	}
};

struct MeshDescriptor
{
	std::uint32_t                         id          = 0;
	std::uint32_t                         vertexCount = 0;
	std::vector<Anjean::Core::MeshVertex> vertices;
};
}        // namespace Anjean::Core