/**
 * @file gen_move_test.c
 * @brief Generate REC files for testing HAR move triggering
 *
 * Usage: gen_move_test <har_id> <opponent_id> <output_file> [options] [hex_byte ...]
 * Example: gen_move_test 2 0 thorn_test.rec --seed 1234 --assert "har1.anim==12@64" 40 01 40 44
 *
 * Options:
 *   --seed <n>       Insert a set-random record at tick 0 (deterministic playback).
 *   --ticks <n>      Total recording length (default: last event tick + 150).
 *   --assert <spec>  Insert an assertion record, e.g. "har1.anim==12@64". Repeatable.
 *                    Operand: har1.<attr> / har2.<attr> or an integer literal.
 *                    Operator: ==, <, >, := (set).
 *                    Attribute: xpos ypos xvel yvel state anim health stamina opp_dist dir.
 *
 * Action bytes are in hex format representing ACT_* constants:
 * - 40 = ACT_RIGHT (forward)
 * - 20 = ACT_LEFT (back)
 * - 10 = ACT_DOWN (down)
 * - 08 = ACT_UP (up)
 * - 04 = ACT_PUNCH
 * - 02 = ACT_KICK
 * - 01 = ACT_STOP (neutral)
 * - 44 = ACT_RIGHT | ACT_PUNCH (forward+punch)
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "controller/controller.h"
#include "formats/error.h"
#include "formats/rec.h"
#include "formats/rec_assertion.h"
#include "utils/allocator.h"
#include "utils/path.h"

#define MAX_INPUTS 256
#define MAX_ASSERTIONS 256

// Parse a single hex byte
static int parse_action_byte(const char *str) {
    return (int)strtol(str, NULL, 16);
}

// Insert a lookup_id 10 record (seed or assertion) for player 0 at the given tick.
static int insert_lookup10(sd_rec_file *rec, uint32_t tick, const uint8_t *data) {
    sd_rec_move move;
    memset(&move, 0, sizeof(sd_rec_move));
    move.player_id = 0;
    move.tick = tick;
    char *extra_data = sd_rec_set_lookup_id(&move, 10);
    if(extra_data == NULL) {
        return 1;
    }
    memcpy(extra_data, data, sd_rec_extra_len(10));
    return sd_rec_insert_action_at_tick(rec, &move);
}

// Insert a set-random record at tick 0.
static int insert_seed(sd_rec_file *rec, uint32_t seed) {
    uint8_t data[8] = {0};
    data[0] = REC_LOOKUP10_SETRANDOM_BYTE;
    memcpy(data + 4, &seed, sizeof(seed));
    return insert_lookup10(rec, 0, data);
}

// Parse an operand: "har1.anim" / "har2.anim", or an integer literal.
static int parse_operand(const char *text, rec_assertion_operand *out) {
    if(strncmp(text, "har1.", 5) == 0) {
        out->is_literal = false;
        out->value.attr.har_id = 0;
        out->value.attr.attribute = rec_assertion_get_har_attr(text + 5);
        return out->value.attr.attribute == ATTR_INVALID ? 1 : 0;
    }
    if(strncmp(text, "har2.", 5) == 0) {
        out->is_literal = false;
        out->value.attr.har_id = 1;
        out->value.attr.attribute = rec_assertion_get_har_attr(text + 5);
        return out->value.attr.attribute == ATTR_INVALID ? 1 : 0;
    }
    out->is_literal = true;
    out->value.literal = (int16_t)atoi(text);
    return 0;
}

// Parse an assertion spec "<lhs><op><rhs>@<tick>" and insert it as a lookup_id 10 record.
static int insert_assertion(sd_rec_file *rec, const char *spec) {
    const char *at = strrchr(spec, '@');
    if(at == NULL) {
        return 1;
    }

    uint32_t tick = (uint32_t)strtoul(at + 1, NULL, 10);

    char expr[256];
    size_t expr_len = (size_t)(at - spec);
    if(expr_len >= sizeof(expr)) {
        return 1;
    }
    memcpy(expr, spec, expr_len);
    expr[expr_len] = '\0';

    const char *op_pos = NULL;
    rec_assertion_operator op;
    if((op_pos = strstr(expr, "==")) != NULL) {
        op = OP_EQ;
    } else if((op_pos = strstr(expr, ":=")) != NULL) {
        op = OP_SET;
    } else if((op_pos = strchr(expr, '<')) != NULL) {
        op = OP_LT;
    } else if((op_pos = strchr(expr, '>')) != NULL) {
        op = OP_GT;
    } else {
        return 1;
    }

    size_t lhs_len = (size_t)(op_pos - expr);
    size_t op_len = (op == OP_LT || op == OP_GT) ? 1 : 2;
    char lhs[128];
    char rhs[128];
    if(lhs_len >= sizeof(lhs)) {
        return 1;
    }
    memcpy(lhs, expr, lhs_len);
    lhs[lhs_len] = '\0';
    snprintf(rhs, sizeof(rhs), "%s", op_pos + op_len);

    rec_assertion ass;
    memset(&ass, 0, sizeof(ass));
    ass.op = op;
    if(parse_operand(lhs, &ass.operand1) != 0 || parse_operand(rhs, &ass.operand2) != 0) {
        return 1;
    }

    uint8_t data[8] = {0};
    if(!encode_assertion(&ass, data)) {
        return 1;
    }
    return insert_lookup10(rec, tick, data);
}

int main(int argc, char *argv[]) {
    if(argc < 4) {
        fprintf(stderr, "Usage: %s <har_id> <opponent_id> <output_file> [options] [hex_byte ...]\n", argv[0]);
        fprintf(stderr, "Example: %s 2 0 thorn_test.rec --seed 1234 --assert \"har1.anim==12@64\" 40 01 40 44\n",
                argv[0]);
        fprintf(stderr, "\nAction bytes (hex):\n");
        fprintf(stderr, "  40 = RIGHT   20 = LEFT    10 = DOWN    08 = UP\n");
        fprintf(stderr, "  04 = PUNCH   02 = KICK    01 = STOP\n");
        fprintf(stderr, "  44 = RIGHT+PUNCH  60 = DOWN+LEFT  etc.\n");
        fprintf(stderr, "\nAssertion spec: <har1|har2>.<attr> <==|:=> <literal> @ <tick>\n");
        return 1;
    }

    int har_id = atoi(argv[1]);
    int opponent_id = atoi(argv[2]);
    const char *output_file = argv[3];

    if(har_id < 0 || har_id >= 11 || opponent_id < 0 || opponent_id >= 11) {
        fprintf(stderr, "Invalid HAR IDs\n");
        return 1;
    }

    // Gather inputs and options.
    int inputs[MAX_INPUTS];
    int num_inputs = 0;
    uint32_t seed = 0;
    bool has_seed = false;
    const char *assertions[MAX_ASSERTIONS];
    int num_assertions = 0;
    uint32_t explicit_ticks = 0;

    for(int i = 4; i < argc; i++) {
        const char *arg = argv[i];
        if(strcmp(arg, "--seed") == 0 && i + 1 < argc) {
            seed = (uint32_t)strtoul(argv[++i], NULL, 10);
            has_seed = true;
        } else if(strcmp(arg, "--ticks") == 0 && i + 1 < argc) {
            explicit_ticks = (uint32_t)strtoul(argv[++i], NULL, 10);
        } else if(strcmp(arg, "--assert") == 0 && i + 1 < argc) {
            if(num_assertions >= MAX_ASSERTIONS) {
                fprintf(stderr, "Too many assertions\n");
                return 1;
            }
            assertions[num_assertions++] = argv[++i];
        } else if(arg[0] == '-' && arg[1] == '-') {
            fprintf(stderr, "Unknown option: %s\n", arg);
            return 1;
        } else {
            if(num_inputs >= MAX_INPUTS) {
                fprintf(stderr, "Too many input bytes\n");
                return 1;
            }
            inputs[num_inputs++] = parse_action_byte(arg);
        }
    }

    if(num_inputs == 0 && num_assertions == 0 && !has_seed) {
        fprintf(stderr, "No inputs, assertions, or seed specified\n");
        return 1;
    }

    // Create REC file
    sd_rec_file rec;
    if(sd_rec_create(&rec) != SD_SUCCESS) {
        fprintf(stderr, "Failed to create REC file\n");
        return 1;
    }

    // Set basic match parameters
    rec.game_mode = REC_GAMEMODE_ARCADE;           // 1/2 player arcade
    rec.arena_id = 0;                              // Stadium (safe neutral arena)
    rec.p1_controller = REC_CONTROLLER_CUSTOM_KEYBOARD1; // Player keyboard
    rec.p2_controller = REC_CONTROLLER_AI;               // CPU opponent
    rec.p2_controller_ = REC_CONTROLLER_AI;
    rec.throw_range = 100;
    rec.hit_pause = 8;
    rec.block_damage = 100;
    rec.vitality = 100;
    rec.jump_height = 100;
    rec.knock_down = 3; // Both kicks and punches cause knockdown
    rec.rehit_mode = 1; // Standard rehit
    rec.def_throws = 1; // Defensive throws on
    rec.power[0] = 5;
    rec.power[1] = 5;
    rec.hazards = 0;    // No arena hazards
    rec.round_type = 0; // Best of 1 (single round)
    rec.hyper_mode = 0; // Hyper mode off

    // Set pilot info
    rec.pilots[0].info.har_id = har_id;
    rec.pilots[0].info.power = 15;
    rec.pilots[0].info.agility = 15;
    rec.pilots[0].info.endurance = 15;

    rec.pilots[1].info.har_id = opponent_id;
    rec.pilots[1].info.power = 15;
    rec.pilots[1].info.agility = 15;
    rec.pilots[1].info.endurance = 15;

    // Insert seed first (tick 0) so it precedes any inputs.
    if(has_seed && insert_seed(&rec, seed) != SD_SUCCESS) {
        fprintf(stderr, "Failed to insert seed record\n");
        sd_rec_free(&rec);
        return 1;
    }

    // Insert input sequence starting at tick 50 (after round intro).
    unsigned tick = 50;
    for(int i = 0; i < num_inputs; i++) {
        sd_rec_move move;
        memset(&move, 0, sizeof(sd_rec_move));
        move.player_id = 0; // Player 0 (the one performing the move)
        move.tick = tick;   // Set tick before inserting

        char *extra_data = sd_rec_set_lookup_id(&move, 2);
        if(extra_data) {
            extra_data[0] = (uint8_t)inputs[i];
        }

        if(sd_rec_insert_action_at_tick(&rec, &move) != SD_SUCCESS) {
            fprintf(stderr, "Failed to insert action at tick %u\n", tick);
            sd_rec_free(&rec);
            return 1;
        }
        tick++;
    }

    // Insert assertions.
    for(int i = 0; i < num_assertions; i++) {
        if(insert_assertion(&rec, assertions[i]) != SD_SUCCESS) {
            fprintf(stderr, "Invalid assertion spec: %s\n", assertions[i]);
            sd_rec_free(&rec);
            return 1;
        }
    }

    // Total length: explicit --ticks, otherwise the last event + 150 frames.
    uint32_t max_event_tick = 0;
    for(unsigned int i = 0; i < rec.move_count; i++) {
        if(rec.moves[i].tick > max_event_tick) {
            max_event_tick = rec.moves[i].tick;
        }
    }
    unsigned total_ticks = explicit_ticks > 0 ? explicit_ticks : max_event_tick + 150;

    // Finish the recording - run for substantial time to allow the move to execute.
    sd_rec_finish(&rec, total_ticks);

    // Save to file
    path output_path;
    path_from_c(&output_path, output_file);
    if(sd_rec_save(&rec, &output_path) != SD_SUCCESS) {
        fprintf(stderr, "Failed to save REC file to %s\n", output_file);
        sd_rec_free(&rec);
        return 1;
    }

    printf("Generated REC: %s (ticks: %u, inputs: %d, assertions: %d)\n", output_file, total_ticks, num_inputs,
           num_assertions);

    sd_rec_free(&rec);
    return 0;
}
