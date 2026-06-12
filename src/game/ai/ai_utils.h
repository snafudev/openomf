/**
 * AI utility helpers
 *
 * Extracted helper functions for range checks, HAR capabilities,
 * move filtering, and command conversion.
 */

#ifndef AI_UTILS_H
#define AI_UTILS_H

#include "controller/controller.h"
#include "game/common_defines.h"
#include "resources/af_move.h"
#include "utils/str.h"
#include <stdbool.h>

enum
{
    RANGE_CRAMPED = 0,
    RANGE_CLOSE,
    RANGE_MID,
    RANGE_FAR
};

int ai_enemy_range_from_positions(float self_x, float enemy_x);
bool ai_enemy_is_stunned_or_stasis(int enemy_state, int stasis_ticks);

int get_enemy_range(const controller *ctrl);
bool enemy_is_stunned_or_stasis(const controller *ctrl);

bool is_special_move(const af_move *move);
bool har_has_projectiles(int har_id);
bool har_has_charge(int har_id);
bool har_has_push(int har_id);

int char_to_act(str *ch, int direction, int *position);

#endif // AI_UTILS_H
