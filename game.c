#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"
#include "stb_image.h"
#include "AL/al.h"
#include "AL/alc.h"
#include "game.h"
#include "sound.h"


int32_t             g_window_width = 800;
int32_t             g_window_height = 600;
SDL_Window *        g_window = NULL;
SDL_Renderer *      g_renderer = NULL;

// ALCcontext *        g_audio_context;
// ALCdevice *         g_audio_device;
// thrd_t              g_audio_thread;

// Mix_Chunk *         g_ball_hit_sound = NULL;

uint32_t            g_brick_count;
struct brick_t *    g_bricks;

struct pad_t        g_pad;
SDL_Texture *       g_pad_texture;
float               g_pad_glow = 0.0f;

struct ball_t       g_ball;
SDL_Texture *       g_ball_texture;
SDL_Texture *       g_balls_left_texture;
uint32_t            g_balls_left = 3;
struct sound_t      g_ball_hit_sound;
struct sound_t      g_ball_boing_sound;
uint32_t            g_combo = 0;
uint32_t            g_best_combo = 0;
float               g_combo_zoom = 0.0f;
uint32_t            g_combo_timer = 0;
uint32_t            g_combo_show_timer = 0;
SDL_Texture *       g_combo_texture;
SDL_Texture *       g_score_texture;
uint32_t            g_score;
struct sound_t      g_applause_sounds[3];
struct sound_t      g_ahh_sounds[2];
struct sound_t      g_boo_sound;

uint32_t            g_ball_launched = 0;
uint32_t            g_game_state = GAME_STATE_INIT;

// float               g_fade_in_oppacity = 0.0f;
// float               g_fade_out_oppacity = 1.0f;
float               g_fade_opacity = 0.0f;
uint32_t            g_fade_state = FADE_STATE_NONE;
// uint32_t            g_fade_next_state;

uint8_t             g_keyboard_state[SDL_NUM_SCANCODES];
uint8_t             g_mouse_state[3];
TTF_Font *          g_font;


SDL_Texture *       g_main_menu_title_texture;
SDL_Texture *       g_main_menu_press_enter_texture;
uint32_t            g_main_menu_delay_timer = 0;
uint32_t            g_main_menu_press_enter_blink_timer = 0;
uint32_t            g_main_menu_press_enter_blink_state = 0;
uint32_t            g_main_menu_fading = 0;
struct sound_t      g_main_menu_start_sound;

SDL_Texture *       g_lost_ball_texture[2];
SDL_Texture *       g_game_over_texture;
uint32_t            g_lost_ball_timer = 0;
uint32_t            g_game_over_timer = 0;

uint32_t            g_game_won_timer;
SDL_Texture *       g_game_won_texture;
// struct sound_t      g_game_won_sound;



struct sound_t      g_bg_intro;
struct sound_t      g_bg_loop;
struct source_t *   g_bg_source;
uint32_t            g_bg_looping;



void  (*g_game_states[])(float delta_time) = {
    [GAME_STATE_INIT]           = init_state,
    [GAME_STATE_MAIN_MENU]      = main_menu_state,
    [GAME_STATE_LOST_BALL]      = lost_ball_state,
    [GAME_STATE_GAME_OVER]      = game_over_state,
    [GAME_STATE_GAME_WON]       = game_won_state,
    [GAME_STATE_START_GAME]     = start_game_state,
    // [GAME_STATE_FADE_IN]        = fade_in_state,
    // [GAME_STATE_FADE_OUT]       = fade_out_state,
    [GAME_STATE_PLAYING]        = playing_state,
    [GAME_STATE_QUIT]           = quit_state,
};

// int audio_thread_func(void *arg)
// {
//     while(g_game_state != GAME_STATE_QUIT)
//     {
//         SDL_Delay(10);
//     }
// }

