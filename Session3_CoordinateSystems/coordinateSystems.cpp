#define STB_IMAGE_IMPLEMENTATION
#define ITU_UNITY_BUILD

#define TEXTURE_PIXELS_PER_UNIT 16
#define CAMERA_PIXELS_PER_UNIT  16*4

#include <SDL3/SDL.h>
#include <itu_lib_engine.hpp>
#include <itu_lib_render.hpp>
#include <itu_lib_sprite.hpp>

#define ENABLE_DIAGNOSTICS

#define TARGET_FRAMERATE SECONDS(1) / 60
#define WINDOW_W         1280
#define WINDOW_H         720

#define ENTITY_COUNT 4096

bool DEBUG_render_textures = true;
bool DEBUG_render_outlines = false;

enum EntityType {
    ENTITY_PLAYER,
};

struct Entity {
    EntityType type;
    Sprite sprite;
    Transform transform;
};

struct GameState {
    Entity *player;
    Entity *entities;
    int entities_alive_count;

    SDL_Texture *atlas;
    SDL_Texture *bg;
};

// Creates a new entity in the game state.
// Returns a pointer to the new entity or nullptr if the entity limit is reached.
static Entity *entity_create(GameState *state) {
    if (state->entities_alive_count >= ENTITY_COUNT) {
        // Maximum number of entities reached
        return nullptr;
    }

    // We have provided 4096 entity slots, so this will get first empty slot and return it which will be used to store entity data
    // Get pointer to the next available entity slot
    Entity *new_entity_pointer = &state->entities[state->entities_alive_count];
    ++state->entities_alive_count; // Increment the count of alive entities
    return new_entity_pointer;
}

// Removes an entity from the game state by swapping it with the last active entity.
// This keeps the entity array packed and decrements the alive count.
static void entity_destroy(GameState *state, Entity *entity) {
    SDL_assert(entity >= state->entities && entity < state->entities + ENTITY_COUNT);

    --state->entities_alive_count;
    *entity = state->entities[state->entities_alive_count];
}

static void game_init(SDLContext *context, GameState *state) {
    // allocate memory
    state->entities = (Entity *) SDL_calloc(ENTITY_COUNT, sizeof(Entity));
    SDL_assert(state->entities);

    // texture atlases
    state->atlas = texture_create(context, "../data/kenney/tiny_dungeon_packed.png", SDL_SCALEMODE_NEAREST);
    if (!state->atlas) {
        SDL_Log("Failed to load atlas: %s", SDL_GetError());
    }

    state->bg = texture_create(context, "../data/kenney/prototype_texture_dark/texture_03.png", SDL_SCALEMODE_LINEAR);
    if (!state->bg) {
        SDL_Log("Failed to load bg: %s", SDL_GetError());
    }
}

// Resets the game state by clearing all entities and reinitializing the background and player.
// This function sets up the initial entities and their properties for a new game session.
static void game_reset(SDLContext *context, GameState *state) {
    state->entities_alive_count = 0;

    // Create entity
    {
        Entity *bg = entity_create(state);
        bg->transform.scale.x = context->window_w / context->camera_active->pixels_per_unit;
        bg->transform.scale.y = context->window_h / context->camera_active->pixels_per_unit;

        constexpr SDL_FRect sprite_rect = SDL_FRect{0, 0, 1024, 1024};
        itu_lib_sprite_init(
            &bg->sprite,
            state->bg,
            sprite_rect
        );
    }

    // Create player entity
    {
        state->player = entity_create(state);
        state->player->type = ENTITY_PLAYER;
        state->player->transform.position = VEC2F_ZERO;
        state->player->transform.scale = VEC2F_ONE;
        itu_lib_sprite_init(
            &state->player->sprite,
            state->atlas,
            itu_lib_sprite_get_rect(0, 9, 16, 16)
        );

        // Raise sprite pivot so the position coincides with the center of the image
        state->player->sprite.pivot.y = 0.3f;
    }
}

// Updates the game state each frame.
// Handles player movement based on input and updates the camera to follow the player.
static void game_update(SDLContext *context, GameState *state) { {
        const float player_speed = 3;

        Entity *entity = state->player;
        vec2f mov = {0};
        if (context->btn_isdown_up)
            mov.y += 1;
        if (context->btn_isdown_down)
            mov.y -= 1;
        if (context->btn_isdown_left)
            mov.x -= 1;
        if (context->btn_isdown_right)
            mov.x += 1;

        entity->transform.position = entity->transform.position + mov * (player_speed * context->delta);
    }

    // camera follows player
    const float zoom_speed = 1;

    context->camera_active->world_position = state->player->transform.position;
    context->camera_active->zoom += context->mouse_scroll * zoom_speed * context->delta;
}

