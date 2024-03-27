#ifndef GAME_H
#define GAME_H

#include <stdint.h>

enum GAME_STATES
{
    GAME_STATE_INIT,
    GAME_STATE_MAIN_MENU,
    GAME_STATE_LOST_BALL,
    GAME_STATE_GAME_OVER,
    GAME_STATE_GAME_WON,
    GAME_STATE_START_GAME,
    GAME_STATE_PLAYING,
    GAME_STATE_PAUSED,
    GAME_STATE_QUIT,
};

enum INPUT_STATES
{
    INPUT_STATE_PRESSED = 1,
    INPUT_STATE_JUST_PRESSED = 1 << 1,
    INPUT_STATE_JUST_RELEASED = 1 << 2
};

enum FADE_STATE
{
    FADE_STATE_NONE,
    FADE_STATE_IN,
    FADE_STATE_OUT,
};
// #define FRACT_BITS 8
// #define MAKE_FIXED(i, f) ((((int32_t)(i)) << FRACT_BITS) | ((f) & 0xff))

#define PAD_WIDTH   90
#define PAD_HEIGHT  18

struct pad_t
{
    float pos[2];
};

#define BALL_RADIUS 8
#define BALL_SPEED 10

struct ball_t
{
    float pos[2];
    float vel[2];
};

#define BRICK_WIDTH     35
#define BRICK_HEIGHT    20
#define MAX_BRICKS      2400

struct brick_t
{
    float     pos[2];
    uint8_t     color[4];
};

#define PLAYFIELD_WIDTH 600

void init_game();

void update_input();

void init_ball();

void init_play_field();

void draw_play_field();

void update_pad();

void update_ball();

void init_state(float delta_time);

void init_main_menu();

void main_menu_state(float delta_time);

void lost_ball_state(float delta_time);

void game_over_state(float delta_time);

void game_won_state(float delta_time);

void start_game_state(float delta_time);

void playing_state(float delta_time);

void quit_state(float delta_time);

void update_game(float delta_time);





#endif