void update_input()
{
    SDL_Event event;

    for(uint32_t index = 0; index < SDL_NUM_SCANCODES; index++)
    {
        g_keyboard_state[index] &= ~(INPUT_STATE_JUST_PRESSED | INPUT_STATE_JUST_RELEASED);
    }

    for(uint32_t index = 0; index < 3; index++)
    {
        g_mouse_state[index] &= ~(INPUT_STATE_JUST_PRESSED | INPUT_STATE_JUST_RELEASED);
    }

    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_KEYDOWN:
                if(!(g_keyboard_state[event.key.keysym.scancode] & INPUT_STATE_PRESSED))
                {
                    g_keyboard_state[event.key.keysym.scancode] |= INPUT_STATE_JUST_PRESSED;
                }

                g_keyboard_state[event.key.keysym.scancode] |= INPUT_STATE_PRESSED;
            break;

            case SDL_KEYUP:
                if(g_keyboard_state[event.key.keysym.scancode] & INPUT_STATE_PRESSED)
                {
                    g_keyboard_state[event.key.keysym.scancode] |= INPUT_STATE_JUST_RELEASED;
                }

                g_keyboard_state[event.key.keysym.scancode] &= ~INPUT_STATE_PRESSED;
            break;

            case SDL_MOUSEBUTTONDOWN:
                if(!(g_mouse_state[event.button.button] & INPUT_STATE_PRESSED))
                {
                    g_mouse_state[event.button.button] |= INPUT_STATE_JUST_PRESSED;
                }

                g_mouse_state[event.button.button] |= INPUT_STATE_PRESSED;
            break;

            case SDL_MOUSEBUTTONUP:
                if(g_mouse_state[event.button.button] & INPUT_STATE_PRESSED)
                {
                    g_mouse_state[event.button.button] |= INPUT_STATE_JUST_RELEASED;
                }

                g_mouse_state[event.button.button] &= ~INPUT_STATE_PRESSED;
            break;

            case SDL_WINDOWEVENT:
                if(event.window.type == SDL_WINDOWEVENT_CLOSE)
                {
                    g_game_state = GAME_STATE_QUIT;
                }
            break;

            case SDL_QUIT:
                g_game_state = GAME_STATE_QUIT;
            break;
        }
    }
}

void init_ball()
{
    g_ball.pos[0] = g_pad.pos[0];
    g_ball.pos[1] = g_pad.pos[1] + PAD_HEIGHT / 2 + BALL_RADIUS;
    g_ball.vel[0] = 0;
    g_ball.vel[1] = 0;
    g_ball_launched = 0;

    g_combo = 0;
    g_combo_timer = 0;
    g_combo_show_timer = 0;
}

void init_play_field()
{
    int32_t column_count = (g_window_width - 80) / (BRICK_WIDTH + 2);
    int32_t row_count = 10;
    for(int32_t brick_row = 0; brick_row < row_count; brick_row++)
    {
        for(int32_t brick_column = 0; brick_column < column_count; brick_column++)
        {
            struct brick_t *brick = g_bricks + brick_row * column_count + brick_column;
            brick->pos[0] = (brick_column - column_count / 2) * (BRICK_WIDTH + 2);
            brick->pos[1] = (g_window_height / 2 - BRICK_HEIGHT) - brick_row * (BRICK_HEIGHT + 2);

            brick->color[0] = rand() % 0x100;
            brick->color[1] = rand() % 0x100;
            brick->color[2] = rand() % 0x100;
        }
    }

    g_brick_count = column_count * row_count;

    g_pad.pos[0] = 0;
    g_pad.pos[1] = -280.0f;

    g_balls_left = 3;

    init_ball();
}

