#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "InputManager.h"

namespace Anjean::Runtime {
  void InputManager::pollEvents()
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_EVENT_MOUSE_MOTION:
            {
                AnjeanInputEvent inputEvent;
                inputEvent.type = AnjeanInputEventType::MouseMotion;
                inputEvent.mouseMotion.windowXPosition = event.motion.x;
                inputEvent.mouseMotion.windowYPosition = event.motion.y;
                inputEvent.mouseMotion.xMotion = event.motion.xrel;
                inputEvent.mouseMotion.yMotion = event.motion.yrel;
                frameEvents.push_back(inputEvent);
                break;
            }

            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
            {
                AnjeanInputEvent inputEvent;
                inputEvent.type = AnjeanInputEventType::Keyboard;
                inputEvent.keyboardEvent.down = event.type == SDL_EVENT_KEY_DOWN;
                inputEvent.keyboardEvent.key = fromSDLScancode(event.key.scancode);
                frameEvents.push_back(inputEvent);
                std::cout << "Keyboard event" << std::endl;

                break;
            }

            default:
                break;
        }
    }
}
  
  void InputManager::handleSDLEvent(const SDL_Event& event)
{
    switch (event.type)
    {
        case SDL_EVENT_MOUSE_MOTION:
        {
            AnjeanInputEvent inputEvent;
            inputEvent.type = AnjeanInputEventType::MouseMotion;
            inputEvent.mouseMotion.windowXPosition = event.motion.x;
            inputEvent.mouseMotion.windowYPosition = event.motion.y;
            inputEvent.mouseMotion.xMotion = event.motion.xrel;
            inputEvent.mouseMotion.yMotion = event.motion.yrel;

            frameEvents.push_back(inputEvent);
            break;
        }

        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
        {
            AnjeanInputEvent inputEvent;
            inputEvent.type = AnjeanInputEventType::Keyboard;
            inputEvent.keyboardEvent.down = event.type == SDL_EVENT_KEY_DOWN;
            inputEvent.keyboardEvent.key = fromSDLScancode(event.key.scancode);

            frameEvents.push_back(inputEvent);

            std::cout << "Keyboard event queued" << std::endl;
            break;
        }

        default:
            break;
    }
}

  void InputManager::updateInputState()
{
    for (const AnjeanInputEvent& event : frameEvents)
    {
        switch (event.type)
        {
            case AnjeanInputEventType::MouseMotion:
            {
                inputState.mouseState.deltaX =
                    event.mouseMotion.windowXPosition - inputState.mouseState.x;

                inputState.mouseState.deltaY =
                    event.mouseMotion.windowYPosition - inputState.mouseState.y;

                inputState.mouseState.x = event.mouseMotion.windowXPosition;
                inputState.mouseState.y = event.mouseMotion.windowYPosition;
                inputState.mouseState.movedThisFrame = true;

                break;
            }

            case AnjeanInputEventType::Keyboard:
            {
                Key key = event.keyboardEvent.key;
                std::cout << "Keyboard event" << std::endl;
                if (key != Key::Unknown)
                {
                    inputState.keyboardState.keys[toIndex(key)].isDown =
                        event.keyboardEvent.down;
                }

                break;
            }
        }
    }

    frameEvents.clear();
}

  Key InputManager::fromSDLScancode(SDL_Scancode scancode)
  {
      switch (scancode)
      {
          case SDL_SCANCODE_A: return Key::A;
          case SDL_SCANCODE_B: return Key::B;
          case SDL_SCANCODE_C: return Key::C;
          case SDL_SCANCODE_D: return Key::D;
          case SDL_SCANCODE_E: return Key::E;
          case SDL_SCANCODE_F: return Key::F;
          case SDL_SCANCODE_G: return Key::G;
          case SDL_SCANCODE_H: return Key::H;
          case SDL_SCANCODE_I: return Key::I;
          case SDL_SCANCODE_J: return Key::J;
          case SDL_SCANCODE_K: return Key::K;
          case SDL_SCANCODE_L: return Key::L;
          case SDL_SCANCODE_M: return Key::M;
          case SDL_SCANCODE_N: return Key::N;
          case SDL_SCANCODE_O: return Key::O;
          case SDL_SCANCODE_P: return Key::P;
          case SDL_SCANCODE_Q: return Key::Q;
          case SDL_SCANCODE_R: return Key::R;
          case SDL_SCANCODE_S: return Key::S;
          case SDL_SCANCODE_T: return Key::T;
          case SDL_SCANCODE_U: return Key::U;
          case SDL_SCANCODE_V: return Key::V;
          case SDL_SCANCODE_W: return Key::W;
          case SDL_SCANCODE_X: return Key::X;
          case SDL_SCANCODE_Y: return Key::Y;
          case SDL_SCANCODE_Z: return Key::Z;

          case SDL_SCANCODE_SPACE: return Key::Space;
          case SDL_SCANCODE_ESCAPE: return Key::Escape;
          case SDL_SCANCODE_RETURN: return Key::Enter;

          default: return Key::Unknown;
      }
  }

  bool InputManager::isKeyDown(Key key) const
  {
    if (key == Key::Unknown || key >= Key::Count)
      {
          return false;
      }

      return inputState.keyboardState.keys[toIndex(key)].isDown;
  }
}