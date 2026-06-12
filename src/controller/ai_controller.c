#include "controller/ai_controller.h"
#include "game/ai/ai_decision_engine.h"
#include "game/ai/ai_har_skills.h"
#include "game/ai/ai_event.h"
#include "game/ai/ai_learning.h"
#include "game/ai/ai_movement.h"
#include "game/ai/ai_state.h"
#include "game/ai/ai_skills_config_loader.h"
#include "game/ai/ai_tactic_engine.h"
#include "game/ai/ai_utils.h"
#include "game/ai/ai_move_selector.h"
#include "game/ai/ai_core_config.h"
#include "controller/controller.h"
#include "formats/pilot.h"
#include "game/game_state.h"
#include "game/objects/har.h"
#include "game/objects/projectile.h"
#include "game/objects/scrap.h"
#include "game/scenes/arena.h"
#include "resources/af_loader.h"
#include "resources/ids.h"
#include "utils/allocator.h"
#include "utils/c_array_util.h"
#include "utils/log.h"
#include "utils/random.h"
#include "utils/vec.h"
#include <math.h>



#define BACK (o->direction == OBJECT_FACE_RIGHT ? ACT_LEFT : ACT_RIGHT)
#define DOWNBACK (o->direction == OBJECT_FACE_RIGHT ? ACT_LEFT : ACT_RIGHT) | ACT_DOWN
#define UPBACK (o->direction == OBJECT_FACE_RIGHT ? ACT_LEFT : ACT_RIGHT) | ACT_UP

#define FORWARD (o->direction == OBJECT_FACE_RIGHT ? ACT_RIGHT : ACT_LEFT)
#define DOWNFORWARD (o->direction == OBJECT_FACE_RIGHT ? ACT_RIGHT : ACT_LEFT) | ACT_DOWN
#define UPFORWARD (o->direction == OBJECT_FACE_RIGHT ? ACT_RIGHT : ACT_LEFT) | ACT_UP

// Type definitions are in ai_types.h (included via ai_decision_engine.h)

// MOVE_DIR_STILL, MOVE_DIR_FWD, MOVE_DIR_BACK are defined in ai_movement.h

/**
 * \brief Chain an array of controller commands in sequence.
 *
 * \param ctrl Controller instance.
 * \param commands An array of controller commands to chain.
 * \param n_commands Number of elements in commands array.
 * \param ev The current controller event.
 *
 * \return Void.
 */
void chain_controller_cmd(controller *ctrl, int commands[], size_t n_commands, ctrl_event **ev) {
    for(size_t i = 0; i < n_commands; i++) {
        controller_cmd(ctrl, commands[i], ev);
    }
}


/**
 * \brief Determine whether the AI would like to use the specified tactic.
 *
 * \param ctrl Controller instance.
 * \param tactic_type An integer identifying the tactic.
 *
 * \return Boolean indicating whether the AI would like to use the tactic..
 */
bool likes_tactic(const controller *ctrl, int tactic_type) {
    return ai_tactic_likes_it(ctrl, tactic_type);
}

/**
 * \brief Queue the specified tactic in AI tactical state object.
 *
 * \param ctrl Controller instance.
 * \param tactic_type An integer identifying the tactic.
 *
 * \return Void.
 */
void queue_tactic(controller *ctrl, int tactic_type) {
    ai_tactic_queue(ctrl, tactic_type);
}

/**
 * \brief Consider each of the provided tactics in order and queue one we like.
 *
 * \param ctrl Controller instance.
 * \param tactics An array of tactics to consider.
 *
 * \return Void.
 */
void chain_consider_tactics(controller *ctrl, int tactics[], size_t n_tactics) {
    ai_tactic_consider_list(ctrl, tactics, n_tactics);
}


/**
 * \brief Determine whether a pilot dislikes a move. Used for random attacks.
 *
 * \param a The AI instance.
 * \param selected_move The move instance.
 *
 * \return A boolean indicating whether move was disliked.
 */
bool dislikes_move(const ai *a, const af_move *move) {
    // check for non-projectile special moves
    if(is_special_move(move)) {
        // pilots with bad special ability dislike special moves
        return !roll_pref(a->pilot->ap_special);
    }

    switch(move->category) {
        case CAT_BASIC:
            // smart AI dislike basic moves
            return !roll_pref(a->pilot->att_normal) && smart_usually(a);
        case CAT_LOW:
            // pilots with bad low ability dislike low moves
            return !roll_pref(a->pilot->att_normal) && !roll_pref(a->pilot->ap_low);
        case CAT_MEDIUM:
            // pilots with bad middle ability dislike middle moves
            return !roll_pref(a->pilot->att_normal) && !roll_pref(a->pilot->ap_middle);
        case CAT_HIGH:
            // pilots with bad high ability dislike high moves
            return !roll_pref(a->pilot->att_normal) && !roll_pref(a->pilot->ap_high);
        case CAT_CLOSE:
            // non-hyper pilots with bad throw ability dislike throw moves
            return !roll_pref(a->pilot->att_hyper) && !roll_pref(a->pilot->ap_throw);
        case CAT_JUMPING:
            // non-jumper pilots with bad jump ability dislike jump moves
            return !roll_pref(a->pilot->att_jump) && !roll_pref(a->pilot->ap_jump);
        case CAT_PROJECTILE:
            // non-sniper pilots with bad special ability dislike projectile moves
            return !roll_pref(a->pilot->att_sniper) && !roll_pref(a->pilot->ap_special);
    }

    return false;
}

/**
 * \brief Determine whether a move is too powerful for AI difficutly.
 *
 * \param a The AI instance.
 * \param selected_move The move instance.
 *
 * \return A boolean indicating whether move is considered too powerful.
 */
bool move_too_powerful(const ai *a, const af_move *move) {
    return is_special_move(move) && dumb_usually(a);
}

