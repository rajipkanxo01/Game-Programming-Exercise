#include <SDL3/SDL.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define ENABLE_DIAGNOSTICS
#define NUM_ASTEROIDS 10


#define VALIDATE(expression) if(!(expression)) { SDL_Log("%s\n", SDL_GetError()); }

#define NANOS(x)   (x)                // converts nanoseconds into nanoseconds
#define MICROS(x)  (NANOS(x) * 1000)  // converts microseconds into nanoseconds
#define MILLIS(x)  (MICROS(x) * 1000) // converts milliseconds into nanoseconds
#define SECONDS(x) (MILLIS(x) * 1000) // converts seconds into nanoseconds

#define NS_TO_MILLIS(x)  ((float)(x)/(float)1000000)    // converts nanoseconds to milliseconds (in floating point precision)



#define NS_TO_SECONDS(x) ((float)(x)/(float)1000000000) // converts nanoseconds to seconds (in floating point precision)


struct SDLContext {
    SDL_Renderer *renderer{};

    float window_width{}; // window width
    float window_height{}; // window height

    float delta{};

    bool btn_pressed_up = false;
    bool btn_pressed_down = false;
    bool btn_pressed_left = false;
    bool btn_pressed_right = false;
} context;


int main(int argc, char *argv[]) {
    SDL_Window *window = SDL_CreateWindow("Exercise 1: Introduction", 800, 600, 0);
    context.window_height = 600;
    context.window_width = 800;

    return 0;
}