static void game_render(SDLContext *context, GameState *state) {
    // get camera values
    vec2f cam_pos = context->camera_active->world_position;
    float cam_zoom = context->camera_active->zoom;
    float ppu = context->camera_active->pixels_per_unit;
    vec2f screen_size = {context->window_w, context->window_h};

    for (int i = 0; i < state->entities_alive_count; ++i) {
        Entity *entity = &state->entities[i];

        // get entity data
        vec2f pos = entity->transform.position;
        vec2f scale = entity->transform.scale;

        // move entity into camera relative space
        pos = pos - cam_pos;

        // convert entity size (world units) into pixels
        float w = ppu * scale.x * cam_zoom;
        float h = ppu * scale.y * cam_zoom;

        // center world at middle of screen, apply scaling, subtract pivot
        float x = screen_size.x * 0.5f + pos.x * ppu * cam_zoom - w * entity->sprite.pivot.x;

        // flip Y axis (because SDL coordinates start top-left)
        float y = screen_size.y * 0.5f - pos.y * ppu * cam_zoom - h * entity->sprite.pivot.y;

        SDL_FRect rect_dst = {x, y, w, h};

        SDL_RenderTexture(context->renderer, entity->sprite.texture, &entity->sprite.rect, &rect_dst);

        if (DEBUG_render_outlines) {
            itu_lib_sprite_render_debug(context, &entity->sprite, &entity->transform);
        }
    }

    // draw magenta border
    SDL_SetRenderDrawColor(context->renderer, 0xFF, 0x00, 0xFF, 0xFF);
    SDL_RenderRect(context->renderer, nullptr);
}

int main(int argc, char *argv[]) {
    bool quit = false;

    SDL_Window *window;
    SDLContext context = {0};
    GameState state = {0};

    context.window_w = WINDOW_W;
    context.window_h = WINDOW_H;

    SDL_CreateWindowAndRenderer("E03 - Coordinate Systems", WINDOW_W, WINDOW_H, 0, &window, &context.renderer);
    SDL_SetRenderDrawBlendMode(context.renderer, SDL_BLENDMODE_BLEND);

    context.camera_default.normalized_screen_size.x = context.window_w;
    context.camera_default.normalized_screen_size.y = context.window_h;
    context.camera_default.normalized_screen_offset.x = 0.0f;
    context.camera_default.normalized_screen_offset.y = 0.0f;
    context.camera_default.zoom = 1.0f;
    context.camera_default.pixels_per_unit = CAMERA_PIXELS_PER_UNIT;

    context.camera_active = &context.camera_default;

    game_init(&context, &state);
    game_reset(&context, &state);

    SDL_Time walltime_frame_beg;
    SDL_Time walltime_frame_end;
    SDL_Time walltime_work_end;
    SDL_Time elapsed_work;
    SDL_Time elapsed_frame;

    SDL_GetCurrentTime(&walltime_frame_beg);
    walltime_frame_end = walltime_frame_beg;

    while (!quit) {
        // input
        SDL_Event event;
        sdl_input_clear(&context);

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    quit = true;
                    break;

                case SDL_EVENT_KEY_DOWN:
                case SDL_EVENT_KEY_UP:
                    switch (event.key.key) {
                        case SDLK_W: sdl_input_key_process(&context, BTN_TYPE_UP, &event);
                            break;
                        case SDLK_A: sdl_input_key_process(&context, BTN_TYPE_LEFT, &event);
                            break;
                        case SDLK_S: sdl_input_key_process(&context, BTN_TYPE_DOWN, &event);
                            break;
                        case SDLK_D: sdl_input_key_process(&context, BTN_TYPE_RIGHT, &event);
                            break;
                        case SDLK_Q: sdl_input_key_process(&context, BTN_TYPE_ACTION_0, &event);
                            break;
                        case SDLK_E: sdl_input_key_process(&context, BTN_TYPE_ACTION_1, &event);
                            break;
                        case SDLK_SPACE: sdl_input_key_process(&context, BTN_TYPE_SPACE, &event);
                            break;
                        default: ;
                    }

                default: ;
            }
        }

        SDL_SetRenderDrawColor(context.renderer, 0x00, 0x00, 0x00, 0x00);
        SDL_RenderClear(context.renderer);

        // update
        game_update(&context, &state);
        game_render(&context, &state);

        SDL_GetCurrentTime(&walltime_work_end);
        elapsed_work = walltime_work_end - walltime_frame_beg;

        if (elapsed_work < TARGET_FRAMERATE)
            SDL_DelayNS(TARGET_FRAMERATE - elapsed_work);
        SDL_GetCurrentTime(&walltime_frame_end);
        elapsed_frame = walltime_frame_end - walltime_frame_beg;

        // render
        SDL_RenderPresent(context.renderer);

        context.delta = (float) elapsed_frame / (float) SECONDS(1);
        context.uptime += context.delta;
        walltime_frame_beg = walltime_frame_end;
    }
}
