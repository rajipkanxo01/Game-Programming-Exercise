#include <SDL3/SDL.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <array>
#include <filesystem>


#define NUM_ASTEROIDS 10
#define MAX_BULLETS 10

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
};

struct Entity {
    SDL_FPoint position;
    float size;
    float velocity;
    float spawn_delay;
    bool active = true;

    SDL_FRect rect;
    SDL_Texture *texture_atlas;
    SDL_FRect texture_rect;
};

struct Bullet {
    SDL_FRect rect;
    float speed;
    bool active;
};

struct GameState {
    Entity player;
    Entity asteroids[NUM_ASTEROIDS];

    SDL_Texture *texture_atlas;

    std::array<Bullet, MAX_BULLETS> bullets; // bullet pool
};

static float distance_between_sq(SDL_FPoint a, SDL_FPoint b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return dx * dx + dy * dy;
}


static void init(SDLContext *context, GameState *game_state) {
    constexpr float entity_size_world = 64;
    constexpr float entity_size_texture = 128;

    constexpr float player_speed = entity_size_world * 5;

    // player sprite position in sprite sheet
    constexpr int player_sprite_coords_x = 3;
    constexpr int player_sprite_coords_y = 1;

    // asteroid sprite position in sprite sheet
    constexpr int asteroid_sprite_coords_x = 3;
    constexpr int asteroid_sprite_coords_y = 4;

    // asteroid minimum and maximum range
    constexpr float asteroid_speed_min = entity_size_world * 0.2;
    constexpr float asteroid_speed_range = entity_size_world * 2;

    // load player and asteroid texture
    {
        int w = 0;
        int h = 0;
        int n = 0;
        // stbi_load fills w, h and n with the image's width, height and channel count if loading succeeds
        SDL_Log("Looking for file in: %s", std::filesystem::current_path().string().c_str());
        unsigned char *pixels = stbi_load("kenney/simpleSpace_tilesheet_2.png", &w, &h, &n, 4);

        SDL_assert(pixels);

        // converting the raw bytes of our sprite sheet into a GPU texture → which SDL can then scale, rotate and draw on the screen
        SDL_Surface *surface = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_ABGR8888, pixels, w * n);
        game_state->texture_atlas = SDL_CreateTextureFromSurface(context->renderer, surface);

        SDL_DestroySurface(surface);
        stbi_image_free(pixels);
    }

    // character
    {
        game_state->player.position.x = context->window_width / 2 - entity_size_world / 2;
        game_state->player.position.y = context->window_height - entity_size_world * 2;
        game_state->player.size = entity_size_world;
        game_state->player.velocity = player_speed;
        game_state->player.texture_atlas = game_state->texture_atlas;
        game_state->player.spawn_delay = 0.1f;

        // player size in the game world
        game_state->player.rect.w = game_state->player.size;
        game_state->player.rect.h = game_state->player.size;

        // sprite size (in the tilemap)
        game_state->player.texture_rect.w = entity_size_texture;
        game_state->player.texture_rect.h = entity_size_texture;

        // sprite position (in the tilemap)
        game_state->player.texture_rect.x = entity_size_texture * player_sprite_coords_x;
        game_state->player.texture_rect.y = entity_size_texture * player_sprite_coords_y;
    }

    // asteroids
    {
        for (int i = 0; i < NUM_ASTEROIDS; ++i) {
            Entity *asteroid_curr = &game_state->asteroids[i];

            asteroid_curr->spawn_delay = SDL_randf() * 5.f;
            asteroid_curr->position.x = entity_size_world + SDL_randf() * (
                                            context->window_width - entity_size_world * 2);
            asteroid_curr->position.y = -entity_size_world; // spawn asteroids off screen (almost)
            asteroid_curr->size = entity_size_world;
            asteroid_curr->velocity = asteroid_speed_min + SDL_randf() * asteroid_speed_range;
            asteroid_curr->texture_atlas = game_state->texture_atlas;

            asteroid_curr->rect.w = asteroid_curr->size;
            asteroid_curr->rect.h = asteroid_curr->size;

            asteroid_curr->texture_rect.w = entity_size_texture;
            asteroid_curr->texture_rect.h = entity_size_texture;

            asteroid_curr->texture_rect.x = entity_size_texture * asteroid_sprite_coords_x;
            asteroid_curr->texture_rect.y = entity_size_texture * asteroid_sprite_coords_y;
        }
    }

    // bullets
    {
        for (auto &bullet: game_state->bullets) {
            bullet.active = false;
            bullet.speed = 600.f;
            bullet.rect.w = 8;
            bullet.rect.h = 16;
        }
    }
}