void draw_play_field()
{
    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_NONE);

    for(uint32_t brick_index = 0; brick_index < g_brick_count; brick_index++)
    {
        struct brick_t *brick = g_bricks + brick_index;

        SDL_Rect rect = {
            .x = (int32_t)(brick->pos[0] - BRICK_WIDTH / 2) + (g_window_width / 2),
            .y = -(int32_t)(brick->pos[1] + BRICK_HEIGHT / 2) + (g_window_height / 2),
            .w = BRICK_WIDTH,
            .h = BRICK_HEIGHT,
        };
        SDL_SetRenderDrawColor(g_renderer, brick->color[0], brick->color[1], brick->color[2], 0xff);
        SDL_RenderFillRect(g_renderer, &rect);
    } 

    SDL_Rect rect = {
        .x = (int32_t)(g_ball.pos[0] - BALL_RADIUS) + (g_window_width / 2),
        .y = -(int32_t)(g_ball.pos[1] + BALL_RADIUS) + (g_window_height / 2),
        .w = BALL_RADIUS * 2, 
        .h = BALL_RADIUS * 2
    };

    // SDL_SetRenderDrawColor(g_renderer, 0xff, 0x00, 0x00, 0xff);
    // SDL_RenderFillRect(g_renderer, &rect);
    SDL_SetTextureColorMod(g_ball_texture, 0xff, 0xff, 0xff);
    SDL_SetTextureBlendMode(g_ball_texture, SDL_BLENDMODE_BLEND);
    SDL_RenderCopy(g_renderer, g_ball_texture, NULL, &rect);





    rect = (struct SDL_Rect){
        .x = (int32_t)(g_pad.pos[0] - PAD_WIDTH / 2) + (g_window_width / 2),
        .y = -(int32_t)(g_pad.pos[1] + PAD_HEIGHT / 2) + (g_window_height / 2),
        .w = PAD_WIDTH, 
        .h = PAD_HEIGHT
    };    
    SDL_SetTextureColorMod(g_pad_texture, 0xff, 0xff, 0xff);
    SDL_SetTextureBlendMode(g_pad_texture, SDL_BLENDMODE_NONE);
    SDL_RenderCopy(g_renderer, g_pad_texture, NULL, &rect);

    SDL_SetTextureBlendMode(g_pad_texture, SDL_BLENDMODE_MOD);
    rect.x += 10;
    rect.y += 5;
    rect.w -= 20;
    rect.h -= 5;

    float glow = 1.0f - fabsf(sinf((g_pad_glow + 0.4f) * 3.14159265f)) * 0.65f;
    SDL_SetTextureColorMod(g_pad_texture, 0xff * glow, 0, 0);
    SDL_RenderCopy(g_renderer, g_pad_texture, NULL, &rect);

    rect.x += 7;
    rect.y += 2;
    rect.w -= 14;
    rect.h -= 2;

    glow = 1.0f - fabsf(sinf((g_pad_glow + 0.3f) * 3.14159265f)) * 0.65f;
    SDL_SetTextureColorMod(g_pad_texture, 0xff * glow, 0, 0);
    SDL_RenderCopy(g_renderer, g_pad_texture, NULL, &rect);

    rect.x += 5;
    rect.y += 2;
    rect.w -= 10;
    rect.h -= 2;

    glow = 1.0f - fabsf(sinf((g_pad_glow + 0.2f) * 3.14159265f)) * 0.65f;
    SDL_SetTextureColorMod(g_pad_texture, 0xff * glow, 0, 0);
    SDL_RenderCopy(g_renderer, g_pad_texture, NULL, &rect);

    rect.x += 5;
    rect.y += 2;
    rect.w -= 10;
    rect.h -= 2;

    glow = 1.0f - fabsf(sinf((g_pad_glow + 0.1f) * 3.14159265f)) * 0.65f;
    SDL_SetTextureColorMod(g_pad_texture, 0xff * glow, 0, 0);
    SDL_RenderCopy(g_renderer, g_pad_texture, NULL, &rect);

    rect.x += 5;
    rect.y += 2;
    rect.w -= 10;
    rect.h -= 2;

    glow = 1.0f - fabsf(sinf(g_pad_glow * 3.14159265f)) * 0.65f;
    SDL_SetTextureColorMod(g_pad_texture, 0xff * glow, 0, 0);
    SDL_RenderCopy(g_renderer, g_pad_texture, NULL, &rect);
    g_pad_glow = fmodf(g_pad_glow + 0.01, 2.0f);


    rect = (struct SDL_Rect){
        .x = 10,
        .y = g_window_height - 20,
        .w = 100,
        .h = 20
    };

    SDL_SetTextureBlendMode(g_score_texture, SDL_BLENDMODE_BLEND);
    SDL_RenderCopy(g_renderer, g_score_texture, NULL, &rect);

    rect.x = g_window_width - 110;

    SDL_SetTextureBlendMode(g_balls_left_texture, SDL_BLENDMODE_BLEND);
    SDL_RenderCopy(g_renderer, g_balls_left_texture, NULL, &rect);
}

