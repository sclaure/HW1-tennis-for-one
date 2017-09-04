#include "Draw.hpp"
#include "GL.hpp"

#include <SDL.h>
#include <glm/glm.hpp>

#include <chrono>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv) {
	//Configuration:
	struct {
		std::string title = "Game0: Tennis For One";
		glm::uvec2 size = glm::uvec2(640, 480);
	} config;

	//------------  initialization ------------

	//Initialize SDL library:
	SDL_Init(SDL_INIT_VIDEO);

	//Ask for an OpenGL context version 3.3, core profile, enable debug:
	SDL_GL_ResetAttributes();
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	//create window:
	SDL_Window *window = SDL_CreateWindow(
		config.title.c_str(),
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		config.size.x, config.size.y,
		SDL_WINDOW_OPENGL /*| SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI*/
	);

	if (!window) {
		std::cerr << "Error creating SDL window: " << SDL_GetError() << std::endl;
		return 1;
	}

	//Create OpenGL context:
	SDL_GLContext context = SDL_GL_CreateContext(window);

	if (!context) {
		SDL_DestroyWindow(window);
		std::cerr << "Error creating OpenGL context: " << SDL_GetError() << std::endl;
		return 1;
	}

	#ifdef _WIN32
	//On windows, load OpenGL extensions:
	if (!init_gl_shims()) {
		std::cerr << "ERROR: failed to initialize shims." << std::endl;
		return 1;
	}
	#endif

	//Set VSYNC + Late Swap (prevents crazy FPS):
	if (SDL_GL_SetSwapInterval(-1) != 0) {
		std::cerr << "NOTE: couldn't set vsync + late swap tearing (" << SDL_GetError() << ")." << std::endl;
		if (SDL_GL_SetSwapInterval(1) != 0) {
			std::cerr << "NOTE: couldn't set vsync (" << SDL_GetError() << ")." << std::endl;
		}
	}

	//Hide mouse cursor (note: showing can be useful for debugging):
	SDL_ShowCursor(SDL_DISABLE);

	//------------  game state ------------

	//initial game locations
	glm::vec2 center = glm::vec2(0.0f, 0.0f);
	glm::vec2 mouse = glm::vec2(0.0f, 0.0f);
	glm::vec2 ball = glm::vec2(0.0f, 0.0f);
	glm::vec2 ball_velocity = glm::vec2(0.5f, 0.5f);
	glm::vec2 ball_original_vel = glm::vec2(0.5f, 0.5f);

	//define user controlled rectangle position
	glm::vec2 player = glm::vec2(0.9f, 0.0f);

	//add time based seed to generate different random numbers each time
	srand((unsigned int)time(NULL));

	//define target rectangle
	float original_target_size = 1.0f;
	float target_size = 1.0f;
	float target_y_spawn = 2.0f * ((rand() / ((float) RAND_MAX)) - 0.5f);

	// if the target spawns out of bounds, place it at an edge
	if ((target_y_spawn - (target_size/2.0f)) < -1.0f) {
		target_y_spawn = -1.0f + (target_size/2.0f);
	}
	else if ((target_y_spawn + (target_size/2.0f)) > 1.0f) {
		target_y_spawn = 1.0f - (target_size/2.0f);
	}

	glm::vec2 target = glm::vec2(-0.9f, target_y_spawn);

	//initialize score and fail count
	int score = 0;
	int fail = 0;

	//boolean to track if the ball is ready to launch
	bool launch = true;

	//------------  game loop ------------

	auto previous_time = std::chrono::high_resolution_clock::now();
	bool should_quit = false;
	while (true) {
		static SDL_Event evt;
		while (SDL_PollEvent(&evt) == 1) {
			//handle input:
			if (evt.type == SDL_MOUSEMOTION) {
				mouse.x = (evt.motion.x + 0.5f) / float(config.size.x) * 2.0f - 1.0f;
				mouse.y = (evt.motion.y + 0.5f) / float(config.size.y) *-2.0f + 1.0f;
			} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
				if (launch){
					float launch_y_vel = 4.0f * ((rand() / ((float) RAND_MAX)) - 0.5f);
					ball_velocity = glm::vec2(0.5f, launch_y_vel);
					launch = false;
				}
			} else if (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_ESCAPE) {
				should_quit = true;
			} else if (evt.type == SDL_QUIT) {
				should_quit = true;
				break;
			}
		}
		if (should_quit) break;

		auto current_time = std::chrono::high_resolution_clock::now();
		float elapsed = std::chrono::duration< float >(current_time - previous_time).count();
		previous_time = current_time;

		{ //update game state:
			//change ball direction during collision with walls
			ball += elapsed * ball_velocity;
			if (ball.x < -1.0f) ball_velocity.x = std::abs(ball_velocity.x);
			//if the ball hits the right edge, reset the ball, increment the fail counter
			if (ball.x >  1.0f){
				fail++;
				ball_velocity.x = 0.0f;
				ball_velocity.y = 0.0f;
				ball = center;
				launch = true;

				//present "game over" and "restart"
				//since the game can only be presented in rectanlges, a score can't easily be displayed,
				//therefore the score gets sent to the console output and the game "restarts"
				if (fail == 3){
					printf("Game Over! User score is: %i\n", score);
					score = 0;
					fail = 0;

					//reinnstantiate the target
					target_size = original_target_size;

					target_y_spawn = 2.0f * ((rand() / ((float) RAND_MAX)) - 0.5f);

					// if the target spawns out of bounds, place it at an edge
					if ((target_y_spawn - (target_size/2.0f)) < -1.0f) {
						target_y_spawn = -1.0f + (target_size/2.0f);
					}
					else if ((target_y_spawn + (target_size/2.0f)) > 1.0f) {
						target_y_spawn = 1.0f - (target_size/2.0f);
					}

					target.y = target_y_spawn;
				}
			}
			if (ball.y < -1.0f) ball_velocity.y = std::abs(ball_velocity.y);
			if (ball.y >  1.0f) ball_velocity.y =-std::abs(ball_velocity.y);

			//add player input into the player rectangle
			if (mouse.y > 0.6f){
				player.y = 0.6f;
			} else if (mouse.y < -0.6f){
				player.y = -0.6f;
			} else {
				player.y = mouse.y;
			}

			//change ball direction during collision with player rectangle
			if ((ball.x + 0.05f) >= (player.x - 0.05f)){
				if ((ball.y >= (player.y - 0.4f)) && (ball.y < (player.y - 0.2f))) {
					ball_velocity.x = -std::abs(ball_original_vel.x);
					ball_velocity.y = -std::abs(2.0f * ball_original_vel.y);
				}
				else if ((ball.y >= (player.y - 0.2f)) && (ball.y < (player.y - 0.0f))) {
					ball_velocity.x = -std::abs(ball_original_vel.x);
					ball_velocity.y = -std::abs(ball_original_vel.y);
				}
				else if (ball.y == player.y) {
					ball_velocity.x = -std::abs(ball_original_vel.x);
					ball_velocity.y =  std::abs(0.0f);
				}
				else if ((ball.y > player.y) && (ball.y < (player.y + 0.2f))) {
					ball_velocity.x = -std::abs(ball_original_vel.x);
					ball_velocity.y =  std::abs(ball_original_vel.y);
				}
				else if ((ball.y >= (player.y + 0.2f)) && (ball.y < (player.y + 0.4f))) {
					ball_velocity.x = -std::abs(ball_original_vel.x);
					ball_velocity.y =  std::abs(2.0f * ball_original_vel.y);
				}
			}

			//collision detection between the target and ball
			if ((ball.x - 0.05f) <= (target.x + 0.05f)){
				if ((ball.y >= (target.y - (target_size/2.0f))) && (ball.y < (target.y + (target_size/2.0f)))) {
					score++;
					ball_velocity.x = 0.0f;
					ball_velocity.y = 0.0f;
					ball = center;
					launch = true;

					//reinnstantiate a smaller target
					target_size = target_size/1.25f;

					target_y_spawn = 2.0f * ((rand() / ((float) RAND_MAX)) - 0.5f);

					// if the target spawns out of bounds, place it at an edge
					if ((target_y_spawn - (target_size/2.0f)) < -1.0f) {
						target_y_spawn = -1.0f + (target_size/2.0f);
					}
					else if ((target_y_spawn + (target_size/2.0f)) > 1.0f) {
						target_y_spawn = 1.0f - (target_size/2.0f);
					}

					target.y = target_y_spawn;
				}
			}
		}

		//draw output:
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT);


		{ //draw game state:
			Draw draw;
			//draw.add_rectangle(mouse + glm::vec2(-0.1f,-0.1f), mouse + glm::vec2(0.1f, 0.1f), glm::u8vec4(0xff, 0xff, 0x00, 0xff));
			draw.add_rectangle(ball + glm::vec2(-0.05f,-0.05f), ball + glm::vec2(0.05f, 0.05f), glm::u8vec4(0xff, 0xff, 0xff, 0xff));
			draw.add_rectangle(player + glm::vec2(-0.05f,-0.4f), player + glm::vec2(0.05f, 0.4f), glm::u8vec4(0xff, 0xff, 0xff, 0xff));
			draw.add_rectangle(target + glm::vec2(-0.05f,-(target_size/2.0f)), target + glm::vec2(0.05f, (target_size/2.0f)), glm::u8vec4(0x00, 0x00, 0xff, 0xff));
			draw.draw();
		}


		SDL_GL_SwapWindow(window);
	}


	//------------  teardown ------------

	SDL_GL_DeleteContext(context);
	context = 0;

	SDL_DestroyWindow(window);
	window = NULL;

	return 0;
}
