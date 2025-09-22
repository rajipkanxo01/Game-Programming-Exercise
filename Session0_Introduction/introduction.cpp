//
// Created by pdlra on 30/08/2025.
//

#include <cstdlib>
#include <SDL3/SDL.h>

void movePlayer(SDL_FRect &player, const bool *keys, SDL_Scancode up, SDL_Scancode down, SDL_Scancode left,
                SDL_Scancode right, float speed, float dt) {
    if (keys[up]) player.y -= speed * dt;
    if (keys[down]) player.y += speed * dt;
    if (keys[left]) player.x -= speed * dt;
    if (keys[right]) player.x += speed * dt;

    if (player.x < 0) player.x = 0;
    if (player.y < 0) player.y = 0;
    if (player.x + player.w > 800) player.x = 800 - player.w;
    if (player.y + player.h > 600) player.y = 600 - player.h;
}

int main() {
    // define rectangle
    SDL_FRect firstPlayer, secondPlayer;
    //TODO: Would it make sense to have a struct OR anything for Player that contains player properties?
    firstPlayer.x = 300.f;
    firstPlayer.y = 200.f;
    firstPlayer.w = 50.f;
    firstPlayer.h = 50.f;

    secondPlayer.x = 500.f;
    secondPlayer.y = 200.f;
    secondPlayer.w = 50.f;
    secondPlayer.h = 50.f;

    // time at start
    float lastNS = SDL_GetTicksNS();

    //TODO: since window settings are not changing, would it make sense to define it.WINDOW_WIDTH, WINDOW_HEIGHT as const int at the top of the file?
    // window + renderer
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;
    SDL_Window *window = SDL_CreateWindow("Exercise 1: Introduction", WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);

    // main game loop
    bool running = true;
    while (running) {
        SDL_Event event;
        // checks SDL event queue.
        // If there is event waiting, it copies into event and returns true.
        // SDL_PollEvent expects pointer.
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;

                case SDL_EVENT_KEY_DOWN:
                    // check if key pressed is ESC to quit
                    if (event.key.key == SDLK_ESCAPE) {
                        SDL_Log("Escape Pressed - quitting");
                        running = false;
                    }
                    break;

                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        SDL_Log("Left mouse button pressed at %f,%f", event.button.x, event.button.y);
                    }

                    if (event.button.button == SDL_BUTTON_RIGHT) {
                        SDL_Log("Left mouse button pressed at %f,%f", event.button.x, event.button.y);
                    }
                    break;

                default: ;
            }
        }

        // calculate delta time
        float nowNS = SDL_GetTicksNS();
        //TODO: It feels a bit weird to divide, and also speed is defined here and just sent to the function
        float dt = (nowNS - lastNS) / 1'000'000'000.0f; // convert to seconds
        lastNS = nowNS;

        // SDL has keyboard state array that tells us every frame, "is this key currently down?"
        const bool *keys = SDL_GetKeyboardState(nullptr);
        const float speed = 320.f;

        // move first player with WASD
        movePlayer(firstPlayer, keys,
                   SDL_SCANCODE_W, SDL_SCANCODE_S,
                   SDL_SCANCODE_A, SDL_SCANCODE_D,
                   speed, dt);

        // move second player with arrow keys
        movePlayer(secondPlayer, keys,
                   SDL_SCANCODE_UP, SDL_SCANCODE_DOWN,
                   SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT,
                   speed, dt);


        // clear screen with dark background
        // sets drawing color for renderer to dark shade
        SDL_SetRenderDrawColor(renderer, 18, 18, 23, 255);
        // clears current rendering target with drawing color
        SDL_RenderClear(renderer);

        // Draw first player in red
        SDL_SetRenderDrawColor(renderer, 220, 60, 60, 255);
        // whenever we call drawing function, SDL uses last defined color
        SDL_RenderFillRect(renderer, &firstPlayer);

        // Draw second player in blue
        SDL_SetRenderDrawColor(renderer, 60, 60, 220, 255);
        SDL_RenderFillRect(renderer, &secondPlayer);

        // Draw outline in yellow on top
        SDL_SetRenderDrawColor(renderer, 240, 200, 90, 255);
        SDL_RenderRect(renderer, &firstPlayer);

        // Present to screen
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}
