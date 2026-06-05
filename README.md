# Anjean the Little Engine That Might

## Basic Architecture

The entirety of the game engine is made of 3 seperate engines:

1. Runtime Engine
2. Physics Engine
3. Graphics Engine
4. Audio Engine

Each Engine could theoretically stand alone as its own engine, albeit with limited capabilits. The World
Engine acts as a database and logic processor, the Physics Engine calculates simulated interaction
between entities from the Runtime Engine, the Graphics Engine generates the visuals of the game based
on input from the Physics or Runtime Engines, and the Audio Engine handle the audio processing.

Whether or not the four engines will have applications disconnectected from the others remains up
in the air, however, care is taken to decrease the coupling between the four as much as possible.

### Runtime Engine
The primary language of the Runtime Engine will be C++. An API layer for C# scripts will be created
to allow for developer defined scripts.

The Runtime Engine itself will be broken down into "mini" engines:
- Input handling
- Data handling
- Scene management
- Logic/Script Processing
- Entity management
- AI Engine
- Event / Messaging Engine
- Resource / Asset Reference Engine
- Timing / Tick Engine
- Engine communication manager
- UI System


### Physics Engine
I'm still in the process of deciding what this will entail. 

### Graphics Engine
The Graphics Engine's primary language will be C++. The Mac implementation is currently being developed
using the Metal framework. For all other platforms, Vulkan is the planned framework. A cross-platofrm
abstraction layer will be developed to make development easier. Currentl, SDL is being used as a
cross-platform implementatino for windowing and input handling. I'm still debating replacing this
with an in-house implementation, but it's lower on the priority list.

### Audio Engine
Nooo idea yet.