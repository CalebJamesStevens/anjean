#pragma once

#include <SDL3/SDL.h>
#include <memory>

#include "GameObject.h"

namespace Anjean::Core
{
    class Camera : public GameObject
    {
      public:
        float fieldOfView;
        float nearClippingPlane;
        float farClippingPlane;
        float aspectRatio;
        float aperture;
        float focalLength;
        GameObjectType getGameObjectType() override {
          return ANJEAN_GAMEOBJECT_CAMERA;
        };
    };
}