static void update(SDLContext *context, GameState *game_state) {
    // player
    {
        // movement
        Entity *entity_player = &game_state->player;
        if (context->btn_pressed_up)
            entity_player->position.y -= context->delta * entity_player->velocity;
        if (context->btn_pressed_down)
            entity_player->position.y += context->delta * entity_player->velocity;
        if (context->btn_pressed_left)
            entity_player->position.x -= context->delta * entity_player->velocity;
        if (context->btn_pressed_right)
            entity_player->position.x += context->delta * entity_player->velocity;

        // clamp position so player stays inside the window
        if (entity_player->position.x < 0) entity_player->position.x = 0;
        if (entity_player->position.y < 0) entity_player->position.y = 0;
        if (entity_player->position.x + entity_player->size > context->window_width)
            entity_player->position.x = context->window_width - entity_player->size;
        if (entity_player->position.y + entity_player->size > context->window_height)
            entity_player->position.y = context->window_height - entity_player->size;

        // update rect position for rendering
        entity_player->rect.x = entity_player->position.x;
        entity_player->rect.y = entity_player->position.y;

        // draw player
        SDL_SetTextureColorMod(entity_player->texture_atlas, 0xFF, 0xFF, 0xFF);
        SDL_RenderTexture(
            context->renderer,
            entity_player->texture_atlas,
            &entity_player->texture_rect,
            &entity_player->rect
        );
    }

    // asteroids
    {
        // how close an asteroid must be before categorizing it as "too close" (100 pixels. We square it because we can avoid doing the square root later)
        constexpr float warning_distance_sq = 200 * 200;

        // how close an asteroid must be before triggering a collision (64 pixels. We square it because we can avoid doing the square root later)
        // the number 64 is obtained by summing together the "radii" of the sprites
        constexpr float collision_distance_sq = 60 * 60;

        for (int i = 0; i < NUM_ASTEROIDS; ++i) {
            Entity *current_asteroid = &game_state->asteroids[i];

            if (!current_asteroid->active) {
                if (current_asteroid->spawn_delay > 0) {
                    current_asteroid->spawn_delay -= context->delta;
                    continue;
                } else {
                    // respawn asteroid at top
                    current_asteroid->active = true;
                    current_asteroid->position.x = current_asteroid->size + SDL_randf() *
                                                   (context->window_width - current_asteroid->size * 2);
                    current_asteroid->position.y = -current_asteroid->size;
                    current_asteroid->velocity = 50.f + SDL_randf() * 150.f; // random speed again
                }
            }

            if (current_asteroid->spawn_delay > 0) {
                current_asteroid->spawn_delay -= context->delta;
                continue;
            }

            current_asteroid->position.y += context->delta * current_asteroid->velocity;

            current_asteroid->rect.x = current_asteroid->position.x;
            current_asteroid->rect.y = current_asteroid->position.y;

            float distance_sq = distance_between_sq(current_asteroid->position, game_state->player.position);
            if (distance_sq < collision_distance_sq) {
                SDL_SetTextureColorMod(current_asteroid->texture_atlas, 0xFF, 0x00, 0x00);
            } else if (distance_sq < warning_distance_sq) {
                SDL_SetTextureColorMod(current_asteroid->texture_atlas, 0xCC, 0xCC, 0x00);
            } else {
                SDL_SetTextureColorMod(current_asteroid->texture_atlas, 0xFF, 0xFF, 0xFF);
            }

            SDL_FPoint rotation_center = {current_asteroid->rect.w / 2, current_asteroid->rect.h / 2};
            float angle = SDL_GetTicks() * 0.05f;

            SDL_RenderTextureRotated(
                context->renderer,
                current_asteroid->texture_atlas,
                &current_asteroid->texture_rect,
                &current_asteroid->rect,
                SDL_GetTicks() * 0.05,
                &rotation_center,
                SDL_FLIP_NONE
            );
        }
    }

    // bullets
    {
        for (auto &bullet: game_state->bullets) {
            if (bullet.active) {
                bullet.rect.y -= bullet.speed * context->delta;
                if (bullet.rect.y + bullet.rect.h < 0) {
                    bullet.active = false;
                }

                // check collision with asteroids
                for (auto &asteroid: game_state->asteroids) {
                    if (!asteroid.active || asteroid.spawn_delay > 0) continue;

                    if (SDL_HasRectIntersectionFloat(&bullet.rect, &asteroid.rect)) {
                        bullet.active = false;
                        asteroid.active = false;
                        asteroid.spawn_delay = 3.0f + SDL_randf() * 5.0f;
                    }
                }

                // draw bullet
                SDL_SetRenderDrawColor(context->renderer, 255, 255, 0, 255);
                SDL_RenderFillRect(context->renderer, &bullet.rect);
            }
        }
    }
}

