/**
 * AI state reset helpers implementation
 */

#include "game/ai/ai_state.h"
#include "game/ai/ai_config_loader.h"
#include "game/ai/ai_core_config.h"
#include "game/common_defines.h"
#include "utils/random.h"

static void apply_special_pilot_profile_overrides(sd_pilot *pilot) {
    if(pilot == NULL) {
        return;
    }

    if(pilot->pilot_id == PILOT_KREISSACK) {
        pilot->att_normal = 55;
        pilot->att_hyper = 90;
        pilot->att_jump = 35;
        pilot->att_def = 18;
        pilot->att_sniper = 30;
        pilot->ap_throw = 100;
        pilot->ap_special = 140;
        pilot->ap_jump = 80;
        pilot->ap_high = 65;
        pilot->ap_low = 45;
        pilot->ap_middle = 55;
        pilot->pref_jump = 20;
        pilot->pref_fwd = 40;
        pilot->pref_back = -8;
        pilot->learning = 4.0f;
        pilot->forget = 0.18f;
    } else if(pilot->pilot_id == PILOT_RAVEN) {
        pilot->att_normal = 50;
        pilot->att_hyper = 65;
        pilot->att_jump = 25;
        pilot->att_def = 10;
        pilot->att_sniper = 35;
        pilot->ap_throw = 100;
        pilot->ap_special = 120;
        pilot->ap_jump = 20;
        pilot->ap_high = 10;
        pilot->ap_low = 10;
        pilot->ap_middle = 15;
        pilot->pref_jump = 10;
        pilot->pref_fwd = 35;
        pilot->pref_back = -12;
        pilot->learning = 3.0f;
        pilot->forget = 0.35f;
    }

    if(pilot->har_id == HAR_NOVA && pilot->pilot_id != PILOT_RAVEN && pilot->pilot_id != PILOT_KREISSACK) {
        pilot->att_normal = 30;
        pilot->att_hyper = 60;
        pilot->att_jump = 20;
        pilot->att_def = 12;
        pilot->att_sniper = 30;
        pilot->ap_throw = 100;
        pilot->ap_special = 130;
        pilot->ap_jump = 60;
        pilot->ap_high = 45;
        pilot->ap_low = 30;
        pilot->ap_middle = 35;
        pilot->pref_jump = 12;
        pilot->pref_fwd = 35;
        pilot->pref_back = -7;
        pilot->learning = 3.2f;
        pilot->forget = 0.45f;
    }
}

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
            pilot->att_normal = 50;
            pilot->att_hyper = 65;
            pilot->att_jump = 25;
            pilot->att_def = 10;
            pilot->att_sniper = 35;
            pilot->ap_throw = 100;
            pilot->ap_special = 120;
            pilot->ap_jump = 20;
            pilot->ap_high = 10;
            pilot->ap_low = 10;
            pilot->ap_middle = 15;
            pilot->pref_jump = 10;
            pilot->pref_fwd = 35;
            pilot->pref_back = -12;
            pilot->learning = 3.0f;
            pilot->forget = 0.35f;
            break;
        case 10:
            pilot->att_normal = 55;
            pilot->att_hyper = 90;
            pilot->att_jump = 35;
            pilot->att_def = 18;
            pilot->att_sniper = 30;
            pilot->ap_throw = 100;
            pilot->ap_special = 140;
            pilot->ap_jump = 80;
            pilot->ap_high = 65;
            pilot->ap_low = 45;
            pilot->ap_middle = 55;
            pilot->pref_jump = 20;
            pilot->pref_fwd = 40;
            pilot->pref_back = -8;
            pilot->learning = 4.0f;
            pilot->forget = 0.18f;
            break;
    }
}

void reset_pilot_personality(sd_pilot *pilot) {
    if(ai_config_load_pilot_personality(pilot)) {
        apply_special_pilot_profile_overrides(pilot);
        return;
    }

    reset_pilot_personality_defaults(pilot);
    apply_special_pilot_profile_overrides(pilot);
}

void reset_act_timer(ai *a) {
    const ai_core_config *config = ai_core_config_get_for_difficulty(a->difficulty);
    a->act_timer = config->base_act_timer - (a->difficulty * 2) - rand_int(3);
}