int ai_har_event(controller *ctrl, har_event event) {
    ai *a = ctrl->data;

    bool has_queued_tactic = ai_event_check_cancel_tactic(ctrl, event);

    switch(event.type) {
        case HAR_EVENT_ATTACK:
        case HAR_EVENT_ENEMY_BLOCK:
        case HAR_EVENT_ENEMY_BLOCK_PROJECTILE:
        case HAR_EVENT_LAND_HIT:
        case HAR_EVENT_LAND_HIT_PROJECTILE:
            a->selected_move = NULL;
            break;
        default:
            break;
    }

    switch(event.type) {
        case HAR_EVENT_LAND_HIT:
        case HAR_EVENT_LAND_HIT_PROJECTILE:
            ai_event_on_land_hit(ctrl, event, has_queued_tactic);
            break;
        case HAR_EVENT_ENEMY_BLOCK:
        case HAR_EVENT_ENEMY_BLOCK_PROJECTILE:
            ai_event_on_enemy_block(ctrl, event, has_queued_tactic);
            break;
        case HAR_EVENT_BLOCK:
        case HAR_EVENT_BLOCK_PROJECTILE:
            ai_event_on_block(ctrl, event, has_queued_tactic);
            break;
        case HAR_EVENT_LAND:
            ai_event_on_land(ctrl, event, has_queued_tactic);
            break;
        case HAR_EVENT_ATTACK:
            a->tactic->move_timer = 0;
            break;
        case HAR_EVENT_HIT_WALL:
            ai_event_on_hit_wall(ctrl, event, has_queued_tactic);
            break;
        case HAR_EVENT_TAKE_HIT:
        case HAR_EVENT_TAKE_HIT_PROJECTILE:
            ai_event_on_take_hit(ctrl, event, has_queued_tactic);
            break;
        case HAR_EVENT_RECOVER:
            ai_event_on_recover(ctrl, event, has_queued_tactic);
            break;
        case HAR_EVENT_ENEMY_HAZARD_HIT:
            ai_event_on_enemy_hazard_hit(ctrl, event, has_queued_tactic);
            break;
        case HAR_EVENT_ENEMY_STUN:
            ai_event_on_enemy_stun(ctrl, event, has_queued_tactic);
            break;
        default:
            break;
    }

    return 0;
}

void ai_controller_free(controller *ctrl) {
    ai *a = ctrl->data;
    vector_free(&a->active_projectiles);
    omf_free(a->tactic);
    omf_free(a);
}

/**
 * \brief Check whether a move is valid and can be initiated.
 *
 * \param move The move instance.
 * \param h The HAR instance.
 *
 * \return A boolean indicating whether the move is valid
 */
bool is_valid_move(const af_move *move, const har *h, bool force_allow_projectile) {
    // If category is any of these, and bot is not close, then
    // do not try to execute any of them. This attempts
    // to make the HARs close up instead of standing in place
    // wawing their hands towards each other. Not a perfect solution.
    switch(move->category) {
        case CAT_CLOSE:
        case CAT_LOW:
        case CAT_MEDIUM:
        case CAT_HIGH:
            // Only allow handwaving if close or jumping
            if(!h->close && h->state != STATE_JUMPING) {
                return false;
            }
    }
    if(move->category == CAT_JUMPING && h->state != STATE_JUMPING) {
        // not jumping but trying to execute a jumping move
        return false;
    }
    if(move->category != CAT_JUMPING && h->state == STATE_JUMPING) {
        // jumping but this move is not a jumping move
        return false;
    }
    if(move->category == CAT_SCRAP && h->state != STATE_VICTORY) {
        return false;
    }
    if(move->category == CAT_DESTRUCTION && h->state != STATE_SCRAP) {
        return false;
    }
    if(move->category == CAT_VICTORY) {
        return false;
    }

    // XXX check for chaining?

    int move_str_len = str_size(&move->move_string);
    char tmp;
    for(int i = 0; i < move_str_len; i++) {
        tmp = str_at(&move->move_string, i);
        if(!((tmp >= '1' && tmp <= '9') || tmp == 'K' || tmp == 'P')) {
            if(force_allow_projectile && move->category == CAT_PROJECTILE) {
                return true; // projectile is always true
            }
            return false;
        }
    }

    if((move->damage > 0 || move->category == CAT_PROJECTILE || move->category == CAT_SCRAP ||
        move->category == CAT_DESTRUCTION) &&
       move_str_len > 0) {
        return true;
    }

    return false;
}

/**
 * \brief Sets the selected move.
 *
 * \param ctrl Controller instance.
 * \param selected_move The move instance.
 *
 * \return Void.
 */
void set_selected_move(controller *ctrl, af_move *selected_move) {
    ai *a = ctrl->data;
    object *o = game_state_find_object(ctrl->gs, ctrl->har_obj_id);
    har *h = object_get_userdata(o);

    a->move_stats[selected_move->id].attempts++;
    a->move_stats[selected_move->id].consecutive++;

    // do the move
    a->selected_move = selected_move;
    a->move_str_pos = str_size(&selected_move->move_string) - 1;
    object *o_enemy =
        game_state_find_object(ctrl->gs, game_state_get_player(ctrl->gs, h->player_id == 1 ? 0 : 1)->har_obj_id);
    a->move_stats[a->selected_move->id].last_dist = fabsf(o->pos.x - o_enemy->pos.x);
    a->blocked = 0;
    // log_debug("AI selected move %s", str_c(&selected_move->move_string));
}

/**
 * \brief Assigns a move by category identifier.
 *
 * \param ctrl Controller instance.
 * \param category An integer identifying the desired category of move.
 * \param highest_damage A boolean indicating whether to pick the highest damage move of category.
 *
 * \return A boolean indicating whether move was assigned.
 */