void update_pad()
{ 
    if(g_keyboard_state[SDL_SCANCODE_LEFT] & INPUT_STATE_PRESSED)
    {
        g_pad.pos[0] -= 8;
    }

    if(g_keyboard_state[SDL_SCANCODE_RIGHT] & INPUT_STATE_PRESSED)
    {
        g_pad.pos[0] += 8;
    }

    if(g_keyboard_state[SDL_SCANCODE_UP] & INPUT_STATE_PRESSED)
    {
        if(g_ball_launched == 0)
        {
            g_ball.vel[1] = BALL_SPEED;
            g_ball_launched = 1;
        }
    }

    if(g_pad.pos[0] - PAD_WIDTH / 2 < -(float)g_window_width / 2)
    {
        g_pad.pos[0] = -(g_window_width / 2 - PAD_WIDTH / 2);
    }

    if(g_pad.pos[0] + PAD_WIDTH / 2 > (float)g_window_width / 2)
    {
        g_pad.pos[0] = g_window_width / 2 - PAD_WIDTH / 2;
    }
}

void update_ball()
{
    float prev_pos[2] = { g_ball.pos[0], g_ball.pos[1] };

    if(!g_ball_launched)
    {
        g_ball.pos[0] = g_pad.pos[0];
    }

    g_ball.pos[0] += g_ball.vel[0];
    g_ball.pos[1] += g_ball.vel[1];

    if(g_ball.pos[0] - BALL_RADIUS < -(float)g_window_width / 2)
    {
        g_ball.pos[0] = -((float)g_window_width / 2 - BALL_RADIUS);
        g_ball.vel[0] = -g_ball.vel[0];
        play_sound(&g_ball_boing_sound, 0, 0.5f);
    }

    if(g_ball.pos[0] + BALL_RADIUS > (float)g_window_width / 2)
    {
        g_ball.pos[0] = (float)g_window_width / 2 - BALL_RADIUS;
        g_ball.vel[0] = -g_ball.vel[0];
        play_sound(&g_ball_boing_sound, 0, 0.5f);
    }
 
    if(g_ball.pos[1] + BALL_RADIUS > (float)g_window_height / 2)
    {
        g_ball.pos[1] = (float)g_window_height / 2 - BALL_RADIUS;
        g_ball.vel[1] = -g_ball.vel[1];
        play_sound(&g_ball_boing_sound, 0, 0.5f);
    }

    if(g_ball.pos[0] + BALL_RADIUS > g_pad.pos[0] - PAD_WIDTH / 2 &&
        g_ball.pos[0] - BALL_RADIUS < g_pad.pos[0] + PAD_WIDTH / 2)
    {
        if(g_ball.pos[1] - BALL_RADIUS < g_pad.pos[1] + PAD_HEIGHT / 2 && 
           g_ball.pos[1] - BALL_RADIUS > g_pad.pos[1] - PAD_HEIGHT / 2)
        {
            g_ball.pos[1] = g_pad.pos[1] + PAD_HEIGHT / 2 + BALL_RADIUS;
            g_ball.vel[0] = (g_ball.pos[0] - g_pad.pos[0]) * 0.1f;
            g_ball.vel[1] = -g_ball.vel[1];

            float length = sqrtf(g_ball.vel[0] * g_ball.vel[0] + g_ball.vel[1] * g_ball.vel[1]);
            g_ball.vel[0] = (g_ball.vel[0] / length) * BALL_SPEED;
            g_ball.vel[1] = (g_ball.vel[1] / length) * BALL_SPEED;  
            play_sound(&g_ball_boing_sound, 0, 0.5f);
            // g_combo = 0;
            g_combo_timer = 1;
        } 
    } 

    float ball_x_max = g_ball.pos[0] + BALL_RADIUS;
    float ball_x_min = g_ball.pos[0] - BALL_RADIUS;
    float ball_y_max = g_ball.pos[1] + BALL_RADIUS;
    float ball_y_min = g_ball.pos[1] - BALL_RADIUS;

    float prev_ball_x_max = prev_pos[0] + BALL_RADIUS;
    float prev_ball_x_min = prev_pos[0] - BALL_RADIUS;
    float prev_ball_y_max = prev_pos[1] + BALL_RADIUS;
    float prev_ball_y_min = prev_pos[1] - BALL_RADIUS;

    if(ball_y_max <= -(float)g_window_height / 2)
    {
        g_balls_left--;
        if(g_balls_left == 0)
        {
            g_game_state = GAME_STATE_GAME_OVER;
        }
        else
        {
            g_game_state = GAME_STATE_LOST_BALL;
        }

        if(g_balls_left_texture != NULL)
        {
            SDL_DestroyTexture(g_balls_left_texture);
            g_balls_left_texture = NULL;
        }

        return;
    }

    for(uint32_t brick_index = 0; brick_index < g_brick_count; brick_index++)
    {
        struct brick_t *brick = g_bricks + brick_index;

        float brick_y_min = brick->pos[1] - BRICK_HEIGHT / 2;
        float brick_y_max = brick->pos[1] + BRICK_HEIGHT / 2;

        float brick_x_min = brick->pos[0] - BRICK_WIDTH / 2;
        float brick_x_max = brick->pos[0] + BRICK_WIDTH / 2;

        if(ball_x_max > brick_x_min && ball_x_min < brick_x_max)
        {
            if(ball_y_max > brick_y_min && ball_y_min < brick_y_max)
            {
                // g_ball.vel[0] = g_ball.pos[0] - brick->pos[0];
                // g_ball.vel[1] = g_ball.pos[1] - brick->pos[1];

                // float length = sqrtf(g_ball.vel[0] * g_ball.vel[0] + g_ball.vel[1] * g_ball.vel[1]);
                // g_ball.vel[0] = (g_ball.vel[0] / length) * BALL_SPEED;
                // g_ball.vel[1] = (g_ball.vel[1] / length) * BALL_SPEED;

                if(prev_pos[1] < brick_y_min && ball_y_max > brick_y_min ||
                   prev_pos[1] > brick_y_max && ball_y_min < brick_y_max)
                {
                    g_ball.vel[1] = -g_ball.vel[1];
                }
                else if(prev_pos[0] < brick_x_min && ball_x_max > brick_x_min ||
                        prev_pos[0] > brick_x_max && ball_x_min < brick_x_max)
                {
                    g_ball.vel[0] = -g_ball.vel[0];
                }
 
                if(brick_index < g_brick_count - 1)
                {
                    g_bricks[brick_index] = g_bricks[g_brick_count - 1];
                }
                g_brick_count--;

                if(g_brick_count == 0)
                {
                    g_game_state = GAME_STATE_GAME_WON;
                    g_game_won_timer = 0;
                }

                play_sound(&g_ball_hit_sound, 0, 0.25f);
                g_combo++;
                g_combo_zoom = 1.0f;
                g_combo_timer = 120;

                if(g_combo_texture != NULL)
                {
                    SDL_DestroyTexture(g_combo_texture);
                }

                if(g_score_texture != NULL)
                {
                    SDL_DestroyTexture(g_score_texture);
                    g_score_texture = NULL;
                }

                g_score += 10 + g_combo * 2;

                char combo_str[512];
                sprintf(combo_str, "X%d Combo", g_combo);
                SDL_Surface *surface = TTF_RenderUTF8_Shaded(g_font, combo_str, (SDL_Color){0xff, 0xff, 0xff, 0xff}, (SDL_Color){});
                g_combo_texture = SDL_CreateTextureFromSurface(g_renderer, surface);
                SDL_FreeSurface(surface);
                break;
            } 
        }
    }
}

