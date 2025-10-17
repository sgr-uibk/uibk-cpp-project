#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include <vector>

bool checkCollision(const SDL_Rect &a, const SDL_Rect &b)
{
	return (a.x < b.x + b.w &&
	        a.x + a.w > b.x &&
	        a.y < b.y + b.h &&
	        a.y + a.h > b.y);
}

int main(int argc, char *argv[])
{
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
	{
		std::cerr << "SDL Init Error: " << SDL_GetError() << "\n";
		return 1;
	}

	if(!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
	{
		std::cerr << "SDL_image Init Error: " << IMG_GetError() << "\n";
		SDL_Quit();
		return 1;
	}

	const int WINDOW_WIDTH = 800;
	const int WINDOW_HEIGHT = 600;
	const int BORDER_THICKNESS = 20; // wall thickness

	SDL_Window *window = SDL_CreateWindow("TankGame",
	                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	                                      WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
	if(!window)
	{
		std::cerr << "SDL CreateWindow Error: " << SDL_GetError() << "\n";
		IMG_Quit();
		SDL_Quit();
		return 1;
	}

	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if(!renderer)
	{
		std::cerr << "SDL CreateRenderer Error: " << SDL_GetError() << "\n";
		SDL_DestroyWindow(window);
		IMG_Quit();
		SDL_Quit();
		return 1;
	}

	SDL_Texture *tankTexture = IMG_LoadTexture(renderer, "../assets/tank.png");
	if(!tankTexture)
	{
		std::cerr << "IMG_LoadTexture Error: " << IMG_GetError() << "\n";
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		IMG_Quit();
		SDL_Quit();
		return 1;
	}

	double tankX = 368.0;
	double tankY = 268.0;
	SDL_Rect tankRect{(int)tankX, (int)tankY, 64, 64};
	double tankAngle = 0.0;

	bool running = true;
	SDL_Event event;

	std::vector<SDL_Rect> walls;

	// Outer and inner walls
	walls.push_back({0, 0, WINDOW_WIDTH, BORDER_THICKNESS});
	walls.push_back({0, WINDOW_HEIGHT - BORDER_THICKNESS, WINDOW_WIDTH, BORDER_THICKNESS});
	walls.push_back({0, 0, BORDER_THICKNESS, WINDOW_HEIGHT});
	walls.push_back({WINDOW_WIDTH - BORDER_THICKNESS, 0, BORDER_THICKNESS, WINDOW_HEIGHT});


	walls.push_back({200, 100, BORDER_THICKNESS, 400}); // vertical
	walls.push_back({400, 200, 200, BORDER_THICKNESS}); // horizontal
	walls.push_back({600, 50, BORDER_THICKNESS, 300}); // vertical

	while(running)
	{
		while(SDL_PollEvent(&event))
		{
			if(event.type == SDL_QUIT)
				running = false;
		}

		const Uint8 *state = SDL_GetKeyboardState(nullptr);
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
		SDL_Rect newTankRect = {(int)newTankX, (int)newTankY, tankRect.w, tankRect.h};

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
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_RenderClear(renderer);

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		for(auto &wall : walls)
		{
			SDL_RenderFillRect(renderer, &wall);
		}

		SDL_RenderCopyEx(renderer, tankTexture, nullptr, &tankRect, tankAngle, nullptr, SDL_FLIP_NONE);

		SDL_RenderPresent(renderer);

		SDL_Delay(16);
	}

	SDL_DestroyTexture(tankTexture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	IMG_Quit();
	SDL_Quit();
	return 0;
}
