#ifndef MODMANAGER_H
#define MODMANAGER_H

#include "formats/pic.h"
#include "formats/tournament.h"
#include "resources/af.h"
#include "resources/af_move.h"
#include "resources/animation.h"
#include "resources/bk_info.h"

bool modmanager_init(void);
void modmanager_shutdown(void);
void modmanager_set_allowed(bool enabled);

bool modmanager_get_bk_background(str *name, sd_vga_image **img);
bool modmanager_get_sprite(animation_source source, str *name, int animation, int frame, sd_sprite **spr);
unsigned int modmanager_count_music(str *name);
bool modmanager_get_music(str *name, unsigned int index, unsigned char **buf, size_t *buflen);

bool modmanager_get_hitcoords(animation_source source, str *name, int animation, int frame, vector *coords,
                              vec2i *origin, vec2i *sprite_offset);
bool modmanager_get_af_move(str *name, int move_id, af_move *move_data);
bool modmanager_get_bk_animation(str *name, int anim_id, bk_info *bk_data);

bool modmanager_get_fighter_header(str *name, af *fighter);

bool modmanager_get_tournament_mod(const char *tournament_name, sd_tournament_file *tourn_data);

bool modmanager_parse_photo_mod(const char *buf, sd_pic_photo *photo);
bool modmanager_get_player_pics(sd_pic_file *pic);

/**
 * Apply JSON overlay buffers from mods for a given relative AI config path.
 *
 * Calls fn(json_buf, userdata) once for each mod overlay buffer found,
 * in mod load order. The path should use forward slashes and match the
 * path used inside the mod zip (e.g. "ai_config/pilots.json").
 *
 * Returns true if at least one overlay was applied.
 */
typedef void (*modmanager_json_overlay_fn)(const char *json_buf, void *userdata);
bool modmanager_apply_json_overlays(const char *rel_path, modmanager_json_overlay_fn fn, void *userdata);

#endif // MODMANAGER_H
