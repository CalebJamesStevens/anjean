#pragma once

#include <vector>

#include "../Core/Core.h"
#include "EntityManager.h"

namespace Anjean::Runtime
{

class RenderSystem : public System
{
  public:
	std::vector<Core::RenderPacket> buildRenderPackets(Coordinator &coordinator) const;
};

}        // namespace Anjean::Runtime