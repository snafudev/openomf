/**
 * AI Event Handlers
 *
 * Per-event tactic response logic extracted from ai_har_event().
 */

#include "game/ai/ai_event.h"

#include "game/ai/ai_decision_engine.h"
#include "game/ai/ai_learning.h"
#include "game/ai/ai_state.h"
#include "game/ai/ai_tactic_engine.h"
#include "game/ai/ai_types.h"
#include "game/game_state.h"
#include "game/objects/har.h"
#include "resources/ids.h"
#include "utils/c_array_util.h"
#include "utils/log.h"

/* -------------------------------------------------------------------------
 * Internal helpers
 * ---------------------------------------------------------------------- */

static inline ai *get_ai(controller *ctrl) {
    return (ai *)ctrl->data;
}

static inline har *get_har(controller *ctrl) {
    object *o = game_state_find_object(ctrl->gs, ctrl->har_obj_id);
    return (har *)object_get_userdata(o);
}

/* -------------------------------------------------------------------------
 * Tactic cancellation
 * ---------------------------------------------------------------------- */

bool ai_event_check_cancel_tactic(controller *ctrl, har_event event) {
    ai *a = get_ai(ctrl);
    bool has_queued_tactic = (a->tactic->tactic_type > 0);

    if(!has_queued_tactic) {
        return false;
    }

    switch(event.type) {
        case HAR_EVENT_BLOCK:
        case HAR_EVENT_BLOCK_PROJECTILE:
            if(a->tactic->tactic_type != TACTIC_COUNTER && a->tactic->tactic_type != TACTIC_TURTLE &&
               a->tactic->tactic_type != TACTIC_TRIP && a->tactic->tactic_type != TACTIC_PUSH &&
               a->tactic->tactic_type != TACTIC_SPAM && a->tactic->tactic_type != TACTIC_FLY &&
               (a->tactic->tactic_type != TACTIC_GRAB || roll_chance(2)) &&
               (a->tactic->chain_hit_on == 0 || a->tactic->chain_hit_on != event.move->category)) {
                reset_tactic_state(a);
                log_debug("\033[90mReset tactic queue: EVENT_BLOCK");
                return false;
            }
            break;
        case HAR_EVENT_TAKE_HIT:
            if(a->tactic->tactic_type == TACTIC_CLOSE || a->tactic->tactic_type == TACTIC_FLY ||
               a->tactic->tactic_type == TACTIC_COUNTER ||
               (a->tactic->tactic_type == TACTIC_TURTLE && !a->pilot->att_def)) {
                reset_tactic_state(a);
                log_debug("\033[90mReset tactic queue: EVENT_TAKE_HIT");
                return false;
            }
            break;
        case HAR_EVENT_ENEMY_STUN:
            if(a->tactic->tactic_type == TACTIC_GRAB || a->tactic->tactic_type == TACTIC_CLOSE ||
               a->tactic->tactic_type == TACTIC_TRIP) {
                log_debug("Extend tactic move timer to capitalize on stun");
                a->tactic->move_timer = TACTIC_MOVE_TIMER_MAX;
            } else if(a->tactic->tactic_type != TACTIC_SHOOT) {
                reset_tactic_state(a);
                log_debug("\033[90mReset tactic queue: EVENT_ENEMY_STUN");
                return false;
            }
            break;
        default:
            break;
    }

    return true;
}

/* -------------------------------------------------------------------------
 * Per-event handlers
 * ---------------------------------------------------------------------- */