bool assign_move_by_cat(controller *ctrl, int category, bool highest_damage) {
    ai *a = ctrl->data;
    object *o = game_state_find_object(ctrl->gs, ctrl->har_obj_id);
    har *h = object_get_userdata(o);

    // Build candidate moves list for this category
    af_move *candidate_moves[70];
    int candidate_count = 0;

    for(int i = 0; i < 70; i++) {
        af_move *move = af_get_move(h->af_data, i);
        if(move && category == move->category) {
            candidate_moves[candidate_count++] = move;
        }
    }

    // Set up evaluation context
    move_stat_context ctx = {
        .move_stats = a->move_stats,
        .har = h,
        .highest_damage = highest_damage,
        .difficulty = a->difficulty,
        .pilot = *a->pilot,
        .enemy_range = 0,  // Not used in assign_move_by_cat
        .last_move_id = a->last_move_id,
        .damage_divisor = 3,  // assign_move_by_cat uses damage/3
        .force_allow_projectile = true,  // assign_move_by_cat force-allows projectiles
    };

    // Select best move from candidates
    af_move *selected_move = ai_move_select_best(candidate_moves, candidate_count, &ctx);

    if(selected_move) {
        for(int i = 0; i < 70; i++) {
            a->move_stats[i].consecutive /= 2;
        }

        set_selected_move(ctrl, selected_move);
        return true;
    }

    return false;
}

/**
 * \brief Assigns a move by move_id.
 *
 * \param ctrl Controller instance.
 * \param move_id An integer identifying the desired move.
 *
 * \return A boolean indicating whether move was assigned.
 */
bool assign_move_by_id(controller *ctrl, int move_id) {
    object *o = game_state_find_object(ctrl->gs, ctrl->har_obj_id);
    har *h = object_get_userdata(o);

    for(int i = 0; i < 70; i++) {
        af_move *move = NULL;
        if((move = af_get_move(h->af_data, i))) {
            if(is_valid_move(move, h, true)) {
                // move_id filter
                if(move_id != move->id) {
                    continue;
                }

                // log_debug("=== assign_move_by_id === id %d", move_id);
                set_selected_move(ctrl, move);
                return true;
            }
        }
    }

    return false;
}

/**
 * \brief Make AI attempt to block attack.
 *
 * \param ctrl Controller instance.
 * \param ev The current controller event.
 *
 * \return A boolean indicating whether the attack was blocked.
 */
int ai_block_har(controller *ctrl, ctrl_event **ev) {
    ai *a = ctrl->data;
    object *o = game_state_find_object(ctrl->gs, ctrl->har_obj_id);
    har *h = object_get_userdata(o);
    object *o_enemy =
        game_state_find_object(ctrl->gs, game_state_get_player(ctrl->gs, h->player_id == 1 ? 0 : 1)->har_obj_id);
    har *h_enemy = object_get_userdata(o_enemy);

    // XXX TODO get maximum move distance from the animation object
    if(fabsf(o_enemy->pos.x - o->pos.x) < 100 && h_enemy->executing_move && smart_usually(a)) {
        if(har_is_crouching(h_enemy)) {
            a->cur_act = DOWNBACK;
            controller_cmd(ctrl, a->cur_act, ev);
        } else {
            a->cur_act = BACK;
            controller_cmd(ctrl, a->cur_act, ev);
        }
        return 1;
    }
    return 0;
}

/**
 * \brief Make AI attempt to block projectile.
 *
 * \param ctrl Controller instance.
 * \param ev The current controller event.
 *
 * \return A boolean indicating whether the projectile was blocked.
 */
int ai_block_projectile(controller *ctrl, ctrl_event **ev) {
    ai *a = ctrl->data;
    object *o = game_state_find_object(ctrl->gs, ctrl->har_obj_id);

    bool remember_shooting = (learning_moment(a) && a->shot >= MAX_TIMES_SHOT);

    iterator it;
    object **o_tmp;
    vector_iter_begin(&a->active_projectiles, &it);
    foreach(it, o_tmp) {
        object *o_prj = *o_tmp;
        if(projectile_get_owner(o_prj) == har_player_id(o)) {
            continue;
        }
        if(o_prj->cur_sprite_id >= 0 && (smart_usually(a) || remember_shooting)) {
            sprite *cur_sprite = animation_get_sprite(o_prj->cur_animation, o_prj->cur_sprite_id);
            if(cur_sprite) {
                vec2i pos_prj = vec2i_add(object_get_pos(o_prj), cur_sprite->pos);
                vec2i size_prj = object_get_size(o_prj);
                if(object_get_direction(o_prj) == OBJECT_FACE_LEFT) {
                    pos_prj.x = object_get_pos(o_prj).x + ((cur_sprite->pos.x * -1) - size_prj.x);
                }
                if(fabsf(pos_prj.x - o->pos.x) < 120) {
                    a->cur_act = DOWNBACK;
                    controller_cmd(ctrl, a->cur_act, ev);
                    return 1;
                }
            }
        }
    }

    return 0;
}

/**
 * \brief Process the current selected move.
 *
 * \param ctrl Controller instance.
 * \param ev The current controller event.
 *
 * \return Void.
 */
void process_selected_move(controller *ctrl, ctrl_event **ev) {
    ai *a = ctrl->data;
    object *o = game_state_find_object(ctrl->gs, ctrl->har_obj_id);

    if(a->input_lag_timer > 0) {
        a->input_lag_timer--;
    } else {
        a->move_str_pos--;
        if(a->move_str_pos <= 0) {
            a->move_str_pos = 0;
        }
        a->input_lag_timer = a->input_lag;
    }

    controller_cmd(ctrl, char_to_act(&a->selected_move->move_string, o->direction, &a->move_str_pos), ev);

    if(a->move_str_pos == 0) {
        a->selected_move = NULL;
    }
}

/**
 * \brief Handle the AI's movement.
 *
 * \param ctrl Controller instance.
 * \param ev The current controller event.
 *
 * \return Void.
 */
