/**
 * AI Learning Module
 *
 * Pilot personality adaptation extracted from ai_har_event().
 */

#include "game/ai/ai_learning.h"
#include "game/ai/ai_decision_engine.h"
#include "game/ai/ai_state.h"
#include "formats/pilot.h"
#include "utils/log.h"

void ai_learning_adjust_from_throw(ai *a) {
    a->thrown++;
    if(learning_moment(a) && a->thrown >= MAX_TIMES_THROWN) {
        log_debug("AI adjusting in response to repeated throws.");
        sd_pilot *pilot = a->pilot;
        // avoid defensive tactics
        if(pilot->att_def > 90) {
            pilot->att_def = 10;
        }
        // favor sniper tactics
        if(pilot->att_sniper < 90) {
            pilot->att_sniper += 10;
        }
        // favor jumping tactics
        if(pilot->att_jump < 90) {
            pilot->att_jump += 10;
        }
        // favor jumping movement
        if(pilot->pref_jump < 90) {
            pilot->pref_jump += 10;
        }
        // favor backwards movement
        if(pilot->pref_back < 90) {
            pilot->pref_back += 10;
        }
        if(pilot->pref_fwd > 90) {
            pilot->pref_fwd -= 10;
        }
    }
}

void ai_learning_adjust_from_projectile(ai *a) {
    a->shot++;
    if(learning_moment(a) && a->shot >= MAX_TIMES_SHOT) {
        log_debug("AI adjusting in response to repeated projectiles.");
        sd_pilot *pilot = a->pilot;
        // avoid defensive tactics
        if(pilot->att_def > 90) {
            pilot->att_def = 10;
        }
        // favor shooting tactics
        if(pilot->att_sniper < 90) {
            pilot->att_sniper += 10;
        }
        // favor aggressive tactics
        if(pilot->att_hyper < 90) {
            pilot->att_hyper += 10;
        }
        // favor jumping tactics
        if(pilot->att_jump < 80) {
            pilot->att_jump += 20;
        }
        if(pilot->pref_jump < 80) {
            pilot->pref_jump += 20;
        }
        // favor forwards movement
        if(pilot->pref_fwd < 90) {
            pilot->pref_fwd += 10;
        }
        if(pilot->pref_back > 90) {
            pilot->pref_back -= 10;
        }
    }
}

bool ai_learning_maybe_forget(ai *a) {
    if(roll_chance(2) && forgetful(a)) {
        reset_pilot_personality(a->pilot);
        a->blocked = 0;
        a->thrown = 0;
        a->shot = 0;
        return true;
    }
    return false;
}
