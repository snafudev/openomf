/**
 * AI Decision Engine Implementation
 *
 * Extracted pure decision functions from ai_controller.c
 */

#include "game/ai/ai_decision_engine.h"
#include "utils/random.h"

bool roll_chance(int roll_x) {
    return roll_x <= 1 ? true : rand_int(roll_x) == 1;
}

bool roll_pref(int pref_val) {
    int rand_roll = rand_int(200);
    int pref_thresh = pref_val + 100;
    return rand_roll <= pref_thresh;
}

bool smart_usually(const ai *a) {
    if (a->difficulty >= 6) {
        // at highest difficulty 92% chance to be smart
        return !roll_chance(12);
    } else if (a->difficulty >= 3) {
        return roll_chance(7 - a->difficulty);
    } else {
        return false;
    }
}

bool dumb_usually(const ai *a) {
    if (a->difficulty == 1) {
        // at lowest difficulty 92% chance to be dumb
        return !roll_chance(12);
    }
    if (a->difficulty <= 2) {
        return roll_chance(a->difficulty + 1);
    } else {
        return false;
    }
}

bool smart_sometimes(const ai *a) {
    if (a->difficulty >= 2) {
        return roll_chance(10 - a->difficulty);
    } else {
        return false;
    }
}

bool dumb_sometimes(const ai *a) {
    if (a->difficulty <= 2) {
        return roll_chance(a->difficulty + 2);
    } else {
        return false;
    }
}

bool diff_scale(const ai *a) {
    int roll = rand_int(36);
    return roll <= (a->difficulty * a->difficulty);
}

bool learning_moment(const ai *a) {
    float roll = (float)rand_int(diff_scale(a) ? 8 : 15);
    return roll <= a->pilot->learning;
}

bool forgetful(const ai *a) {
    float roll = (float)rand_int(diff_scale(a) ? 3 : 2);
    return roll <= a->pilot->forget;
}
