#ifndef SUPER_ROCKET_GAME_H
#define SUPER_ROCKET_GAME_H

#define GAME_MAX_ASTEROIDS 64
#define GAME_MAX_PARTICLES 300
#define GAME_MAX_STARS    400

#define GAME_WORLD_LENGTH 1800.0
#define GAME_EARTH_RADIUS 7.0
#define GAME_MARS_RADIUS  4.5
#define GAME_MARS_ORBIT   16.5
#define GAME_STAR_SPAN    2400.0
#define GAME_HULL_MAX     100.0

#define GAME_VERSION "0.9.0"

typedef enum {
    GAME_MENU,
    GAME_PLAYING,
    GAME_PAUSED,
    GAME_WON,
    GAME_DEAD
} GameState;

typedef enum {
    DIFFICULTY_EASY,
    DIFFICULTY_NORMAL,
    DIFFICULTY_HARD,
    DIFFICULTY_COUNT
} Difficulty;

typedef struct {
    double x;
    double y;
    double vx;
    double vy;
    int radius;
    int alive;
} Asteroid;

typedef struct {
    double x;
    double y;
    double vx;
    double vy;
    double life;
    char ch;
    unsigned char color;
} Particle;

typedef struct {
    double x;
    int y;
    char ch;
    double factor;
} Star;

typedef struct {
    int width;
    int height;
    int play_height;
    double world_mid;

    GameState state;
    Difficulty difficulty;

    double player_x;
    double player_y;
    double player_vx;
    double player_vy;
    double thrust[4];
    double camera_x;
    double fuel;
    double fuel_max;
    double hull;
    double mission_time;
    double invuln;
    double shake;
    double spawn_timer;
    double gravity_scale;
    double spawn_interval;

    int score;
    int best[DIFFICULTY_COUNT];
    int new_best;
    unsigned long frame;

    Asteroid asteroids[GAME_MAX_ASTEROIDS];
    Particle particles[GAME_MAX_PARTICLES];
    Star stars[GAME_MAX_STARS];
    int star_count;
} Game;

void game_init(Game *g, int width, int height);
void game_reset(Game *g);
void game_start(Game *g);
void game_update(Game *g);

void game_thrust(Game *g, int up, int down, int left, int right);
void game_pause(Game *g);
void game_resume(Game *g);
void game_to_menu(Game *g);
void game_set_difficulty(Game *g, Difficulty difficulty);
void game_load_scores(Game *g);
void game_save_scores(const Game *g);
const char *game_difficulty_name(Difficulty difficulty);

#endif