static void reset_game(SDLContext *context, GameState *game_state) {
    // reset player position
    game_state->player.position.x = context->window_width / 2 - game_state->player.size / 2;
    game_state->player.position.y = context->window_height - game_state->player.size * 2;
    game_state->player.rect.x = game_state->player.position.x;
    game_state->player.rect.y = game_state->player.position.y;

    // reset bullets
    for (auto &bullet: game_state->bullets) {
        bullet.active = false;
    }

    // reset asteroids
    for (int i = 0; i < NUM_ASTEROIDS; ++i) {
        Entity *asteroid = &game_state->asteroids[i];
        asteroid->active = true;
        asteroid->spawn_delay = SDL_randf() * 5.0f;
        asteroid->position.x = asteroid->size + SDL_randf() * (context->window_width - asteroid->size * 2);
        asteroid->position.y = -asteroid->size;
        asteroid->velocity = 50.f + SDL_randf() * 150.f;
    }
}

int main(int argc, char *argv[]) {
    SDLContext context = {};
    GameState game_state = {};

    bool running = true;
    SDL_Event event;

    constexpr float window_w = 600;
    constexpr float window_h = 800;

    // start of each frame
    SDL_Time frame_start_time;
    SDL_Time frame_end_time;
    SDL_Time work_end_time;

    // Game timing
    SDL_Time delta_time_seconds;
    SDL_Time target_framerate = SECONDS(1) / 60;

    SDL_Window *window = SDL_CreateWindow("E01 - Rendering", window_w, window_h, 0);
    context.renderer = SDL_CreateRenderer(window, nullptr);
    context.window_width = window_w;
    context.window_height = window_h;

    init(&context, &game_state);
    reset_game(&context, &game_state);

    SDL_GetCurrentTime(&frame_start_time);

    while (running) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;

                case SDL_EVENT_KEY_UP:
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.key == SDLK_ESCAPE) {
                        SDL_Log("Escape Pressed - quitting");
                        running = false;
                    }

                    if (event.key.key == SDLK_W)
                        context.btn_pressed_up = event.key.down;
                    if (event.key.key == SDLK_A)
                        context.btn_pressed_left = event.key.down;
                    if (event.key.key == SDLK_S)
                        context.btn_pressed_down = event.key.down;
                    if (event.key.key == SDLK_D)
                        context.btn_pressed_right = event.key.down;

                    if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_SPACE) {
                        for (auto &bullet: game_state.bullets) {
                            if (!bullet.active) {
                                bullet.active = true;
                                bullet.rect.x = game_state.player.position.x +
                                                game_state.player.size / 2 - bullet.rect.w / 2;
                                bullet.rect.y = game_state.player.position.y - bullet.rect.h;
                                break;
                            }
                        }
                    }

                    break;

                default: ;
            }
        }

        // clear screen
        SDL_SetRenderDrawColor(context.renderer, 0x00, 0x00, 0x00, 0xFF);
        SDL_RenderClear(context.renderer);

        update(&context, &game_state);

        // calculate how much time is take for one update
        SDL_GetCurrentTime(&work_end_time);
        SDL_Time work_duration = work_end_time - frame_start_time;

        if (target_framerate > work_duration) {
            SDL_DelayPrecise(target_framerate - work_duration);
        }

        SDL_GetCurrentTime(&frame_end_time);
        SDL_Time frame_duration = frame_end_time - frame_start_time;

        context.delta = NS_TO_SECONDS(frame_duration);

        // render
        SDL_RenderPresent(context.renderer);

        frame_start_time = frame_end_time;
    }

    return 0;
}
