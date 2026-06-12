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
            controller_cmd(ctrl, ACT_STOP, ev);
            return true;
        case STATE_STANDING:
            return true;
        default:
            return false;
    }
}

static bool eval_move_conditions(controller *ctrl, ai *a, ai_move_condition conditions) {
    if((conditions & MOVE_COND_HIGH_DIFFICULTY) && !diff_scale(a)) {
        return false;
    }
    if((conditions & MOVE_COND_SPECIAL_PREF) && !roll_pref(a->pilot->ap_special)) {
        return false;
    }
    if((conditions & MOVE_COND_LOW_PREFERRED) && !roll_pref(a->pilot->ap_low)) {
        return false;
    }
    if((conditions & MOVE_COND_JUMP_PREFERRED) && !roll_pref(a->pilot->att_jump)) {
        return false;
    }
    if((conditions & MOVE_COND_ROLL_D2) && !roll_chance(2)) {
        return false;
    }
    if((conditions & MOVE_COND_ROLL_D3) && !roll_chance(3)) {
        return false;
    }
    if((conditions & MOVE_COND_ROLL_D4) && !roll_chance(4)) {
        return false;
    }
    if((conditions & MOVE_COND_ROLL_D10) && !roll_chance(10)) {
        return false;
    }
    if((conditions & MOVE_COND_ROLL_D20) && !roll_chance(20)) {
        return false;
    }
    if((conditions & MOVE_COND_ENEMY_NOT_STUNNED) && enemy_is_stunned_or_stasis(ctrl)) {
        return false;
    }
    if((conditions & MOVE_COND_ENEMY_STUNNED) && !enemy_is_stunned_or_stasis(ctrl)) {
        return false;
    }
    return true;
}

static bool ai_har_execute_move_list(controller *ctrl, object *o, ai *a, const ai_move_def *moves,
                                      uint8_t move_count, int enemy_range, ctrl_event **ev) {
    for(uint8_t i = 0; i < move_count; i++) {
        const ai_move_def *move = &moves[i];
        if(enemy_range < (int)move->range_min) {
            continue;
        }
        if(enemy_range > (int)move->range_max) {
            continue;
        }
        if(!eval_move_conditions(ctrl, a, move->conditions)) {
            continue;
        }

        for(uint8_t j = 0; j < move->input_count; j++) {
            int resolved = ai_resolve_input(move->inputs[j], o->direction);
            controller_cmd(ctrl, resolved, ev);
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

    return false;
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

    (void)char_cfg;

    object *o = game_state_find_object(ctrl->gs, ctrl->har_obj_id);
    if(o == NULL) {
        return false;
    }

    har *h = object_get_userdata(o);
    if(h == NULL || !can_start_ground_attack(ctrl, h, ev)) {
        return false;
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

    if(h->state == STATE_WALKTO || h->state == STATE_WALKFROM || h->state == STATE_CROUCHBLOCK) {
        controller_cmd(ctrl, ACT_STOP, ev);
    }

    return ai_har_execute_move_list(ctrl, o, a, char_cfg->projectile_moves, char_cfg->projectile_move_count,
                                     enemy_range, ev);
}