void handle_movement(controller *ctrl, ctrl_event **ev) {
    ai *a = ctrl->data;
    object *o = game_state_find_object(ctrl->gs, ctrl->har_obj_id);
    har *h = object_get_userdata(o);

    // default mid-action jump chance
    int jump_chance = ai_movement_jump_chance(a);

    // Change action after act_timer runs out
    if(a->act_timer <= 0 && (roll_chance(ai_core_config_get()->base_act_chance) || diff_scale(a))) {
        int enemy_range = get_enemy_range(ctrl);
        int move_dir = ai_movement_decide(a, enemy_range, h->is_wallhugging, h->id);

        switch(move_dir) {
            case MOVE_DIR_FWD:
                // walk forward
                a->cur_act = FORWARD;
                jump_chance = ai_core_config_get()->base_fwd_jump_chance;
                if(diff_scale(a)) {
                    jump_chance -= 2;
                }
                break;
            case MOVE_DIR_BACK:
                // walk backward
                a->cur_act = BACK;
                jump_chance = ai_core_config_get()->base_back_jump_chance;
                if(diff_scale(a)) {
                    jump_chance -= 2;
                }
                break;
            case MOVE_DIR_STILL:
            default:
                if(smart_usually(a) || roll_pref(a->pilot->att_def)) {
                    // crouch and block
                    a->cur_act = DOWNBACK;
                    jump_chance = 0;
                } else {
                    // do nothing
                    a->cur_act = ACT_STOP;
                    jump_chance = ai_core_config_get()->base_still_jump_chance;
                    if(diff_scale(a)) {
                        jump_chance -= 5;
                    }
                }
                break;
        }

        reset_act_timer(a);
        controller_cmd(ctrl, a->cur_act, ev);
    }

    // Jump once in a while if they like to jump
    if(jump_chance > 0 && roll_chance(jump_chance) && roll_pref(a->pilot->pref_jump)) {
        // log_debug("Jump chance %d", jump_chance);
        if(smart_usually(a) && roll_pref(a->pilot->att_jump)) {
            // double jump
            controller_cmd(ctrl, ACT_DOWN, ev);
        }
        if(o->vel.x < 0) {
            controller_cmd(ctrl, ACT_UP | ACT_LEFT, ev);
        } else if(o->vel.x > 0) {
            controller_cmd(ctrl, ACT_UP | ACT_RIGHT, ev);
        } else {
            controller_cmd(ctrl, ACT_UP, ev);
        }
        // release the jump button
        controller_cmd(ctrl, ACT_STOP, ev);
    }
}

/**
 * \brief Attempt to select a random attack.
 *
 * \param ctrl Controller instance.
 * \param highest_damage A boolean indicating whether to pick the highest damage move that is valid.
 *
 * \return Boolean indicating whether an attack was selected.
 */
bool attempt_attack(controller *ctrl, bool highest_damage) {
    ai *a = ctrl->data;
    object *o = game_state_find_object(ctrl->gs, ctrl->har_obj_id);
    har *h = object_get_userdata(o);

    // Build candidate moves list (all 70 moves)
    af_move *candidate_moves[70];
    int candidate_count = 0;

    for(int i = 0; i < 70; i++) {
        af_move *move = af_get_move(h->af_data, i);
        if(move) {
            candidate_moves[candidate_count++] = move;
        }
    }

    // Get enemy range for filtering
    int enemy_range = get_enemy_range(ctrl);

    // Set up evaluation context
    move_stat_context ctx = {
        .move_stats = a->move_stats,
        .har = h,
        .highest_damage = highest_damage,
        .difficulty = a->difficulty,
        .pilot = *a->pilot,
        .enemy_range = enemy_range,
        .last_move_id = a->last_move_id,
        .damage_divisor = 4,  // attempt_attack uses damage/4
        .force_allow_projectile = false,  // attempt_attack does not force-allow projectiles
    };

    // Select best move from candidates
    af_move *selected_move = ai_move_select_best(candidate_moves, candidate_count, &ctx);

    if(selected_move) {
        for(int i = 0; i < 70; i++) {
            a->move_stats[i].consecutive /= 2;
        }

        set_selected_move(ctrl, selected_move);
        return true;
    }

    return false;
}

/**
 * \brief Attempt to initiate a charge atack using direct keyboard combinations.
 *
 * \param ctrl Controller instance.
 * \param ev The current controller event.
 *
 * \return Boolean indicating whether an attack was initiated.
 */
bool attempt_charge_attack(controller *ctrl, ctrl_event **ev) {
    object *o = game_state_find_object(ctrl->gs, ctrl->har_obj_id);
    if(o == NULL) {
        return false;
    }

    har *h = object_get_userdata(o);
    const ai_har_config *char_cfg = ai_skills_config_get(h->id);
    return ai_har_execute_charge(ctrl, char_cfg, ev);
}

/**
 * \brief Attempt to initiate a push atack using direct keyboard combinations.
 *
 * \param ctrl Controller instance.
 * \param ev The current controller event.
 *
 * \return Boolean indicating whether an attack was initiated.
 */
bool attempt_push_attack(controller *ctrl, ctrl_event **ev) {
    object *o = game_state_find_object(ctrl->gs, ctrl->har_obj_id);
    if(o == NULL) {
        return false;
    }

    har *h = object_get_userdata(o);
    const ai_har_config *char_cfg = ai_skills_config_get(h->id);
    return ai_har_execute_push(ctrl, char_cfg, ev);
}

/**
 * \brief Attempt to initiate a push atack using direct keyboard combinations.
 *
 * \param ctrl Controller instance.
 * \param ev The current controller event.
 *
 * \return Boolean indicating whether an attack was initiated.
 */
bool attempt_trip_attack(controller *ctrl, ctrl_event **ev) {
    object *o = game_state_find_object(ctrl->gs, ctrl->har_obj_id);
    if(o == NULL) {
        return false;
    }

    har *h = object_get_userdata(o);
    const ai_har_config *char_cfg = ai_skills_config_get(h->id);
    return ai_har_execute_trip(ctrl, char_cfg, ev);
}

