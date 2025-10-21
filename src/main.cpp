#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_render.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3/SDL_video.h>
#include <cstdlib>
#include <iostream>
#include <vector>

bool checkCollision(const SDL_FRect &a, const SDL_FRect &b)
{
	return (a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y);
}

struct GameTextures
{
	SDL_Texture *tank;
};

class Main
{
  public:
	Main();
	~Main();
	GameTextures loadTextures();
	SDL_Window *pWindow;
	SDL_Renderer *pRenderer;
};

Main::Main()
{
	bool bSuccess = true;

	bSuccess = SDL_SetAppMetadata("TankGame", "0.1", "at.ac.uibk.advancedcpp.team8.tankgame");
	if(!bSuccess)
	{
		SDL_Log("Failed to set app metadata!\n");
		goto END_FAIL;
	}

	bSuccess = SDL_Init(SDL_INIT_VIDEO);
	if(!bSuccess)
	{
		SDL_Log("Failed to initialize SDL! Error: %s\n", SDL_GetError());
		goto END_FAIL;
	}

END:
	return;

END_FAIL:
	char const *errstr = SDL_GetError();
	SDL_Log("Last Error: %s\n", errstr);
	throw errstr;
}

GameTextures Main::loadTextures()
{
	GameTextures textures = {};
	textures.tank = IMG_LoadTexture(pRenderer, "../assets/tank.png");
	if(!textures.tank)
	{
		SDL_Log("Failed to Load Texture. Error %s\n", SDL_GetError());
	}
	textures.tank = textures.tank;

	return textures;
}

Main::~Main()
{
	SDL_DestroyWindow(pWindow);
	SDL_DestroyRenderer(pRenderer);

	pWindow = nullptr;
	pRenderer = nullptr;

	SDL_Quit();
}

int main(int argc, char *argv[])
{
	Main sdl;

	const int WINDOW_WIDTH = 800;
	const int WINDOW_HEIGHT = 600;
	const int WALL_THICKNESS = 20;

	sdl.pWindow =
		SDL_CreateWindow("Tank Game", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_MOUSE_FOCUS);
	if(!sdl.pWindow)
	{
		SDL_Log("SDL CreateWindow Error: %s\n", SDL_GetError());
		return EXIT_FAILURE;
	}

	sdl.pRenderer = SDL_CreateRenderer(sdl.pWindow, NULL);
	if(!sdl.pRenderer)
	{
		std::cerr << "SDL CreateRenderer Error: " << SDL_GetError() << "\n";
		return EXIT_FAILURE;
	}

	GameTextures textures = sdl.loadTextures();

	float tankX = 368.0f;
	float tankY = 268.0f;
	SDL_FRect tankRect{tankX, tankY, 64, 64};
	double tankAngle = 0.0;

	bool running = true;
	SDL_Event event;

	std::vector<SDL_FRect> walls;

	// Outer and inner walls
	walls.push_back({0, 0, WINDOW_WIDTH, WALL_THICKNESS});
	walls.push_back({0, WINDOW_HEIGHT - WALL_THICKNESS, WINDOW_WIDTH, WALL_THICKNESS});
	walls.push_back({0, 0, WALL_THICKNESS, WINDOW_HEIGHT});
	walls.push_back({WINDOW_WIDTH - WALL_THICKNESS, 0, WALL_THICKNESS, WINDOW_HEIGHT});

	walls.push_back({200, 100, WALL_THICKNESS, 400}); // vertical
	walls.push_back({400, 200, 200, WALL_THICKNESS}); // horizontal
	walls.push_back({600, 50, WALL_THICKNESS, 300});  // vertical

	while(running)
	{
		while(SDL_PollEvent(&event))
		{
			if(event.type == SDL_EVENT_QUIT)
				running = false;
		}

		const bool *state = SDL_GetKeyboardState(nullptr);
		const double moveSpeed = 5.0;

		int moveX = 0;
		int moveY = 0;

		if(state[SDL_SCANCODE_W])
			moveY = -1;
		if(state[SDL_SCANCODE_S])
			moveY = 1;
		if(state[SDL_SCANCODE_A])
			moveX = -1;
		if(state[SDL_SCANCODE_D])
			moveX = 1;

		double newTankX = tankX;
		double newTankY = tankY;

		if(moveX != 0 && moveY != 0)
		{
			const double diagonalFactor = 0.7071;
			newTankX += moveX * moveSpeed * diagonalFactor;
			newTankY += moveY * moveSpeed * diagonalFactor;
		}
		else
		{
			newTankX += moveX * moveSpeed;
			newTankY += moveY * moveSpeed;
		}

		// temporary rectangle for collision detection
		SDL_FRect newTankRect = {static_cast<float>(newTankX), static_cast<float>(newTankY), tankRect.w, tankRect.h};

		// check collisions against walls
		bool collision = false;
		for(auto &wall : walls)
		{
			if(checkCollision(newTankRect, wall))
			{
				collision = true;
				break;
			}
		}

		if(!collision)
		{
			tankX = newTankX;
			tankY = newTankY;
		}

		if(state[SDL_SCANCODE_W])
		{
			if(state[SDL_SCANCODE_A])
				tankAngle = 315.0;
			else if(state[SDL_SCANCODE_D])
				tankAngle = 45.0;
			else
				tankAngle = 0.0;
		}
		else if(state[SDL_SCANCODE_S])
		{
			if(state[SDL_SCANCODE_A])
				tankAngle = 225.0;
			else if(state[SDL_SCANCODE_D])
				tankAngle = 135.0;
			else
				tankAngle = 180.0;
		}
		else if(state[SDL_SCANCODE_A])
		{
			tankAngle = 270.0;
		}
		else if(state[SDL_SCANCODE_D])
		{
			tankAngle = 90.0;
		}

		tankRect.x = (int)tankX;
		tankRect.y = (int)tankY;

		// white background
		SDL_SetRenderDrawColor(sdl.pRenderer, 255, 255, 255, 255);
		SDL_RenderClear(sdl.pRenderer);

		SDL_SetRenderDrawColor(sdl.pRenderer, 0, 0, 0, 255);
		for(auto &wall : walls)
		{
			SDL_RenderFillRect(sdl.pRenderer, &wall);
		}

		SDL_RenderTextureRotated(sdl.pRenderer, textures.tank, nullptr, &tankRect, tankAngle, nullptr, SDL_FLIP_NONE);

		SDL_RenderPresent(sdl.pRenderer);

		SDL_Delay(16);
	}

	SDL_DestroyTexture(textures.tank);
	SDL_DestroyRenderer(sdl.pRenderer);
	SDL_DestroyWindow(sdl.pWindow);
	SDL_Quit();
	return 0;
}
