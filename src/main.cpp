#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

#include "core/AppState.h"
#include "core/objects/GameObject.h"
#include "window/Window.h"
#include "rendering/Renderer.h"
#include "rendering/RenderTypes.h"

using namespace Anjean;

#define STEP_RATE_IN_MILLISECONDS 16

static Anjean::Mesh CreateMesh(
    Anjean::Renderer& renderer,
    const Anjean::Vertex2D* vertices,
    std::uint32_t vertexCount
)
{
    Anjean::BufferDesc desc;
    desc.data = vertices;
    desc.size = sizeof(Anjean::Vertex2D) * vertexCount;

    Anjean::Mesh mesh;
    mesh.vertexBuffer = renderer.createBuffer(desc);
    mesh.vertexCount = vertexCount;

    return mesh;
}

static Anjean::Mesh CreateTextureMesh(
    Anjean::Renderer& renderer,
    const Anjean::TexturedVertex2D* vertices,
    std::uint32_t vertexCount
)
{
    Anjean::BufferDesc desc;
    desc.data = vertices;
    desc.size = sizeof(vertices[0]) * vertexCount;

    Anjean::Mesh mesh;
    mesh.vertexBuffer = renderer.createBuffer(desc);
    mesh.vertexCount = vertexCount;

    return mesh;
}

static Anjean::Mesh CreateTextureMesh3D(
    Anjean::Renderer& renderer,
    const Anjean::TexturedVertex3D* vertices,
    std::uint32_t vertexCount
)
{
    Anjean::BufferDesc desc;
    desc.data = vertices;
    desc.size = sizeof(vertices[0]) * vertexCount;

    Anjean::Mesh mesh;
    mesh.vertexBuffer = renderer.createBuffer(desc);
    mesh.vertexCount = vertexCount;

    return mesh;
}

simd::float4x4 makeRotationY(float radians)
{
    float c = std::cos(radians);
    float s = std::sin(radians);

    return simd::float4x4{
        simd_make_float4( c, 0, -s, 0),
        simd_make_float4( 0, 1,  0, 0),
        simd_make_float4( s, 0,  c, 0),
        simd_make_float4( 0, 0,  0, 1)
    };
}