/**
 * \brief Attempt to initiate a projectile attack using direct keyboard combinations.
 *
 * \param ctrl Controller instance.
 * \param ev The current controller event.
 *
 * \return Boolean indicating whether an attack was initiated.
 */
bool attempt_projectile_attack(controller *ctrl, ctrl_event **ev) {
    object *o = game_state_find_object(ctrl->gs, ctrl->har_obj_id);
    if(o == NULL) {
        return false;
    }

    har *h = object_get_userdata(o);
    const ai_har_config *char_cfg = ai_skills_config_get(h->id);
    return ai_har_execute_projectile(ctrl, char_cfg, ev);
}

/**
 * \brief Handle the next phase of the currently queued tactic.
 *
 * \param ctrl Controller instance.
 * \param ev The current controller event.
 *
 * \return Boolean indicating whether AI moved or attacked.
 */
bool handle_queued_tactic(controller *ctrl, ctrl_event **ev) {
    ai *a = ctrl->data;
    object *o = game_state_find_object(ctrl->gs, ctrl->har_obj_id);
    har *h = object_get_userdata(o);
    tactic_state *tactic = a->tactic;
    int enemy_close = h->close;
    int enemy_range = get_enemy_range(ctrl);
    bool wall_close = h->is_wallhugging;

    bool acted = false;
    if(tactic->move_type > 0 && tactic->move_timer > 0) {
        acted = true;
        // handle movement phase of tactic
        switch(tactic->move_type) {
            case MOVE_CLOSE:
                if(!enemy_close) {
                    // take a step closer
                    a->cur_act = FORWARD;
                    controller_cmd(ctrl, a->cur_act, ev);
                    tactic->move_timer--;
                } else {
                    tactic->move_timer = 0;
                    // log_debug("Movement close success: %d", h->id);
                    if(tactic->attack_type == 0 && smart_usually(a)) {
                        queue_tactic(ctrl, TACTIC_GRAB);
                    }
                }
                break;
            case MOVE_AVOID:
                if(enemy_range == RANGE_FAR) {
                    tactic->move_timer = 0;
                    acted = false;
                } else {
                    if(enemy_range == RANGE_CRAMPED || !roll_pref(a->pilot->pref_jump)) {
                        // take a step away
                        a->cur_act = BACK;
                    } else {
                        if(smart_usually(a)) {
                            // do super jump
                            controller_cmd(ctrl, ACT_DOWN, ev);
                        }
                        // jump away
                        a->cur_act = UPBACK;
                    }

                    controller_cmd(ctrl, a->cur_act, ev);
                    controller_cmd(ctrl, ACT_STOP, ev);
                    tactic->move_timer--;
                }

                // if (tactic->move_timer == 0) log_debug("Movement avoid finished: %d", h->id);
                break;
            case MOVE_JUMP:
            case MOVE_HIGH_JUMP:
                if(enemy_range > RANGE_CRAMPED) {
                    if((enemy_range == RANGE_FAR && smart_usually(a)) || tactic->move_type == MOVE_HIGH_JUMP) {
                        // do high jump
                        controller_cmd(ctrl, ACT_DOWN, ev);
                    }
                    // jump closer
                    a->cur_act = UPFORWARD;
                    controller_cmd(ctrl, a->cur_act, ev);
                    controller_cmd(ctrl, ACT_STOP, ev);
                    if(roll_pref(a->pilot->pref_jump)) {
                        tactic->move_timer--;
                    } else {
                        tactic->move_timer = 0;
                    }
                } else if(tactic->tactic_type == TACTIC_FLY) {
                    if(roll_pref(a->pilot->att_jump) && smart_sometimes(a)) {
                        // do high jump
                        controller_cmd(ctrl, ACT_DOWN, ev);
                    }
                    // jump over enemy
                    a->cur_act = UPFORWARD;
                    controller_cmd(ctrl, a->cur_act, ev);
                    controller_cmd(ctrl, ACT_STOP, ev);
                    tactic->move_timer = 0;
                } else {
                    tactic->move_timer = 0;
                }

                // if (tactic->move_timer == 0) log_debug("Movement jump finished: %d", h->id);
                break;
            case MOVE_BLOCK:
                if(wall_close || har_is_crouching(h)) {
                    // crouch & block
                    a->cur_act = DOWNBACK;
                } else {
                    // retreat & block
                    a->cur_act = BACK;
                }

                controller_cmd(ctrl, a->cur_act, ev);
                tactic->move_timer--;

                // if (tactic->move_timer == 0) log_debug("Movement block finished: %d", h->id);
                break;
            default:
                // log_debug("Flushing invalid move type: %d", h->id);
                tactic->move_type = 0;
                tactic->move_timer = 0;
                acted = false;
        }
    } else if(tactic->attack_type > 0 && tactic->attack_timer > 0) {
        // handle attack phase of tactic
        bool in_attempt_range = (enemy_range <= RANGE_CLOSE || (enemy_range <= RANGE_MID && dumb_sometimes(a)));
        acted = true;
        tactic->attack_timer--;
        if(tactic->attack_on == 0) {
            int attack_cat = 0;
            switch(tactic->attack_type) {
                case ATTACK_ID: {
                    if(!in_attempt_range) {
                        break;
                    }

                    if(assign_move_by_id(ctrl, tactic->attack_id)) {
                        reset_tactic_state(a);
                        // log_debug("Specific attack success: %d", h->id);
                    }
                } break;
                case ATTACK_TRIP: {
                    if(attempt_trip_attack(ctrl, ev)) {
                        reset_tactic_state(a);

                        // chain another tactic
                        int tacs[] = {TACTIC_QUICK, TACTIC_GRAB, TACTIC_ESCAPE, TACTIC_SHOOT};
                        chain_consider_tactics(ctrl, tacs, N_ELEMENTS(tacs));
                        if(a->tactic->tactic_type > 0) {
                            a->tactic->chain_hit_on = CAT_LOW;
                        }
                    }
                } break;
                case ATTACK_GRAB: {
                    if(!enemy_close) {
                        break;
                    }

                    if(assign_move_by_cat(ctrl, CAT_CLOSE, true)) {
                        attack_cat = CAT_CLOSE;
                    }

                    if(attack_cat > 0) {
                        reset_tactic_state(a);
                        // log_debug("Grab attack success: %d", h->id);

                        // chain another tactic
                        if(smart_sometimes(a)) {
                            if(likes_tactic(ctrl, TACTIC_PUSH)) {
                                // set chain tactic to push if attack hits
                                a->tactic->chain_hit_on = attack_cat;
                                a->tactic->chain_hit_tactic = TACTIC_PUSH;
                            } else if(likes_tactic(ctrl, TACTIC_FLY)) {
                                // set chain tactic to fly if attack hits
                                a->tactic->chain_hit_on = attack_cat;
                                a->tactic->chain_hit_tactic = TACTIC_FLY;
                            } else if(likes_tactic(ctrl, TACTIC_COUNTER)) {
                                // set chain tactic to counter if attack hits
                                a->tactic->chain_hit_on = attack_cat;
                                a->tactic->chain_hit_tactic = TACTIC_COUNTER;
                            } else if(likes_tactic(ctrl, TACTIC_SHOOT)) {
                                // set chain tactic to shoot if attack hits
                                a->tactic->chain_hit_on = attack_cat;
                                a->tactic->chain_hit_tactic = TACTIC_SHOOT;
                            }
                        }
                    }
                } break;
                case ATTACK_LIGHT: {
                    if(!in_attempt_range) {
                        break;
                    }

                    int light_cat = roll_chance(2) ? CAT_BASIC : CAT_MEDIUM;
                    if(assign_move_by_cat(ctrl, light_cat, false)) {
                        reset_tactic_state(a);
                        // log_debug("Light attack success: %d", h->id);

                        // chain another tactic
                        if(smart_sometimes(a)) {
                            if(likes_tactic(ctrl, TACTIC_PUSH)) {
                                // set chain tactic to push if attack hits
                                a->tactic->chain_hit_on = light_cat;
                                a->tactic->chain_hit_tactic = TACTIC_PUSH;
                            } else if(likes_tactic(ctrl, TACTIC_TRIP)) {
                                // set chain tactic to trip if attack hits
                                a->tactic->chain_hit_on = light_cat;
                                a->tactic->chain_hit_tactic = TACTIC_TRIP;
                            } else if(likes_tactic(ctrl, TACTIC_FLY)) {
                                // set chain tactic to fly if attack hits
                                a->tactic->chain_hit_on = light_cat;
                                a->tactic->chain_hit_tactic = TACTIC_FLY;
                            }
                        }
                    }
                } break;
                case ATTACK_HEAVY: {
                    if(!in_attempt_range) {
                        break;
                    }

                    int heavy_cat = roll_chance(2) ? CAT_MEDIUM : CAT_HIGH;
                    if(assign_move_by_cat(ctrl, heavy_cat, true)) {
                        reset_tactic_state(a);
                        // log_debug("Heavy attack success: %d", h->id);

                        // chain another tactic
                        if(smart_sometimes(a)) {
                            if(likes_tactic(ctrl, TACTIC_TRIP)) {
                                // set chain tactic to trip if attack hits
                                a->tactic->chain_hit_on = heavy_cat;
                                a->tactic->chain_hit_tactic = TACTIC_TRIP;
                            } else if(likes_tactic(ctrl, TACTIC_COUNTER)) {
                                // set chain tactic to counter if attack hits
                                a->tactic->chain_hit_on = heavy_cat;
                                a->tactic->chain_hit_tactic = TACTIC_COUNTER;
                            } else if(likes_tactic(ctrl, TACTIC_QUICK)) {
                                // set chain tactic to quick if attack hits
                                a->tactic->chain_hit_on = heavy_cat;
                                a->tactic->chain_hit_tactic = TACTIC_QUICK;
                            }
                        }
                    }
                } break;
                case ATTACK_JUMP: {
                    if(!in_attempt_range && a->tactic->attack_timer > 0) {
                        // log_debug("Waiting for jump attack range");
                        // when not in range we wait until last tick of attack timer
                        // that way the attack won't fizzle out before we reach them
                        return acted;
                    }

                    if(attempt_attack(ctrl, false)) {
                        reset_tactic_state(a);
                        // log_debug("Jump attack success: %d", h->id);

                        // chain another tactic
                        if(smart_usually(a)) {
                            if(likes_tactic(ctrl, TACTIC_TRIP)) {
                                // set chain tactic to counter if jumping attack hits
                                a->tactic->chain_hit_on = a->selected_move->category;
                                a->tactic->chain_hit_tactic = TACTIC_TRIP;
                            } else if(likes_tactic(ctrl, TACTIC_GRAB)) {
                                // set chain tactic to counter if jumping attack hits
                                a->tactic->chain_hit_on = a->selected_move->category;
                                a->tactic->chain_hit_tactic = TACTIC_GRAB;
                            } else if(likes_tactic(ctrl, TACTIC_PUSH)) {
                                // set chain tactic to counter if jumping attack hits
                                a->tactic->chain_hit_on = a->selected_move->category;
                                a->tactic->chain_hit_tactic = TACTIC_PUSH;
                            }
                        }
                    }
                } break;
                case ATTACK_RANGED: {
                    if(attempt_projectile_attack(ctrl, ev)) {
                        reset_tactic_state(a);

                        // chain another tactic
                        if(smart_sometimes(a)) {
                            if(a->pilot->att_sniper && likes_tactic(ctrl, TACTIC_SHOOT)) {
                                // set chain tactic to shoot if projectile hits
                                a->tactic->chain_hit_on = CAT_PROJECTILE;
                                a->tactic->chain_hit_tactic = TACTIC_SHOOT;
                            } else if(likes_tactic(ctrl, TACTIC_FLY)) {
                                // set chain tactic to fly if projectile hits
                                a->tactic->chain_hit_on = CAT_PROJECTILE;
                                a->tactic->chain_hit_tactic = TACTIC_FLY;
                            } else if(likes_tactic(ctrl, TACTIC_COUNTER)) {
                                // set chain tactic to counter if projectile hits
                                a->tactic->chain_hit_on = CAT_PROJECTILE;
                                a->tactic->chain_hit_tactic = TACTIC_COUNTER;
                            }
                        }
                    }
                } break;
                case ATTACK_CHARGE: {
                    if(attempt_charge_attack(ctrl, ev)) {
                        reset_tactic_state(a);

                        if(h->id == HAR_SHADOW) {
                            // shadow charge is long range
                            // use this free time to consider a new tactic
                            int tacs[] = {TACTIC_SHOOT, TACTIC_GRAB, TACTIC_FLY};
                            chain_consider_tactics(ctrl, tacs, N_ELEMENTS(tacs));
                        }
                    }
                } break;
                case ATTACK_PUSH: {
                    if(attempt_push_attack(ctrl, ev)) {
                        reset_tactic_state(a);
                    }
                } break;
                case ATTACK_RANDOM: {
                    if(attempt_attack(ctrl, false)) {
                        reset_tactic_state(a);
                        // log_debug("Random attack success: %d", h->id);

                        // chain another tactic
                        if(smart_usually(a)) {
                            if(likes_tactic(ctrl, TACTIC_TRIP)) {
                                // set chain tactic to counter if jumping attack hits
                                a->tactic->chain_hit_on = a->selected_move->category;
                                a->tactic->chain_hit_tactic = TACTIC_TRIP;
                            } else if(likes_tactic(ctrl, TACTIC_GRAB)) {
                                // set chain tactic to counter if jumping attack hits
                                a->tactic->chain_hit_on = a->selected_move->category;
                                a->tactic->chain_hit_tactic = TACTIC_GRAB;
                            } else if(likes_tactic(ctrl, TACTIC_PUSH)) {
                                // set chain tactic to counter if jumping attack hits
                                a->tactic->chain_hit_on = a->selected_move->category;
                                a->tactic->chain_hit_tactic = TACTIC_PUSH;
                            }
                        }
                    }
                } break;
                default:
                    log_debug("Flushing invalid attack type: %d", h->id);
                    tactic->attack_type = 0;
                    tactic->attack_timer = 0;
            }
        }
    } else {
        // reset queued tactic
        reset_tactic_state(a);
        // log_debug("Flushing failed tactic queue: %d", h->id);
    }

    return acted;
}

