/**
 * AI utility helpers implementation
 */

#include "game/ai/ai_utils.h"
#include "game/game_state.h"
#include "game/game_player.h"
#include "game/objects/har.h"
#include "utils/log.h"
#include <math.h>

int ai_enemy_range_from_positions(float self_x, float enemy_x) {
    int range_units = (int)(fabsf(enemy_x - self_x) / 30.0f);
    switch(range_units) {
        case 0:
        case 1:
            return RANGE_CRAMPED;
        case 2:
            return RANGE_CLOSE;
        case 3:
        case 4:
            return RANGE_MID;
        default:
            return RANGE_FAR;
    }
}

bool ai_enemy_is_stunned_or_stasis(int enemy_state, int stasis_ticks) {
    return enemy_state == STATE_STUNNED || stasis_ticks > 0;
}

int get_enemy_range(const controller *ctrl) {
    object *o = game_state_find_object(ctrl->gs, ctrl->har_obj_id);
    har *h = object_get_userdata(o);
    object *o_enemy =
        game_state_find_object(ctrl->gs, game_state_get_player(ctrl->gs, h->player_id == 1 ? 0 : 1)->har_obj_id);

    return ai_enemy_range_from_positions(o->pos.x, o_enemy->pos.x);
}

bool enemy_is_stunned_or_stasis(const controller *ctrl) {
    object *o = game_state_find_object(ctrl->gs, ctrl->har_obj_id);
    har *h = object_get_userdata(o);
    object *o_enemy =
        game_state_find_object(ctrl->gs, game_state_get_player(ctrl->gs, h->player_id == 1 ? 0 : 1)->har_obj_id);

    har *h_enemy = object_get_userdata(o_enemy);

    return ai_enemy_is_stunned_or_stasis(h_enemy->state, h_enemy->in_stasis_ticks);
}

bool is_special_move(const af_move *move) {
    if(str_equal_c(&move->move_string, "K") || str_equal_c(&move->move_string, "K1") ||
       str_equal_c(&move->move_string, "K2") || str_equal_c(&move->move_string, "K3") ||
       str_equal_c(&move->move_string, "K4") || str_equal_c(&move->move_string, "K6") ||
       str_equal_c(&move->move_string, "P") || str_equal_c(&move->move_string, "P1") ||
       str_equal_c(&move->move_string, "P2") || str_equal_c(&move->move_string, "P3") ||
       str_equal_c(&move->move_string, "P4") || str_equal_c(&move->move_string, "P6")) {
        return false;
    }
    return true;
}

bool har_has_projectiles(int har_id) {
    switch(har_id) {
        case HAR_JAGUAR:
        case HAR_SHADOW:
        case HAR_ELECTRA:
        case HAR_SHREDDER:
        case HAR_CHRONOS:
        case HAR_NOVA:
            return true;
    }

    return false;
}

bool har_has_charge(int har_id) {
    switch(har_id) {
        case HAR_JAGUAR:
        case HAR_SHADOW:
        case HAR_KATANA:
        case HAR_FLAIL:
        case HAR_THORN:
        case HAR_PYROS:
        case HAR_ELECTRA:
        case HAR_SHREDDER:
        case HAR_CHRONOS:
        case HAR_GARGOYLE:
            return true;
    }
    return false;
}

bool har_has_push(int har_id) {
    switch(har_id) {
        case HAR_JAGUAR:
        case HAR_KATANA:
        case HAR_FLAIL:
        case HAR_THORN:
        case HAR_PYROS:
        case HAR_ELECTRA:
        case HAR_NOVA:
            return true;
    }
    return false;
}

int char_to_act(str *ch, int direction, int *position) {
    int action = 0;
    switch(str_at(ch, *position)) {
        case '8':
            action = ACT_UP;
            break;
        case '2':
            action = ACT_DOWN;
            break;
        case '6':
            if(direction == OBJECT_FACE_LEFT) {
                action = ACT_LEFT;
            } else {
                action = ACT_RIGHT;
            }
            break;
        case '4':
            if(direction == OBJECT_FACE_LEFT) {
                action = ACT_RIGHT;
            } else {
                action = ACT_LEFT;
            }
            break;
        case '7':
            if(direction == OBJECT_FACE_LEFT) {
                action = ACT_UP | ACT_RIGHT;
            } else {
                action = ACT_UP | ACT_LEFT;
            }
            break;
        case '9':
            if(direction == OBJECT_FACE_LEFT) {
                action = ACT_UP | ACT_LEFT;
            } else {
                action = ACT_UP | ACT_RIGHT;
            }
            break;
        case '1':
            if(direction == OBJECT_FACE_LEFT) {
                action = ACT_DOWN | ACT_RIGHT;
            } else {
                action = ACT_DOWN | ACT_LEFT;
            }
            break;
        case '3':
            if(direction == OBJECT_FACE_LEFT) {
                action = ACT_DOWN | ACT_LEFT;
            } else {
                action = ACT_DOWN | ACT_RIGHT;
            }
            break;
        case 'K':
            action = ACT_KICK;
            break;
        case 'P':
            action = ACT_PUNCH;
            break;
        case '5':
            return ACT_STOP;
        default:
            break;
    }

    for(int i = 0; i < 2 && (*position) > 0; i++) {
        switch(str_at(ch, (*position) - 1)) {
            case 'K':
                log_debug("adding in extra kick to %d at string %s position %d", action, str_c(ch), *position - 1);
                action |= ACT_KICK;
                (*position)--;
                break;
            case 'P':
                log_debug("adding in extra punch to %d at string %s position %d", action, str_c(ch), *position - 1);
                action |= ACT_PUNCH;
                (*position)--;
                break;
            default:
                return action;
        }
    }

    return action;
}