simd_float4x4 makePerspective(
    float fovYRadians,
    float aspect,
    float nearZ,
    float farZ
)
{
    float yScale = 1.0f / tanf(fovYRadians * 0.5f);
    float xScale = yScale / aspect;
    float zRange = nearZ - farZ;

    return simd_float4x4{
        simd_make_float4(xScale, 0,      0,                      0),
        simd_make_float4(0,      yScale, 0,                      0),
        simd_make_float4(0,      0,      farZ / zRange,         -1),
        simd_make_float4(0,      0,      nearZ * farZ / zRange,  0)
    };
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    if (!SDL_SetAppMetadata("Anjean Editor", "1.0", "com.Anjean.Editor"))
    {
        return SDL_APP_FAILURE;
    }

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK))
    {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    auto* as = new Core::AppState();

    *appstate = as;

    as->last_step = SDL_GetTicks();

    auto projectRoot = std::filesystem::current_path();
    std::cout << "Project root: " << projectRoot << std::endl;
    std::cout << "File: " << argv[6] << std::endl;

    if (argc > 1)
    {
        std::cout << "Command: " << argv[1] << std::endl;
    }

    int screenWidth = std::stoi(argv[2]);
    int screenHeight = std::stoi(argv[3]);
    float textWidth = std::stof(argv[4]);
    float textHeight = std::stof(argv[5]);
    const char* filename = argv[6];
    as->rotationSpeed = std::stof(argv[7]);


    try
    {
        as->window = std::make_unique<Window>(
            "Anjean",
            screenWidth,
            screenHeight,
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_METAL
        );

        int wWidth = 0;
        int wHeight = 0;

        // 2. Pass the addresses of those variables using '&'
        SDL_GetWindowSize(as->window->getNativeWindow(), &wWidth, &wHeight);
        float aspect = (float)wWidth / (float)wHeight;

        as->cameraPos = { 0.0f, 0.0f, 2.0f };
        as->projectionMatrix = makePerspective(
            70.0f * M_PI / 180.0f,
            aspect,
            0.1f,
            100.0f
        );

        as->renderer = std::make_unique<Renderer>(*as->window);
        PipelineDesc pipelineDesc;
        pipelineDesc.vertexFunction = "sprite_vertex_shader";
        pipelineDesc.fragmentFunction = "sprite_fragment_shader";
        
        as->basicColorPipeline = as->renderer->createPipeline(pipelineDesc);
        TextureDesc textureDesc;
        textureDesc.channels = 4;
        textureDesc.filename = filename;
        as->demoTexture = as->renderer->createTexture(textureDesc);

        float hw = textWidth / 2.0f;
        float hh = textHeight / 2.0f;
        float hd = textHeight / 2.0f;

        TexturedVertex3D quadVertices[] = {
            // { {  textWidth/2, -textHeight/2, 0 }, { 1.0f, 1.0f } },
            // { { -textWidth/2, -textHeight/2, 0 }, { 0.0f, 1.0f } },
            // { { -textWidth/2,  textHeight/2, 0 }, { 0.0f, 0.0f } },
            // { {  textWidth/2, -textHeight/2, 0 }, { 1.0f, 1.0f } },
            // { { -textWidth/2,  textHeight/2, 0 }, { 0.0f, 0.0f } },
            // { {  textWidth/2,  textHeight/2, 0 }, { 1.0f, 0.0f } },

            { { -hw, -hh,  hd }, { 0.0f, 1.0f } },
    { {  hw, -hh,  hd }, { 1.0f, 1.0f } },
    { { -hw,  hh,  hd }, { 0.0f, 0.0f } },
    { {  hw, -hh,  hd }, { 1.0f, 1.0f } },
    { {  hw,  hh,  hd }, { 1.0f, 0.0f } },
    { { -hw,  hh,  hd }, { 0.0f, 0.0f } },

    // Back, -Z
    { {  hw, -hh, -hd }, { 0.0f, 1.0f } },
    { { -hw, -hh, -hd }, { 1.0f, 1.0f } },
    { {  hw,  hh, -hd }, { 0.0f, 0.0f } },
    { { -hw, -hh, -hd }, { 1.0f, 1.0f } },
    { { -hw,  hh, -hd }, { 1.0f, 0.0f } },
    { {  hw,  hh, -hd }, { 0.0f, 0.0f } },

    // Top, +Y
    { { -hw,  hh,  hd }, { 0.0f, 1.0f } },
    { {  hw,  hh,  hd }, { 1.0f, 1.0f } },
    { { -hw,  hh, -hd }, { 0.0f, 0.0f } },
    { {  hw,  hh,  hd }, { 1.0f, 1.0f } },
    { {  hw,  hh, -hd }, { 1.0f, 0.0f } },
    { { -hw,  hh, -hd }, { 0.0f, 0.0f } },

    // Bottom, -Y
    { { -hw, -hh, -hd }, { 0.0f, 1.0f } },
    { {  hw, -hh, -hd }, { 1.0f, 1.0f } },
    { { -hw, -hh,  hd }, { 0.0f, 0.0f } },
    { {  hw, -hh, -hd }, { 1.0f, 1.0f } },
    { {  hw, -hh,  hd }, { 1.0f, 0.0f } },
    { { -hw, -hh,  hd }, { 0.0f, 0.0f } },

    // Left, -X
    { { -hw, -hh,  hd }, { 0.0f, 1.0f } },
    { { -hw,  hh,  hd }, { 1.0f, 1.0f } },
    { { -hw, -hh, -hd }, { 0.0f, 0.0f } },
    { { -hw,  hh,  hd }, { 1.0f, 1.0f } },
    { { -hw,  hh, -hd }, { 1.0f, 0.0f } },
    { { -hw, -hh, -hd }, { 0.0f, 0.0f } },

    // Right, +X
    { {  hw, -hh, -hd }, { 0.0f, 1.0f } },
    { {  hw,  hh, -hd }, { 1.0f, 1.0f } },
    { {  hw, -hh,  hd }, { 0.0f, 0.0f } },
    { {  hw,  hh, -hd }, { 1.0f, 1.0f } },
    { {  hw,  hh,  hd }, { 1.0f, 0.0f } },
    { {  hw, -hh,  hd }, { 0.0f, 0.0f } },
        };
        
        auto vertexCount = static_cast<std::uint32_t>(
            sizeof(quadVertices) / sizeof(quadVertices[0])
        );
        as->demoMesh = CreateTextureMesh3D(
            *as->renderer,
            quadVertices,
            vertexCount
        );
        
        as->camera = std::make_unique<Core::GameObject>();

        as->cube1 = std::make_unique<Core::GameObject>();
        as->cube1->mesh = as->demoMesh;
        as->cube1->transform.position.x = 0.0f;
        as->cube1->transform.position.y = 0.0f;
        as->cube1->transform.position.z = 0.0f;
        
        as->cube2 = std::make_unique<Core::GameObject>();
        as->cube2->mesh = as->demoMesh;
        as->cube2->transform.position.x = 0.6f;
        as->cube2->transform.position.y = 0.0f;
        as->cube2->transform.position.z = -2.0f;

        as->gameObjectsToRender.push_back(as->cube1.get());
        as->gameObjectsToRender.push_back(as->cube2.get());
    }
    catch (const std::exception& e)
    {
        SDL_Log("Startup failed: %s", e.what());
        delete as;
        *appstate = nullptr;
        return SDL_APP_FAILURE;
    }


    return SDL_APP_CONTINUE;
}

