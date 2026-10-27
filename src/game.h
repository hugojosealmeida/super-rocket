#ifndef SUPER_ROCKET_GAME_H
#define SUPER_ROCKET_GAME_H

#define GAME_MAX_ASTEROIDS 64
#define GAME_MAX_PARTICLES 300
#define GAME_MAX_PICKUPS   12
#define GAME_MAX_LASERS    24
#define GAME_MAX_UFOS      6
#define GAME_MAX_SHOTS     32
#define GAME_MAX_SECTORS   3
#define GAME_MAX_LEVELS    2
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
    GAME_DEAD,
    GAME_UPGRADE,
    GAME_INITIALS
} GameState;

typedef enum {
    UPGRADE_HULL,
    UPGRADE_FUEL,
    UPGRADE_SHIELD,
    UPGRADE_WEAPON,
    UPGRADE_RATE,
    UPGRADE_COUNT
} UpgradeType;

typedef enum {
    DIFFICULTY_EASY,
    DIFFICULTY_NORMAL,
    DIFFICULTY_HARD,
    DIFFICULTY_COUNT
} Difficulty;

#define GAME_THEME_COUNT 3

typedef struct {
    int theme;
    int sound;
    int show_fps;
} Settings;

#define GAME_STAT_COUNT 11
#define GAME_LEADERBOARD_SIZE 5

#define ACH_FIRST_WIN    0x01
#define ACH_NO_DAMAGE    0x02
#define ACH_SPEEDRUN     0x04
#define ACH_ROCK_BREAKER 0x08
#define ACH_UFO_HUNTER   0x10
#define ACH_COLLECTOR    0x20
#define GAME_ACHIEVEMENT_COUNT 6

typedef struct {
    char name[4];
    int score;
    int difficulty;
} ScoreEntry;

typedef struct {
    int runs;
    int wins;
    int deaths;
    int asteroids_destroyed;
    int pickups_collected;
    int lasers_fired;
    int best_time;
    int total_distance;
    int total_fuel_used;
    int ufos_destroyed;
    int achievements;
} Stats;

typedef struct {
    double x;
    double y;
    double vx;
    double vy;
    int radius;
    int hp;
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

typedef enum {
    PICKUP_FUEL,
    PICKUP_REPAIR,
    PICKUP_SHIELD
} PickupType;

typedef struct {
    double x;
    double y;
    double bob;
    PickupType type;
    int alive;
} Pickup;

typedef struct {
    double x;
    double y;
    double vx;
    double vy;
    double life;
    int alive;
} Laser;

typedef struct {
    double x;
    double y;
    double vx;
    double vy;
    double fire_timer;
    double wobble;
    int hp;
    int alive;
} Ufo;

typedef struct {
    double x;
    double y;
    double vx;
    double vy;
    double life;
    int alive;
} EnemyShot;

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
    double pickup_timer;
    double shield;
    double fire_cooldown;
    double fire_delay;
    int weapon_level;
    double ufo_timer;
    int sector;
    double sector_banner;
    int level;
    double level_banner;
    UpgradeType upgrades[3];
    int leaderboard_rank;
    char initials[4];
    ScoreEntry leaderboard[GAME_LEADERBOARD_SIZE];
    double gravity_scale;
    double spawn_interval;

    int score;
    int best[DIFFICULTY_COUNT];
    int new_best;
    unsigned long frame;
    Settings settings;
    Stats stats;
    double fps;
    int show_help;
    int show_scores;
    double toast_timer;
    char toast[48];
    int took_damage;
    int combo;
    double combo_timer;

    Asteroid asteroids[GAME_MAX_ASTEROIDS];
    Particle particles[GAME_MAX_PARTICLES];
    Pickup pickups[GAME_MAX_PICKUPS];
    Laser lasers[GAME_MAX_LASERS];
    Ufo ufos[GAME_MAX_UFOS];
    EnemyShot shots[GAME_MAX_SHOTS];
    Star stars[GAME_MAX_STARS];
    int star_count;
} Game;

void game_init(Game *g, int width, int height);

/*
 * Applies a new terminal size mid-game (issue #5): updates only the canvas
 * geometry (width, height, play_height, world_mid) and regenerates the
 * starfield for the new width, leaving score, hull, fuel, position, level
 * and all entities untouched, so a resize never interrupts the flight.
 * The camera is clamped into the same world bounds game_update() steers
 * towards, which matter when the window grows and the far bound shrinks.
 * In the menu the camera belongs to the starfield drift instead, so it is
 * left alone there.  Sizes that cannot host a playable canvas (no columns,
 * or too few rows for the play area plus the HUD line) are ignored and the
 * last valid geometry keeps rendering.
 */
void game_resize(Game *g, int width, int height);

void game_reset(Game *g);
void game_start(Game *g);
void game_update(Game *g);

void game_thrust(Game *g, int up, int down, int left, int right);
void game_fire(Game *g);
void game_roll_upgrades(Game *g);
void game_choose_upgrade(Game *g, int index);
const char *game_upgrade_name(UpgradeType type);
const char *game_achievement_name(int index);
int game_unlock_achievement(Game *g, int flag);
void game_pause(Game *g);
void game_resume(Game *g);
void game_to_menu(Game *g);
void game_set_difficulty(Game *g, Difficulty difficulty);
void game_load_scores(Game *g);
void game_save_scores(const Game *g);
void game_load_settings(Game *g);
void game_save_settings(const Game *g);
void game_load_stats(Game *g);
void game_save_stats(const Game *g);
void game_reset_stats(Game *g);
void game_load_leaderboard(Game *g);
void game_save_leaderboard(const Game *g);
int game_leaderboard_position(const Game *g, int score);
void game_commit_initials(Game *g);
void game_initial_push(Game *g, char letter);
void game_initial_pop(Game *g);
void game_toggle_help(Game *g);
void game_toggle_scores(Game *g);
void game_cycle_theme(Game *g);
void game_toggle_sound(Game *g);
void game_toggle_fps(Game *g);
const char *game_difficulty_name(Difficulty difficulty);
const char *game_sector_name(int sector);
const char *game_level_name(int level);

#endif
