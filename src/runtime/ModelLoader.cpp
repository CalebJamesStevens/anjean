#include "ModelLoader.h"

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

namespace Anjean::Runtime
{
namespace
{
const unsigned char *accessorData(const tinygltf::Model &model, const tinygltf::Accessor &accessor)
{
	const auto &view   = model.bufferViews[accessor.bufferView];
	const auto &buffer = model.buffers[view.buffer];
	return buffer.data.data() + view.byteOffset + accessor.byteOffset;
}

size_t accessorStride(const tinygltf::Model &model, const tinygltf::Accessor &accessor, size_t fallback)
{
	const auto &view = model.bufferViews[accessor.bufferView];
	return view.byteStride != 0 ? view.byteStride : fallback;
}

uint32_t readIndex(const unsigned char *data, size_t index, int componentType, size_t stride)
{
	const unsigned char *ptr = data + index * stride;

	switch (componentType)
	{
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
			return *reinterpret_cast<const uint8_t *>(ptr);
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
			return *reinterpret_cast<const uint16_t *>(ptr);
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
			return *reinterpret_cast<const uint32_t *>(ptr);
		default:
			throw std::runtime_error("Unsupported glTF index component type");
	}
}

glm::mat4 readNodeTransform(const tinygltf::Node &node)
{
	if (node.matrix.size() == 16)
	{
		return glm::mat4(
		    static_cast<float>(node.matrix[0]), static_cast<float>(node.matrix[1]),
		    static_cast<float>(node.matrix[2]), static_cast<float>(node.matrix[3]),
		    static_cast<float>(node.matrix[4]), static_cast<float>(node.matrix[5]),
		    static_cast<float>(node.matrix[6]), static_cast<float>(node.matrix[7]),
		    static_cast<float>(node.matrix[8]), static_cast<float>(node.matrix[9]),
		    static_cast<float>(node.matrix[10]), static_cast<float>(node.matrix[11]),
		    static_cast<float>(node.matrix[12]), static_cast<float>(node.matrix[13]),
		    static_cast<float>(node.matrix[14]), static_cast<float>(node.matrix[15]));
	}

	glm::mat4 translation{1.0f};
	glm::mat4 rotation{1.0f};
	glm::mat4 scale{1.0f};

	if (node.translation.size() == 3)
	{
		translation = glm::translate(
		    glm::mat4{1.0f},
		    glm::vec3{
		        static_cast<float>(node.translation[0]),
		        static_cast<float>(node.translation[1]),
		        static_cast<float>(node.translation[2])});
	}

	if (node.rotation.size() == 4)
	{
		glm::quat q{
		    static_cast<float>(node.rotation[3]),
		    static_cast<float>(node.rotation[0]),
		    static_cast<float>(node.rotation[1]),
		    static_cast<float>(node.rotation[2])};
		rotation = glm::mat4_cast(q);
	}

	if (node.scale.size() == 3)
	{
		scale = glm::scale(
		    glm::mat4{1.0f},
		    glm::vec3{
		        static_cast<float>(node.scale[0]),
		        static_cast<float>(node.scale[1]),
		        static_cast<float>(node.scale[2])});
	}

	return translation * rotation * scale;
}

uint32_t textureImageIndex(const tinygltf::Model &model, int textureIndex)
{
	if (textureIndex < 0 || textureIndex >= static_cast<int>(model.textures.size()))
		return Core::InvalidImportIndex;

	const tinygltf::Texture &texture = model.textures[textureIndex];
	if (texture.source >= 0)
		return static_cast<uint32_t>(texture.source);

	return Core::InvalidImportIndex;
}

uint32_t textureSamplerIndex(const tinygltf::Model &model, int textureIndex)
{
	if (textureIndex < 0 || textureIndex >= static_cast<int>(model.textures.size()))
		return Core::InvalidImportIndex;

	const tinygltf::Texture &texture = model.textures[textureIndex];
	if (texture.sampler >= 0)
		return static_cast<uint32_t>(texture.sampler);

	return Core::InvalidImportIndex;
}
}        // namespace

Core::ModelAssetId MeshLoader::loadModelAsset(const std::filesystem::path &path)
{
	const std::filesystem::path normalizedPath =
	    std::filesystem::absolute(path).lexically_normal();

	auto it = pathToModelId.find(normalizedPath);
	if (it != pathToModelId.end())
	{
		return it->second;
	}

	Core::ImportedModel imported = importModelFile(normalizedPath);

	Core::ModelAssetId id = nextModelAssetId++;
	imported.id           = id;
	imported.sourcePath   = normalizedPath;

	pathToModelId.emplace(normalizedPath, id);
	models.emplace(id, std::move(imported));

	return id;
}

const Core::ImportedModel *MeshLoader::getModel(Core::ModelAssetId id) const
{
	auto it = models.find(id);
	if (it == models.end())
	{
		return nullptr;
	}

	return &it->second;
}

const Core::ImportedModel *MeshLoader::getModel(const std::filesystem::path &path) const
{
	const std::filesystem::path normalizedPath =
	    std::filesystem::absolute(path).lexically_normal();

	auto idIt = pathToModelId.find(normalizedPath);
	if (idIt == pathToModelId.end())
	{
		return nullptr;
	}

	return getModel(idIt->second);
}

bool MeshLoader::hasModel(Core::ModelAssetId id) const
{
	return models.contains(id);
}

Core::ImportedModel MeshLoader::importModelFile(const std::filesystem::path &path) const
{
	tinygltf::Model    model;
	tinygltf::TinyGLTF loader;
	std::string        err;
	std::string        warn;

	bool ok = path.extension() == ".glb" ? loader.LoadBinaryFromFile(&model, &err, &warn, path.string()) : loader.LoadASCIIFromFile(&model, &err, &warn, path.string());

	if (!warn.empty())
		std::cout << "glTF warning: " << warn << std::endl;

	if (!err.empty())
		std::cout << "glTF error: " << err << std::endl;

	if (!ok)
		throw std::runtime_error("Failed to load glTF model: " + path.string());

	Core::ImportedModel result{};

	result.textures.reserve(model.images.size());

	for (tinygltf::Image &image : model.images)
	{
		auto &texture = result.textures.emplace_back();

		texture.name     = std::move(image.name);
		texture.uri      = std::move(image.uri);
		texture.bytes    = std::move(image.image);
		texture.width    = image.width;
		texture.height   = image.height;
		texture.channels = image.component;
	}

	result.samplers.reserve(model.samplers.size());
	for (const tinygltf::Sampler &gltfSampler : model.samplers)
	{
		Core::ImportedSampler sampler{};
		sampler.magFilter = gltfSampler.magFilter;
		sampler.minFilter = gltfSampler.minFilter;
		sampler.wrapS     = gltfSampler.wrapS;
		sampler.wrapT     = gltfSampler.wrapT;
		result.samplers.push_back(sampler);
	}

	result.materials.reserve(model.materials.size());
	for (const tinygltf::Material &gltfMaterial : model.materials)
	{
		Core::ImportedMaterial material{};
		material.name = gltfMaterial.name;

		const auto &pbr = gltfMaterial.pbrMetallicRoughness;

		if (pbr.baseColorFactor.size() == 4)
		{
			material.baseColorFactor = {
			    static_cast<float>(pbr.baseColorFactor[0]),
			    static_cast<float>(pbr.baseColorFactor[1]),
			    static_cast<float>(pbr.baseColorFactor[2]),
			    static_cast<float>(pbr.baseColorFactor[3])};
		}

		material.metallicFactor  = static_cast<float>(pbr.metallicFactor);
		material.roughnessFactor = static_cast<float>(pbr.roughnessFactor);
		material.alphaCutoff     = static_cast<float>(gltfMaterial.alphaCutoff);

		material.baseColorTextureIndex = textureImageIndex(model, pbr.baseColorTexture.index);
		material.baseColorSamplerIndex = textureSamplerIndex(model, pbr.baseColorTexture.index);

		material.normalTextureIndex = textureImageIndex(model, gltfMaterial.normalTexture.index);
		material.normalSamplerIndex = textureSamplerIndex(model, gltfMaterial.normalTexture.index);

		material.metallicRoughnessTextureIndex =
		    textureImageIndex(model, pbr.metallicRoughnessTexture.index);
		material.metallicRoughnessSamplerIndex =
		    textureSamplerIndex(model, pbr.metallicRoughnessTexture.index);

		material.emissiveTextureIndex = textureImageIndex(model, gltfMaterial.emissiveTexture.index);
		material.emissiveSamplerIndex = textureSamplerIndex(model, gltfMaterial.emissiveTexture.index);

		result.materials.push_back(material);
	}

	result.meshes.reserve(model.meshes.size());

	for (const tinygltf::Mesh &gltfMesh : model.meshes)
	{
		Core::ImportedMesh mesh{};
		mesh.name               = gltfMesh.name;
		mesh.firstMeshPrimitive = static_cast<uint32_t>(result.meshPrimitives.size());

		for (const tinygltf::Primitive &gltfPrimitive : gltfMesh.primitives)
		{
			auto posIt = gltfPrimitive.attributes.find("POSITION");
			if (posIt == gltfPrimitive.attributes.end())
				continue;

			const tinygltf::Accessor &posAccessor = model.accessors[posIt->second];

			if (posAccessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT ||
			    posAccessor.type != TINYGLTF_TYPE_VEC3)
			{
				throw std::runtime_error("Unsupported glTF POSITION format");
			}

			const unsigned char *positions      = accessorData(model, posAccessor);
			size_t               positionStride = accessorStride(model, posAccessor, sizeof(float) * 3);

			const unsigned char *normals      = nullptr;
			size_t               normalStride = 0;
			auto                 normalIt     = gltfPrimitive.attributes.find("NORMAL");
			if (normalIt != gltfPrimitive.attributes.end())
			{
				const tinygltf::Accessor &normalAccessor = model.accessors[normalIt->second];
				normals                                  = accessorData(model, normalAccessor);
				normalStride                             = accessorStride(model, normalAccessor, sizeof(float) * 3);
			}

			const unsigned char *uvs      = nullptr;
			size_t               uvStride = 0;
			size_t               uvCount  = 0;
			auto                 uvIt     = gltfPrimitive.attributes.find("TEXCOORD_0");
			if (uvIt != gltfPrimitive.attributes.end())
			{
				const tinygltf::Accessor &uvAccessor = model.accessors[uvIt->second];
				if (uvAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT &&
				    uvAccessor.type == TINYGLTF_TYPE_VEC2)
				{
					uvs      = accessorData(model, uvAccessor);
					uvStride = accessorStride(model, uvAccessor, sizeof(float) * 2);
					uvCount  = uvAccessor.count;
				}
			}

			Core::ImportedPrimitive primitive{};
			primitive.firstVertex = static_cast<uint32_t>(result.vertices.size());
			primitive.vertexCount = static_cast<uint32_t>(posAccessor.count);
			primitive.firstIndex  = static_cast<uint32_t>(result.indices.size());

			for (size_t i = 0; i < posAccessor.count; ++i)
			{
				const float *position =
				    reinterpret_cast<const float *>(positions + i * positionStride);

				Core::ImportedVertex vertex{};
				vertex.position = {
				    position[0],
				    position[1],
				    position[2],
				};

				if (normals != nullptr)
				{
					const float *normal =
					    reinterpret_cast<const float *>(normals + i * normalStride);
					vertex.normal = {normal[0], normal[1], normal[2]};
				}

				if (uvs != nullptr && i < uvCount)
				{
					const float *uv =
					    reinterpret_cast<const float *>(uvs + i * uvStride);
					vertex.uv = {uv[0], uv[1]};
				}

				result.vertices.push_back(vertex);
			}

			if (gltfPrimitive.indices >= 0)
			{
				const tinygltf::Accessor &indexAccessor = model.accessors[gltfPrimitive.indices];
				const unsigned char      *indexData     = accessorData(model, indexAccessor);

				size_t indexStride = 0;
				switch (indexAccessor.componentType)
				{
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
						indexStride = sizeof(uint8_t);
						break;
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
						indexStride = sizeof(uint16_t);
						break;
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
						indexStride = sizeof(uint32_t);
						break;
					default:
						throw std::runtime_error("Unsupported glTF index component type");
				}

				primitive.indexCount = static_cast<uint32_t>(indexAccessor.count);
				result.indices.reserve(result.indices.size() + indexAccessor.count);

				for (size_t i = 0; i < indexAccessor.count; ++i)
				{
					result.indices.push_back(
					    readIndex(indexData, i, indexAccessor.componentType, indexStride));
				}
			}
			else
			{
				primitive.indexCount = primitive.vertexCount;
				for (uint32_t i = 0; i < primitive.vertexCount; ++i)
					result.indices.push_back(i);
			}

			uint32_t primitiveIndex = static_cast<uint32_t>(result.primitives.size());
			result.primitives.push_back(primitive);

			Core::ImportedMeshPrimitive meshPrimitive{};
			meshPrimitive.primitiveIndex = primitiveIndex;
			meshPrimitive.materialIndex  = gltfPrimitive.material >= 0 ? static_cast<uint32_t>(gltfPrimitive.material) : Core::InvalidImportIndex;

			result.meshPrimitives.push_back(meshPrimitive);
			mesh.meshPrimitiveCount++;
		}

		result.meshes.push_back(mesh);
	}

	std::function<void(int, const glm::mat4 &)> traverseNode =
	    [&](int nodeIndex, const glm::mat4 &parentTransform) {
		    if (nodeIndex < 0 || nodeIndex >= static_cast<int>(model.nodes.size()))
			    return;

		    const tinygltf::Node &gltfNode       = model.nodes[nodeIndex];
		    glm::mat4             worldTransform = parentTransform * readNodeTransform(gltfNode);

		    if (gltfNode.mesh >= 0)
		    {
			    Core::ImportedNode node{};
			    node.name      = gltfNode.name;
			    node.meshIndex = static_cast<uint32_t>(gltfNode.mesh);
			    node.transform = worldTransform;
			    result.nodes.push_back(node);
		    }

		    for (int childIndex : gltfNode.children)
			    traverseNode(childIndex, worldTransform);
	    };

	int sceneIndex = model.defaultScene >= 0 ? model.defaultScene : 0;
	if (sceneIndex >= 0 && sceneIndex < static_cast<int>(model.scenes.size()))
	{
		for (int rootNode : model.scenes[sceneIndex].nodes)
			traverseNode(rootNode, glm::mat4{1.0f});
	}

	return result;
}
}        // namespace Anjean::Runtime