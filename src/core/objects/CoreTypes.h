namespace Anjean::Core
{
  struct Vec3
  {
      float x = 0.0f;
      float y = 0.0f;
      float z = 0.0f;
  };

  struct Transform 
  {
      Vec3 position;
      Vec3 rotation;
      Vec3 scale = { 1.0f, 1.0f, 1.0f };
  };

  enum GameObjectType {
    ANJEAN_GAMEOBJECT,
    ANJEAN_GAMEOBJECT_CAMERA
  };
}