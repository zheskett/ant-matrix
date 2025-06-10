/**
 * @file simulation.h
 * @author Zachary Heskett (zheskett@gmail.com)
 * @brief Header file for the ant simulation program.
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#ifndef SIMULATION_H
#define SIMULATION_H

#include "entities/ant.h"

// Multiple networks per ant?
#define PER_ANT_NETWORK 1

#define LEARN_RATE 0.15l
#define LEARN_RATE_DECAY 0.99999999l
// 1 angle, 2 positions (spawn/food), 1 has_food, 1 near food, 1 is_coliding
#define ANN_INPUTS 10

// 3 turn actions, 3 actions
#define ANN_OUTPUTS 6

#define ANN_NEURON_COUNTS {ANN_INPUTS, 16, ANN_OUTPUTS}

#define ANN_BATCH_SIZE 1000

#define TARGET_FPS 0
#define TICK_RATE 30
#define SCREEN_W 1920
#define SCREEN_H 1080
#define WORLD_SCALE 2.0
#define WORLD_W ((int)(SCREEN_W * WORLD_SCALE))
#define WORLD_H ((int)(SCREEN_H * WORLD_SCALE))

#define CAM_SPEED 1000

#define MAX_DELTA 5.0
#define WARP_SPEED 100.0
#define RESET_TIME 60.0

extern dyn_arr_ant_t g_ant_list;
extern dyn_arr_food_t g_food_list;

/**
 * @brief Start the simulation.
 *
 * Initializes the simulation, sets up the window, and runs the main loop.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Exit code of the simulation.
 */
int start(int argc, char **argv);

#endif /* SIMULATION_H */