void init_state(float delta_time)
{
    init_main_menu();
    g_bg_source = play_sound(&g_bg_intro, 0, 0.6f);
    queue_sound(g_bg_source, &g_bg_loop);
}

void init_main_menu()
{
    g_main_menu_delay_timer = 60;
    g_main_menu_press_enter_blink_state = 1;
    g_main_menu_press_enter_blink_timer = 30;
    g_game_state = GAME_STATE_MAIN_MENU;
    g_fade_opacity = 1.0f;
    g_fade_state = FADE_STATE_IN;
    g_main_menu_fading = 0;
}

void main_menu_state(float delta_time)
{
    if(g_keyboard_state[SDL_SCANCODE_RETURN] & INPUT_STATE_JUST_PRESSED)
    {
        if(g_main_menu_delay_timer > 0)
        {
            g_main_menu_delay_timer = 0;
            g_fade_opacity = 0.0f;
        }
        else if(g_main_menu_fading == 0)
        {
            g_main_menu_fading = 1;
            g_fade_opacity = 0.0f;
            g_fade_state = FADE_STATE_OUT;
            play_sound(&g_main_menu_start_sound, 0, 0.5f);
        }
    }

    SDL_Rect rect = {
        .x = -5,
        .w = g_window_width, 
        .h = g_window_height / 2,
    };

    SDL_RenderCopy(g_renderer, g_main_menu_title_texture, NULL, &rect);

    if(g_main_menu_delay_timer > 0)
    {
        g_main_menu_delay_timer--;
    }
    else
    {
        if(g_main_menu_press_enter_blink_state)
        {
            SDL_Rect rect = {
                .x = g_window_width / 2 - 60,
                .y = g_window_height - 100,
                .w = 120, 
                .h = 40,
            };

            SDL_RenderCopy(g_renderer, g_main_menu_press_enter_texture, NULL, &rect);
        }

        g_main_menu_press_enter_blink_timer--;

        if(g_main_menu_press_enter_blink_timer == 0)
        {
            g_main_menu_press_enter_blink_timer = 30;
            g_main_menu_press_enter_blink_state = !g_main_menu_press_enter_blink_state;
        }
    }

    if(g_fade_state == FADE_STATE_OUT)
    {
        if(g_fade_opacity >= 1.0f)
        {
            g_game_state = GAME_STATE_START_GAME;
        }
    }
}