void ai_event_on_land_hit(controller *ctrl, har_event event, bool has_queued_tactic) {
    ai *a = get_ai(ctrl);
    move_stat *ms = &a->move_stats[event.move->id];

    // in the heat of the moment they might forget what they have learnt
    ai_learning_maybe_forget(a);

    if(ms->max_hit_dist == -1 || ms->last_dist > ms->max_hit_dist) {
        ms->max_hit_dist = ms->last_dist;
    }

    if(ms->min_hit_dist == -1 || ms->last_dist < ms->min_hit_dist) {
        ms->min_hit_dist = ms->last_dist;
    }

    ms->value++;
    if(ms->value > 10) {
        ms->value = 10;
    }

    a->last_move_id = event.move->id;

    if(a->tactic->chain_hit_on == event.move->category) {
        log_debug("Queueing chained tactic");
        ai_tactic_queue(ctrl, a->tactic->chain_hit_tactic);
        return;
    }

    if(has_queued_tactic || !smart_usually(a)) {
        return;
    }

    if(event.type == HAR_EVENT_LAND_HIT_PROJECTILE) {
        // we hit with a projectile
        int tacs[] = {TACTIC_FLY, TACTIC_TURTLE, TACTIC_CLOSE, TACTIC_SHOOT};
        ai_tactic_consider_list(ctrl, tacs, N_ELEMENTS(tacs));
    } else {
        // we hit with a HAR attack
        int tacs[] = {TACTIC_QUICK, TACTIC_TRIP,  TACTIC_GRAB,   TACTIC_PUSH,
                      TACTIC_CLOSE, TACTIC_SHOOT, TACTIC_TURTLE, TACTIC_SPAM};
        ai_tactic_consider_list(ctrl, tacs, N_ELEMENTS(tacs));
    }
}

void ai_event_on_enemy_block(controller *ctrl, har_event event, bool has_queued_tactic) {
    ai *a = get_ai(ctrl);
    move_stat *ms = &a->move_stats[event.move->id];

    if(a->blocked) {
        return;
    }
    a->blocked = 1;
    ms->value--;

    a->last_move_id = event.move->id;

    if(has_queued_tactic || !smart_usually(a)) {
        return;
    }

    if(event.type == HAR_EVENT_ENEMY_BLOCK_PROJECTILE) {
        // enemy blocked our projectile
        int tacs[] = {TACTIC_FLY, TACTIC_ESCAPE, TACTIC_TURTLE, TACTIC_CLOSE, TACTIC_SHOOT};
        ai_tactic_consider_list(ctrl, tacs, N_ELEMENTS(tacs));
    } else {
        // enemy blocked our HAR attack
        int tacs[] = {TACTIC_GRAB,   TACTIC_TRIP, TACTIC_PUSH,  TACTIC_COUNTER, TACTIC_TURTLE,
                      TACTIC_ESCAPE, TACTIC_FLY,  TACTIC_QUICK, TACTIC_SPAM};
        ai_tactic_consider_list(ctrl, tacs, N_ELEMENTS(tacs));
    }
}

void ai_event_on_block(controller *ctrl, har_event event, bool has_queued_tactic) {
    ai *a = get_ai(ctrl);

    if(has_queued_tactic && a->tactic->attack_on == HAR_EVENT_BLOCK) {
        // do the attack now
        log_debug("\033[94mAttempting counter move");
        a->tactic->move_timer = 0;
        return;
    }

    if(has_queued_tactic || !smart_usually(a)) {
        return;
    }

    if(event.type == HAR_EVENT_BLOCK_PROJECTILE) {
        // count this as being shot to respond to spam quicker
        a->shot++;
        // we blocked a projectile
        int tacs[] = {TACTIC_FLY, TACTIC_SHOOT, TACTIC_CLOSE, TACTIC_TURTLE};
        ai_tactic_consider_list(ctrl, tacs, N_ELEMENTS(tacs));
    } else {
        // we blocked a HAR attack
        int tacs[] = {TACTIC_TRIP,   TACTIC_PUSH,  TACTIC_TURTLE, TACTIC_GRAB,
                      TACTIC_ESCAPE, TACTIC_QUICK, TACTIC_SPAM};
        ai_tactic_consider_list(ctrl, tacs, N_ELEMENTS(tacs));
    }
}

void ai_event_on_land(controller *ctrl, har_event event, bool has_queued_tactic) {
    ai *a = get_ai(ctrl);
    (void)event;

    if(has_queued_tactic && a->tactic->attack_on == HAR_EVENT_LAND) {
        har *h = get_har(ctrl);
        if(h->state == STATE_STANDING) {
            // do the attack now
            log_debug("\033[94mAttempting landing move");
            a->tactic->move_timer = 0;
            a->tactic->attack_on = 0;
            return;
        }
    }

    a->act_timer = 0;

    if(!has_queued_tactic && smart_usually(a)) {
        int tacs[] = {TACTIC_TRIP,  TACTIC_QUICK,   TACTIC_PUSH,   TACTIC_GRAB,
                      TACTIC_SHOOT, TACTIC_COUNTER, TACTIC_TURTLE, TACTIC_CLOSE};
        ai_tactic_consider_list(ctrl, tacs, N_ELEMENTS(tacs));
    }
}

