#include "RenderSystem.h"

#include <algorithm>

namespace Anjean::Runtime
{

std::vector<Core::RenderPacket>
    RenderSystem::buildRenderPackets(Coordinator &coordinator) const
{
	std::vector<Core::RenderPacket> packets;
	packets.reserve(mEntities.size());

	for (Entity entity : mEntities)
	{
		const auto &transform =
		    coordinator.GetComponent<Core::TransformComponent>(entity);

		const auto &render =
		    coordinator.GetComponent<Core::RenderComponent>(entity);

		if (render.meshId == 0)
		{
			continue;
		}

		packets.push_back({
		    .objectId = entity,
		    .meshId   = render.meshId,
		    .position = transform.position,
		    .rotation = transform.rotation,
		    .scale    = transform.scale,
		});
	}

	std::ranges::sort(packets, {}, &Core::RenderPacket::objectId);
	return packets;
}

}        // namespace Anjean::Runtime