int ai_controller_poll(controller *ctrl, ctrl_event **ev) {
    ai *a = ctrl->data;
    object *o = game_state_find_object(ctrl->gs, ctrl->har_obj_id);
    scene *scene = game_state_get_scene(ctrl->gs);
    if(scene->id == SCENE_VS || scene->id == SCENE_NEWSROOM) {
        if(ctrl->gs->warp_speed || (scene->static_ticks_since_start + 1) % 256 == 0) {
            controller_cmd(ctrl, ACT_PUNCH, ev);
        }
    }
    if(!o) {
        return 1;
    }

    har *h = object_get_userdata(o);

    // Do not run AI while the game is paused
    if(game_state_is_paused(o->gs)) {
        return 0;
    }

    // Do not run AI while match is starting or ending
    // XXX this prevents the AI from doing scrap/destruction moves
    // XXX this could be fixed by providing a "scene changed" event
    if(scene_is_arena(game_state_get_scene(o->gs)) &&
       arena_get_state(game_state_get_scene(o->gs)) != ARENA_STATE_FIGHTING) {

        // null out selected move to fix the "AI not moving problem"
        a->selected_move = NULL;
        return 0;
    }

    // decrement act_timer
    a->act_timer--;

    // Grab all projectiles on screen
    vector_clear(&a->active_projectiles);
    game_state_get_projectiles(o->gs, &a->active_projectiles);

    // Try to block har
    if(ai_block_har(ctrl, ev)) {
        return 0;
    }

    // Try to block projectiles
    if(ai_block_projectile(ctrl, ev)) {
        return 0;
    }

    // handle selected move
    if(a->selected_move) {
        // finish doing the selected move first
        process_selected_move(ctrl, ev);
        // log_debug("=== POLL === process_selected_move");
        return 0;
    }

    bool can_move = (h->state == STATE_STANDING || h->state == STATE_WALKTO || h->state == STATE_WALKFROM ||
                     h->state == STATE_CROUCHING || h->state == STATE_CROUCHBLOCK);

    bool can_interupt_tactic = (a->tactic->tactic_type == 0 ||
                                !(a->tactic->attack_type == ATTACK_CHARGE || a->tactic->attack_type == ATTACK_PUSH ||
                                  a->tactic->attack_type == ATTACK_TRIP));

    if(o) {
        object *enemy = game_state_find_object(ctrl->gs, o->animation_state.enemy_obj_id);
        // UJ tag signals to the AI it should probably jump
        if(can_move && can_interupt_tactic && player_frame_isset(enemy, "uj") && smart_sometimes(a)) {
            reset_tactic_state(a);
            controller_cmd(ctrl, ACT_UP, ev);
            controller_cmd(ctrl, ACT_STOP, ev);
            return 0;
        }
    }

    // be wary of repeated throws while attempting to complete a tactic
    if(can_move && can_interupt_tactic && a->thrown > 1 && a->difficulty > 2) {
        // attempt a quick attack to disrupt their grab/throw
        int enemy_range = get_enemy_range(ctrl);
        if((enemy_range == RANGE_CRAMPED || (enemy_range == RANGE_CLOSE && a->thrown >= 2)) &&
           (assign_move_by_cat(ctrl, CAT_LOW, false) || attempt_attack(ctrl, false))) {
            // log_debug("Spamming random attacks to avoid being thrown");
            reset_tactic_state(a);
            return 0;
        }
    }

    // attempt queued tactic
    if(a->tactic->tactic_type > 0 &&
       (can_move || (a->tactic->attack_type == ATTACK_JUMP && h->state == STATE_JUMPING))) {
        bool acted = handle_queued_tactic(ctrl, ev);
        // check if tactic is complete
        if(a->tactic->tactic_type == 0) {
            // reset movement act timer
            // set higher than zero to avoid glitching when bailing on tactics
            a->act_timer = 3;
        }

        if(acted) {
            return 0; // wait for next poll
        }
    }

    int enemy_range = get_enemy_range(ctrl);

    // attempt a random attack
    if((roll_chance(ai_core_config_get()->random_attack_chance) || diff_scale(a)) && (enemy_range <= RANGE_CLOSE || dumb_sometimes(a)) &&
       attempt_attack(ctrl, false)) {
        // log_debug("Random attack: %d", h->id);
        // reset movement act timer
        reset_act_timer(a);
        return 0;
    }

    // handle movement
    if(can_move) {
        handle_movement(ctrl, ev);
    }
    // log_debug("=== POLL === handle_movement");

    // queue a random tactic for next poll
    if((a->last_move_id == 0 || a->tactic->tactic_type == 0 || (roll_chance(ai_core_config_get()->random_attack_chance) && diff_scale(a))) &&
       can_move) {
        // log_debug("Attempt to queue random tactic[0m");
        int tacs[] = {TACTIC_SHOOT, TACTIC_CLOSE, TACTIC_FLY, TACTIC_PUSH, TACTIC_TRIP, TACTIC_GRAB, TACTIC_QUICK};
        chain_consider_tactics(ctrl, tacs, N_ELEMENTS(tacs));
    }

    return 0;
}

