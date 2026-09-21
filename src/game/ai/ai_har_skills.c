/**
 * AI character skills implementation
 */

#include "game/ai/ai_har_skills.h"

#include "game/ai/ai_decision_engine.h"
#include "game/ai/ai_tactic_engine.h"
#include "game/ai/ai_types.h"
#include "game/ai/ai_utils.h"
#include "game/game_state.h"
#include "game/objects/har.h"
#include "utils/c_array_util.h"
#include <limits.h>

static int act_back(const object *o) {
    return o->direction == OBJECT_FACE_RIGHT ? ACT_LEFT : ACT_RIGHT;
}

static int act_down_back(const object *o) {
    return act_back(o) | ACT_DOWN;
}

int ai_resolve_input(int input, int direction) {
    if(direction == OBJECT_FACE_LEFT) {
        int has_left = input & ACT_LEFT;
        int has_right = input & ACT_RIGHT;
        input &= ~(ACT_LEFT | ACT_RIGHT);
        if(has_left) {
            input |= ACT_RIGHT;
        }
        if(has_right) {
            input |= ACT_LEFT;
        }
    }
    return input;
}

int ai_resolve_move_input(int input, int direction, int prev_dirs) {
    int resolved = ai_resolve_input(input, direction);

    if((resolved & (ACT_PUNCH | ACT_KICK)) && (resolved & ACT_Mask_Dirs) == 0 && prev_dirs != 0) {
        resolved |= prev_dirs;
    }

    return resolved;
}

int ai_move_def_score(const ai_move_def *move, int enemy_range, bool enemy_stunned, const ai *ai_data) {
    if(move == NULL) {
        return 0;
    }

    if(move->range_min != MOVE_RANGE_ANY && enemy_range < (int)move->range_min) {
        return -999;
    }
    if(move->range_max != MOVE_RANGE_ANY && enemy_range > (int)move->range_max) {
        return -999;
    }

    int score = 0;

    if((move->conditions & MOVE_COND_ENEMY_STUNNED) != 0) {
        score += enemy_stunned ? 50 : -25;
    }
    if((move->conditions & MOVE_COND_ENEMY_NOT_STUNNED) != 0) {
        score += enemy_stunned ? -25 : 15;
    }
    if((move->conditions & MOVE_COND_HIGH_DIFFICULTY) != 0) {
        score += ai_data != NULL && ai_data->difficulty >= 3 ? 8 : 0;
    }
    if((move->conditions & MOVE_COND_SPECIAL_PREF) != 0) {
        score += 4;
    }
    if((move->conditions & MOVE_COND_LOW_PREFERRED) != 0) {
        score += 2;
    }
    if((move->conditions & MOVE_COND_JUMP_PREFERRED) != 0) {
        score += 2;
    }

    return score;
}

static void chain_controller_cmd(controller *ctrl, int commands[], size_t n_commands, ctrl_event **ev) {
    for(size_t i = 0; i < n_commands; i++) {
        controller_cmd(ctrl, commands[i], ev);
    }
}

static bool can_start_ground_attack(controller *ctrl, har *h, ctrl_event **ev) {
    switch(h->state) {
        case STATE_WALKTO:
        case STATE_WALKFROM:
        case STATE_CROUCHBLOCK:
        case STATE_CROUCHING:
        case STATE_STANDING:
            controller_cmd(ctrl, ACT_STOP, ev);
            return true;
        default:
            return false;
    }
}

const ai_move_def *ai_har_select_best_move(const ai_move_def *moves, uint8_t move_count, int enemy_range,
                                          bool enemy_stunned, const ai *ai_data) {
    if(moves == NULL || move_count == 0 || ai_data == NULL || ai_data->pilot == NULL) {
        return NULL;
    }

    const ai_move_def *best_move = NULL;
    int best_score = INT_MIN;

    for(uint8_t i = 0; i < move_count; i++) {
        const ai_move_def *move = &moves[i];
        if(move == NULL) {
            continue;
        }

        if(enemy_range < (int)move->range_min) {
            continue;
        }
        if(enemy_range > (int)move->range_max) {
            continue;
        }
        if((move->conditions & MOVE_COND_HIGH_DIFFICULTY) != 0 && !diff_scale((ai *)ai_data)) {
            continue;
        }
        if((move->conditions & MOVE_COND_SPECIAL_PREF) != 0 && !roll_pref(ai_data->pilot->ap_special)) {
            continue;
        }
        if((move->conditions & MOVE_COND_LOW_PREFERRED) != 0 && !roll_pref(ai_data->pilot->ap_low)) {
            continue;
        }
        if((move->conditions & MOVE_COND_JUMP_PREFERRED) != 0 && !roll_pref(ai_data->pilot->att_jump)) {
            continue;
        }
        if((move->conditions & MOVE_COND_ROLL_D2) != 0 && !roll_chance(2)) {
            continue;
        }
        if((move->conditions & MOVE_COND_ROLL_D3) != 0 && !roll_chance(3)) {
            continue;
        }
        if((move->conditions & MOVE_COND_ROLL_D4) != 0 && !roll_chance(4)) {
            continue;
        }
        if((move->conditions & MOVE_COND_ROLL_D10) != 0 && !roll_chance(10)) {
            continue;
        }
        if((move->conditions & MOVE_COND_ROLL_D20) != 0 && !roll_chance(20)) {
            continue;
        }
        if((move->conditions & MOVE_COND_ENEMY_NOT_STUNNED) != 0 && enemy_stunned) {
            continue;
        }
        if((move->conditions & MOVE_COND_ENEMY_STUNNED) != 0 && !enemy_stunned) {
            continue;
        }

        int score = ai_move_def_score(move, enemy_range, enemy_stunned, ai_data);
        if(score <= -999) {
            continue;
        }

        if(best_move == NULL || score > best_score) {
            best_move = move;
            best_score = score;
        }
    }

    return best_move;
}