void lost_ball_state(float delta_time)
{
    draw_play_field();

    if(g_lost_ball_timer == 0)
    {
        g_lost_ball_timer = 120;
        play_sound(&g_ahh_sounds[rand() % 2], 0, 0.75f);
    }
    else
    {
        g_lost_ball_timer--;

        SDL_Rect rect = {
            .x = g_window_width / 2 - 200,            
            .y = g_window_height / 2 - 50,            
            .w = 400,
            .h = 100
        };

        if(g_lost_ball_timer == 0)
        {
            init_ball();
            g_game_state = GAME_STATE_PLAYING;
        }

        SDL_RenderCopy(g_renderer, g_lost_ball_texture[g_lost_ball_timer == 0 && g_balls_left == 2], NULL, &rect);
    }
}

void game_over_state(float delta_time)
{
    draw_play_field();

    if(g_game_over_timer == 0)
    {
        g_game_over_timer = 180;
        play_sound(&g_boo_sound, 0, 0.75f);
    }
    else
    {
        g_game_over_timer--;
        if(g_game_over_timer == 0)
        {
            g_game_state = GAME_STATE_MAIN_MENU;
            init_main_menu();
        }

        SDL_Rect rect = {
            .x = g_window_width / 2 - 150,
            .y = g_window_height / 2 - 60,
            .w = 300,
            .h = 120
        };

        SDL_RenderCopy(g_renderer, g_game_over_texture, NULL, &rect);
    }
}

void game_won_state(float delta_time)
{
    draw_play_field();

    if(g_game_won_timer == 0)
    {
        g_game_won_timer = 300;
        play_sound(&g_applause_sounds[2], 0, 0.25f);

        if(g_game_won_texture != NULL)
        {
            SDL_DestroyTexture(g_game_won_texture);
        }

        char score_str[512];
        sprintf(score_str, "YOU WIN!\nYour score: %d", g_score);
        SDL_Surface *surface = TTF_RenderUTF8_Shaded(g_font, score_str, (SDL_Color){0xff, 0xff, 0xff, 0xff}, (SDL_Color){});
        g_game_won_texture = SDL_CreateTextureFromSurface(g_renderer, surface);
        SDL_FreeSurface(surface);
    }
    else
    {
        g_game_won_timer--;

        if(g_game_won_timer == 0)
        {
            g_game_state = GAME_STATE_MAIN_MENU;
            init_main_menu();
        }

        SDL_Rect rect = {
            .x = g_window_width / 2 - 150,
            .y = g_window_height / 2 - 30,
            .w = 300,
            .h = 60
        };

        SDL_SetTextureBlendMode(g_game_won_texture, SDL_BLENDMODE_BLEND);
        SDL_RenderCopy(g_renderer, g_game_won_texture, NULL, &rect);
    }
}

