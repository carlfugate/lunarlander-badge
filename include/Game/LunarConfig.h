#ifndef LUNAR_CONFIG_H
#define LUNAR_CONFIG_H

// Physics
#define LN_GRAVITY           30.0f
#define LN_THRUST_POWER      60.0f
#define LN_ROTATION_SPEED    3.0f
#define LN_FUEL_CONSUMPTION  10.0f
#define LN_INITIAL_FUEL      1000.0f
#define LN_MAX_LANDING_SPEED 8.0f
#define LN_MAX_LANDING_ANGLE 0.3f
#define LN_PHYSICS_DT        (1.0f / 60.0f)

// World
#define LN_WORLD_W           1200
#define LN_WORLD_H           800
#define LN_START_X           600.0f
#define LN_START_Y           100.0f

// Display
#define LN_SCREEN_W          320
#define LN_SCREEN_H          240

// Terrain
#define LN_MAX_TERRAIN_POINTS 60

// Difficulty: step, variation, zone_width
#define LN_DIFF_EASY_STEP    50
#define LN_DIFF_EASY_VAR     20
#define LN_DIFF_EASY_ZONE    100
#define LN_DIFF_MED_STEP     40
#define LN_DIFF_MED_VAR      30
#define LN_DIFF_MED_ZONE     80
#define LN_DIFF_HARD_STEP    30
#define LN_DIFF_HARD_VAR     50
#define LN_DIFF_HARD_ZONE    60

// Scoring
#define LN_SCORE_BASE        1000
#define LN_SCORE_FUEL_MAX    500
#define LN_SCORE_TIME_MAX    300
#define LN_SCORE_TIME_FLOOR  20.0f
#define LN_SCORE_TIME_DECAY  5.0f

// Difficulty multipliers (x10 to avoid float in define)
#define LN_MULT_EASY         10
#define LN_MULT_MED          15
#define LN_MULT_HARD         20

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#endif
