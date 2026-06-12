/**
 * AI Movement Module implementation
 */

#include "game/ai/ai_movement.h"
#include "game/ai/ai_decision_engine.h"
#include "game/ai/ai_utils.h"
#include "game/common_defines.h"

int ai_movement_decide(const ai *a, int enemy_range, bool is_wallhugging, int har_id) {
    int move_dir = MOVE_DIR_STILL;

    if(!is_wallhugging && enemy_range == RANGE_CRAMPED) {
        // we are face-hugging already so no need to go forward
        move_dir = roll_pref(a->pilot->pref_back) ? MOVE_DIR_BACK : MOVE_DIR_STILL;
    } else if(roll_pref(a->pilot->pref_fwd)) {
        // pilot prefers forward
        move_dir = MOVE_DIR_FWD;
    } else if(!is_wallhugging && roll_pref(a->pilot->pref_back)) {
        // pilot prefers backward
        move_dir = MOVE_DIR_BACK;
    } else if((har_id == HAR_FLAIL || har_id == HAR_THORN || har_id == HAR_NOVA) && smart_usually(a)) {
        // brawlers are more likely to face-hug
        move_dir = MOVE_DIR_FWD;
    }

    return move_dir;
}

int ai_movement_jump_chance(const ai *a) {
    int jump_chance = 100;
    if(roll_pref(a->pilot->pref_jump)) {
        jump_chance -= 10;
    }
    if(diff_scale(a)) {
        jump_chance -= 10;
    }
    return jump_chance;
}