void start_game_state(float delta_time)
{
    init_play_field();
    g_fade_opacity = 1.0f;
    g_fade_state = FADE_STATE_IN;
    g_game_state = GAME_STATE_PLAYING;
    g_best_combo = 0;
    g_score = 0;

    if(g_score_texture != NULL)
    {
        SDL_DestroyTexture(g_score_texture);
        g_score_texture = NULL;
    }

    if(g_balls_left_texture != NULL)
    {
        SDL_DestroyTexture(g_balls_left_texture);
        g_balls_left_texture = NULL;
    }
}

void playing_state(float delta_time)
{
    if(g_fade_opacity == 0.0f)
    {
        g_fade_state = FADE_STATE_NONE;
    }

    if(g_fade_state == FADE_STATE_NONE)
    {
        update_pad();
        update_ball();
    }

    if(g_combo_timer > 0)
    {
        g_combo_timer--;

        if(g_combo_timer == 0)
        {
            if(g_combo >= 25)
            {
                play_sound(&g_applause_sounds[2], 0, 0.25f);
            }
            else if(g_combo >= 10)
            {
                play_sound(&g_applause_sounds[1], 0, 0.35f);
            }
            else if(g_combo >= 5)
            {
                play_sound(&g_applause_sounds[0], 0, 0.5f);
            }

            g_combo = 0;
            g_combo_show_timer = 120;
        }
    }

    if(g_score_texture == NULL)
    {
        char score_str[512];
        sprintf(score_str, "Score: %d", g_score);
        SDL_Surface *surface = TTF_RenderUTF8_Shaded(g_font, score_str, (SDL_Color){0xff, 0xff, 0xff, 0xff}, (SDL_Color){});
        g_score_texture = SDL_CreateTextureFromSurface(g_renderer, surface);
        SDL_FreeSurface(surface);
    }

    if(g_balls_left_texture == NULL)
    {
        char balls_str[512];
        if(g_balls_left > 0)
        {
            sprintf(balls_str, "Balls: %d", g_balls_left);
        }
        else
        {
            strcpy(balls_str, "Balls: NONE");
        }

        SDL_Surface *surface = TTF_RenderUTF8_Shaded(g_font, balls_str, (SDL_Color){0xff, 0xff, 0xff, 0xff}, (SDL_Color){});
        g_balls_left_texture = SDL_CreateTextureFromSurface(g_renderer, surface);
        SDL_FreeSurface(surface);
    }

    draw_play_field();

    if(g_combo >= 5)
    {
        g_combo_zoom -= delta_time * 10.0f;

        if(g_combo_zoom < 0.0f)
        {
            g_combo_zoom = 0.0f;
        }

        SDL_Rect rect = {
            .x = g_window_width - (110 + 10 * g_combo_zoom),
            .y = g_window_height - (80 - 10 * g_combo_zoom),
            .w = 100 + 20 * g_combo_zoom,
            .h = 60 + 20 * g_combo_zoom
        };

        SDL_SetTextureBlendMode(g_combo_texture, SDL_BLENDMODE_BLEND);
        SDL_RenderCopy(g_renderer, g_combo_texture, NULL, &rect);
    }
}

void quit_state(float delta_time)
{

}

