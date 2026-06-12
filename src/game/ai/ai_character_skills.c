/**
 * AI character skills implementation
 */

#include "game/ai/ai_character_skills.h"

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

static int act_forward(const object *o) {
    return o->direction == OBJECT_FACE_RIGHT ? ACT_RIGHT : ACT_LEFT;
}

static int act_down_back(const object *o) {
    return act_back(o) | ACT_DOWN;
}

static int act_down_forward(const object *o) {
    return act_forward(o) | ACT_DOWN;
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

bool ai_char_execute_charge(controller *ctrl, const ai_char_config *char_cfg, ctrl_event **ev) {
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

    if(char_cfg != NULL && !char_cfg->has_charge_moves) {
        return false;
    }

    int enemy_range = get_enemy_range(ctrl);

    switch(h->id) {
        case HAR_JAGUAR: {
            if(enemy_range >= RANGE_MID && roll_pref(a->pilot->ap_special) && diff_scale(a)) {
                int cmds[] = {act_back(o), act_down_back(o)};
                chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
            }
            int cmds[] = {ACT_DOWN, act_down_forward(o), act_forward(o) | ACT_PUNCH};
            chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
        } break;
        case HAR_SHADOW: {
            int cmds[] = {ACT_DOWN, ACT_STOP, ACT_DOWN | ACT_PUNCH};
            chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
        } break;
        case HAR_KATANA: {
            if(roll_chance(2) && roll_pref(a->pilot->ap_low)) {
                int cmds[] = {act_down_back(o) | ACT_KICK};
                chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
            } else if(enemy_range >= RANGE_MID && roll_chance(2)) {
                int cmds[] = {ACT_DOWN, act_down_forward(o), act_forward(o) | ACT_KICK};
                chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
            } else {
                if(enemy_range >= RANGE_CLOSE && roll_pref(a->pilot->ap_special) && diff_scale(a)) {
                    int cmds[] = {act_back(o), act_down_back(o)};
                    chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
                }
                int cmds[] = {ACT_DOWN, act_down_forward(o), act_forward(o) | ACT_PUNCH};
                chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
            }
        } break;
        case HAR_FLAIL: {
            if(enemy_range > RANGE_MID && roll_pref(a->pilot->ap_special) && diff_scale(a)) {
                int cmds[] = {ACT_DOWN, act_down_back(o), act_back(o), ACT_STOP, act_back(o) | ACT_PUNCH};
                chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
            } else {
                int cmds[] = {act_back(o), ACT_STOP, act_back(o) | ACT_PUNCH};
                chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
            }
        } break;
        case HAR_THORN: {
            int cmds[] = {act_forward(o), ACT_STOP, act_forward(o) | ACT_PUNCH};
            chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
        } break;
        case HAR_PYROS: {
            if(enemy_range > RANGE_MID && roll_pref(a->pilot->ap_special) && diff_scale(a)) {
                int cmds[] = {act_forward(o), ACT_STOP};
                chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
            }
            int cmds[] = {act_forward(o), ACT_STOP, act_forward(o) | ACT_PUNCH};
            chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
        } break;
        case HAR_ELECTRA: {
            if(enemy_range >= RANGE_MID && roll_pref(a->pilot->ap_special) && diff_scale(a)) {
                int cmds[] = {ACT_DOWN, act_down_forward(o)};
                chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
            }
            int cmds[] = {act_forward(o), ACT_STOP, act_forward(o) | ACT_PUNCH};
            chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
        } break;
        case HAR_CHRONOS: {
            if(enemy_range >= RANGE_MID && roll_pref(a->pilot->ap_special) && diff_scale(a)) {
                int cmds[] = {ACT_DOWN, ACT_PUNCH};
                chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
                int tacs[] = {TACTIC_GRAB, TACTIC_PUSH, TACTIC_SHOOT, TACTIC_SPAM, TACTIC_TRIP};
                ai_tactic_consider_list(ctrl, tacs, N_ELEMENTS(tacs));
            } else {
                int cmds[] = {act_down_back(o) | ACT_KICK};
                chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
            }
        } break;
        case HAR_SHREDDER: {
            if(enemy_range > RANGE_MID && roll_pref(a->pilot->att_jump) && diff_scale(a)) {
                int cmds[] = {ACT_DOWN, ACT_STOP, ACT_DOWN | ACT_KICK};
                chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
            } else {
                if(enemy_range >= RANGE_MID && roll_pref(a->pilot->ap_special) && diff_scale(a)) {
                    int cmds[] = {act_back(o), act_down_back(o)};
                    chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
                }
                int cmds[] = {ACT_DOWN, act_down_forward(o), act_forward(o) | ACT_PUNCH};
                chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
            }
        } break;
        case HAR_GARGOYLE: {
            if(enemy_range > RANGE_MID && roll_pref(a->pilot->att_jump) && diff_scale(a)) {
                int cmds[] = {act_forward(o), ACT_STOP, act_forward(o), ACT_PUNCH};
                chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
            } else {
                if(enemy_range >= RANGE_MID && roll_pref(a->pilot->ap_special) && diff_scale(a)) {
                    int cmds[] = {act_back(o), act_down_back(o)};
                    chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
                }
                int cmds[] = {ACT_DOWN, act_down_forward(o), act_forward(o), ACT_PUNCH};
                chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
            }
        } break;
        default:
            return false;
    }

    return true;
}

bool ai_char_execute_push(controller *ctrl, const ai_char_config *char_cfg, ctrl_event **ev) {
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

    if(char_cfg != NULL && !char_cfg->has_push_moves) {
        return false;
    }

    int enemy_range = get_enemy_range(ctrl);

    switch(h->id) {
        case HAR_JAGUAR: {
            int cmds[] = {act_back(o) | ACT_KICK};
            chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
        } break;
        case HAR_KATANA: {
            if(enemy_range >= RANGE_CLOSE && roll_pref(a->pilot->ap_special) && diff_scale(a)) {
                int cmds[] = {act_back(o), act_down_back(o)};
                chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
            }
            int cmds[] = {ACT_DOWN, act_down_forward(o), act_forward(o) | ACT_PUNCH};
            chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
        } break;
        case HAR_FLAIL: {
            if(roll_chance(3)) {
                int cmds[] = {ACT_DOWN, ACT_KICK};
                chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
            } else {
                int cmds[] = {ACT_DOWN, ACT_PUNCH};
                chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
            }
        } break;
        case HAR_THORN: {
            if(enemy_range >= RANGE_CLOSE && roll_pref(a->pilot->ap_special) && diff_scale(a)) {
                int cmds[] = {act_back(o), act_down_back(o)};
                chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
            }
            int cmds[] = {ACT_DOWN, act_down_forward(o), act_forward(o) | ACT_KICK};
            chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
        } break;
        case HAR_PYROS: {
            int cmds[] = {ACT_DOWN, ACT_PUNCH};
            chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
        } break;
        case HAR_ELECTRA: {
            int cmds[] = {ACT_DOWN, act_down_forward(o), act_forward(o) | ACT_PUNCH};
            chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
        } break;
        case HAR_NOVA: {
            if(diff_scale(a)) {
                int cmds[] = {ACT_DOWN, ACT_STOP, ACT_DOWN | ACT_PUNCH};
                chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
            } else {
                int cmds[] = {act_back(o) | ACT_KICK};
                chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
            }
        } break;
        default:
            return false;
    }

    return true;
}

bool ai_char_execute_trip(controller *ctrl, const ai_char_config *char_cfg, ctrl_event **ev) {
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

bool ai_char_execute_projectile(controller *ctrl, const ai_char_config *char_cfg, ctrl_event **ev) {
    if(ctrl == NULL) {
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

    if(char_cfg != NULL && !char_cfg->has_projectile_moves) {
        return false;
    }

    int enemy_range = get_enemy_range(ctrl);

    if(h->state == STATE_WALKTO || h->state == STATE_WALKFROM || h->state == STATE_CROUCHBLOCK) {
        controller_cmd(ctrl, ACT_STOP, ev);
    }

    switch(h->id) {
        case HAR_JAGUAR:
        case HAR_ELECTRA:
        case HAR_SHREDDER: {
            if(h->id == HAR_SHREDDER && enemy_range > RANGE_MID) {
                return false;
            }
            int cmds[] = {ACT_DOWN, act_down_back(o), act_back(o) | ACT_PUNCH};
            chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
            return true;
        }
        case HAR_SHADOW: {
            int cmds[] = {ACT_DOWN, act_down_back(o), act_back(o)};
            chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
            if(roll_chance(2)) {
                int cmds2[] = {ACT_PUNCH};
                chain_controller_cmd(ctrl, cmds2, N_ELEMENTS(cmds2), ev);
            } else {
                int cmds2[] = {ACT_KICK};
                chain_controller_cmd(ctrl, cmds2, N_ELEMENTS(cmds2), ev);
            }
            return true;
        }
        case HAR_CHRONOS: {
            if(enemy_range < RANGE_MID || enemy_is_stunned_or_stasis(ctrl)) {
                return false;
            }
            int cmds[] = {ACT_DOWN, act_down_back(o), act_back(o) | ACT_PUNCH};
            chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
            return true;
        }
        case HAR_NOVA: {
            controller_cmd(ctrl, ACT_DOWN, ev);
            if(roll_chance(3) && enemy_range >= RANGE_MID) {
                int cmds[] = {ACT_DOWN, act_down_back(o), act_back(o) | ACT_PUNCH};
                chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
            } else {
                int cmds[] = {ACT_DOWN, act_down_forward(o), act_forward(o) | ACT_PUNCH};
                chain_controller_cmd(ctrl, cmds, N_ELEMENTS(cmds), ev);
            }
            return true;
        }
        default:
            return false;
    }
}
