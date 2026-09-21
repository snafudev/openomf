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

static int ai_projectile_cooldown_for_difficulty(int difficulty) {
    int cooldown = 18 - (difficulty * 2);
    if(cooldown < 4) {
        cooldown = 4;
    }
    if(cooldown > 18) {
        cooldown = 18;
    }
    return cooldown;
}

static int ai_pressure_cooldown_for_difficulty(int difficulty) {
    int cooldown = 20 - (difficulty * 2);
    if(cooldown < 4) {
        cooldown = 4;
    }
    if(cooldown > 20) {
        cooldown = 20;
    }
    return cooldown;
}

int ai_projectile_max_streak(int difficulty) {
    int max = 2 + (difficulty / 3);
    if(max < 2) {
        return 2;
    }
    if(max > 5) {
        return 5;
    }
    return max;
}

int ai_pressure_max_streak(int difficulty) {
    int max = 2 + (difficulty / 4);
    if(max < 2) {
        return 2;
    }
    if(max > 4) {
        return 4;
    }
    return max;
}

void ai_projectile_tick(ai *a) {
    if(a == NULL) {
        return;
    }

    if(a->projectile_cooldown > 0) {
        a->projectile_cooldown--;
    }

    if(a->projectile_cooldown == 0) {
        a->projectile_streak = 0;
    }
}

void ai_pressure_tick(ai *a) {
    if(a == NULL) {
        return;
    }

    if(a->pressure_cooldown > 0) {
        a->pressure_cooldown--;
    }

    if(a->pressure_cooldown == 0) {
        a->pressure_streak = 0;
    }
}

bool ai_projectile_is_allowed(const ai *a) {
    if(a == NULL) {
        return false;
    }

    if(a->projectile_cooldown > 0) {
        return false;
    }

    return a->projectile_streak < ai_projectile_max_streak(a->difficulty);
}

bool ai_pressure_is_allowed(const ai *a) {
    if(a == NULL) {
        return false;
    }

    if(a->pressure_cooldown > 0) {
        return false;
    }

    return a->pressure_streak < ai_pressure_max_streak(a->difficulty);
}

void ai_projectile_use(ai *a) {
    if(a == NULL) {
        return;
    }

    if(a->projectile_cooldown > 0) {
        return;
    }

    a->projectile_streak++;
    a->projectile_cooldown = ai_projectile_cooldown_for_difficulty(a->difficulty);
    if(a->projectile_streak >= ai_projectile_max_streak(a->difficulty)) {
        a->projectile_cooldown += 2;
    }
}

void ai_pressure_use(ai *a) {
    if(a == NULL) {
        return;
    }

    if(a->pressure_cooldown > 0) {
        return;
    }

    a->pressure_streak++;
    a->pressure_cooldown = ai_pressure_cooldown_for_difficulty(a->difficulty);
    if(a->pressure_streak >= ai_pressure_max_streak(a->difficulty)) {
        a->pressure_cooldown += 2;
    }
}

bool ai_learning_maybe_forget(ai *a) {
    if(roll_chance(2) && forgetful(a)) {
        reset_pilot_personality(a->pilot);
        a->blocked = 0;
        a->thrown = 0;
        a->shot = 0;
        a->projectile_streak = 0;
        a->projectile_cooldown = 0;
        a->pressure_streak = 0;
        a->pressure_cooldown = 0;
        return true;
    }
    return false;
}
