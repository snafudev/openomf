/**
 * AI state reset helpers implementation
 */

#include "game/ai/ai_state.h"
#include "game/ai/ai_config_loader.h"
#include "game/ai/ai_core_config.h"
#include "utils/random.h"

void reset_tactic_state(ai *a) {
    a->tactic->last_tactic = a->tactic->tactic_type ? a->tactic->tactic_type : 0;
    a->tactic->tactic_type = 0;
    a->tactic->move_type = 0;
    a->tactic->move_timer = 0;
    a->tactic->attack_type = 0;
    a->tactic->attack_id = 0;
    a->tactic->attack_timer = 0;
    a->tactic->attack_on = 0;
    a->tactic->chain_hit_on = 0;
    a->tactic->chain_hit_tactic = 0;
}

static void reset_pilot_personality_defaults(sd_pilot *pilot) {
    switch(pilot->pilot_id) {
        case 0:
            pilot->att_normal = 30;
            pilot->att_hyper = 10;
            pilot->att_jump = 10;
            pilot->att_sniper = 20;
            pilot->ap_throw = 100;
            pilot->ap_special = 75;
            pilot->ap_jump = -30;
            pilot->ap_high = -50;
            pilot->ap_low = -50;
            pilot->ap_middle = -50;
            pilot->pref_jump = -10;
            pilot->pref_fwd = 30;
            pilot->pref_back = 10;
            pilot->learning = 1.5f;
            pilot->forget = 0.25f;
            break;
        case 1:
            pilot->att_normal = 40;
            pilot->att_hyper = 60;
            pilot->att_jump = 30;
            pilot->ap_throw = 25;
            pilot->ap_special = 20;
            pilot->ap_high = -75;
            pilot->ap_low = 75;
            pilot->ap_middle = 50;
            pilot->pref_jump = 6;
            pilot->pref_fwd = 20;
            pilot->pref_back = -9;
            pilot->learning = 1.0f;
            pilot->forget = 0.4f;
            break;
        case 2:
            pilot->att_normal = 20;
            pilot->att_hyper = 30;
            pilot->att_jump = 40;
            pilot->att_sniper = 20;
            pilot->ap_throw = -50;
            pilot->ap_special = -50;
            pilot->ap_jump = -50;
            pilot->ap_high = 50;
            pilot->ap_low = 50;
            pilot->ap_middle = 50;
            pilot->pref_jump = 8;
            pilot->pref_fwd = 30;
            pilot->pref_back = -3;
            pilot->learning = 0.9f;
            pilot->forget = 0.1f;
            break;
        case 3:
            pilot->att_normal = 20;
            pilot->att_hyper = 15;
            pilot->att_def = 30;
            pilot->att_sniper = 10;
            pilot->ap_throw = 30;
            pilot->ap_special = 25;
            pilot->ap_jump = 30;
            pilot->ap_low = -25;
            pilot->ap_middle = 20;
            pilot->pref_jump = 2;
            pilot->pref_fwd = 10;
            pilot->pref_back = -10;
            pilot->learning = 2.5f;
            pilot->forget = 0.35f;
            break;
        case 4:
            pilot->att_normal = 15;
            pilot->att_hyper = 5;
            pilot->att_jump = 5;
            pilot->att_def = 20;
            pilot->att_sniper = 4;
            pilot->ap_throw = 75;
            pilot->ap_special = 50;
            pilot->ap_jump = -50;
            pilot->ap_high = -50;
            pilot->ap_low = -50;
            pilot->ap_middle = -50;
            pilot->pref_jump = -20;
            pilot->pref_fwd = 10;
            pilot->pref_back = 10;
            pilot->learning = 2.0f;
            pilot->forget = 0.2f;
            break;
        case 5:
            pilot->att_normal = 20;
            pilot->att_hyper = 10;
            pilot->att_jump = 20;
            pilot->att_def = 30;
            pilot->att_sniper = 45;
            pilot->ap_throw = -50;
            pilot->ap_special = 75;
            pilot->ap_jump = 100;
            pilot->ap_high = -50;
            pilot->ap_low = 100;
            pilot->ap_middle = -50;
            pilot->pref_fwd = 20;
            pilot->learning = 1.2f;
            pilot->forget = 0.07f;
            break;
        case 6:
            pilot->att_normal = 40;
            pilot->att_hyper = 5;
            pilot->att_jump = 5;
            pilot->att_def = 50;
            pilot->att_sniper = 7;
            pilot->ap_special = 50;
            pilot->ap_jump = -50;
            pilot->ap_high = 50;
            pilot->ap_low = 50;
            pilot->ap_middle = 50;
            pilot->pref_jump = 2;
            pilot->pref_fwd = 10;
            pilot->pref_back = -10;
            pilot->learning = 2.5f;
            pilot->forget = 0.05f;
            break;
        case 7:
            pilot->att_normal = 40;
            pilot->att_hyper = 60;
            pilot->att_jump = 30;
            pilot->ap_throw = 25;
            pilot->ap_special = 20;
            pilot->ap_jump = 100;
            pilot->ap_high = -75;
            pilot->ap_low = 75;
            pilot->ap_middle = 50;
            pilot->pref_jump = 40;
            pilot->pref_fwd = 40;
            pilot->pref_back = -9;
            pilot->learning = 3.0f;
            pilot->forget = 0.15f;
            break;
        case 8:
            pilot->att_normal = 50;
            pilot->att_hyper = 5;
            pilot->att_jump = 5;
            pilot->att_def = 5;
            pilot->att_sniper = 5;
            pilot->ap_throw = 25;
            pilot->ap_special = -50;
            pilot->ap_jump = -50;
            pilot->ap_high = -25;
            pilot->ap_low = 10;
            pilot->ap_middle = -50;
            pilot->pref_jump = -10;
            pilot->pref_back = 10;
            pilot->learning = 0.7f;
            pilot->forget = 0.2f;
            break;
        case 9:
            pilot->att_normal = 30;
            pilot->att_hyper = 40;
            pilot->ap_throw = 100;
            pilot->ap_special = 100;
            pilot->ap_jump = 100;
            pilot->ap_high = 100;
            pilot->ap_low = 100;
            pilot->ap_middle = 100;
            pilot->pref_jump = 12;
            pilot->pref_fwd = 30;
            pilot->pref_back = -7;
            pilot->learning = 3.0f;
            pilot->forget = 0.5f;
            break;
        case 10:
            pilot->att_normal = 30;
            pilot->att_hyper = 75;
            pilot->att_sniper = 25;
            pilot->ap_throw = 100;
            pilot->ap_special = 100;
            pilot->learning = 3.0f;
            pilot->forget = 0.25f;
            break;
    }
}

void reset_pilot_personality(sd_pilot *pilot) {
    if(ai_config_load_pilot_personality(pilot)) {
        return;
    }

    reset_pilot_personality_defaults(pilot);
}

void reset_act_timer(ai *a) {
    a->act_timer = ai_core_config_get()->base_act_timer - (a->difficulty * 2) - rand_int(3);
}