float rotationY = 0.0f;
SDL_AppResult SDL_AppIterate(void* appstate)
{
    auto* as = static_cast<Core::AppState*>(appstate);

    const Uint64 now = SDL_GetTicks();


    
    
    while ((now - as->last_step) >= STEP_RATE_IN_MILLISECONDS)
    {
        as->last_step += STEP_RATE_IN_MILLISECONDS;
        rotationY += as->rotationSpeed;
        const bool* state = SDL_GetKeyboardState(NULL);
        if (state[SDL_SCANCODE_W]) {
            // Space bar is currently held down
            as->cameraPos[2] -= .005;
        }
        if (state[SDL_SCANCODE_A]) {
            // Space bar is currently held down
            as->cameraPos[0] -= .005;
        }
        if (state[SDL_SCANCODE_S]) {
            // Space bar is currently held down
            as->cameraPos[2] += .005;
        }
        if (state[SDL_SCANCODE_D]) {
            // Space bar is currently held down
            as->cameraPos[0] += .005;
        }        
    }

    as->cube2->transform.rotation.y = rotationY;
    
    as->cameraMatrix= matrix_multiply(
      matrix_multiply({
          simd_make_float4(1,0,0,0),
          simd_make_float4(0,1,0,0),
          simd_make_float4(0,0,1,0),
          simd_make_float4(-as->cameraPos[0],-as->cameraPos[1],-as->cameraPos[2],1)
        }, 
        makeRotationY(0)
      ),
      {
        simd_make_float4(1,0,0,0),
        simd_make_float4(0,1,0,0),
        simd_make_float4(0,0,1,0),
        simd_make_float4(0,0,0,1)
      }
    );
    
    as->renderer->beginFrame({ 0.1f, 0.1f, 0.12f, 1.0f });
    for (Core::GameObject* gO : as->gameObjectsToRender) {
      ObjectUniformHandle oUH1;
      oUH1.modelTranslation = {
        simd_make_float4(1,0,0,0),
        simd_make_float4(0,1,0,0),
        simd_make_float4(0,0,1,0),
        simd_make_float4(gO->transform.position.x,gO->transform.position.y,gO->transform.position.z,1)
      };
      oUH1.modelRot = makeRotationY(gO->transform.rotation.y);
      oUH1.modelScale = {
        simd_make_float4(1,0,0,0),
        simd_make_float4(0,1,0,0),
        simd_make_float4(0,0,1,0),
        simd_make_float4(0,0,0,1)
      };
      oUH1.model = matrix_multiply(
          matrix_multiply(oUH1.modelTranslation, oUH1.modelRot),
          oUH1.modelScale
      );
      oUH1.viewProjection = matrix_multiply(as->projectionMatrix, as->cameraMatrix);
      as->renderer->drawSprite(as->basicColorPipeline, as->demoMesh, as->demoTexture, oUH1);
    }

    as->renderer->endFrame();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    (void)appstate;

    switch (event->type)
    {
        case SDL_EVENT_MOUSE_MOTION:
            std::cout << "Mouse Motion Detected - "
                << "x: " << event->motion.x
                << ", y: " << event->motion.y << '\n' << std::endl;
            break;
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;

        default:
            std::cout << event->type;
            break;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    (void)result;

    if (appstate != nullptr)
    {
        auto* as = static_cast<Core::AppState*>(appstate);
        delete as;
    }

    SDL_Quit();
}