const char *ai_tactic(int tactic_type) {
    switch(tactic_type) {
        case TACTIC_ESCAPE:
            return "escape";
        case TACTIC_TURTLE:
            return "turtle";
        case TACTIC_GRAB:
            return "grab";
        case TACTIC_SPAM:
            return "spam";
        case TACTIC_SHOOT:
            return "shoot";
        case TACTIC_TRIP:
            return "trip";
        case TACTIC_QUICK:
            return "quick";
        case TACTIC_CLOSE:
            return "close";
        case TACTIC_FLY:
            return "fly";
        case TACTIC_PUSH:
            return "push";
        case TACTIC_COUNTER:
            return "counter";
    }
    return "?"; // Handle unexpected input
}

const char *ai_move(int move_type) {
    switch(move_type) {
        case MOVE_CLOSE:
            return "close";
        case MOVE_AVOID:
            return "avoid";
        case MOVE_JUMP:
            return "jump";
        case MOVE_HIGH_JUMP:
            return "high_jump";
        case MOVE_BLOCK:
            return "block";
    }
    return "?";
}

const char *ai_attack(int attack_type) {
    switch(attack_type) {
        case ATTACK_ID:
            return "id";
        case ATTACK_TRIP:
            return "trip";
        case ATTACK_GRAB:
            return "grab";
        case ATTACK_LIGHT:
            return "light";
        case ATTACK_HEAVY:
            return "heavy";
        case ATTACK_JUMP:
            return "jump";
        case ATTACK_RANGED:
            return "ranged";
        case ATTACK_CHARGE:
            return "charge";
        case ATTACK_PUSH:
            return "push";
        case ATTACK_RANDOM:
            return "random";
    }
    return "?";
}

