#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

#include "Orchestrator/Orchestrator.hpp"
#include "init.h"

using namespace Anjean;

#define STEP_RATE_IN_MILLISECONDS 16

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
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

	if (argc > 1)
	{
		if (std::string(argv[1]) == "init")
		{
			try
			{
				if (argc >= 3)
				{
					Anjean::initProject(argv[2]);
				}
				else
				{
					Anjean::initProject();
				}

				return SDL_APP_SUCCESS;
			}
			catch (const std::exception &e)
			{
				std::cerr << "Init failed: " << e.what() << std::endl;
				return SDL_APP_FAILURE;
			}
		}
		else
		{
			auto *orchestrator = new Orchestrator::Orchestrator();
			*appstate          = orchestrator;
		}
		std::cout << "Command: " << argv[1] << std::endl;
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
	auto *orchestrator = static_cast<Orchestrator::Orchestrator *>(appstate);

	if (orchestrator)
	{
		orchestrator->Tick();
	}

	const Uint64 now = SDL_GetTicks();

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
	auto *orchestrator = static_cast<Orchestrator::Orchestrator *>(appstate);

	if (event->type == SDL_EVENT_QUIT)
	{
		return SDL_APP_SUCCESS;
	}

	if (orchestrator)
	{
		orchestrator->HandleSDLEvent(*event);
	}

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
	(void) result;

	SDL_Quit();
}