static bool ai_har_execute_move_list(controller *ctrl, object *o, ai *a, const ai_move_def *moves,
                                      uint8_t move_count, int enemy_range, ctrl_event **ev) {
    const bool enemy_stunned = enemy_is_stunned_or_stasis(ctrl);
    const ai_move_def *move = ai_har_select_best_move(moves, move_count, enemy_range, enemy_stunned, a);
    if(move == NULL) {
        return false;
    }

    int prev_dirs = 0;
    for(uint8_t j = 0; j < move->input_count; j++) {
        int resolved = ai_resolve_move_input(move->inputs[j], o->direction, prev_dirs);
        controller_cmd(ctrl, resolved, ev);

        int dirs = resolved & ACT_Mask_Dirs;
        if(dirs != 0) {
            prev_dirs = dirs;
        } else if(resolved == ACT_STOP) {
            // Neutral input releases any held direction (charge moves).
            prev_dirs = 0;
        }
    }

    if(move->follow_up_tactic_count > 0) {
        int tactics[AI_MOVE_MAX_FOLLOW_TACTICS] = {0};
        for(uint8_t j = 0; j < move->follow_up_tactic_count; j++) {
            tactics[j] = move->follow_up_tactics[j];
        }
        ai_tactic_consider_list(ctrl, tactics, move->follow_up_tactic_count);
    }

    return true;
}

bool ai_har_execute_charge(controller *ctrl, const ai_har_config *char_cfg, ctrl_event **ev) {
    if(ctrl == NULL) {
        return false;
    }

    ai *a = ctrl->data;
    object *o = game_state_find_object(ctrl->gs, ctrl->har_obj_id);
    if(o == NULL || a == NULL) {
        return false;
    }

    har *h = object_get_userdata(o);
    if(h == NULL || !can_start_ground_attack(ctrl, h, ev)) {
        return false;
    }

    if(char_cfg == NULL || !char_cfg->has_charge_moves) {
        return false;
    }

    int enemy_range = get_enemy_range(ctrl);
    return ai_har_execute_move_list(ctrl, o, a, char_cfg->charge_moves, char_cfg->charge_move_count, enemy_range,
                                     ev);
}

bool ai_har_execute_push(controller *ctrl, const ai_har_config *char_cfg, ctrl_event **ev) {
    if(ctrl == NULL) {
        return false;
    }

    ai *a = ctrl->data;
    object *o = game_state_find_object(ctrl->gs, ctrl->har_obj_id);
    if(o == NULL || a == NULL) {
        return false;
    }

    har *h = object_get_userdata(o);
    if(h == NULL || !can_start_ground_attack(ctrl, h, ev)) {
        return false;
    }

    if(char_cfg == NULL || !char_cfg->has_push_moves) {
        return false;
    }

    int enemy_range = get_enemy_range(ctrl);
    return ai_har_execute_move_list(ctrl, o, a, char_cfg->push_moves, char_cfg->push_move_count, enemy_range, ev);
}

bool ai_har_execute_trip(controller *ctrl, const ai_har_config *char_cfg, ctrl_event **ev) {
    if(ctrl == NULL) {
        return false;
    }

    object *o = game_state_find_object(ctrl->gs, ctrl->har_obj_id);
    if(o == NULL) {
        return false;
    }

    har *h = object_get_userdata(o);
    if(h == NULL || !can_start_ground_attack(ctrl, h, ev)) {
        return false;
    }

    if(char_cfg != NULL && char_cfg->has_trip_moves) {
        ai *a = ctrl->data;
        if(a != NULL) {
            int enemy_range = get_enemy_range(ctrl);
            if(ai_har_execute_move_list(ctrl, o, a, char_cfg->trip_moves, char_cfg->trip_move_count, enemy_range,
                                       ev)) {
                return true;
            }
        }
    }

    int cmds[] = {act_down_back(o) | ACT_KICK};
    chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
    return true;
}

bool ai_har_execute_projectile(controller *ctrl, const ai_har_config *char_cfg, ctrl_event **ev) {
    if(ctrl == NULL) {
        return false;
    }

    ai *a = ctrl->data;
    if(a == NULL) {
        return false;
    }

    object *o = game_state_find_object(ctrl->gs, ctrl->har_obj_id);
    if(o == NULL) {
        return false;
    }

    har *h = object_get_userdata(o);
    if(h == NULL) {
        return false;
    }

    if(char_cfg == NULL || !char_cfg->has_projectile_moves) {
        return false;
    }

    int enemy_range = get_enemy_range(ctrl);

    if(h->state == STATE_WALKTO || h->state == STATE_WALKFROM || h->state == STATE_CROUCHBLOCK ||
       h->state == STATE_STANDING) {
        controller_cmd(ctrl, ACT_STOP, ev);
    }

    return ai_har_execute_move_list(ctrl, o, a, char_cfg->projectile_moves, char_cfg->projectile_move_count,
                                     enemy_range, ev);
}
