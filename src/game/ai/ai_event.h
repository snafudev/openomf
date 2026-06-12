/**
 * AI Event Handlers
 *
 * Per-event response helpers extracted from ai_har_event() in ai_controller.c.
 * Each function handles tactic queuing logic for one class of HAR events.
 */

#ifndef AI_EVENT_H
#define AI_EVENT_H

#include "controller/controller.h"
#include "game/objects/har.h"
#include <stdbool.h>

/**
 * \brief Cancel the queued tactic when the incoming event warrants it.
 *
 * Inspects the current tactic and the event type.  If the tactic should be
 * cancelled (e.g. we were blocked while running a non-defensive tactic), it
 * is reset via reset_tactic_state().
 *
 * \param ctrl Controller instance.
 * \param event The current HAR event.
 * \return true if a tactic is still queued after the check, false if it was reset.
 */
bool ai_event_check_cancel_tactic(controller *ctrl, har_event event);

/**
 * \brief Handle HAR_EVENT_LAND_HIT / HAR_EVENT_LAND_HIT_PROJECTILE.
 *
 * Updates move statistics, triggers chained tactics, and considers follow-up
 * tactics when no tactic is already queued.
 *
 * \param ctrl Controller instance.
 * \param event The current HAR event (must be LAND_HIT or LAND_HIT_PROJECTILE).
 * \param has_queued_tactic Whether a tactic was queued before this event.
 */
void ai_event_on_land_hit(controller *ctrl, har_event event, bool has_queued_tactic);

/**
 * \brief Handle HAR_EVENT_ENEMY_BLOCK / HAR_EVENT_ENEMY_BLOCK_PROJECTILE.
 *
 * Decrements move value and considers counter-tactics when the first block
 * is detected this combo.
 *
 * \param ctrl Controller instance.
 * \param event The current HAR event (must be ENEMY_BLOCK or ENEMY_BLOCK_PROJECTILE).
 * \param has_queued_tactic Whether a tactic was queued before this event.
 */
void ai_event_on_enemy_block(controller *ctrl, har_event event, bool has_queued_tactic);

/**
 * \brief Handle HAR_EVENT_BLOCK / HAR_EVENT_BLOCK_PROJECTILE.
 *
 * Triggers immediate counter attack if configured, otherwise considers
 * reactive tactics.
 *
 * \param ctrl Controller instance.
 * \param event The current HAR event (must be BLOCK or BLOCK_PROJECTILE).
 * \param has_queued_tactic Whether a tactic was queued before this event.
 */
void ai_event_on_block(controller *ctrl, har_event event, bool has_queued_tactic);

/**
 * \brief Handle HAR_EVENT_LAND.
 *
 * Triggers a queued landing-attack if applicable; otherwise resets act timer
 * and considers follow-up tactics.
 *
 * \param ctrl Controller instance.
 * \param event The current HAR event (must be LAND).
 * \param has_queued_tactic Whether a tactic was queued before this event.
 */
void ai_event_on_land(controller *ctrl, har_event event, bool has_queued_tactic);

/**
 * \brief Handle HAR_EVENT_HIT_WALL.
 *
 * Considers pressure tactics when the enemy hits a wall.
 *
 * \param ctrl Controller instance.
 * \param event The current HAR event (must be HIT_WALL).
 * \param has_queued_tactic Whether a tactic was queued before this event.
 */
void ai_event_on_hit_wall(controller *ctrl, har_event event, bool has_queued_tactic);

/**
 * \brief Handle HAR_EVENT_TAKE_HIT / HAR_EVENT_TAKE_HIT_PROJECTILE.
 *
 * Runs learning adjustments for repeated throws or projectiles, then
 * considers reactive tactics.
 *
 * \param ctrl Controller instance.
 * \param event The current HAR event (must be TAKE_HIT or TAKE_HIT_PROJECTILE).
 * \param has_queued_tactic Whether a tactic was queued before this event.
 */
void ai_event_on_take_hit(controller *ctrl, har_event event, bool has_queued_tactic);

/**
 * \brief Handle HAR_EVENT_RECOVER.
 *
 * Considers defensive and escape tactics after regaining control.
 *
 * \param ctrl Controller instance.
 * \param event The current HAR event (must be RECOVER).
 * \param has_queued_tactic Whether a tactic was queued before this event.
 */
void ai_event_on_recover(controller *ctrl, har_event event, bool has_queued_tactic);

/**
 * \brief Handle HAR_EVENT_ENEMY_HAZARD_HIT.
 *
 * Considers capitalizing tactics when the enemy is hit by a hazard.
 *
 * \param ctrl Controller instance.
 * \param event The current HAR event (must be ENEMY_HAZARD_HIT).
 * \param has_queued_tactic Whether a tactic was queued before this event.
 */
void ai_event_on_enemy_hazard_hit(controller *ctrl, har_event event, bool has_queued_tactic);

/**
 * \brief Handle HAR_EVENT_ENEMY_STUN (tactic queuing only).
 *
 * Considers capitalizing tactics when the enemy is stunned and no tactic is
 * already queued.  Tactic cancellation/extension for stun is handled earlier
 * by ai_event_check_cancel_tactic().
 *
 * \param ctrl Controller instance.
 * \param event The current HAR event (must be ENEMY_STUN).
 * \param has_queued_tactic Whether a tactic was queued before this event.
 */
void ai_event_on_enemy_stun(controller *ctrl, har_event event, bool has_queued_tactic);

#endif // AI_EVENT_H
