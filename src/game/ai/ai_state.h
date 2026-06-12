/**
 * AI state reset helpers
 */

#ifndef AI_STATE_H
#define AI_STATE_H

#include "game/ai/ai_types.h"

void reset_tactic_state(ai *a);
void reset_act_timer(ai *a);
void reset_pilot_personality(sd_pilot *pilot);

#endif // AI_STATE_H
