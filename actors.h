/*!
 * \file
 * \ingroup	display
 * \brief	This file holds information about actors appearance etc. used for displaying the actors.
 */
#ifndef __ACTORS_H__
#define __ACTORS_H__
#include <SDL_mutex.h>
#include "bbox_tree.h"
#include "cal_types.h"
#include "client_serv.h"
#include "platform.h"
#include "tiles.h"
#include "buffs.h"
#include "eye_candy_types.h"
#ifdef __cplusplus
extern "C" {
#endif
#define MAX_FILE_PATH   128 // the max chars allowed int a path/filename for actor textures/masks
#define MAX_ACTOR_DEFS  256
#define MAX_ACTORS      1000 /*!< The maximum number of actors the client can hold */
extern int yourself; /*!< This variable holds the actor_id (as the server sees it, not the position in the actors_list) of your character.*/
extern int you_sit; /*!< Specifies if you are currently sitting down.*/
extern int sit_lock; /*!< The sit_lock variable holds you in a sitting position.*/
extern float name_zoom; /*!< The name_zoom defines how large the text used for drawing the names should be*/
extern float titre_zoom; /*< Taille pour le texte des titres */
extern int use_alpha_banner; /*!< Use_alpha_banner defines if an alpha background is drawn behind the name/health banner.*/
/*!
 * \name	Actor types
 * 		Defines the colour of the name.
 */
/*! \{ */
#define HUMAN 1 /*!< Draw the actors name in white*/
#define NPC 2 /*!< Draw the actors name in blue*/
#define COMPUTER_CONTROLLED_HUMAN 3 /*!< Draw the actors name in white*/
#define PKABLE_HUMAN 4 /*!< Draw the actors name in red*/
#define PKABLE_COMPUTER_CONTROLLED 5 /*!< Draw the actors name in red*/
/*! \} */
/*! Max text len to display into bubbles overhead*/
#define MAX_CURRENT_DISPLAYED_TEXT_LEN  60
// default duration in ms of a step when an actor is walking
#define DEFAULT_STEP_DURATION 250
/*!
 * \name	Glow colours
 * 		The colours used for giving the items a glowing halo
 */
/*! \{ */
/*! The colours used in the glowing swords (magic, thermal, ice, fire)*/
typedef struct {
	float r; /*!< Red (0<=r<=1)*/
	float g; /*!< Green (0<=g<=1)*/
	float b; /*!< Blue (0<=b<=1)*/
} glow_color;
// GLOWS
#define GLOW_NONE 0 /*!< RGB: 0.0, 0.0, 0.0*/
#define GLOW_FIRE 1 /*!< RGB: 0.5, 0.1, 0.1*/
#define GLOW_COLD 2 /*!< RGB: 0.1, 0.1, 0.5*/
#define GLOW_THERMAL 3 /*!< RGB: 0.5, 0.1, 0.5*/
#define GLOW_MAGIC 4 /*!< RGB: 0.5, 0.4, 0.0*/
extern glow_color glow_colors[10]; /*!< Holds the glow colours defined in GLOW_**/
/*! \} */
/*!
 * The near_actor structure holds information about the actors within range. It is filled once every frame.
 */
typedef struct {
	int actor; // offset in the actors_list
	int select;
	int buffs; // The buffs on this actor
	int type;
	int alpha;
	int ghost; // If it's a ghost or not
} near_actor;
extern int no_near_actors;
extern int no_near_enhanced_actors;
extern float distanceSq_to_near_enhanced_actors;
extern near_actor near_actors[MAX_ACTORS];
/*!
 * The enhanced actor structure holds information about the actors extensions such as if the actor is wearing any armour, weapons etc.
 */
typedef struct {
	int uniq_id;
	int guild_id;
	int guild_tag_color;
	int legs_meshindex;
	int head_meshindex;
	int torso_meshindex;
	int weapon_meshindex;
	int shield_meshindex;
	int helmet_meshindex;
	int neck_meshindex;
	int cape_meshindex;
	int boots_meshindex;
	/*! \name The texture names*/
	/*! \{ */
	char pants_tex[MAX_FILE_PATH];
	char pants_mask[MAX_FILE_PATH];
	char boots_tex[MAX_FILE_PATH];
	char boots_mask[MAX_FILE_PATH];
	char torso_tex[MAX_FILE_PATH];
	char arms_tex[MAX_FILE_PATH];
	char torso_mask[MAX_FILE_PATH];
	char arms_mask[MAX_FILE_PATH];
	char hands_tex[MAX_FILE_PATH];
	char head_tex[MAX_FILE_PATH];
	char hands_mask[MAX_FILE_PATH];
	char head_mask[MAX_FILE_PATH];
	char head_base[MAX_FILE_PATH];
	char body_base[MAX_FILE_PATH];
	char arms_base[MAX_FILE_PATH];
	char legs_base[MAX_FILE_PATH];
	char boots_base[MAX_FILE_PATH];
	char hair_tex[MAX_FILE_PATH];
	char weapon_tex[MAX_FILE_PATH];
	char shield_tex[MAX_FILE_PATH];
	char helmet_tex[MAX_FILE_PATH];
	char neck_tex[MAX_FILE_PATH];
	char cape_tex[MAX_FILE_PATH];
	char hands_tex_save[MAX_FILE_PATH];
	/*! \} */
	/*! \name Specifies the glow of each worn item*/
	/*! \{ */
	int weapon_glow;
	int shield_glow;
	int helmet_glow;
	int neck_glow;
	int cape_glow;
	int legs_glow;
	/*! \} */
} enhanced_actor;
/*! Sets the main model type*/
typedef struct {
	char model_name[MAX_FILE_PATH];
	char skin_name[MAX_FILE_PATH];
	char skin_mask[MAX_FILE_PATH];
	int glow;
	int mesh_index;
} body_part;
/*! Sets the shield type*/
typedef struct {
	char model_name[MAX_FILE_PATH];
	char skin_name[MAX_FILE_PATH];
	char skin_mask[MAX_FILE_PATH];
	int glow;
	int mesh_index;
} shield_part;
/*! Sets the weapon type (including animation frame names)*/
typedef struct {
	char model_name[MAX_FILE_PATH];
	char skin_name[MAX_FILE_PATH];
	char skin_mask[MAX_FILE_PATH];
	int glow;
	int mesh_index;
	struct cal_anim cal_frames[NUM_WEAPON_FRAMES];
} weapon_part;
/*! Defines the main models looks*/
typedef struct {
	char model_name[MAX_FILE_PATH];
	char arms_name[MAX_FILE_PATH];
	char torso_name[MAX_FILE_PATH];
	char arms_mask[MAX_FILE_PATH];
	char torso_mask[MAX_FILE_PATH];
	int mesh_index;
} shirt_part;
/*! Sets the models hands and head*/
typedef struct {
	char hands_name[MAX_FILE_PATH];
	char head_name[MAX_FILE_PATH];
	char arms_name[MAX_FILE_PATH];
	char body_name[MAX_FILE_PATH];
	char legs_name[MAX_FILE_PATH];
	char feet_name[MAX_FILE_PATH];
	int mesh_index;
} skin_part;
/*! Sets the models hair name*/
typedef struct {
	char hair_name[MAX_FILE_PATH];
	int mesh_index;
} hair_part;
/*! Holds info about the boots */
typedef struct {
	char boots_name[MAX_FILE_PATH];
	char model_name[MAX_FILE_PATH];
	char boots_mask[MAX_FILE_PATH];
	int glow;
	int mesh_index;
} boots_part;
/*! Holds info about the legs type*/
typedef struct {
	char legs_name[MAX_FILE_PATH];
	char model_name[MAX_FILE_PATH];
	char legs_mask[MAX_FILE_PATH];
	int glow;
	int mesh_index;
} legs_part;
/*! A structure used when loading the actor definitions
 * \sa init_actor_defs*/
typedef struct cal_anim_group {
	char name[32];
	int count;
	struct cal_anim anim[16];
} cal_animations;
typedef struct {
	int sound;
	float scale;
} act_extra_sound;
typedef struct {
	int is_holder; /*!< Specifies if this type of actor hold the actor to which it is attached or if he is held */
	int parent_bone_id; /*!< The bone to use on the actor to which it is attached */
	int local_bone_id; /*!< The bone to use on the actor that is attached */
	float shift[3]; /*!< The shift to apply to the actor that is held */
	struct cal_anim cal_frames[NUM_ATTACHED_ACTOR_FRAMES];
} attachment_props;
/*!
 * Structure containing how an actor type is attached to all other actors types
 */
typedef struct {
	attachment_props actor_type[MAX_ACTOR_DEFS]; /*!< Attachment properties for each kind of actor */
} attached_actors_types;
typedef enum {
	ACTOR_HEAD_SIZE = 0, ACTOR_SHIELD_SIZE, ACTOR_CAPE_SIZE, ACTOR_HELMET_SIZE, ACTOR_WEAPON_SIZE, ACTOR_SHIRT_SIZE, ACTOR_SKIN_SIZE, ACTOR_HAIR_SIZE, ACTOR_BOOTS_SIZE, ACTOR_LEGS_SIZE, ACTOR_NECK_SIZE, ACTOR_NUM_PARTS,
} actor_parts_enum;
typedef struct {
	/*! \name Model data*/
	/*! \{ */
	int actor_type;
	char actor_name[66];
	char skin_name[MAX_FILE_PATH];
	char file_name[256];
	/*! \} */
	float actor_scale;
	float scale;
	float mesh_scale;
	float skel_scale;
	struct CalCoreModel *coremodel;
	struct CalHardwareModel *hardware_model;
	GLuint vertex_buffer;
	GLuint index_buffer;
	GLenum index_type;
	Uint32 index_size;
	// Animation indexes
	struct cal_anim_group idle_group[16]; // 16 animation groups
	int group_count;
	struct cal_anim cal_frames[NUM_ACTOR_FRAMES];
	int skeleton_type;
	// Extra sounds
	act_extra_sound battlecry;
	/*! \name The different body parts (different head shapes, different armour/weapon shapes etc.)*/
	/*! \{ */
	body_part *head;
	shield_part *shield;
	body_part *cape;
	body_part *helmet;
	body_part *neck;
	weapon_part *weapon;
	/*! \} */
	/*! \name Clothing*/
	/*! \{ */
	shirt_part *shirt;
	skin_part *skin;
	hair_part *hair;
	boots_part *boots;
	legs_part *legs;
	/*! \} */
	/*! \name The current actors walk/run speeds*/
	/*! \{ */
	double walk_speed; // unused
	double run_speed; // unused
	char ghost;
	/*! \} */
} actor_types;
#define MY_HORSE(a) (actors_list[actors_list[a]->attached_actor])
#define MY_HORSE_ID(a) (actors_list[a]->attached_actor)
#define HAS_HORSE(a) ((MY_HORSE_ID(a) >= 0) && (MY_HORSE(a)->actor_id < 0))
#define IS_HORSE(a) (actors_list[a]->attached_actor >= 0 && actors_list[a]->actor_id < 0)
#define ACTOR(a) (actors_list[a])
#define ACTOR_WEAPON(a) (&(actors_defs[ACTOR(a)->actor_type].weapon[ACTOR(a)->cur_weapon]))
/*! The main actor structure.*/
#define MAX_CMD_QUEUE   31
#define MAX_RANGE_ACTION_QUEUE 16
#define MAX_ITEM_CHANGES_QUEUE 16
typedef struct {
	/*! \name Misc.*/
	/*! \{ */
	int actor_id; /*!< The actor ID from the server*/
	int actor_type; /*!< Specifies the type of actor (race, sex etc.)*/
	/*! \} */
	struct CalModel *calmodel;
	struct cal_anim cur_anim;
	unsigned int cur_anim_sound_cookie; /*!< The currently played animation sound*/
	struct cal_anim cur_idle_anims[16];
	int IsOnIdle;
	float anim_time;
	Uint32 last_anim_update;
	AABBOX bbox;
	int delayed_item_changes[MAX_ITEM_CHANGES_QUEUE]; /*!< Used to delay a sword/shield equip while in range mode (-1: item removed; >= 0: item equipped) */
	int delayed_item_type_changes[MAX_ITEM_CHANGES_QUEUE]; /*!< Used to delay a sword/shield equip while in range mode */
	int delayed_item_changes_count; /*!< The number of delayed items */
	char delay_texture_item_changes; /*!< To tell if the item changes should get delayed */
	/*! \} */
	/*! \name Actors positions
	 *  \brief Updated in the timer thread
	 */
	/*! \{ */
	double x_pos; /*!< Specifies the x position of the actor */
	double y_pos; /*!< Specifies the y position of the actor */
	double z_pos; /*!< Specifies the z position of the actor */
	float scale; /*!< Specidies the custom scaling for the actor model */
	int x_tile_pos; /*!< Specifies the x tile position - updated in the timer thread*/
	int y_tile_pos; /*!< Specifies the y tile position - updated in the timer thread \n*/
	/*! \} */
	/*! \name Actor rotation*/
	/*! \{ */
	float x_rot; /*!< Sets the current x rotation*/
	float y_rot; /*!< Sets the current y rotation*/
	float z_rot; /*!< Sets the current z rotation*/
	/*! \} */
	float max_z;
	/*! \name Actors worn item IDs*/
	/*! \{ */
	int boots; /*!< Sets the boots ID (loaded from the actor_defs array)*/
	int hair; /*!< Sets the hair ID (loaded from the actor_defs array)*/
	int skin; /*!< Sets the skin ID (loaded from the actor_defs array)*/
	int pants; /*!< Sets the pants ID (loaded from the actor_defs array)*/
	int shirt; /*!< Sets the shirt ID (loaded from the actor_defs array)*/
	int cur_weapon; /*!< Sets the current weapon of the actor*/
	int cur_shield; /*!< Sets the current shield of the actor*/
	/*! \} */
	/*! \{ */
	int is_enhanced_model; /*!< Specifies if we have the enhanced_actor structure below*/
	enhanced_actor *body_parts; /*!< A pointer to the enhanced actor extension (holds information about weapons, helmets etc)*/
	/*! \} */
	/*! \{ */
	char remapped_colors; /*!< If the actors colours are remapped it will holds the texture in actor->texture_id*/
	GLuint texture_id; /*!< Sets the texture ID, if the remapped_colors==1 - remember to glDeleteTextures*/
	char skin_name[256]; /*!< Sets the skin name*/
	char actor_name[256]; /*!< Sets the actors name - holds the guild name as well after a special 127+color character*/
	char titre[20];
	/*! \} */
	/*! \name Command queue and current animations*/
	/*! \{ */
	actor_commands que[MAX_CMD_QUEUE + 1]; /*!< Holds the current command queue*/
	char last_command; /*!< Holds the last command*/
	char busy; /*!< if the actor is busy executing the current command*/
	char sitting; /*!< Specifies if the actor is currently sitting*/
	char fighting; /*!< Specifies if the actor is currently fighting*/
	/*! \} */
	/*!
	 * \name Movement
	 */
	/*! \{ */
	double move_x_speed; /*!< Sets the current movement speed in the x direction (used for updating the actor in the timer thread)*/
	double move_y_speed; /*!< Sets the current movement speed in the y direction (used for updating the actor in the timer thread)*/
	double move_z_speed; /*!< Sets the current movement speed in the z direction (used for updating the actor in the timer thread)*/
	int movement_time_left; /*!< Specifies the time left for the actor movement before it goes in idle */
	float rotate_x_speed; /*!< Sets the x rotation speed (used for updating the actor in the timer thread)*/
	float rotate_y_speed; /*!< Sets the y rotation speed (used for updating the actor in the timer thread)*/
	float rotate_z_speed; /*!< Sets the z rotation speed (used for updating the actor in the timer thread)*/
	int rotate_time_left; /*!< Specifies the time left for the actor rotation before it goes in idle */
	/*! \} */
	/*! \name Misc. animations*/
	/*! \{ */
	char moving; /*!< Specifies if the actor is currently on the move*/
	char rotating; /*!< Specifies if the actor is currently rotating*/
	char stop_animation; /*!< Don't loop trough the current animation (like for die, jump, etc.)*/
	char stand_idle; /*!< Sets the actor in an idle stand position*/
	char sit_idle; /*!< Sets the actor in an idle sit position*/
	char dead; /*!< Used when the actor is dead (render the dead position)*/
	int damage; /*!< Sets the damage the actor has been given*/
	int damage_ms; /*!< Defines the remaining time in which the actor damage will be shown*/
	int last_health_loss; /*!< Defines the time of damage*/
	Uint16 cur_health; /*!< Sets the current health of the actor*/
	Uint16 max_health; /*!< Sets the maximum health of the actor*/
	char ghost; /*!< Sets the actor type to ghost (Disable lightning, enable blending (GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA))*/
	char has_alpha; /*!< is alpha blending needed for this actor? */
	int kind_of_actor; /*!< Defines the kind_of_actor (NPC, HUMAN, COMPUTER_CONTROLLED_HUMAN, PKABLE, PKABLE_COMPUTER_CONTROLLED)*/
	Uint32 buffs; /*!<Contains the buffs on this actor as bits (currently only invisibility)*/
	/*! \} */
	/*! \name Overhead text (text bubbles)*/
	/*! \{ */
	char current_displayed_text[160]; /*!< If the text is displayed in a bubble over the actor, this holds the text*/
	int current_displayed_text_time_left; /*!< Defines the remaining time the overhead text should be displayed*/
	/*! \} */
	/*! \name Unused variables*/
	/*! \{ */
	double x_speed; /*!< Unused?*/
	double y_speed; /*!< Unused?*/
	double z_speed; /*!< Unused?*/
	/*! \} */
	int async_fighting;
	int async_x_tile_pos;
	int async_y_tile_pos;
	int async_z_rot;
	int last_range_attacker_id;
	int attached_actor;
	float attachment_shift[3];
	short cluster;
	ec_reference ec_buff_reference[NUM_BUFFS];
} actor;
#define DEFAULT_RENDER_PASS     0
#define REFLECTION_RENDER_PASS  1
#define DEPTH_RENDER_PASS       2
#define SHADOW_RENDER_PASS      3
#define SELECTION_RENDER_PASS   4
extern SDL_mutex *actors_lists_mutex; /*!< Used for locking between the timer and main threads*/
extern actor *actors_list[MAX_ACTORS]; /*!< A list holding all of the actors*/
extern actor *your_actor; /*!< A pointer to your own character, if available. Shares a mutex with \see actors_list */
extern int max_actors; /*!< The current number of actors in the actors_list + 1*/
extern actor_types actors_defs[MAX_ACTOR_DEFS]; /*!< The actor definitions*/
extern attached_actors_types attached_actors_defs[MAX_ACTOR_DEFS]; /*!< The definitions for the attached actors */
/*!
 * \ingroup	display_actors
 * \brief	Draws the actors banner (healthbar, name, etc)
 *
 * 		This function is used for drawing the healthbar, the name, the damage, the healthpoints (cur/max) and the text bubbles
 *
 * \param	actor_id Is a pointer to the actor we wish to draw
 * \param	offset_z Is the z offset, found by the current MD2 frames max_z.
 *
 * \callgraph
 */
void draw_actor_banner(actor *actor_id, float offset_z);
/*!
 * \ingroup	display_actors
 * \brief	The main actor loop - draws all actors within range
 *
 * 		The function draws the actor if it's within a range of 12*12
 *
 * \callgraph
 */
void display_actors(int banner, int render_pass);
void add_actor_attachment(int actor_id, int attachment_type);
void remove_actor_attachment(int actor_id);
/*!
 * \ingroup	network_actors
 * \brief	Adds an actor from the in_data
 *
 * 		Is called when the client gets an ADD_NEW_ACTOR command from the server. Parses the data pointed to by in_data, then adds the actor to the actors list
 *
 * \param	in_data The data from the server
 * \param   len The length of the supplied data
 *
 * \callgraph
 */
void add_actor_from_server(const char *in_data, int len);
/*!
 * \ingroup	display_actors
 * \brief	Inititates the actors_list (sets all pointers to NULL).
 *
 * 		Sets all actor pointers in the actors_list to NULL and creates the actors_list mutex.
 *
 * \sa		actors_list
 * \sa		LOCK_ACTORS_LISTS
 */
extern void     init_actors_lists();
/*!
 * \ingroup mutex
 * \name Actor list thread synchronization
 */
/*! @{ */
#define LOCK_ACTORS_LISTS()     SDL_LockMutex(actors_lists_mutex)
#define UNLOCK_ACTORS_LISTS() SDL_UnlockMutex(actors_lists_mutex)
/*! @} */
/*!
 * \ingroup	network_text
 * \brief	Adds the text to the actor given by actor_ptr
 *
 * 		Adds text from the actor to overhead text.
 *
 * \param	actor_ptr A pointer to the actor
 * \param	text The text we wish to add to the current_displayed_text buffer in the actors structure.
 */
void    add_displayed_text_to_actor(actor *actor_ptr, const char *text);
/*!
 * \ingroup	misc_utils
 * \brief	Gets a pointer to the actor given by the actor_id
 *
 * 		The function is used for getting a pointer to the actor with the given actor_id (the server-side actor id).
 *
 * \param	actor_id The server-side actor_id - NOT the position in the actors_list
 * \retval actor*	A pointer to the actor with the given ID. If the actor is not found it returns NULL
 * \sa		get_our_actor
 */
actor *get_actor_ptr_from_id(int actor_id);
void end_actors_lists(void);
int on_the_move(const actor *act);
/*!
 * \ingroup	display_actors
 * \brief	Return a pointer to your own character, if available
 *
 *	Return a pointer to your own character, if available. This inline
 *	function simply returns \ref your_actor.
 *
 * \return A pointer to your character, or NULL if that is not available.
 */
static __inline__ actor *get_our_actor() {
	return your_actor;
}
/*!
 * \ingroup	display_actors
 * \brief	Set the pointer to your own character
 *
 *	Set the pointer to your own character to \a act.
 *
 * \param act New pointer to your character.
 */
static __inline__ void set_our_actor(actor *act) {
	your_actor = act;
}
/*!
 * \brief Get the Z position of an actor according to its position on the height map
 * \param a the actor
 * \return the Z position of the actor
 */
static __inline__ float get_actor_z(actor *a) {
	return get_tile_height(a->x_tile_pos, a->y_tile_pos);
}
/*!
 * \brief Get the scale factor of an actor
 * \param a the actor
 * \return the scale factor of the actor
 */
static __inline__ float get_actor_scale(actor *a) {
	float scale = a->scale;
	scale *= actors_defs[a->actor_type].actor_scale;
	return scale;
}
/*!
 * \brief Computes the rotation matrix of an actor
 * \param in_act the actor
 * \param out_rot the resulting matrix (3x3 matrix: 9 floats)
 */
void get_actor_rotation_matrix(actor *in_act, float *out_rot);
/*!
 * \brief Transforms a local position on a char to an absolute position
 * \param in_act the actor
 * \param in_local_pos the local position
 * \param in_act_rot the rotation matrix of the actor (computed inside if NULL)
 * \param out_pos the resulting position
 */
void transform_actor_local_position_to_absolute(actor *in_act, float *in_local_pos, float *in_act_rot, float *out_pos);
void draw_actor_without_banner(actor *actor_id, Uint32 use_lightning, Uint32 use_textures, Uint32 use_glow);
static __inline__ int is_actor_held(actor *act) {
	return (act->attached_actor >= 0) && ((act->actor_id < 0 && // the actor is the attachment
					       !attached_actors_defs[act->actor_type].actor_type[actors_list[act->attached_actor]->actor_type].is_holder) || (act->actor_id >= 0 && // the actor is the parent of the attachment
																			      attached_actors_defs[actors_list[act->attached_actor]->actor_type].actor_type[act->actor_type].is_holder));
}
static __inline__ attachment_props *get_attachment_props_if_held(actor *act) {
	if (act->attached_actor < 0) {
		return NULL;
	} else if (act->actor_id < 0 && // the actor is the attachment
		   !attached_actors_defs[act->actor_type].actor_type[actors_list[act->attached_actor]->actor_type].is_holder) {
		return &attached_actors_defs[act->actor_type].actor_type[actors_list[act->attached_actor]->actor_type];
	} else if (act->actor_id >= 0 && // the actor is the parent of the attachment
		   attached_actors_defs[actors_list[act->attached_actor]->actor_type].actor_type[act->actor_type].is_holder) {
		return &attached_actors_defs[actors_list[act->attached_actor]->actor_type].actor_type[act->actor_type];
	} else {
		return NULL;
	}
}
static __inline__ int get_held_actor_motion_frame(actor *act) {
	if (act->attached_actor < 0) {
		return cal_attached_walk_frame;
	} else if (actors_list[act->attached_actor]->buffs & BUFF_DOUBLE_SPEED) {
		return cal_attached_run_frame;
	} else {
		return cal_attached_walk_frame;
	}
}
static __inline__ int get_actor_motion_frame(actor *act) {
	return act->buffs & BUFF_DOUBLE_SPEED ? cal_actor_run_frame : cal_actor_walk_frame;
}
#ifdef __cplusplus
} // extern "C"
#endif
#endif // __ACTORS_H__
