#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iostream>
#include <queue> 

namespace Anjean::Runtime {
  enum class Key : int32_t
{
    Unknown = 0,

    A = 1,
    B = 2,
    C = 3,
    D = 4,
    E = 5,
    F = 6,
    G = 7,
    H = 8,
    I = 9,
    J = 10,
    K = 11,
    L = 12,
    M = 13,
    N = 14,
    O = 15,
    P = 16,
    Q = 17,
    R = 18,
    S = 19,
    T = 20,
    U = 21,
    V = 22,
    W = 23,
    X = 24,
    Y = 25,
    Z = 26,

    Space = 27,
    Escape = 28,
    Enter = 29,

    Count = 30
};
  struct MouseState
  {
    float x = 0.0f;
    float y = 0.0f;

    float deltaX = 0.0f;
    float deltaY = 0.0f;

    float velocityX = 0.0f;
    float velocityY = 0.0f;

    bool leftDown = false;
    bool rightDown = false;
    bool middleDown = false;

    bool movedThisFrame = false;
  };
  
  struct KeyState {
    bool isDown = false;
  };

  struct KeyboardState
  {
    std::array<KeyState, static_cast<std::size_t>(Key::Count)> keys;
  };

  inline std::size_t toIndex(Key key)
  {
      return static_cast<std::size_t>(key);
  }

  struct InputState {
    MouseState mouseState;
    KeyboardState keyboardState;
  };
  
  struct AnjeanInputEventMouseEvent {
    // TODO: Implement our own event emitters
    /**
     * SDL Mouse Motion Event give us this:
     *  float x;            < X coordinate, relative to window 
     *  float y;            < Y coordinate, relative to window 
     *  float xrel;         < The relative motion in the X direction 
     *  float yrel;         < The relative motion in the Y direction 
     */

     float windowXPosition;
     float windowYPosition;
     float xMotion;
     float yMotion;
  };
  
  struct AnjeanInputEventKeyboardEvent {
    Key key = Key::Unknown;
    bool down = false;
  };

  enum class AnjeanInputEventType
  {
      MouseMotion,
      MouseButton,
      Keyboard
  };

  struct AnjeanInputEvent{
    AnjeanInputEventType type;
    AnjeanInputEventMouseEvent mouseMotion;
    AnjeanInputEventKeyboardEvent keyboardEvent;
    uint8_t padding[128];
  };

  class InputManager {
    public:
      std::vector<AnjeanInputEvent> frameEvents;
      InputState inputState;
      void pollEvents();
      void updateInputState();
      bool isKeyDown(Key key) const;
      static Key fromSDLScancode(SDL_Scancode scancode);
      void handleSDLEvent(const SDL_Event& event);
  };
}