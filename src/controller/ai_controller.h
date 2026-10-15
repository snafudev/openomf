#ifndef AI_CONTROLLER_H
#define AI_CONTROLLER_H

#include "formats/pilot.h"
#include "game/objects/har.h"

typedef struct controller_t controller;

void ai_controller_free(controller *ctrl);
void ai_controller_create(controller *ctrl, int difficulty, sd_pilot *pilot, int pilot_id);
int ai_har_event(controller *ctrl, har_event event);

void ai_controller_print_state(controller *ctrl, char *buf, size_t bufsize);

#endif // AI_CONTROLLER_H