void ai_controller_print_state(controller *ctrl, char *buf, size_t bufsize) {
    ai *a = ctrl->data;
    snprintf(buf, bufsize, "%s %s %s %d", ai_tactic(a->tactic->tactic_type), ai_move(a->tactic->move_type),
             ai_attack(a->tactic->attack_type), a->last_move_id);
}

void ai_controller_create(controller *ctrl, int difficulty, sd_pilot *pilot, int pilot_id) {
    ai *a = omf_calloc(1, sizeof(ai));
    a->difficulty = difficulty + 1;
    a->act_timer = 0;
    a->cur_act = 0;
    a->input_lag = 3;
    a->input_lag_timer = a->input_lag;
    a->selected_move = NULL;
    a->last_move_id = 0;
    a->move_str_pos = 0;
    memset(a->move_stats, 0, sizeof(a->move_stats));
    for(int i = 0; i < 70; i++) {
        a->move_stats[i].max_hit_dist = -1;
        a->move_stats[i].min_hit_dist = -1;
        a->move_stats[i].last_dist = -1;
    }
    a->blocked = 0;
    a->thrown = 0;
    a->shot = 0;
    vector_create(&a->active_projectiles, sizeof(object *));
    pilot->pilot_id = pilot_id;
    a->pilot = pilot;

    // set pilot personality manually until we start reading them from binary
    reset_pilot_personality(pilot);

    // set initial tactical state
    tactic_state *tactic = omf_calloc(1, sizeof(tactic_state));
    tactic->tactic_type = 0;
    tactic->last_tactic = 0;
    tactic->move_type = 0;
    tactic->move_timer = 0;
    tactic->attack_type = 0;
    tactic->attack_id = 0;
    tactic->attack_timer = 0;
    tactic->attack_on = 0;
    a->tactic = tactic;
    reset_tactic_state(a);

    ctrl->data = a;
    ctrl->type = CTRL_TYPE_AI;
    ctrl->poll_fun = &ai_controller_poll;
    ctrl->har_hook = &ai_har_event;
    ctrl->free_fun = &ai_controller_free;
}