void ai_event_on_hit_wall(controller *ctrl, har_event event, bool has_queued_tactic) {
    (void)event;

    if(has_queued_tactic || !smart_usually(get_ai(ctrl))) {
        return;
    }

    int tacs[] = {TACTIC_SHOOT, TACTIC_PUSH,   TACTIC_TURTLE,  TACTIC_TRIP,
                  TACTIC_FLY,   TACTIC_ESCAPE, TACTIC_COUNTER, TACTIC_CLOSE};
    ai_tactic_consider_list(ctrl, tacs, N_ELEMENTS(tacs));
}

void ai_event_on_take_hit(controller *ctrl, har_event event, bool has_queued_tactic) {
    ai *a = get_ai(ctrl);

    if(event.move->category == CAT_CLOSE) {
        ai_learning_adjust_from_throw(a);
    } else if(event.type == HAR_EVENT_TAKE_HIT_PROJECTILE) {
        ai_learning_adjust_from_projectile(a);
    }

    if(has_queued_tactic || !smart_usually(a)) {
        return;
    }

    if(event.move->category == CAT_CLOSE) {
        // distance gaining tactics
        int tacs[] = {TACTIC_ESCAPE, TACTIC_PUSH, TACTIC_FLY};
        ai_tactic_consider_list(ctrl, tacs, N_ELEMENTS(tacs));
    } else if(event.type == HAR_EVENT_TAKE_HIT_PROJECTILE) {
        // aggressive tactics
        int tacs[] = {TACTIC_CLOSE, TACTIC_FLY, TACTIC_SHOOT, TACTIC_GRAB};
        ai_tactic_consider_list(ctrl, tacs, N_ELEMENTS(tacs));
    } else {
        // defensive tactics
        int tacs[] = {TACTIC_COUNTER, TACTIC_TURTLE, TACTIC_ESCAPE, TACTIC_PUSH,
                      TACTIC_TRIP,    TACTIC_QUICK,  TACTIC_SPAM};
        ai_tactic_consider_list(ctrl, tacs, N_ELEMENTS(tacs));
    }
}

void ai_event_on_recover(controller *ctrl, har_event event, bool has_queued_tactic) {
    (void)event;

    if(has_queued_tactic || !smart_usually(get_ai(ctrl))) {
        return;
    }

    int tacs[] = {TACTIC_SHOOT, TACTIC_COUNTER, TACTIC_TURTLE, TACTIC_ESCAPE};
    ai_tactic_consider_list(ctrl, tacs, N_ELEMENTS(tacs));
}

void ai_event_on_enemy_hazard_hit(controller *ctrl, har_event event, bool has_queued_tactic) {
    (void)event;
    ai *a = get_ai(ctrl);

    if(has_queued_tactic || !smart_usually(a)) {
        return;
    }

    har *h = get_har(ctrl);
    log_debug("HAR capitalize on hazard: %d", h->id);

    int tacs[] = {TACTIC_GRAB, TACTIC_TRIP, TACTIC_QUICK, TACTIC_CLOSE, TACTIC_SHOOT};
    ai_tactic_consider_list(ctrl, tacs, N_ELEMENTS(tacs));
}

void ai_event_on_enemy_stun(controller *ctrl, har_event event, bool has_queued_tactic) {
    (void)event;
    ai *a = get_ai(ctrl);

    if(has_queued_tactic || !smart_usually(a)) {
        return;
    }

    har *h = get_har(ctrl);
    log_debug("HAR capitalize on stun: %d", h->id);

    int tacs[] = {TACTIC_GRAB, TACTIC_CLOSE, TACTIC_TRIP, TACTIC_SHOOT};
    ai_tactic_consider_list(ctrl, tacs, N_ELEMENTS(tacs));
}