int main(int argc, char *argv[])
{
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
    {
        printf("well, shit...\n");
        return -1;
    }

    if(TTF_Init() < 0)
    {
        printf("well, fuck...\n");
        return -1;
    }

    if(!sound_init())
    {
        return -1;
    }

    g_window = SDL_CreateWindow("BRICKER", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, g_window_width, g_window_height, 0);
    g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 1);

    g_font = TTF_OpenFont("res/font/ProggyClean.ttf", 16);
    SDL_Surface *surface = TTF_RenderUTF8_Shaded(g_font, "BRICKER", (SDL_Color){0xff, 0xff, 0xff, 0xff}, (SDL_Color){});
    g_main_menu_title_texture = SDL_CreateTextureFromSurface(g_renderer, surface);
    SDL_FreeSurface(surface);

    surface = TTF_RenderUTF8_Shaded(g_font, "Press enter", (SDL_Color){0xff, 0xff, 0xff, 0xff}, (SDL_Color){});
    g_main_menu_press_enter_texture = SDL_CreateTextureFromSurface(g_renderer, surface);
    SDL_FreeSurface(surface);

    surface = TTF_RenderUTF8_Shaded(g_font, "You lost your BALL!", (SDL_Color){0xff, 0xff, 0xff, 0xff}, (SDL_Color){});
    g_lost_ball_texture[0] = SDL_CreateTextureFromSurface(g_renderer, surface);
    SDL_FreeSurface(surface);

    surface = TTF_RenderUTF8_Shaded(g_font, "You lost your BALL! >:)", (SDL_Color){0xff, 0xff, 0xff, 0xff}, (SDL_Color){});
    g_lost_ball_texture[1] = SDL_CreateTextureFromSurface(g_renderer, surface);
    SDL_FreeSurface(surface);

    surface = TTF_RenderUTF8_Shaded(g_font, "GAME OVER", (SDL_Color){0xff, 0xff, 0xff, 0xff}, (SDL_Color){});
    g_game_over_texture = SDL_CreateTextureFromSurface(g_renderer, surface);
    SDL_FreeSurface(surface);

    int width;
    int height;
    int channels;

    char *pixels = stbi_load("res/texture/pad_base.png", &width, &height, &channels, STBI_rgb_alpha);
    g_pad_texture = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, width, height);
    SDL_UpdateTexture(g_pad_texture, NULL, pixels, width * 4);
    free(pixels);

    pixels = stbi_load("res/texture/ball.png", &width, &height, &channels, STBI_rgb_alpha);
    g_ball_texture = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_BGRA8888, SDL_TEXTUREACCESS_STATIC, width, height);
    SDL_UpdateTexture(g_ball_texture, NULL, pixels, width * 4);
    free(pixels);

    g_ball_hit_sound = load_sound("res/sound/ball_hit.ogg");
    g_ball_boing_sound = load_sound("res/sound/boing.ogg");
    g_main_menu_start_sound = load_sound("res/sound/start.ogg");
    g_bg_intro = load_sound("res/sound/bg_intro.ogg");
    g_bg_loop = load_sound("res/sound/bg_loop.ogg");

    g_applause_sounds[0] = load_sound("res/sound/applause0.ogg");
    g_applause_sounds[1] = load_sound("res/sound/applause1.ogg");
    g_applause_sounds[2] = load_sound("res/sound/applause2.ogg");

    g_ahh_sounds[0] = load_sound("res/sound/ahh0.ogg");
    g_ahh_sounds[1] = load_sound("res/sound/ahh1.ogg");
    g_boo_sound = load_sound("res/sound/booo.ogg");

    g_bricks = calloc(MAX_BRICKS, sizeof(struct brick_t));

    float delta_time = 0.0166f;

    while(g_game_state != GAME_STATE_QUIT)
    {
        update_input();
        SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 0xff);
        SDL_RenderClear(g_renderer);
        g_game_states[g_game_state](delta_time);

        if(g_bg_source->current_buffer > 0 && g_bg_looping == 0)
        {
            g_bg_looping = 1;
            loop_source(g_bg_source, 1);
        }

        if(g_fade_state == FADE_STATE_IN)
        {
            g_fade_opacity -= delta_time * 2.0f;

            if(g_fade_opacity <= 0.0f)
            {
                g_fade_opacity = 0.0f;
            }
        }

        if(g_fade_state == FADE_STATE_OUT)
        {
            g_fade_opacity += delta_time * 2.0f;

            if(g_fade_opacity >= 1.0f)
            {
                g_fade_opacity = 1.0f;
            }
        }

        if(g_fade_opacity != 0.0f)
        {
            SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 0xff * g_fade_opacity);

            SDL_Rect rect = {
                .w = g_window_width,
                .h = g_window_height
            };

            SDL_RenderFillRect(g_renderer, &rect);
        }

        SDL_RenderPresent(g_renderer);
    }

    free(g_bricks);
    TTF_CloseFont(g_font);
    SDL_DestroyTexture(g_main_menu_press_enter_texture);
    SDL_DestroyTexture(g_main_menu_title_texture);

    SDL_DestroyRenderer(g_renderer);
    SDL_DestroyWindow(g_window);
}