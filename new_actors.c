#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include "new_actors.h"
#include "actor_scripts.h"
#include "asc.h"
#include "bbox_tree.h"
#include "buffs.h"
#include "cal.h"
#include "console.h"
#include "dialogues.h"
#include "draw_scene.h"
#include "errors.h"
#include "filter.h"
#include "global.h"
#include "init.h"
#include "sound.h"
#include "textures.h"
#include "tiles.h"
#include "translate.h"
#include "cluster.h"
#include "eye_candy_wrapper.h"
#include "io/elfilewrapper.h"
#include "actor_init.h"
int nom_change = 0;
float sitting = 1.0f;
glow_color glow_colors[10];
//build the glow color table
void build_glow_color_table() {
	glow_colors[GLOW_NONE].r = 0;
	glow_colors[GLOW_NONE].g = 0;
	glow_colors[GLOW_NONE].b = 0;
	glow_colors[GLOW_FIRE].r = 0.3f;
	glow_colors[GLOW_FIRE].g = 0.02f;
	glow_colors[GLOW_FIRE].b = 0.0f;
	glow_colors[GLOW_COLD].r = 0.2f;
	glow_colors[GLOW_COLD].g = 0.3f;
	glow_colors[GLOW_COLD].b = 0.8f;
	glow_colors[GLOW_THERMAL].r = 0.4f;
	glow_colors[GLOW_THERMAL].g = 0.4f;
	glow_colors[GLOW_THERMAL].b = 0.6f;
	glow_colors[GLOW_MAGIC].r = 0.5f;
	glow_colors[GLOW_MAGIC].g = 0.3f;
	glow_colors[GLOW_MAGIC].b = 0.0f;
}
//return the ID (number in the actors_list[]) of the new allocated actor
int add_enhanced_actor(enhanced_actor *this_actor, float x_pos, float y_pos, float z_pos, float z_rot, float scale, int actor_id, const char *name) {
	int texture_id;
	int i;
	int k;
	actor *our_actor;
	int x, y;
	no_bounding_box = 1;
	//get the skin
	texture_id = load_enhanced_actor(this_actor, name);
	our_actor = calloc(1, sizeof(actor));
	memset(our_actor->current_displayed_text, 0, MAX_CURRENT_DISPLAYED_TEXT_LEN);
	our_actor->current_displayed_text_time_left = 0;
	our_actor->texture_id = texture_id;
	our_actor->is_enhanced_model = 1;
	our_actor->actor_id = actor_id;
	our_actor->delayed_item_changes_count = 0;
	our_actor->delay_texture_item_changes = 1;
	our_actor->x_pos = x_pos;
	our_actor->y_pos = y_pos;
	our_actor->z_pos = z_pos;
	our_actor->scale = scale;
	our_actor->x_speed = 0;
	our_actor->y_speed = 0;
	our_actor->z_speed = 0;
	our_actor->x_rot = 0;
	our_actor->y_rot = 0;
	our_actor->z_rot = z_rot;
	our_actor->last_range_attacker_id = -1;
	//reset the script related things
	our_actor->move_x_speed = 0;
	our_actor->move_y_speed = 0;
	our_actor->move_z_speed = 0;
	our_actor->rotate_x_speed = 0;
	our_actor->rotate_y_speed = 0;
	our_actor->rotate_z_speed = 0;
	our_actor->movement_time_left = 0;
	our_actor->moving = 0;
	our_actor->rotating = 0;
	our_actor->busy = 0;
	our_actor->last_command = nothing;
	//clear the que
	for (k = 0; k < MAX_CMD_QUEUE; k++) {
		our_actor->que[k] = nothing;
	}
//	our_actor->model_data=0;
	our_actor->stand_idle = 0;
	our_actor->sit_idle = 0;
	our_actor->body_parts = this_actor;
	our_actor->attached_actor = -1;
	our_actor->attachment_shift[0] = our_actor->attachment_shift[1] = our_actor->attachment_shift[2] = 0.0;
	x = (int)(our_actor->x_pos / 0.5f);
	y = (int)(our_actor->y_pos / 0.5f);
	our_actor->cluster = get_cluster(x, y);
	//find a free spot, in the actors_list
	LOCK_ACTORS_LISTS();    //lock it to avoid timing issues
	for (i = 0; i < max_actors; i++) {
		if (!actors_list[i]) {
			break;
		}
	}
	if (actor_id == yourself) {
		// We have just returned from limbo (after teleport,
		// map change, disconnect, etc.) Since our position may
		// have changed, tell the bbox tree to update so
		// that our environment is recomputed.
		set_all_intersect_update_needed(main_bbox_tree);
		set_our_actor(our_actor);
	}
	actors_list[i] = our_actor;
	if (i >= max_actors) {
		max_actors = i + 1;
	}
	no_bounding_box = 0;
	//Actors list will be unlocked later
	return i;
}
Uint32 delay_texture_item_change(actor *a, const int which_part, const int which_id) {
	if (a == 0) {
		return 0;
	}
	if (a->delay_texture_item_changes != 0) {
		change_enhanced_actor(a->texture_id, a->body_parts);
		if (a->delayed_item_changes_count < MAX_ITEM_CHANGES_QUEUE) {
			a->delayed_item_changes[a->delayed_item_changes_count] = which_id;
			a->delayed_item_type_changes[a->delayed_item_changes_count] = which_part;
			a->delayed_item_changes_count++;
			return 1;
		}
	}
	return 0;
}
void unwear_item_from_actor(int actor_id, Uint8 which_part) {
	int i;
	for (i = 0; i < max_actors; i++) {
		if (actors_list[i]) {
			if (actors_list[i]->actor_id == actor_id) {
				if (which_part == KIND_OF_WEAPON) {
					ec_remove_weapon(actors_list[i]);
					if (actors_list[i]->cur_weapon == GLOVE_FUR || actors_list[i]->cur_weapon == GLOVE_LEATHER || actors_list[i]->cur_weapon == GANTS_CUIR_NOIR || actors_list[i]->cur_weapon == GLOVE_FUR_LEO || actors_list[i]->cur_weapon == GLOVE_LEATHER_3) {
						my_strcp(actors_list[i]->body_parts->hands_tex, actors_list[i]->body_parts->hands_tex_save);
					}
					if (delay_texture_item_change(actors_list[i], which_part, -1)) {
						return;
					}
					model_detach_mesh(actors_list[i], actors_defs[actors_list[i]->actor_type].weapon[actors_list[i]->cur_weapon].mesh_index);
					actors_list[i]->body_parts->weapon_tex[0] = 0;
					actors_list[i]->cur_weapon = WEAPON_NONE;
					actors_list[i]->body_parts->weapon_meshindex = -1;
					return;
				}
				if (which_part == KIND_OF_SHIELD) {
					model_detach_mesh(actors_list[i], actors_list[i]->body_parts->shield_meshindex);
					actors_list[i]->body_parts->shield_tex[0] = 0;
					actors_list[i]->cur_shield = SHIELD_NONE;
					actors_list[i]->body_parts->shield_meshindex = -1;
					return;
				}
				if (which_part == KIND_OF_CAPE) {
					model_detach_mesh(actors_list[i], actors_list[i]->body_parts->cape_meshindex);
					actors_list[i]->body_parts->cape_tex[0] = 0;
					actors_list[i]->body_parts->cape_meshindex = -1;
					return;
				}
				if (which_part == KIND_OF_HELMET) {
					model_detach_mesh(actors_list[i], actors_list[i]->body_parts->helmet_meshindex);
					actors_list[i]->body_parts->helmet_tex[0] = 0;
					actors_list[i]->body_parts->helmet_meshindex = -1;
					return;
				}
				if (which_part == KIND_OF_MEDAILLON) {
					model_detach_mesh(actors_list[i], actors_list[i]->body_parts->neck_meshindex);
					actors_list[i]->body_parts->neck_tex[0] = 0;
					actors_list[i]->body_parts->neck_meshindex = -1;
					return;
				}
				return;
			}
		}
	}
}
void actor_wear_item(int actor_id, Uint8 which_part, Uint8 which_id) {
	int i;
	for (i = 0; i < max_actors; i++) {
		if (actors_list[i]) {
			if (actors_list[i]->actor_id == actor_id) {
				if (which_part == KIND_OF_WEAPON) {
					if (which_id == GLOVE_FUR || which_id == GLOVE_LEATHER || which_id == GANTS_CUIR_NOIR || which_id == GLOVE_FUR_LEO || which_id == GLOVE_LEATHER_3) {
						my_strcp(actors_list[i]->body_parts->hands_tex, actors_defs[actors_list[i]->actor_type].weapon[which_id].skin_name);
						my_strcp(actors_list[i]->body_parts->hands_mask, actors_defs[actors_list[i]->actor_type].weapon[which_id].skin_mask);
					} else {
						my_strcp(actors_list[i]->body_parts->weapon_tex, actors_defs[actors_list[i]->actor_type].weapon[which_id].skin_name);
					}
					if (delay_texture_item_change(actors_list[i], which_part, which_id)) {
						return;
					}
					model_attach_mesh(actors_list[i], actors_defs[actors_list[i]->actor_type].weapon[which_id].mesh_index);
					actors_list[i]->cur_weapon = which_id;
					actors_list[i]->body_parts->weapon_meshindex = actors_defs[actors_list[i]->actor_type].weapon[which_id].mesh_index;
					actors_list[i]->body_parts->weapon_glow = actors_defs[actors_list[i]->actor_type].weapon[which_id].glow;
					switch (which_id) {
					case SWORD_1_FIRE:
					case SWORD_2_FIRE:
					case SWORD_3_FIRE:
					case SWORD_4_FIRE:
					case SWORD_4_THERMAL:
					case SWORD_5_FIRE:
					case SWORD_5_THERMAL:
					case SWORD_6_FIRE:
					case SWORD_6_THERMAL:
					case SWORD_7_FIRE:
					case SWORD_7_THERMAL:
						ec_create_sword_of_fire(actors_list[i], (poor_man ? 6 : 10));
						break;
					case SWORD_2_COLD:
					case SWORD_3_COLD:
					case SWORD_4_COLD:
					case SWORD_5_COLD:
					case SWORD_6_COLD:
					case SWORD_7_COLD:
						ec_create_sword_of_ice(actors_list[i], (poor_man ? 6 : 10));
						break;
					case SWORD_3_MAGIC:
					case SWORD_4_MAGIC:
					case SWORD_5_MAGIC:
					case SWORD_6_MAGIC:
					case SWORD_7_MAGIC:
						ec_create_sword_of_magic(actors_list[i], (poor_man ? 6 : 10));
						break;
					case SWORD_EMERALD_CLAYMORE:
						ec_create_sword_emerald_claymore(actors_list[i], (poor_man ? 6 : 10));
						break;
					case SWORD_CUTLASS:
						ec_create_sword_cutlass(actors_list[i], (poor_man ? 6 : 10));
						break;
					case SWORD_SUNBREAKER:
						ec_create_sword_sunbreaker(actors_list[i], (poor_man ? 6 : 10));
						break;
					case SWORD_ORC_SLAYER:
						ec_create_sword_orc_slayer(actors_list[i], (poor_man ? 6 : 10));
						break;
					case SWORD_EAGLE_WING:
						ec_create_sword_eagle_wing(actors_list[i], (poor_man ? 6 : 10));
						break;
					case SWORD_JAGGED_SABER:
						ec_create_sword_jagged_saber(actors_list[i], (poor_man ? 6 : 10));
						break;
					case STAFF_4:                                 // staff of the mage
					case BATON_MAGE_DIAMANT:
						ec_create_staff_of_the_mage(actors_list[i], (poor_man ? 6 : 10));
						break;
					case SWORD_EMERALD_CLAYMORE_F:
						ec_create_sword_emerald_claymore_feu(actors_list[i], (poor_man ? 6 : 10));
						break;
					case SWORD_EMERALD_CLAYMORE_G:
						ec_create_sword_emerald_claymore_glace(actors_list[i], (poor_man ? 6 : 10));
						break;
					case SWORD_EMERALD_CLAYMORE_A:
						ec_create_sword_emerald_claymore_arcane(actors_list[i], (poor_man ? 6 : 10));
						break;
					case SWORD_EMERALD_CLAYMORE_L:
						ec_create_sword_emerald_claymore_lumiere(actors_list[i], (poor_man ? 6 : 10));
						break;
					case SWORD_CUTLASS_F:
						ec_create_sword_cutlass_feu(actors_list[i], (poor_man ? 6 : 10));
						break;
					case SWORD_CUTLASS_G:
						ec_create_sword_cutlass_glace(actors_list[i], (poor_man ? 6 : 10));
						break;
					case SWORD_CUTLASS_A:
						ec_create_sword_cutlass_arcane(actors_list[i], (poor_man ? 6 : 10));
						break;
					case SWORD_CUTLASS_L:
						ec_create_sword_cutlass_lumiere(actors_list[i], (poor_man ? 6 : 10));
						break;
					case SWORD_SUNBREAKER_F:
						ec_create_sword_sunbreaker_feu(actors_list[i], (poor_man ? 6 : 10));
						break;
					case SWORD_SUNBREAKER_G:
						ec_create_sword_sunbreaker_glace(actors_list[i], (poor_man ? 6 : 10));
						break;
					case SWORD_SUNBREAKER_A:
						ec_create_sword_sunbreaker_arcane(actors_list[i], (poor_man ? 6 : 10));
						break;
					case SWORD_SUNBREAKER_L:
						ec_create_sword_sunbreaker_lumiere(actors_list[i], (poor_man ? 6 : 10));
						break;
					case SWORD_ORC_SLAYER_F:
						ec_create_sword_orc_slayer_feu(actors_list[i], (poor_man ? 6 : 10));
						break;
					case SWORD_ORC_SLAYER_G:
						ec_create_sword_orc_slayer_glace(actors_list[i], (poor_man ? 6 : 10));
						break;
					case SWORD_ORC_SLAYER_A:
						ec_create_sword_orc_slayer_arcane(actors_list[i], (poor_man ? 6 : 10));
						break;
					case SWORD_ORC_SLAYER_L:
						ec_create_sword_orc_slayer_lumiere(actors_list[i], (poor_man ? 6 : 10));
						break;
					case SWORD_EAGLE_WING_F:
						ec_create_sword_eagle_wing_feu(actors_list[i], (poor_man ? 6 : 10));
						break;
					case SWORD_EAGLE_WING_G:
						ec_create_sword_eagle_wing_glace(actors_list[i], (poor_man ? 6 : 10));
						break;
					case SWORD_EAGLE_WING_A:
						ec_create_sword_eagle_wing_arcane(actors_list[i], (poor_man ? 6 : 10));
						break;
					case SWORD_EAGLE_WING_L:
						ec_create_sword_eagle_wing_lumiere(actors_list[i], (poor_man ? 6 : 10));
						break;
					case SWORD_JAGGED_SABER_F:
						ec_create_sword_jagged_saber_feu(actors_list[i], (poor_man ? 6 : 10));
						break;
					case SWORD_JAGGED_SABER_G:
						ec_create_sword_jagged_saber_glace(actors_list[i], (poor_man ? 6 : 10));
						break;
					case SWORD_JAGGED_SABER_A:
						ec_create_sword_jagged_saber_arcane(actors_list[i], (poor_man ? 6 : 10));
						break;
					case SWORD_JAGGED_SABER_L:
						ec_create_sword_jagged_saber_lumiere(actors_list[i], (poor_man ? 6 : 10));
						break;
					}
				} else if (which_part == KIND_OF_SHIELD) {
					my_strcp(actors_list[i]->body_parts->shield_tex, actors_defs[actors_list[i]->actor_type].shield[which_id].skin_name);
					if (delay_texture_item_change(actors_list[i], which_part, which_id)) {
						return;
					}
					model_attach_mesh(actors_list[i], actors_defs[actors_list[i]->actor_type].shield[which_id].mesh_index);
					actors_list[i]->body_parts->shield_meshindex = actors_defs[actors_list[i]->actor_type].shield[which_id].mesh_index;
					actors_list[i]->cur_shield = which_id;
					actors_list[i]->body_parts->shield_meshindex = actors_defs[actors_list[i]->actor_type].shield[which_id].mesh_index;
				} else if (which_part == KIND_OF_CAPE) {
					my_strcp(actors_list[i]->body_parts->cape_tex, actors_defs[actors_list[i]->actor_type].cape[which_id].skin_name);
					if (delay_texture_item_change(actors_list[i], which_part, which_id)) {
						return;
					}
					model_attach_mesh(actors_list[i], actors_defs[actors_list[i]->actor_type].cape[which_id].mesh_index);
					actors_list[i]->body_parts->cape_meshindex = actors_defs[actors_list[i]->actor_type].cape[which_id].mesh_index;
				} else if (which_part == KIND_OF_HELMET) {
					my_strcp(actors_list[i]->body_parts->helmet_tex, actors_defs[actors_list[i]->actor_type].helmet[which_id].skin_name);
					if (delay_texture_item_change(actors_list[i], which_part, which_id)) {
						return;
					}
					model_attach_mesh(actors_list[i], actors_defs[actors_list[i]->actor_type].helmet[which_id].mesh_index);
					actors_list[i]->body_parts->helmet_meshindex = actors_defs[actors_list[i]->actor_type].helmet[which_id].mesh_index;
				} else if (which_part == KIND_OF_MEDAILLON) {
					assert(!"Using old client data" || actors_defs[actors_list[i]->actor_type].neck != NULL);
					my_strcp(actors_list[i]->body_parts->neck_tex, actors_defs[actors_list[i]->actor_type].neck[which_id].skin_name);
					if (delay_texture_item_change(actors_list[i], which_part, which_id)) {
						return;
					}
					model_attach_mesh(actors_list[i], actors_defs[actors_list[i]->actor_type].neck[which_id].mesh_index);
					actors_list[i]->body_parts->neck_meshindex = actors_defs[actors_list[i]->actor_type].neck[which_id].mesh_index;
				} else if (which_part == KIND_OF_BODY_ARMOR) {
					my_strcp(actors_list[i]->body_parts->arms_tex, actors_defs[actors_list[i]->actor_type].shirt[which_id].arms_name);
					my_strcp(actors_list[i]->body_parts->torso_tex, actors_defs[actors_list[i]->actor_type].shirt[which_id].torso_name);
					my_strcp(actors_list[i]->body_parts->arms_mask, actors_defs[actors_list[i]->actor_type].shirt[which_id].arms_mask);
					my_strcp(actors_list[i]->body_parts->torso_mask, actors_defs[actors_list[i]->actor_type].shirt[which_id].torso_mask);
					if (delay_texture_item_change(actors_list[i], which_part, which_id)) {
						return;
					}
					if (actors_defs[actors_list[i]->actor_type].shirt[which_id].mesh_index != actors_list[i]->body_parts->torso_meshindex) {
						model_detach_mesh(actors_list[i], actors_list[i]->body_parts->torso_meshindex);
						model_attach_mesh(actors_list[i], actors_defs[actors_list[i]->actor_type].shirt[which_id].mesh_index);
						actors_list[i]->body_parts->torso_meshindex = actors_defs[actors_list[i]->actor_type].shirt[which_id].mesh_index;
					}
				} else if (which_part == KIND_OF_LEG_ARMOR) {
					my_strcp(actors_list[i]->body_parts->pants_tex, actors_defs[actors_list[i]->actor_type].legs[which_id].legs_name);
					my_strcp(actors_list[i]->body_parts->pants_mask, actors_defs[actors_list[i]->actor_type].legs[which_id].legs_mask);
					if (delay_texture_item_change(actors_list[i], which_part, which_id)) {
						return;
					}
					if (actors_defs[actors_list[i]->actor_type].legs[which_id].mesh_index != actors_list[i]->body_parts->legs_meshindex) {
						model_detach_mesh(actors_list[i], actors_list[i]->body_parts->legs_meshindex);
						model_attach_mesh(actors_list[i], actors_defs[actors_list[i]->actor_type].legs[which_id].mesh_index);
						actors_list[i]->body_parts->legs_meshindex = actors_defs[actors_list[i]->actor_type].legs[which_id].mesh_index;
					}
				} else if (which_part == KIND_OF_BOOT_ARMOR) {
					my_strcp(actors_list[i]->body_parts->boots_tex, actors_defs[actors_list[i]->actor_type].boots[which_id].boots_name);
					my_strcp(actors_list[i]->body_parts->boots_mask, actors_defs[actors_list[i]->actor_type].boots[which_id].boots_mask);
					if (delay_texture_item_change(actors_list[i], which_part, which_id)) {
						return;
					}
					if (actors_defs[actors_list[i]->actor_type].boots[which_id].mesh_index != actors_list[i]->body_parts->boots_meshindex) {
						model_detach_mesh(actors_list[i], actors_list[i]->body_parts->boots_meshindex);
						model_attach_mesh(actors_list[i], actors_defs[actors_list[i]->actor_type].boots[which_id].mesh_index);
						actors_list[i]->body_parts->boots_meshindex = actors_defs[actors_list[i]->actor_type].boots[which_id].mesh_index;
					}
				} else {
					return;
				}
				return;
			}
		}
	}
}
void ajout_titre(const char *donnees, int longueur) {
	short acteur_id;
	int i;
	acteur_id = SDL_SwapLE16(*((short *)(donnees)));
	for (i = 0 ; i < max_actors ; i++) {
		if (actors_list[i]) {
			if (actors_list[i]->actor_id == acteur_id) {
				// Il n'y a pas de titre, on met donc 0
				if (longueur <= 2) {
					safe_snprintf(actors_list[i]->titre, 2, "0");
				} else {
					memcpy(actors_list[i]->titre, donnees + 2, longueur - 2);
				}
			}
		}
	}
}
void add_enhanced_actor_from_server(const char *in_data, int len) {
	short actor_id;
	Uint32 buffs;
	short x_pos;
	short y_pos;
	short z_rot;
	short max_health;
	short cur_health;
	Uint32 actor_type;
	Uint8 skin;
	Uint8 hair;
	Uint8 shirt;
	Uint8 pants;
	Uint8 boots;
	Uint8 frame;
	Uint8 cape;
	Uint8 head;
	Uint8 shield;
	Uint8 weapon;
	Uint8 helmet;
	Uint8 neck;
	int i;
	int dead = 0;
	int kind_of_actor;
	enhanced_actor *this_actor;
	char onlyname[32] = {0};
	Uint32 uniq_id; // - Post ported.... We'll come up with something later...
	Uint32 guild_id;
	double f_x_pos, f_y_pos, f_z_rot;
	float scale = 1.0f;
	int attachment_type = -1;
	actor_id = SDL_SwapLE16(*((short *)(in_data)));
	buffs = (((SDL_SwapLE16(*((char *)(in_data + 3))) >> 3) & 0x1F) | (SDL_SwapLE16(((*((char *)(in_data + 5))) >> 3) & 0x1F) << 5)); // Strip the last 5 bits of the X and Y coords for the buffs
	x_pos = SDL_SwapLE16(*((short *)(in_data + 2))) & 0x7FF;
	y_pos = SDL_SwapLE16(*((short *)(in_data + 4))) & 0x7FF;
	buffs |= (SDL_SwapLE16(*((short *)(in_data + 6))) & 0xFF80) << 3; // we get the 9 MSB for the buffs and leave the 7 LSB for a further use
	z_rot = SDL_SwapLE16(*((short *)(in_data + 8)));
	actor_type = *(in_data + 10);
	skin = *(in_data + 12);
	hair = *(in_data + 13);
	shirt = *(in_data + 14);
	pants = *(in_data + 15);
	boots = *(in_data + 16);
	head = *(in_data + 17);
	shield = *(in_data + 18);
	weapon = *(in_data + 19);
	cape = *(in_data + 20);
	helmet = *(in_data + 21);
	if (actor_type >= MAX_ACTOR_DEFS || (actor_type > 0 && actors_defs[actor_type].actor_type != actor_type)) {
		char str[256];
		safe_snprintf(str, sizeof(str), "Illegal/missing enhanced actor definition %d", actor_type);
		LOG_ERROR(str);
		return;         // We cannot load an actor without a def (seg fault) so bail here.
	}
	frame = *(in_data + 22);
	max_health = SDL_SwapLE16(*((short *)(in_data + 23)));
	cur_health = SDL_SwapLE16(*((short *)(in_data + 25)));
	kind_of_actor = *(in_data + 27);
	if (len > 28 + (int)strlen(in_data + 28) + 2) {
		scale = ((float)SDL_SwapLE16(*((short *)(in_data + 28 + strlen(in_data + 28) + 1))) / ((float)ACTOR_SCALE_BASE));
		if (len > 28 + (int)strlen(in_data + 28) + 3) {
			attachment_type = (unsigned char)in_data[28 + strlen(in_data + 28) + 3];
		}
		// Le serveur ne gère pas les acteurs liés
		attachment_type = -1;
	}
	//the last byte of the packet even if scale+attachment is not sent
	neck = *(in_data + len - 1);
	//the last bytes of the packet even if scale+attachment is not sent
	//translate from tile to world
	f_x_pos = x_pos * 0.5;
	f_y_pos = y_pos * 0.5;
	f_z_rot = z_rot;
	//get the current frame
	switch (frame) {
	case frame_walk:
	case frame_run:
		break;
	case frame_die1:
		dead = 1;
		break;
	case frame_die2:
		dead = 1;
		break;
	case frame_pain1:
	case frame_pain2:
	case frame_drop:
	case frame_idle:
	case frame_sit_idle:
	case frame_harvest:
	case frame_cast:
	case frame_ranged:
	case frame_attack_up_1:
	case frame_attack_up_2:
	case frame_attack_up_3:
	case frame_attack_up_4:
	case frame_attack_up_5:
	case frame_attack_up_6:
	case frame_attack_up_7:
	case frame_attack_up_8:
	case frame_attack_up_9:
	case frame_attack_up_10:
	case frame_attack_down_1:
	case frame_attack_down_2:
	case frame_attack_down_3:
	case frame_attack_down_4:
	case frame_attack_down_5:
	case frame_attack_down_6:
	case frame_attack_down_7:
	case frame_attack_down_8:
	case frame_attack_down_9:
	case frame_attack_down_10:
	case frame_combat_idle:
	case frame_salut:
		break;
	default:
		LOG_ERROR("%s %d - %s\n", unknown_frame, frame, &in_data[28]);
	}
	//find out if there is another actor with that ID
	//ideally this shouldn't happen, but just in case
	for (i = 0; i < max_actors; i++) {
		if (actors_list[i]) {
			if (actors_list[i]->actor_id == actor_id) {
				LOG_ERROR("%s %d = %s => %s\n", duplicate_actors_str, actor_id, actors_list[i]->actor_name, &in_data[28]);
				destroy_actor(actors_list[i]->actor_id);                        //we don't want two actors with the same ID
				i--;                        // last actor was put here, he needs to be checked too
			} else if (kind_of_actor == COMPUTER_CONTROLLED_HUMAN && (actors_list[i]->kind_of_actor == COMPUTER_CONTROLLED_HUMAN || actors_list[i]->kind_of_actor == PKABLE_COMPUTER_CONTROLLED) && !my_strcompare(&in_data[28], actors_list[i]->actor_name)) {
				LOG_ERROR("%s(%d) = %s => %s\n", duplicate_npc_actor, actor_id, actors_list[i]->actor_name, &in_data[28]);
				destroy_actor(actors_list[i]->actor_id);                        //we don't want two actors with the same ID
				i--;                        // last actor was put here, he needs to be checked too
			}
		}
	}
	this_actor = calloc(1, sizeof(enhanced_actor));
	/* build a clean player name and a guild id */
	{
		/* get the name string into a working buffer */
		char buffer[256], *name, *guild;
		my_strncp(buffer, &in_data[28], sizeof(buffer));
		uniq_id = 0;
		name = buffer;
		/* search for string end or color mark */
		this_actor->guild_tag_color = 0;
		for (guild = name; *guild && is_printable(*guild); guild++) {}
		if (*guild) {
			/* separate the two strings */
			this_actor->guild_tag_color = from_color_char(*guild);
			*guild = 0;
			guild++;
		}
		/* perform case insensitive comparison/hashing */
		my_tolower(name);
		my_tolower(onlyname);
		//perfect hashing of guildtag
		switch (strlen(guild)) {
		case 0:
			guild_id = 0;
			break;
		case 1:
			guild_id = guild[0];
			break;
		case 2:
			guild_id = guild[0] + (guild[1] << 8);
			break;
		case 3:
			guild_id = guild[0] + (guild[1] << 8) + (guild[2] << 16);
			break;
		default:
			guild_id = guild[0] + (guild[1] << 8) + (guild[2] << 16) + (guild[3] << 24);
			break;
		}
	}
	/* store the ids */
	this_actor->uniq_id = uniq_id;
	this_actor->guild_id = guild_id;
	//get the torso
	my_strncp(this_actor->arms_tex, actors_defs[actor_type].shirt[shirt].arms_name, sizeof(this_actor->arms_tex));
	my_strncp(this_actor->torso_tex, actors_defs[actor_type].shirt[shirt].torso_name, sizeof(this_actor->torso_tex));
	my_strncp(this_actor->arms_mask, actors_defs[actor_type].shirt[shirt].arms_mask, sizeof(this_actor->arms_mask));
	my_strncp(this_actor->torso_mask, actors_defs[actor_type].shirt[shirt].torso_mask, sizeof(this_actor->torso_mask));
	//skin
	my_strncp(this_actor->hands_tex, actors_defs[actor_type].skin[skin].hands_name, sizeof(this_actor->hands_tex));
	my_strncp(this_actor->hands_tex_save, actors_defs[actor_type].skin[skin].hands_name, sizeof(this_actor->hands_tex_save));
	my_strncp(this_actor->hands_mask, "", sizeof(this_actor->hands_mask));    // by default, nothing
	my_strncp(this_actor->head_tex, actors_defs[actor_type].skin[skin].head_name, sizeof(this_actor->head_tex));
	my_strncp(this_actor->head_base, actors_defs[actor_type].skin[skin].head_name, sizeof(this_actor->head_base));
	my_strncp(this_actor->head_mask, "", sizeof(this_actor->head_mask));      // by default, nothing
	if (*actors_defs[actor_type].head[head].skin_name) {
		my_strncp(this_actor->head_tex, actors_defs[actor_type].head[head].skin_name, sizeof(this_actor->head_tex));
	}
	if (*actors_defs[actor_type].head[head].skin_mask) {
		my_strncp(this_actor->head_mask, actors_defs[actor_type].head[head].skin_mask, sizeof(this_actor->head_mask));
	}
	my_strncp(this_actor->body_base, actors_defs[actor_type].skin[skin].body_name, sizeof(this_actor->body_base));
	my_strncp(this_actor->arms_base, actors_defs[actor_type].skin[skin].arms_name, sizeof(this_actor->arms_base));
	my_strncp(this_actor->legs_base, actors_defs[actor_type].skin[skin].legs_name, sizeof(this_actor->legs_base));
	my_strncp(this_actor->boots_base, actors_defs[actor_type].skin[skin].feet_name, sizeof(this_actor->boots_base));
	//hair
	my_strncp(this_actor->hair_tex, actors_defs[actor_type].hair[hair].hair_name, sizeof(this_actor->hair_tex));
	//boots
	my_strncp(this_actor->boots_tex, actors_defs[actor_type].boots[boots].boots_name, sizeof(this_actor->boots_tex));
	my_strncp(this_actor->boots_mask, actors_defs[actor_type].boots[boots].boots_mask, sizeof(this_actor->boots_mask));
	//legs
	my_strncp(this_actor->pants_tex, actors_defs[actor_type].legs[pants].legs_name, sizeof(this_actor->pants_tex));
	my_strncp(this_actor->pants_mask, actors_defs[actor_type].legs[pants].legs_mask, sizeof(this_actor->pants_mask));
	//cape
	if (cape != CAPE_NONE) {
		my_strncp(this_actor->cape_tex, actors_defs[actor_type].cape[cape].skin_name, sizeof(this_actor->cape_tex));
	} else {
		my_strncp(this_actor->cape_tex, "", sizeof(this_actor->cape_tex));
	}
	//shield
	if (shield != SHIELD_NONE) {
		my_strncp(this_actor->shield_tex, actors_defs[actor_type].shield[shield].skin_name, sizeof(this_actor->shield_tex));
	} else {
		my_strncp(this_actor->shield_tex, "", sizeof(this_actor->shield_tex));
	}
	if (weapon == GLOVE_FUR || weapon == GLOVE_LEATHER) {
		my_strncp(this_actor->hands_tex, actors_defs[actor_type].weapon[weapon].skin_name, sizeof(this_actor->hands_tex));
	} else {
		my_strncp(this_actor->weapon_tex, actors_defs[actor_type].weapon[weapon].skin_name, sizeof(this_actor->weapon_tex));
	}
	this_actor->weapon_glow = actors_defs[actor_type].weapon[weapon].glow;
	if (weapon == GLOVE_FUR || weapon == GLOVE_LEATHER || weapon == GANTS_CUIR_NOIR || weapon == GLOVE_FUR_LEO || weapon == GLOVE_LEATHER_3) {
		my_strncp(this_actor->hands_tex, actors_defs[actor_type].weapon[weapon].skin_name, sizeof(this_actor->hands_tex));
	}
	//helmet
	if (helmet != HELMET_NONE) {
		my_strncp(this_actor->helmet_tex, actors_defs[actor_type].helmet[helmet].skin_name, sizeof(this_actor->helmet_tex));
	} else {
		my_strncp(this_actor->helmet_tex, "", sizeof(this_actor->helmet_tex));
	}
	//neck
	if (neck != MEDAILLON_NONE) {
		assert(!"Using old client data" || actors_defs[actor_type].neck != NULL);
		my_strncp(this_actor->neck_tex, actors_defs[actor_type].neck[neck].skin_name, sizeof(this_actor->neck_tex));
	} else {
		my_strncp(this_actor->neck_tex, "", sizeof(this_actor->neck_tex));
	}
	i = add_enhanced_actor(this_actor, f_x_pos, f_y_pos, 0.0, f_z_rot, scale, actor_id, onlyname);
	//The actors list is already locked here
	actors_list[i]->async_fighting = 0;
	actors_list[i]->async_x_tile_pos = x_pos;
	actors_list[i]->async_y_tile_pos = y_pos;
	actors_list[i]->async_z_rot = z_rot;
	actors_list[i]->x_tile_pos = x_pos;
	actors_list[i]->y_tile_pos = y_pos;
	actors_list[i]->buffs = buffs;
	actors_list[i]->actor_type = actor_type;
	actors_list[i]->damage = 0;
	actors_list[i]->damage_ms = 0;
	actors_list[i]->sitting = 0;
	actors_list[i]->fighting = 0;
	//test only
	actors_list[i]->max_health = max_health;
	actors_list[i]->cur_health = cur_health;
	actors_list[i]->z_pos = get_actor_z(actors_list[i]);
	if (frame == frame_sit_idle) {
		if (actors_list[i]->actor_id == yourself) {
			you_sit = 1;
		}
		actors_list[i]->sitting = 1;
	} else {
		if (actors_list[i]->actor_id == yourself) {
			you_sit = 0;
		}
		if (frame == frame_combat_idle) {
			actors_list[i]->fighting = 1;
		}
	}
	//ghost or not?
	actors_list[i]->ghost = actors_defs[actor_type].ghost;
	actors_list[i]->dead = dead;
	actors_list[i]->stop_animation = 1;//helps when the actor is dead...
	actors_list[i]->cur_weapon = weapon;
	actors_list[i]->cur_shield = shield;
	actors_list[i]->kind_of_actor = kind_of_actor;
	if (strlen(&in_data[28]) >= 30) {
		LOG_ERROR("%s (%d): %s/%d\n", bad_actor_name_length, actors_list[i]->actor_type, &in_data[28], (int)strlen(&in_data[28]));
	} else {
		/* Extract the name for use in the tab completion list. */
		const unsigned char *name = (unsigned char *)in_data + 28;
		char *ptr;
		char buffer[32];
		if (kind_of_actor != NPC) {
			/* Skip leading color codes */
			for (; *name && is_color(*name); name++) { /* nothing */
			}
			safe_snprintf(buffer, sizeof(buffer), "%.30s", name);
			/* Remove guild tag, etc. */
			for (ptr = buffer; *ptr && *ptr > 0 && !isspace(*ptr); ptr++) {}
			*ptr = '\0';
			add_name_to_tablist(buffer);
		}
		my_strncp(actors_list[i]->actor_name, &in_data[28], sizeof(actors_list[i]->actor_name));
		if (caps_filter && my_isupper(actors_list[i]->actor_name, -1)) {
			my_tolower(actors_list[i]->actor_name);
		}
	}
	if (attachment_type >= 0 && attachment_type < 255) { //255 is not necessary, but it suppresses a warning in errorlog
		add_actor_attachment(actor_id, attachment_type);
	}
	if (actors_defs[actor_type].coremodel != NULL) {
		actors_list[i]->calmodel = model_new(actors_defs[actor_type].coremodel);
		if (actors_list[i]->calmodel != NULL) {
			//Setup cal3d model
			//actors_list[i]->calmodel=CalModel_New(actors_defs[actor_type].coremodel);
			//Attach meshes
			model_attach_mesh(actors_list[i], actors_defs[actor_type].head[head].mesh_index);
			model_attach_mesh(actors_list[i], actors_defs[actor_type].shirt[shirt].mesh_index);
			model_attach_mesh(actors_list[i], actors_defs[actor_type].legs[pants].mesh_index);
			model_attach_mesh(actors_list[i], actors_defs[actor_type].boots[boots].mesh_index);
			actors_list[i]->body_parts->torso_meshindex = actors_defs[actor_type].shirt[shirt].mesh_index;
			actors_list[i]->body_parts->legs_meshindex = actors_defs[actor_type].legs[pants].mesh_index;
			actors_list[i]->body_parts->head_meshindex = actors_defs[actor_type].head[head].mesh_index;
			actors_list[i]->body_parts->boots_meshindex = actors_defs[actor_type].boots[boots].mesh_index;
			if (cape != CAPE_NONE) {
				model_attach_mesh(actors_list[i], actors_defs[actor_type].cape[cape].mesh_index);
			}
			if (helmet != HELMET_NONE) {
				model_attach_mesh(actors_list[i], actors_defs[actor_type].helmet[helmet].mesh_index);
			}
			if (weapon != WEAPON_NONE) {
				model_attach_mesh(actors_list[i], actors_defs[actor_type].weapon[weapon].mesh_index);
			}
			if (shield != SHIELD_NONE) {
				model_attach_mesh(actors_list[i], actors_defs[actor_type].shield[shield].mesh_index);
			}
			assert(!"Using old client data" || actors_defs[actor_type].neck != NULL);
			if (neck != MEDAILLON_NONE) {
				model_attach_mesh(actors_list[i], actors_defs[actor_type].neck[neck].mesh_index);
			}
			actors_list[i]->body_parts->neck_meshindex = actors_defs[actor_type].neck[neck].mesh_index;
			actors_list[i]->body_parts->helmet_meshindex = actors_defs[actor_type].helmet[helmet].mesh_index;
			actors_list[i]->body_parts->cape_meshindex = actors_defs[actor_type].cape[cape].mesh_index;
			actors_list[i]->body_parts->shield_meshindex = actors_defs[actor_type].shield[shield].mesh_index;
			actors_list[i]->cur_anim.anim_index = -1;
			stop_sound(actors_list[i]->cur_anim_sound_cookie);
			actors_list[i]->cur_anim_sound_cookie = 0;
			actors_list[i]->anim_time = 0.0;
			actors_list[i]->last_anim_update = cur_time;
			if (dead) {
				cal_actor_set_anim(i, actors_defs[actors_list[i]->actor_type].cal_frames[cal_actor_die1_frame]);
				actors_list[i]->stop_animation = 1;
				CalModel_Update(actors_list[i]->calmodel, 1000);
			} else {
				/* Schmurk: we explicitly go on idle here to avoid weird
				 * flickering when actors appear */
				set_on_idle(i);
				/* CalModel_Update(actors_list[i]->calmodel,0); */
			}
			build_actor_bounding_box(actors_list[i]);
			switch (weapon) {
			case SWORD_1_FIRE:
			case SWORD_2_FIRE:
			case SWORD_3_FIRE:
			case SWORD_4_FIRE:
			case SWORD_4_THERMAL:
			case SWORD_5_FIRE:
			case SWORD_5_THERMAL:
			case SWORD_6_FIRE:
			case SWORD_6_THERMAL:
			case SWORD_7_FIRE:
			case SWORD_7_THERMAL:
				ec_create_sword_of_fire(actors_list[i], (poor_man ? 6 : 10));
				break;
			case SWORD_2_COLD:
			case SWORD_3_COLD:
			case SWORD_4_COLD:
			case SWORD_5_COLD:
			case SWORD_6_COLD:
			case SWORD_7_COLD:
				ec_create_sword_of_ice(actors_list[i], (poor_man ? 6 : 10));
				break;
			case SWORD_3_MAGIC:
			case SWORD_4_MAGIC:
			case SWORD_5_MAGIC:
			case SWORD_6_MAGIC:
			case SWORD_7_MAGIC:
				ec_create_sword_of_magic(actors_list[i], (poor_man ? 6 : 10));
				break;
			case SWORD_EMERALD_CLAYMORE:
				ec_create_sword_emerald_claymore(actors_list[i], (poor_man ? 6 : 10));
				break;
			case SWORD_CUTLASS:
				ec_create_sword_cutlass(actors_list[i], (poor_man ? 6 : 10));
				break;
			case SWORD_SUNBREAKER:
				ec_create_sword_sunbreaker(actors_list[i], (poor_man ? 6 : 10));
				break;
			case SWORD_ORC_SLAYER:
				ec_create_sword_orc_slayer(actors_list[i], (poor_man ? 6 : 10));
				break;
			case SWORD_EAGLE_WING:
				ec_create_sword_eagle_wing(actors_list[i], (poor_man ? 6 : 10));
				break;
			case SWORD_JAGGED_SABER:
				ec_create_sword_jagged_saber(actors_list[i], (poor_man ? 6 : 10));
				break;
			case STAFF_4:         // staff of the mage
				ec_create_staff_of_the_mage(actors_list[i], (poor_man ? 6 : 10));
				break;
			case SWORD_EMERALD_CLAYMORE_F:
				ec_create_sword_emerald_claymore_feu(actors_list[i], (poor_man ? 6 : 10));
				break;
			case SWORD_EMERALD_CLAYMORE_G:
				ec_create_sword_emerald_claymore_glace(actors_list[i], (poor_man ? 6 : 10));
				break;
			case SWORD_EMERALD_CLAYMORE_A:
				ec_create_sword_emerald_claymore_arcane(actors_list[i], (poor_man ? 6 : 10));
				break;
			case SWORD_EMERALD_CLAYMORE_L:
				ec_create_sword_emerald_claymore_lumiere(actors_list[i], (poor_man ? 6 : 10));
				break;
			case SWORD_CUTLASS_F:
				ec_create_sword_cutlass_feu(actors_list[i], (poor_man ? 6 : 10));
				break;
			case SWORD_CUTLASS_G:
				ec_create_sword_cutlass_glace(actors_list[i], (poor_man ? 6 : 10));
				break;
			case SWORD_CUTLASS_A:
				ec_create_sword_cutlass_arcane(actors_list[i], (poor_man ? 6 : 10));
				break;
			case SWORD_CUTLASS_L:
				ec_create_sword_cutlass_lumiere(actors_list[i], (poor_man ? 6 : 10));
				break;
			}
		}
	} else {
		actors_list[i]->calmodel = NULL;
	}
	/* //DEBUG
	   if (actors_list[i]->actor_id==yourself) {
	        //actor_wear_item(actors_list[i]->actor_id,KIND_OF_WEAPON,SWORD_1);
	        //unwear_item_from_actor(actors_list[i]->actor_id,KIND_OF_WEAPON);
	        //actor_wear_item(actors_list[i]->actor_id,KIND_OF_WEAPON,SWORD_2);
	        //actor_wear_item(actors_list[i]->actor_id,KIND_OF_HELMET,HELMET_IRON);
	   }*/
	if (actor_id == yourself) {
		reset_camera_at_next_update = 1;
	}
	update_actor_buffs(actor_id, buffs);
	UNLOCK_ACTORS_LISTS();  //unlock it
	if (actor_id == yourself) {
		if (strcasecmp(onlyname, username_str) != 0) {
			nom_change = 1;
		} else {
			nom_change = 0;
		}
	}
}
actor *add_actor_interface(float x, float y, float z_rot, float scale, int actor_type, short skin, short hair, short shirt, short pants, short boots, short head) {
	enhanced_actor *this_actor = calloc(1, sizeof(enhanced_actor));
	actor *a;
	//get the torso
	my_strncp(this_actor->arms_tex, actors_defs[actor_type].shirt[shirt].arms_name, sizeof(this_actor->arms_tex));
	my_strncp(this_actor->arms_mask, actors_defs[actor_type].shirt[shirt].arms_mask, sizeof(this_actor->arms_mask));
	my_strncp(this_actor->torso_tex, actors_defs[actor_type].shirt[shirt].torso_name, sizeof(this_actor->torso_tex));
	my_strncp(this_actor->torso_mask, actors_defs[actor_type].shirt[shirt].torso_mask, sizeof(this_actor->torso_mask));
	my_strncp(this_actor->hands_tex, actors_defs[actor_type].skin[skin].hands_name, sizeof(this_actor->hands_tex));
	my_strncp(this_actor->head_tex, actors_defs[actor_type].skin[skin].head_name, sizeof(this_actor->head_tex));
	my_strncp(this_actor->hair_tex, actors_defs[actor_type].hair[hair].hair_name, sizeof(this_actor->hair_tex));
	my_strncp(this_actor->boots_tex, actors_defs[actor_type].boots[boots].boots_name, sizeof(this_actor->boots_tex));
	my_strncp(this_actor->boots_mask, actors_defs[actor_type].boots[boots].boots_mask, sizeof(this_actor->boots_mask));
	my_strncp(this_actor->pants_tex, actors_defs[actor_type].legs[pants].legs_name, sizeof(this_actor->pants_tex));
	my_strncp(this_actor->pants_mask, actors_defs[actor_type].legs[pants].legs_mask, sizeof(this_actor->pants_mask));
	a = actors_list[add_enhanced_actor(this_actor, x * 0.5f, y * 0.5f, 0.00000001f, z_rot, scale, 0, 0)];
	a->x_tile_pos = x;
	a->y_tile_pos = y;
	a->actor_type = actor_type;
	//test only
	a->max_health = 20;
	a->cur_health = 20;
	a->stop_animation = 1;//helps when the actor is dead...
	a->kind_of_actor = HUMAN;
	safe_snprintf(a->actor_name, sizeof(a->actor_name), "Nouveau Personnage");
	if (actors_defs[actor_type].coremodel != NULL) {
		a->calmodel = model_new(actors_defs[actor_type].coremodel);
		if (a->calmodel != NULL) {
			//Setup cal3d model
			//a->calmodel=CalModel_New(actors_defs[actor_type].coremodel);
			//Attach meshes
			model_attach_mesh(a, actors_defs[actor_type].head[head].mesh_index);
			model_attach_mesh(a, actors_defs[actor_type].shirt[shirt].mesh_index);
			model_attach_mesh(a, actors_defs[actor_type].legs[pants].mesh_index);
			model_attach_mesh(a, actors_defs[actor_type].boots[boots].mesh_index);
			a->body_parts->torso_meshindex = actors_defs[actor_type].shirt[shirt].mesh_index;
			a->body_parts->legs_meshindex = actors_defs[actor_type].legs[pants].mesh_index;
			a->body_parts->head_meshindex = actors_defs[actor_type].head[head].mesh_index;
			a->body_parts->boots_meshindex = actors_defs[actor_type].boots[boots].mesh_index;
			assert(!"Using old client data" || actors_defs[actor_type].neck != NULL);
			a->body_parts->neck_meshindex = actors_defs[actor_type].neck[MEDAILLON_NONE].mesh_index;
			a->body_parts->helmet_meshindex = actors_defs[actor_type].helmet[HELMET_NONE].mesh_index;
			a->body_parts->cape_meshindex = actors_defs[actor_type].cape[CAPE_NONE].mesh_index;
			a->body_parts->shield_meshindex = actors_defs[actor_type].shield[SHIELD_NONE].mesh_index;
			a->cur_anim.anim_index = -1;
			a->anim_time = 0.0;
			a->last_anim_update = cur_time;
			CalModel_Update(a->calmodel, 0);
			build_actor_bounding_box(a);
		}
	} else {
		a->calmodel = NULL;
	}
	UNLOCK_ACTORS_LISTS();  //unlock it
	return a;
}
