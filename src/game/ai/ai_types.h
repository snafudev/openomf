/**
 * AI Types - Shared type definitions for AI controller and decision engine
 *
 * This header defines the core AI state struct so both ai_controller.c
 * and ai_decision_engine.c can share the type definition without circular includes.
 */

#ifndef AI_TYPES_H
#define AI_TYPES_H

#include "formats/pilot.h"
#include "game/objects/har.h"
#include "resources/af_loader.h"
#include "utils/vec.h"

typedef struct {
    int max_hit_dist;
    int min_hit_dist;
    int value;
    int attempts;
    int consecutive;
    int last_dist;
} move_stat;

typedef struct {
    int tactic_type;
    int last_tactic;
    int move_type;
    int move_timer;
    int attack_type;
    int attack_id;
    int attack_timer;
    int attack_on;
    int chain_hit_on;
    int chain_hit_tactic;
} tactic_state;

typedef struct ai {
    int difficulty;
    int act_timer;
    int cur_act;
    int input_lag;
    int input_lag_timer;

    // move stats
    af_move *selected_move;
    int last_move_id;
    int move_str_pos;
    move_stat move_stats[70];
    int blocked;
    int thrown;
    int shot;

    // tactical state
    tactic_state *tactic;

    sd_pilot *pilot;

    // all projectiles currently on screen (vector of projectile object*)
    vector active_projectiles;
} ai;

#endif // AI_TYPES_H
