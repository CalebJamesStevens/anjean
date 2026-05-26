#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

#include "core/AppState.h"
#include "window/Window.h"
#include "rendering/Renderer.h"
#include "rendering/RenderTypes.h"

using namespace Anjean;

#define STEP_RATE_IN_MILLISECONDS 125

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

    auto* as = new AppState();

    *appstate = as;

    as->last_step = SDL_GetTicks();

    auto projectRoot = std::filesystem::current_path();
    std::cout << "Project root: " << projectRoot << std::endl;

    if (argc > 1)
    {
        std::cout << "Command: " << argv[1] << std::endl;
    }

    try
    {
        as->window = std::make_unique<Window>(
            "Anjean",
            800,
            600,
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_METAL
        );

        as->renderer = std::make_unique<Renderer>(*as->window);
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

SDL_AppResult SDL_AppIterate(void* appstate)
{
    auto* as = static_cast<AppState*>(appstate);

    const Uint64 now = SDL_GetTicks();

    while ((now - as->last_step) >= STEP_RATE_IN_MILLISECONDS)
    {
        as->last_step += STEP_RATE_IN_MILLISECONDS;

        // Game update logic goes here later.
    }

    as->renderer->beginFrame({ 0.1f, 0.1f, 0.12f, 1.0f });
    as->renderer->drawTestTriangle();
    as->renderer->endFrame();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    (void)appstate;

    switch (event->type)
    {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;

        default:
            break;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    (void)result;

    if (appstate != nullptr)
    {
        auto* as = static_cast<AppState*>(appstate);
        delete as;
    }

    SDL_Quit();
}