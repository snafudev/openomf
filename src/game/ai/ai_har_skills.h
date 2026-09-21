/**
 * AI character skills
 *
 * Executes HAR-specific charge/push/projectile/trip actions.
 */

#ifndef AI_HAR_SKILLS_H
#define AI_HAR_SKILLS_H

#include "controller/controller.h"
#include "game/ai/ai_skills_config_loader.h"
#include "game/ai/ai_types.h"
#include <stdbool.h>

int ai_move_def_score(const ai_move_def *move, int enemy_range, bool enemy_stunned, const ai *ai_data);
const ai_move_def *ai_har_select_best_move(const ai_move_def *moves, uint8_t move_count, int enemy_range,
                                          bool enemy_stunned, const ai *ai_data);
bool ai_har_execute_charge(controller *ctrl, const ai_har_config *char_cfg, ctrl_event **ev);
bool ai_har_execute_push(controller *ctrl, const ai_har_config *char_cfg, ctrl_event **ev);
bool ai_har_execute_projectile(controller *ctrl, const ai_har_config *char_cfg, ctrl_event **ev);
bool ai_har_execute_trip(controller *ctrl, const ai_har_config *char_cfg, ctrl_event **ev);
int ai_resolve_input(int input, int direction);

/**
 * Resolve a single move sequence input for the current facing.
 *
 * The game's move matcher requires the attack button to be pressed while the
 * motion's final direction is still held; a bare P/K would otherwise insert a
 * neutral '5' into the input buffer and fail to match motion+button moves. When
 * `input` is a bare button and `prev_dirs` holds the last resolved direction,
 * the two are combined so the move registers correctly.
 */
int ai_resolve_move_input(int input, int direction, int prev_dirs);

#endif // AI_HAR_SKILLS_H
