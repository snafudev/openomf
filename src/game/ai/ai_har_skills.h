/**
 * AI character skills
 *
 * Executes HAR-specific charge/push/projectile/trip actions.
 */

#ifndef AI_HAR_SKILLS_H
#define AI_HAR_SKILLS_H

#include "controller/controller.h"
#include "game/ai/ai_skills_config_loader.h"
#include <stdbool.h>

bool ai_char_execute_charge(controller *ctrl, const ai_char_config *char_cfg, ctrl_event **ev);
bool ai_char_execute_push(controller *ctrl, const ai_char_config *char_cfg, ctrl_event **ev);
bool ai_char_execute_projectile(controller *ctrl, const ai_char_config *char_cfg, ctrl_event **ev);
bool ai_char_execute_trip(controller *ctrl, const ai_char_config *char_cfg, ctrl_event **ev);

#endif // AI_HAR_SKILLS_H
