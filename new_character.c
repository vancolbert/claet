#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "new_character.h"
#include "2d_objects.h"
#include "3d_objects.h"
#include "actors.h"
#include "actor_scripts.h"
#include "asc.h"
#include "bbox_tree.h"
#include "books.h"
#include "cal3d_wrapper.h"
#include "chat.h"
#include "consolewin.h"
#include "cursors.h"
#include "draw_scene.h"
#include "elconfig.h"
#include "gamewin.h"
#include "gl_init.h"
#include "global.h"
#include "hud.h"
#include "init.h"
#include "interface.h"
#include "lights.h"
#include "loginwin.h"
#include "map.h"
#include "misc.h"
#include "multiplayer.h"
#include "new_actors.h"
#include "particles.h"
#include "reflection.h"
#include "shadows.h"
#include "sky.h"
#include "tabs.h"
#include "textures.h"
#include "tiles.h"
#include "translate.h"
#include "weather.h"
#ifdef NEW_NEW_CHAR_WINDOW
#include "widgets.h"
#endif
#include "actor_init.h"

void add_text_to_buffer(int color, char * text, int time_to_display);

#ifdef FR_VERSION
// Liste des cadrages camera pr�d�finis en fonction de la carte
struct cadrage {
	int rx;
	int ry;
	int rz;
} cadrages[8] = {
	{-90, 0, -20}, // Eldorian   N
	{-90, 0, -10}, // Haut-Elfe  NE
	{-90, 0, 310}, // Nain       NO
	{-90, 0, 270}, // Kultar     O
	{-90, 0, 180}, // Galdur     S
	{-90, 0, 280}, // Homme bleu SO
	{-90, 0,  70}, // Sinan      E
	{-90, 0, 120}, // Elfe noir  SE
};
#endif //FR_VERSION

typedef int my_enum;//This enumeration will decrease, then wrap to top, increase and then wrap to bottom, when using the inc() and dec() functions. Special purpose though, since you have to have between 2 and 255 values in the enumeration and you have to have the same value in enum[0] as in enum[max] - otherwise we'll probably segfault...

my_enum	normal_skin_enum[]	= { SKIN_BROWN, SKIN_NORMAL, SKIN_PALE, SKIN_TAN, SKIN_BROWN };
#ifdef ENGLISH
my_enum	elf_skin_enum[]		= { SKIN_BROWN, SKIN_NORMAL, SKIN_PALE, SKIN_TAN, SKIN_DARK_BLUE, SKIN_BROWN };
my_enum	draegoni_skin_enum[]	= { SKIN_BROWN, SKIN_NORMAL, SKIN_PALE, SKIN_TAN, SKIN_WHITE, SKIN_BROWN };
my_enum	normal_hair_enum[]	= { HAIR_BLACK, HAIR_BLOND, HAIR_BROWN, HAIR_GRAY, HAIR_RED, HAIR_WHITE, HAIR_DARK_BROWN, HAIR_STRAWBERRY, HAIR_LIGHT_BLOND, HAIR_DIRTY_BLOND, HAIR_BROWN_GRAY, HAIR_DARK_GRAY, HAIR_DARK_RED, HAIR_BLACK };

my_enum        draegoni_hair_enum[]= { HAIR_BLACK, HAIR_BLOND, HAIR_BROWN, HAIR_GRAY, HAIR_RED, HAIR_WHITE, HAIR_DARK_BROWN, HAIR_STRAWBERRY, HAIR_LIGHT_BLOND, HAIR_DIRTY_BLOND, HAIR_BROWN_GRAY, HAIR_DARK_GRAY, HAIR_DARK_RED, HAIR_BLUE, HAIR_GREEN, HAIR_PURPLE, HAIR_BLACK };
#else //ENGLISH
#ifdef FR_VERSION
my_enum        haut_elfe_skin_enum[] = { SKIN_NORMAL, SKIN_PALE, SKIN_TAN, SKIN_NORMAL };
my_enum        elfe_noir_skin_enum[] = { SKIN_BROWN, SKIN_EN_CLAIR, SKIN_EN_FONCE, SKIN_EN_GRIS, SKIN_EN_MEDIUM, SKIN_BROWN };
#else //FR_VERSION
my_enum        elf_skin_enum[]         = { SKIN_BROWN, SKIN_NORMAL, SKIN_PALE, SKIN_TAN, SKIN_BROWN };
#endif //FR_VERSION
my_enum        normal_hair_enum[]      = { HAIR_BLACK, HAIR_BLOND, HAIR_BROWN, HAIR_GRAY, HAIR_RED, HAIR_WHITE, HAIR_BLACK };
my_enum        draegonim_hair_enum[]={ HAIR_BLACK, HAIR_BROWN, HAIR_GRAY, HAIR_RED, HAIR_WHITE, HAIR_BLACK };
my_enum        draegonif_hair_enum[]={ HAIR_BLACK, HAIR_BLOND, HAIR_BROWN, HAIR_GRAY, HAIR_RED, HAIR_WHITE, HAIR_BLACK };
#endif //ENGLISH
#ifdef NEW_EYES
my_enum	eyes_enum[]		= { EYES_BROWN, EYES_DARK_BROWN, EYES_BROWN_RED, EYES_LIGHT_BLUE, EYES_BLUE, EYES_DARK_BLUE, EYES_LIGHT_GREEN, EYES_GREEN, EYES_DARK_GREEN, EYES_LAVENDER, EYES_VIOLET, EYES_GOLD, EYES_BROWN };
#endif //NEW_EYES
my_enum	male_shirt_enum[]	= { SHIRT_BLACK, SHIRT_BLUE, SHIRT_BROWN, SHIRT_GREY, SHIRT_GREEN, SHIRT_LIGHTBROWN, SHIRT_ORANGE, SHIRT_PURPLE, SHIRT_RED, SHIRT_WHITE, SHIRT_YELLOW, SHIRT_BLACK };
my_enum	normal_shirt_enum[]	= { SHIRT_BLACK, SHIRT_BLUE, SHIRT_BROWN, SHIRT_GREY, SHIRT_GREEN, SHIRT_LIGHTBROWN, SHIRT_ORANGE, SHIRT_PINK, SHIRT_PURPLE, SHIRT_RED, SHIRT_WHITE, SHIRT_YELLOW, SHIRT_BLACK };
my_enum	normal_pants_enum[]	= { PANTS_BLACK, PANTS_BLUE, PANTS_BROWN, PANTS_DARKBROWN, PANTS_GREY, PANTS_GREEN, PANTS_LIGHTBROWN, PANTS_RED, PANTS_WHITE, PANTS_BLACK };
my_enum	normal_boots_enum[]	= { BOOTS_BLACK, BOOTS_BROWN, BOOTS_DARKBROWN, BOOTS_DULLBROWN, BOOTS_LIGHTBROWN, BOOTS_ORANGE, BOOTS_BLACK };
my_enum	normal_head_enum[]	= { HEAD_1, HEAD_2, HEAD_3, HEAD_4, HEAD_1 };
my_enum	human_head_enum[]	= { HEAD_1, HEAD_2, HEAD_3, HEAD_4, HEAD_5, HEAD_1 };
#ifdef FR_VERSION
my_enum normal_scale_enum[] = { SCALE_2, SCALE_3, SCALE_4, SCALE_5, SCALE_6, SCALE_7, SCALE_8, SCALE_9, SCALE_10, SCALE_2 };
#endif //FR_VERSION

struct race_def {
	int type;
	my_enum *skin;
	my_enum *hair;
#ifdef NEW_EYES
	my_enum *eyes;
#endif //NEW_EYES
	my_enum *shirts;
	my_enum *pants;
	my_enum *boots;
	my_enum *head;
#ifdef FR_VERSION
	my_enum *scale;
#endif //FR_VERSION
	float x, y, z_rot;
#ifdef FR_VERSION
} races[16] = {
	{     eldo_female, normal_skin_enum,      normal_hair_enum, normal_shirt_enum, normal_pants_enum, normal_boots_enum, human_head_enum,  normal_scale_enum,  92.0f, 170.0f, 180.0f},
	{       eldo_male, normal_skin_enum,      normal_hair_enum,   male_shirt_enum, normal_pants_enum, normal_boots_enum, human_head_enum,  normal_scale_enum,  92.0f, 170.0f, 180.0f},
	{haut_elfe_female, haut_elfe_skin_enum,   normal_hair_enum, normal_shirt_enum, normal_pants_enum, normal_boots_enum, normal_head_enum, normal_scale_enum, 160.0f, 156.0f, 200.0f},
	{  haut_elfe_male, haut_elfe_skin_enum,   normal_hair_enum,   male_shirt_enum, normal_pants_enum, normal_boots_enum, normal_head_enum, normal_scale_enum, 160.0f, 156.0f, 200.0f},
	{    dwarf_female, normal_skin_enum,      normal_hair_enum, normal_shirt_enum, normal_pants_enum, normal_boots_enum, normal_head_enum, normal_scale_enum,  31.0f, 159.0f, 130.0f},
	{      dwarf_male, normal_skin_enum,      normal_hair_enum,   male_shirt_enum, normal_pants_enum, normal_boots_enum, normal_head_enum, normal_scale_enum,  31.0f, 159.0f, 130.0f},
	{    gnome_female, normal_skin_enum,       normal_hair_enum, normal_shirt_enum, normal_pants_enum, normal_boots_enum, normal_head_enum,normal_scale_enum,  20.0f,  99.0f,  90.0f},
	{      gnome_male, normal_skin_enum,       normal_hair_enum,   male_shirt_enum, normal_pants_enum, normal_boots_enum, normal_head_enum,normal_scale_enum,  20.0f,  99.0f,  90.0f},
	{   orchan_female, normal_skin_enum,       normal_hair_enum, normal_shirt_enum, normal_pants_enum, normal_boots_enum, normal_head_enum,normal_scale_enum, 101.0f,  24.0f,   0.0f},
	{     orchan_male, normal_skin_enum,       normal_hair_enum,   male_shirt_enum, normal_pants_enum, normal_boots_enum, normal_head_enum,normal_scale_enum, 101.0f,  24.0f,   0.0f},
	{ draegoni_female, normal_skin_enum,    draegonif_hair_enum, normal_shirt_enum, normal_pants_enum, normal_boots_enum, normal_head_enum,normal_scale_enum,  29.0f,  45.0f,  70.0f},
	{   draegoni_male, normal_skin_enum,    draegonim_hair_enum,   male_shirt_enum, normal_pants_enum, normal_boots_enum, normal_head_enum,normal_scale_enum,  29.0f,  45.0f,  70.0f},
	{    sinan_female, normal_skin_enum,       normal_hair_enum, normal_shirt_enum, normal_pants_enum, normal_boots_enum, human_head_enum, normal_scale_enum, 170.0f,  99.0f, 270.0f},
	{      sinan_male, normal_skin_enum,       normal_hair_enum,   male_shirt_enum, normal_pants_enum, normal_boots_enum, human_head_enum, normal_scale_enum, 170.0f,  99.0f, 270.0f},
	{elfe_noir_female, elfe_noir_skin_enum,    normal_hair_enum, normal_shirt_enum, normal_pants_enum, normal_boots_enum, normal_head_enum,normal_scale_enum, 152.0f,  36.0f, 310.0f},
	{  elfe_noir_male, elfe_noir_skin_enum,    normal_hair_enum,   male_shirt_enum, normal_pants_enum, normal_boots_enum, normal_head_enum, normal_scale_enum, 152.0f,  36.0f, 310.0f},
#else //FR_VERSION
} races[12] = {
	{human_female, 		normal_skin_enum, normal_hair_enum, 	eyes_enum,	normal_shirt_enum, 	normal_pants_enum, normal_boots_enum, human_head_enum, 43.0f,	156.0f,	140.0f},
	{human_male, 		normal_skin_enum, normal_hair_enum, 	eyes_enum,	male_shirt_enum, 	normal_pants_enum, normal_boots_enum, human_head_enum, 43.0f,	156.0f,	140.0f},
	{elf_female, 		elf_skin_enum,    normal_hair_enum,	eyes_enum,	normal_shirt_enum, 	normal_pants_enum, normal_boots_enum, normal_head_enum, 42.0f,	92.0f,	180.0f},
	{elf_male,		elf_skin_enum,    normal_hair_enum,	eyes_enum,	male_shirt_enum, 	normal_pants_enum, normal_boots_enum, normal_head_enum, 42.0f,	92.0f,	180.0f},
	{dwarf_female,		normal_skin_enum, normal_hair_enum, 	eyes_enum,	normal_shirt_enum, 	normal_pants_enum, normal_boots_enum, normal_head_enum, 100.0f,	149.0f,	180.0f},
	{dwarf_male,		normal_skin_enum, normal_hair_enum, 	eyes_enum,	male_shirt_enum, 	normal_pants_enum, normal_boots_enum, normal_head_enum, 100.0f,	149.0f,	180.0f},
	{gnome_female,		normal_skin_enum, normal_hair_enum, 	eyes_enum,	normal_shirt_enum, 	normal_pants_enum, normal_boots_enum, normal_head_enum, 43.0f,	156.0f,	180.0f},
	{gnome_male,		normal_skin_enum, normal_hair_enum, 	eyes_enum,	male_shirt_enum, 	normal_pants_enum, normal_boots_enum, normal_head_enum, 43.0f,	156.0f,	180.0f},
	{orchan_female,		normal_skin_enum, normal_hair_enum, 	eyes_enum,	normal_shirt_enum, 	normal_pants_enum, normal_boots_enum, normal_head_enum, 42.0f,	92.0f,	180.0f},
	{orchan_male,		normal_skin_enum, normal_hair_enum, 	eyes_enum,	male_shirt_enum, 	normal_pants_enum, normal_boots_enum, normal_head_enum, 42.0f,	92.0f,	180.0f},
	{draegoni_female,	draegoni_skin_enum, draegoni_hair_enum, eyes_enum,	normal_shirt_enum, 	normal_pants_enum, normal_boots_enum, normal_head_enum, 100.0f,	149.0f,	180.0f},
	{draegoni_male,		draegoni_skin_enum, draegoni_hair_enum, eyes_enum,	male_shirt_enum, 	normal_pants_enum, normal_boots_enum, normal_head_enum, 100.0f,	149.0f,	180.0f},
#endif //FR_VERSION
};

struct char_def {
	int male;
	int race_id;//races[race_id]
	int race;
	int skin;
	int hair;
#ifdef NEW_EYES
	int eyes;
#endif //NEW_EYES
	int shirt;
	int pants;
	int boots;
	int head;
#ifdef FR_VERSION
	int scale;
#endif //FR_VERSION
	struct race_def * def;
	actor * our_model;
} our_actor = {
#ifdef FR_VERSION
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
#else //FR_VERSION
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
#endif //FR_VERSION
	NULL,
	NULL
};

//Enum handling

int find_pos_in_enum(my_enum * def, int val)
{
	int i;

	for(i=1;i<255;i++){
		if(def[i]==val) return i;
		else if(def[i]==def[0])return 0;
	}

	return 0;
}

int inc(my_enum * def, int val, int no_steps)
{
	my_enum * here=&def[find_pos_in_enum(def, val)];

	while(no_steps--){
		if(*here==def[0])here=&def[1];
		else here++;
	}

	return *here;
}

int dec(my_enum * def, int val, int no_steps)
{
	my_enum * top=&def[find_pos_in_enum(def, def[0])];
	my_enum * here=&def[find_pos_in_enum(def, val)];

	while(no_steps--){
		if(*here==*top)here=top;
		here--;
	}

	return *here;
}

//New char interface

static char create_char_error_str[520] = {0};
#ifndef NEW_NEW_CHAR_WINDOW
static int textboxy = 9*SMALL_FONT_Y_LEN; /* 8 lines of text + top/bottom space */
#else
int old_use_windowed_chat;
#endif
int display_time=0;

static struct input_text {
	char str[40];
	int pos;
} inputs[3] = {
#ifdef ENGLISH
	{"Player", 6},
#else //ENGLISH
	// pourquoi mettre un nom par d�faut ? (utilis� uniquement dans le textfield)
	{"", 0},
#endif //ENGLISH
	{"", 0},
	{"", 0}
};

int creating_char = 1;

void set_create_char_error (const char *msg, int len)
{
	char buf[512];

	if (len <= 0)
	{
		// server didn't send a message, use the default
		safe_snprintf (create_char_error_str, sizeof(create_char_error_str), "%s: %s", reg_error_str, char_name_in_use);
	}
	else
	{
#ifdef FR_VERSION
		safe_snprintf (create_char_error_str, sizeof (create_char_error_str), "%s : %.*s", reg_error_str, len, msg);
#else //FR_VERSION
		safe_snprintf (create_char_error_str, sizeof (create_char_error_str), "%s: %.*s", reg_error_str, len, msg);
#endif //FR_VERSION
		reset_soft_breaks (create_char_error_str, strlen (create_char_error_str), sizeof (create_char_error_str), 1.0, window_width - 20, NULL, NULL);
	}

	LOG_TO_CONSOLE(c_red1, create_char_error_str);

	put_small_colored_text_in_box(c_red1, (unsigned char*)create_char_error_str, strlen(create_char_error_str), 200, buf);
	safe_snprintf(create_char_error_str, sizeof(create_char_error_str), "%s", buf);
	display_time=cur_time+6000;

	creating_char=1;
}

#ifdef FR_VERSION
// Application du cadrage camera pr�d�finis selon la race/peuple
void recadrage_camera()
{
	int race = floor(our_actor.race_id/2);

	reset_camera_at_next_update=1;
	rx = cadrages[race].rx;
	ry = cadrages[race].ry;
	rz = cadrages[race].rz;
	// on applique un zoom par d�faut s'il est trop large pour �viter d'�tre derri�re le d�cor
	if (zoom_level > 2.0f) zoom_level = 2.0f;
}
#endif //FR_VERSION

void change_actor (void)
{
	// We only need to reload the core model, and attach all the correct mesh types.
	if (our_actor.our_model){
		if(our_actor.our_model->calmodel!=NULL)
			model_delete(our_actor.our_model->calmodel);

		our_actor.our_model->calmodel = model_new(actors_defs[our_actor.race].coremodel);
		our_actor.our_model->actor_type = our_actor.race;

		// Attach the Meshes.
		model_attach_mesh(our_actor.our_model,
			actors_defs[our_actor.race].head[our_actor.head].mesh_index);
		model_attach_mesh(our_actor.our_model,
			actors_defs[our_actor.race].shirt[our_actor.shirt].mesh_index);
		model_attach_mesh(our_actor.our_model,
			actors_defs[our_actor.race].legs[our_actor.pants].mesh_index);
		model_attach_mesh(our_actor.our_model,
			actors_defs[our_actor.race].boots[our_actor.boots].mesh_index);

		// Save which mesh is which.
		our_actor.our_model->body_parts->head_meshindex =
			actors_defs[our_actor.race].head[our_actor.head].mesh_index;
		our_actor.our_model->body_parts->torso_meshindex =
			actors_defs[our_actor.race].shirt[our_actor.shirt].mesh_index;
		our_actor.our_model->body_parts->legs_meshindex =
			actors_defs[our_actor.race].legs[our_actor.pants].mesh_index;
		our_actor.our_model->body_parts->boots_meshindex =
			actors_defs[our_actor.race].boots[our_actor.boots].mesh_index;

		// Recopy all of the textures.
		my_strncp(our_actor.our_model->body_parts->hands_tex,actors_defs[our_actor.race].skin[our_actor.skin].hands_name,sizeof(our_actor.our_model->body_parts->hands_tex));
		my_strncp(our_actor.our_model->body_parts->head_tex,actors_defs[our_actor.race].skin[our_actor.skin].head_name,sizeof(our_actor.our_model->body_parts->head_tex));

		my_strncp(our_actor.our_model->body_parts->hair_tex,actors_defs[our_actor.race].hair[our_actor.hair].hair_name,sizeof(our_actor.our_model->body_parts->hair_tex));
#ifdef NEW_EYES
		my_strncp(our_actor.our_model->body_parts->eyes_tex,actors_defs[our_actor.race].eyes[our_actor.eyes].eyes_name,sizeof(our_actor.our_model->body_parts->eyes_tex));
#endif
		my_strncp(our_actor.our_model->body_parts->arms_tex,actors_defs[our_actor.race].shirt[our_actor.shirt].arms_name,sizeof(our_actor.our_model->body_parts->arms_tex));
		my_strncp(our_actor.our_model->body_parts->torso_tex,actors_defs[our_actor.race].shirt[our_actor.shirt].torso_name,sizeof(our_actor.our_model->body_parts->torso_tex));

		my_strncp(our_actor.our_model->body_parts->pants_tex,actors_defs[our_actor.race].legs[our_actor.pants].legs_name,sizeof(our_actor.our_model->body_parts->pants_tex));

		my_strncp(our_actor.our_model->body_parts->boots_tex,actors_defs[our_actor.race].boots[our_actor.boots].boots_name,sizeof(our_actor.our_model->body_parts->boots_tex));

#ifdef	NEW_TEXTURES
		free_actor_texture(our_actor.our_model->texture_id);
		our_actor.our_model->texture_id = load_enhanced_actor(our_actor.our_model->body_parts, 0);	// Rebuild the actor's textures.
#else	/* NEW_TEXTURES */
		glDeleteTextures(1,&our_actor.our_model->texture_id); // Free the textures
		our_actor.our_model->texture_id = load_bmp8_enhanced_actor(our_actor.our_model->body_parts, 255);	// Rebuild the textures.
#endif	/* NEW_TEXTURES */

		// Move the actor. Could be a little disorienting, though.
		our_actor.our_model->x_tile_pos = our_actor.def->x;
		our_actor.our_model->y_tile_pos = our_actor.def->y;
		our_actor.our_model->x_pos = our_actor.def->x*0.5f;
		our_actor.our_model->y_pos = our_actor.def->y*0.5f;
		our_actor.our_model->z_rot = our_actor.def->z_rot;

#ifdef FR_VERSION
		our_actor.our_model->cur_anim.anim_index=-1;
		our_actor.our_model->stand_idle=0;
#endif //FR_VERSION
	}
}

//////////////////////////////////////////////////////////////////////////

// New character window code below.

int newchar_root_win = -1;
int color_race_win = -1;
int namepass_win = -1;
int newchar_advice_win = -1;
#ifdef NEW_NEW_CHAR_WINDOW
int newchar_hud_win = -1;
#endif
static int start_y = 50;

//	Display the "Character creation screen" and creation step help window.
//
int display_advice_handler (window_info *win)
{
	static int lastw = -1, lasth = -1;
	static Uint32 last_time = 0;
	static int flash = 0;
	static char *last_help = NULL;
	static char *help_str = NULL;
	int sep = 5;

	// Resize and move the window on first use and if window size changed.
	// Place centred, just down from the top of the screen.
	if ((lastw!=window_width) || (lasth!=window_height))
	{
		int len_x = (int)(2*sep + strlen(newchar_warning) * DEFAULT_FONT_X_LEN);
		int len_y = (int)(2*sep + DEFAULT_FONT_Y_LEN);
#ifdef NEW_NEW_CHAR_WINDOW
		int pos_x = (int)((window_width - len_x - hud_x) / 2);
#else
		int pos_x = (int)((window_width - len_x - HUD_MARGIN_X) / 2);
#endif
		resize_window(win->window_id, len_x, len_y);
		move_window(win->window_id, win->pos_id, win->pos_loc, pos_x, sep);
		start_y = sep*6 + len_y;
		lastw = window_width;
		lasth = window_height;
	}

	// Draw the waning text.
	glEnable(GL_TEXTURE_2D);
	glColor3f(1.0f,1.0f,1.0f);
	draw_string(sep, sep, (const unsigned char *)newchar_warning, 1);

	// Give eye icon help, then credentials icon help then "done" help.
	if (color_race_win < 0)
		help_str = newchar_cust_help;
	else if (!get_show_window(namepass_win))
		help_str = newchar_cred_help;
	else
		help_str = newchar_done_help;

	// Remember the last help string so we can flash when it changes
	if (help_str != last_help)
	{
		flash = 16;
		last_help = help_str;
	}

	// Time flashing, flash for a few seconds then remain on.
	if (flash && abs(SDL_GetTicks()-last_time) > 250)
	{
		flash--;
		last_time = SDL_GetTicks();
	}

	// Either always on or 1 in 4 off.
	if (!flash || (flash & 3))
#ifndef NEW_NEW_CHAR_WINDOW
		show_help(help_str, (int)((win->len_x - strlen(help_str) * SMALL_FONT_X_LEN)/2), window_height - HUD_MARGIN_Y - sep - DEFAULT_FONT_Y_LEN);
#else
	{
#ifdef FR_VERSION
		show_help(help_str, 0, window_height - HUD_MARGIN_Y - 40);
#else //FR_VERSION
		int len = strlen(help_str);
		int x = (int)((win->len_x - len * SMALL_FONT_X_LEN)/2);
		int y = window_height - HUD_MARGIN_Y - 2*sep - SMALL_FONT_Y_LEN;
		if(x >= -win->cur_x) //Does everything fit in one line?
		{
			show_help(help_str, x, y);
		}
		else
		{
			int i;
			for(i = len/2; i<=len; i++) //Find first space after the middle of the text
			{
				if(help_str[i] == ' ')
					break;
			}
			help_str[i] = 0;
			y -= 2 + SMALL_FONT_Y_LEN;
			x = (win->len_x - i*SMALL_FONT_X_LEN)/2;
			glColor4f(0.0f,0.0f,0.0f,0.5f);
			glDisable(GL_TEXTURE_2D);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_BLEND);
			glBegin(GL_QUADS);
			glVertex3i(x - 1,y + 2 + 2*SMALL_FONT_Y_LEN, 0);
			glVertex3i(x - 1,y,0);
			glVertex3i(x + i*SMALL_FONT_X_LEN, y, 0);
			glVertex3i(x + i*SMALL_FONT_X_LEN, y + 2 + 2*SMALL_FONT_Y_LEN, 0);
			glEnd();
			glDisable(GL_BLEND);
			glEnable(GL_TEXTURE_2D);

			glColor3f(1.0f,1.0f,1.0f);
			draw_string_small(x, y, (unsigned char*)help_str, 1);
			draw_string_small((win->len_x - (len - i - 1)*SMALL_FONT_X_LEN)/2, y + 2 + SMALL_FONT_Y_LEN, (unsigned char*)(help_str + i + 1), 1);
			help_str[i] = ' ';
		}
#endif //FR_VERSION
	}
#endif // NEW_NEW_CHAR_WINDOW

#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
	return 1;
}


int display_newchar_handler (window_info *win)
{
	int any_reflection;
	static int main_count = 0;

	if(disconnected)
	{
		static int nested_flag = 0;
		/* connect_to_server() calls draw_scene() so we need to prevent recursion */
		if (!nested_flag)
		{
#ifndef NEW_NEW_CHAR_WINDOW
			add_text_to_buffer(c_red2, "Connection failed, please try again", 6000);
#else
			LOG_TO_CONSOLE(c_red2, "Connection failed, please try again");
#endif
			creating_char = 1;	/* this was clear before the failed connect so reset */
			nested_flag = 1;
			connect_to_server();
			nested_flag = 0;
		}
	}

	//see if we have to load a model (male or female)
	if (creating_char && !our_actor.our_model){
		move_camera();//Make sure we lag a little...
#ifdef NEW_EYES
		our_actor.our_model = add_actor_interface (our_actor.def->x, our_actor.def->y, our_actor.def->z_rot, 1.0f, our_actor.race, our_actor.skin, our_actor.hair, our_actor.eyes, our_actor.shirt, our_actor.pants, our_actor.boots, our_actor.head);
#else //NEW_EYES
		our_actor.our_model = add_actor_interface (our_actor.def->x, our_actor.def->y, our_actor.def->z_rot, 1.0f, our_actor.race, our_actor.skin, our_actor.hair, our_actor.shirt, our_actor.pants, our_actor.boots, our_actor.head);
#endif //NEW_EYES
		yourself = 0;
		LOCK_ACTORS_LISTS();
		set_our_actor (our_actor.our_model);
		UNLOCK_ACTORS_LISTS();
	}

	if (!(main_count%10))
		read_mouse_now = 1;
	else
		read_mouse_now = 0;

	//This window is a bit special since it's not fully 2D
	Leave2DMode ();
	glPushMatrix ();

    update_camera();

	if (new_zoom_level != zoom_level) {
		zoom_level = new_zoom_level;
		resize_root_window ();
	}

	move_camera ();

	CalculateFrustum ();
	set_click_line();
	any_reflection = find_reflection ();
	CHECK_GL_ERRORS ();

	reset_under_the_mouse();

	if (SDL_GetAppState() & SDL_APPACTIVE)
	{

		draw_global_light ();

		if (skybox_show_sky)
        {
			if (skybox_update_delay < 1)
				skybox_update_colors();
            skybox_compute_z_position();
            glPushMatrix();
            glTranslatef(0.0, 0.0, skybox_get_z_position());
			skybox_display();
            glPopMatrix();
        }

		update_scene_lights();
		draw_lights();
		CHECK_GL_ERRORS ();

		if (shadows_on && is_day) {
			render_light_view();
			CHECK_GL_ERRORS ();
		}

		if (use_fog)
			weather_render_fog();
		if (any_reflection > 1) {
			draw_sky_background ();
			CHECK_GL_ERRORS ();
#ifdef ENGLISH
            // Les reflets posent soucis sur notre carte de creation
            // de personnage.
			if (show_reflection) display_3d_reflection ();
#endif //ENGLISH
		}

		CHECK_GL_ERRORS ();

		if (shadows_on && is_day) {
			draw_sun_shadowed_scene (any_reflection);
		} else {
			glNormal3f (0.0f,0.0f,1.0f);
			if (any_reflection) draw_lake_tiles ();
			draw_tile_map ();
			CHECK_GL_ERRORS ();
			display_2d_objects ();
			CHECK_GL_ERRORS ();
			anything_under_the_mouse (0, UNDER_MOUSE_NOTHING);
			display_objects ();
			display_ground_objects();
			display_actors (1, DEFAULT_RENDER_PASS);
			display_alpha_objects();
			display_blended_objects();
		}

		CHECK_GL_ERRORS ();
	}

	//particles should be last, we have no Z writting
	display_particles ();
	CHECK_GL_ERRORS ();

	Enter2DMode ();

	glColor3f(1.0f,1.0f,1.0f);

	draw_hud_frame();

	CHECK_GL_ERRORS ();

	{
#ifdef NEW_NEW_CHAR_WINDOW
		int msg, offset;
		if ( find_last_lines_time (&msg, &offset, current_filter, console_text_width) ){
			set_font(chat_font);    // switch to the chat font
			draw_messages (10, 40, display_text_buffer, DISPLAY_TEXT_BUFFER_SIZE, FILTER_ALL, msg, offset, -1, win->len_x - hud_x - 20, win->len_y, chat_zoom, NULL);
			set_font (0);   // switch to fixed
		}
#else
		int msg, offset, ytext, filter;
		ytext = use_windowed_chat == 1 ? 25 : 20;
		filter = use_windowed_chat == 1 ? current_filter : FILTER_ALL;
		if ( find_last_lines_time (&msg, &offset, current_filter, console_text_width) ){
			set_font(chat_font);    // switch to the chat font
			draw_messages (10, ytext, display_text_buffer, DISPLAY_TEXT_BUFFER_SIZE, filter, msg, offset, -1, win->len_x - hud_x - 20, win->len_y, chat_zoom, NULL);
			set_font (0);   // switch to fixed
		}
#endif
	}

#ifdef NEW_NEW_CHAR_WINDOW
#ifdef FR_VERSION
	draw_string_small_shadowed(98, win->len_y-hud_y+15, (unsigned char*)zoom_in_out, 1, 1.0f,0.9f,0.8f, 0.0f,0.0f,0.0f);
	draw_string_small_shadowed(98, win->len_y-hud_y+32, (unsigned char*)rotate_camera, 1, 1.0f,0.9f,0.8f, 0.0f,0.0f,0.0f);
#else //FR_VERSION
	glColor3f(251/255.0f, 250/255.0f, 190/255.0f);
	draw_string_small(68, win->len_y-hud_y+15, (unsigned char*)zoom_in_out, 1);
	draw_string_small(68, win->len_y-hud_y+32, (unsigned char*)rotate_camera, 1);
#endif //FR_VERSION
#endif
	Leave2DMode ();

	glEnable (GL_LIGHTING);
	glPopMatrix (); // restore the state
	Enter2DMode ();

	main_count++;
#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
	return 1;
}

int mouseover_newchar_handler (window_info *win, int mx, int my)
{
	return 1;
}

int click_newchar_handler (window_info *win, int mx, int my, Uint32 flags)
{
	if (flags & ELW_WHEEL_UP) {
		if (camera_zoom_dir == -1)
			camera_zoom_duration += 100;
		else
			camera_zoom_duration = 100;
		camera_zoom_dir = -1;
		return 1;
	}

	if (flags & ELW_WHEEL_DOWN) {
		if (camera_zoom_dir == 1)
			camera_zoom_duration += 100;
		else
			camera_zoom_duration = 100;
		camera_zoom_dir = 1;
		return 1;
	}

	return 1; // we captured this mouseclick
}

int keypress_newchar_handler (window_info *win, int mx, int my, Uint32 key, Uint32 unikey)
{
	static int last_time=0;
	int alt_on = key & ELW_ALT;
	int ctrl_on = key & ELW_CTRL;

	if ( check_quit_or_fullscreen (key) ) {
		return 1;
	} else if(disconnected && !alt_on && !ctrl_on){
		connect_to_server();
	} else if (key == K_CAMERAUP) {
		camera_tilt_speed = -normal_camera_rotation_speed * 0.0005;
		camera_tilt_duration += 100;
        camera_tilt_deceleration = normal_camera_deceleration*0.5E-3;
	} else if (key == K_CAMERADOWN) {
		camera_tilt_speed = normal_camera_rotation_speed * 0.0005;
		camera_tilt_duration += 100;
        camera_tilt_deceleration = normal_camera_deceleration*0.5E-3;
	} else if (key == K_ZOOMIN) {
		if (camera_zoom_dir == -1)
			camera_zoom_duration += 100;
		else
			camera_zoom_duration = 100;
		camera_zoom_dir = -1;
	} else if (key == K_ZOOMOUT) {
		if (camera_zoom_dir == 1)
			camera_zoom_duration += 100;
		else
			camera_zoom_duration = 100;
		camera_zoom_dir = 1;
	} else if(key==K_OPTIONS){
		view_window(&elconfig_win, 0);
	} else if(key==K_ENCYCLOPEDIA){
		view_tab(&tab_help_win, &tab_help_collection_id, HELP_TAB_ENCYCLOPEDIA);
	} else if(key==K_HELP) {
		view_tab(&tab_help_win, &tab_help_collection_id, HELP_TAB_HELP);
	} else if (key == K_RULES) {
		view_tab(&tab_help_win, &tab_help_collection_id, HELP_TAB_RULES);
	} else if (key == K_ROTATELEFT) {
		camera_rotation_speed = normal_camera_rotation_speed / 800.0;
		camera_rotation_duration = 800;
        camera_rotation_deceleration = normal_camera_deceleration*0.5E-3;
		if (fol_cam && !fol_cam_behind)
		{
			hold_camera += camera_kludge - last_kludge;
			last_kludge = camera_kludge;
		}
	} else if (key == K_FROTATELEFT) {
		camera_rotation_speed = fine_camera_rotation_speed / 200.0;
		camera_rotation_duration = 200;
		camera_rotation_speed /= 4.0;
        camera_rotation_deceleration = normal_camera_deceleration*0.5E-3;
		if (fol_cam && !fol_cam_behind)
		{
			hold_camera += camera_kludge - last_kludge;
			last_kludge = camera_kludge;
		}
	} else if (key == K_ROTATERIGHT) {
		camera_rotation_speed = -normal_camera_rotation_speed / 800.0;
		camera_rotation_duration = 800;
        camera_rotation_deceleration = normal_camera_deceleration*0.5E-3;
		if (fol_cam && !fol_cam_behind)
		{
			hold_camera += camera_kludge - last_kludge;
			last_kludge = camera_kludge;
		}
	} else if (key == K_FROTATERIGHT) {
		camera_rotation_speed = -fine_camera_rotation_speed / 200.0;
		camera_rotation_duration = 200;
		camera_rotation_speed /= 4.0;
        camera_rotation_deceleration = normal_camera_deceleration*0.5E-3;
		if (fol_cam && !fol_cam_behind)
		{
			hold_camera += camera_kludge - last_kludge;
			last_kludge = camera_kludge;
		}
	} else if(key==K_TURNLEFT){
		if(last_time+666<cur_time){
			add_command_to_actor(0, turn_left);
			last_time=cur_time;
		}
	} else if(key==K_TURNRIGHT){
		if(last_time+666<cur_time){
			add_command_to_actor(0, turn_right);
			last_time=cur_time;
		}
	}
	return 1;
}


int show_newchar_handler (window_info *win) {
	init_hud_interface (HUD_INTERFACE_NEW_CHAR);
	show_hud_windows();

	return 1;
}

#ifdef NEW_NEW_CHAR_WINDOW
void create_newchar_hud_window(void);
#endif

void create_newchar_root_window (void)
{
	if (newchar_root_win < 0)
	{
#ifdef FR_VERSION
		// on remet le perso initial al�atoire (srand(0) g�nant supprim� dans sky.c)
		our_actor.race_id=RAND(0, 15);
#else //FR_VERSION
		our_actor.race_id=RAND(0, 5);
#endif //FR_VERSION
		our_actor.def=&races[our_actor.race_id];//6 "races" - counting women as their own race, of course ;-) We cannot include the new races in the random function since they are p2p
		our_actor.skin = inc(our_actor.def->skin, SKIN_BROWN, RAND (SKIN_BROWN, SKIN_TAN));//Increment a random # of times.
		our_actor.hair = inc(our_actor.def->hair, HAIR_BLACK, RAND (HAIR_BLACK, our_actor.def->type >= draegoni_female ? HAIR_PURPLE:HAIR_WHITE));
#ifdef NEW_EYES
		our_actor.eyes = inc(our_actor.def->eyes, EYES_BROWN, RAND (EYES_BROWN, EYES_GOLD));
#endif //NEW_EYES
		our_actor.shirt = inc(our_actor.def->shirts, SHIRT_BLACK, RAND (SHIRT_BLACK, SHIRT_YELLOW));
		our_actor.pants = inc(our_actor.def->pants, PANTS_BLACK, RAND (PANTS_BLACK, PANTS_WHITE));
		our_actor.boots = inc(our_actor.def->boots, BOOTS_BLACK, RAND (BOOTS_BLACK, BOOTS_ORANGE));
#ifdef FR_VERSION
		our_actor.head = inc(our_actor.def->head, HEAD_1, RAND (HEAD_1, our_actor.def->type==eldo_female?HEAD_5:HEAD_4));
#else //FR_VERSION
		our_actor.head = inc(our_actor.def->head, HEAD_1, RAND (HEAD_1, our_actor.def->type==human_female?HEAD_5:HEAD_4));
#endif //FR_VERSION
		our_actor.race = our_actor.def->type;
		our_actor.male = our_actor.race<gnome_female?our_actor.race%2:!(our_actor.race%2);
#ifdef FR_VERSION
		our_actor.scale = inc(our_actor.def->scale, SCALE_5, 1);
#endif //FR_VERSION

		game_minute = 120;	//Midday. So that it's bright and sunny.
		real_game_minute = game_minute;

#ifdef FR_VERSION
		change_map ("./maps/0_nouveau_perso");
#else //FR_VERSION
		change_map ("./maps/newcharactermap.elm");
#endif //FR_VERSION

		newchar_root_win = create_window (win_newchar, -1, -1, 0, 0, window_width, window_height, ELW_TITLE_NONE|ELW_SHOW_LAST);

		set_window_handler (newchar_root_win, ELW_HANDLER_DISPLAY, &display_newchar_handler);
		set_window_handler (newchar_root_win, ELW_HANDLER_MOUSEOVER, &mouseover_newchar_handler);
		set_window_handler (newchar_root_win, ELW_HANDLER_CLICK, &click_newchar_handler);
		set_window_handler (newchar_root_win, ELW_HANDLER_KEYPRESS, &keypress_newchar_handler);
		set_window_handler (newchar_root_win, ELW_HANDLER_SHOW, &show_newchar_handler);
		set_window_handler (newchar_root_win, ELW_HANDLER_AFTER_SHOW, &update_have_display);
		set_window_handler (newchar_root_win, ELW_HANDLER_HIDE, &update_have_display);

		newchar_advice_win = create_window ("Advice", newchar_root_win, 0, 100, 10, 200, 100, ELW_USE_BACKGROUND|ELW_USE_BORDER|ELW_SHOW|ELW_ALPHA_BORDER);
		set_window_handler (newchar_advice_win, ELW_HANDLER_DISPLAY, &display_advice_handler);
#ifdef NEW_NEW_CHAR_WINDOW
		create_newchar_hud_window();
#endif
#ifdef FR_VERSION
		recadrage_camera();
#endif //FR_VERSION

		LOG_TO_CONSOLE(c_green1, char_help);
	} else {
		show_window(newchar_root_win);
		show_window(newchar_advice_win);
#ifdef NEW_NEW_CHAR_WINDOW
		show_window(newchar_hud_win);
		show_window(color_race_win);
		hide_window(namepass_win);
#endif
	}
#ifdef NEW_NEW_CHAR_WINDOW
	old_use_windowed_chat = use_windowed_chat;
	use_windowed_chat = 0;
#endif
}

int active=0;
#ifdef NEW_NEW_CHAR_WINDOW
int hidden=0;

int are_you_sure=1;
#else
int hidden=1;

int are_you_sure=0;
#endif
int numbers_in_name=0;

char * get_pass_str(int l)
{
	static char str[20];

	memset(str, '*', l);
	str[l]=0;

	return str;
}

//Returns 1 if it's valid, 0 if invalid and -1 if there's too many numbers in the name
int check_character(int type, char ch)
{
	int retval=0;

	if(type==0){
		//name
#ifndef ENGLISH
		//@tosh : seul les caract�res alphab�tiques et _ sont autoris�s dans le pseudo
		if(isalpha(ch) || ch=='_')
			retval=1;
#else //ENGLISH
		if(isdigit(ch)){
			// no more then two digits in a name
			if (numbers_in_name >= 2)
			{
				retval=-1;
			} else {
				numbers_in_name++;
				retval=1;
			}
		} else if(isalnum(ch)||ch=='_'){
			retval=1;
		} else if ((ch>= 33 && ch<=47)||(ch>=58 && ch<=64)||(ch>=91&&ch<=96)||(ch>=122 && ch<=126)){
			// not permitted in a name
			retval=-1;
		}
#endif //ENGLISH
	} else {	// password
		if(ch>=33 && ch<126) retval=1;
	}

	return retval;
}

void add_text_to_buffer(int color, char * text, int time_to_display)
{
	put_small_colored_text_in_box(color, (unsigned char*)text, strlen(text), 200, create_char_error_str);
	display_time=cur_time+time_to_display;
}

void create_character(void)
{
	if(!strncasecmp(inputs[1].str, actors_list[0]->actor_name, strlen(actors_list[0]->actor_name))){
#ifdef NEW_NEW_CHAR_WINDOW
		add_text_to_buffer(c_red2, error_bad_pass, 6000);
#else
		add_text_to_buffer(c_red2, "Bad password!", 6000);
		//add_text_to_buffer(c_red2, error_bad_pass, 6000);
#endif
		return;
	} else if(strcmp(inputs[1].str, inputs[2].str)){
		add_text_to_buffer(c_red2, error_pass_no_match, 6000);
		return;
	} else if(inputs[0].pos<3){
		add_text_to_buffer(c_red2, error_username_length, 6000);
		return;
	} else if(inputs[1].pos<4){
		add_text_to_buffer(c_red2, error_password_length, 6000);
		return;
	}

	if(are_you_sure){
		creating_char=0;
		// Clear the error message, if necessary
		create_char_error_str[0] = '\0';
#ifdef FR_VERSION
		send_new_char(inputs[0].str, inputs[1].str, our_actor.skin, our_actor.hair, our_actor.shirt, our_actor.pants, our_actor.boots, our_actor.head, our_actor.race, our_actor.scale);
#else //FR_VERSION
		send_new_char(inputs[0].str, inputs[1].str, our_actor.skin, our_actor.hair, our_actor.eyes, our_actor.shirt, our_actor.pants, our_actor.boots, our_actor.head, our_actor.race);
#endif //FR_VERSION
	} else {
		are_you_sure=1;

		add_text_to_buffer(c_orange3, error_confirm_create_char, 6000);
		LOG_TO_CONSOLE(c_green2, "\n");
		LOG_TO_CONSOLE(c_green2, remember_change_appearance);

#ifndef NEW_NEW_CHAR_WINDOW
		show_color_race_win();
#endif
	}
}

void login_from_new_char(void)
{
	safe_snprintf(username_str, sizeof(username_str), "%s", inputs[0].str);
	safe_snprintf(password_str, sizeof(password_str), "%s", inputs[1].str);

	// now destroy reference to ourself, otherwise we'll mess up the ID's
	destroy_all_actors();
	our_actor.our_model=NULL;

	// close help and setting windows
	if (tab_help_win >= 0) hide_window (tab_help_win);
	if (elconfig_win >= 0) hide_window (elconfig_win);

#ifdef NEW_NEW_CHAR_WINDOW
	//restore use_windowed_chat
	use_windowed_chat = old_use_windowed_chat;
	hide_window(newchar_hud_win);
#endif

	//now send the log in info
	send_login_info();
}

#ifndef NEW_NEW_CHAR_WINDOW
int display_namepass_handler (window_info * win)
{
	glColor3f(0.77f,0.57f,0.39f);

	draw_string_small(20, 20, (unsigned char*)login_username_str, 1);
	draw_string_small(20, 60, (unsigned char*)login_password_str, 1);
	draw_string_small(20, 90, (unsigned char*)confirm_password, 1);
	draw_smooth_button(inputs[0].str, DEFAULT_SMALL_RATIO, 100, 16, 120, 1, 0.77f, 0.57f ,0.39f, active == 0, 0.32f, 0.23f, 0.15f, 0.5f);
	draw_smooth_button(hidden?get_pass_str(inputs[1].pos):inputs[1].str, DEFAULT_SMALL_RATIO, 100, 56, 120, 1, 0.77f, 0.57f ,0.39f, active == 1, 0.32f, 0.23f, 0.15f, 0.5f);
	draw_smooth_button(hidden?get_pass_str(inputs[2].pos):inputs[2].str, DEFAULT_SMALL_RATIO, 100, 86, 120, 1, 0.77f, 0.57f ,0.39f, active == 2, 0.32f, 0.23f, 0.15f, 0.5f);

	draw_smooth_button(hidden?show_password:hide_password, DEFAULT_SMALL_RATIO, 20, 120, 200, 1, 0.77f, 0.57f ,0.39f, hidden, 0.32f, 0.23f, 0.15f, 0.5f);

	draw_box(NULL, 20, 160, 220, textboxy, 0);

	draw_smooth_button(char_done, DEFAULT_SMALL_RATIO, 20, 170+textboxy, 60, 1, 0.77f, 0.57f ,0.39f, are_you_sure, 0.32f, 0.23f, 0.15f, 0.5f);
	draw_smooth_button(char_back, DEFAULT_SMALL_RATIO, 160, 170+textboxy, 60, 1, 0.77f, 0.57f ,0.39f, 0, 0.32f, 0.23f, 0.15f, 0.5f);

	//why remove the massage after a time delay, I may have missed it
	//debate and remove the code if really not wanted - pjbroad/bluap
	//if(display_time>cur_time){
		draw_string_small(30, 168, (unsigned char*)create_char_error_str, 8);
	//}

	return 1;
}

int keypress_namepass_handler (window_info *win, int mx, int my, Uint32 key, Uint32 unikey)
{
	Uint8 ch = key_to_char (unikey);
	int ret=0;
	struct input_text * t=&inputs[active];

#ifdef FR_VERSION
	// puisque le d�lai d'affichage des messages est d�sactiv� (cf display_namepass_handler)
	// alors on efface manuellement le message courant avant une nouvelle action utilisateur
	create_char_error_str[0] = '\0';
#endif //FR_VERSION
	if ((ret=check_character (active > 0, ch)))
	{
		if (ret==-1)
		{
			add_text_to_buffer (c_red1, error_max_digits, 6000);
		}
		else if (t->pos + 1 > MAX_USERNAME_LENGTH - 1)		// MAX_USERNAME_LENGTH includes the null terminator and t->pos counts from 0
		{
			add_text_to_buffer (c_red2, error_length, 6000);
		}
		else
		{
			// don't allow a character to start with player
			if(active == 0 && !strcasecmp(t->str, "player")) t->pos= 0;
			// add the character to the buffer
			t->str[t->pos++]=ch;
			t->str[t->pos]=0;
			ret=1;	//Reused to show that a letter has been added
		}
	}
	else if (unikey == SDLK_TAB || unikey == SDLK_RETURN)
	{
		active++;
		if(active>2) active=0;
	}
#ifndef OSX
	else if (unikey == SDLK_BACKSPACE && t->pos>0)
#else
        else if (key == SDLK_BACKSPACE && t->pos>0)
#endif
	{
		t->pos--;
		if (isdigit (t->str[t->pos])) numbers_in_name--;
		t->str[t->pos] = 0;
		ret = 1;	// Reused to show that a letter has been removed
	}
	else
	{
		//only send error messages on non-null characters
		if(ch != 0){
			add_text_to_buffer (c_red2, error_illegal_character, 6000);
		}
	}

	if(active>0){
		//Password/confirm
		if(ret){
			if(!strncasecmp(inputs[1].str, actors_list[0]->actor_name, strlen(actors_list[0]->actor_name))){
				add_text_to_buffer(c_red2, error_bad_pass, 6000);
			} else if(strcmp(inputs[1].str, inputs[2].str)){
				add_text_to_buffer(c_red2, error_pass_no_match, 6000);
			} else {
				add_text_to_buffer(c_green1, passwords_match, 6000);
			}
		}
	} else {
		safe_snprintf(actors_list[0]->actor_name, sizeof(actors_list[0]->actor_name), "%s", inputs[0].str);
	}

	return 1;
}

int click_namepass_handler(window_info * win, int mx, int my, Uint32 flags)
{
	if ( (flags & ELW_MOUSE_BUTTON) == 0) return 0;

	if(mx>100 && mx<260){
		if(my>16 && my<38){
			active=0;
		} else if(my>56 && my<78){
			active=1;
		} else if(my>86 && my<108){
			active=2;
		}
	}

	if(mx>20 && mx<220 && my>120 && my<144){
		hidden=!hidden;
	}

	if(my>170+textboxy && my<192+textboxy){
		if(mx>20 && mx<100){
			create_character();
		} if(mx>160 && mx<240){
			destroy_all_actors();
			our_actor.our_model=NULL;
			hide_window (newchar_root_win);
			show_window (login_root_win);
			hide_hud_windows ();
		}
	}


	return 0;
}

void show_account_win (void)
{
	if (namepass_win < 0){
	        // Create the window
	        namepass_win = create_window (win_name_pass, newchar_root_win, 0, 10, start_y, 270, 200+textboxy, ELW_WIN_DEFAULT);
	        set_window_handler (namepass_win, ELW_HANDLER_DISPLAY, &display_namepass_handler);
	        set_window_handler (namepass_win, ELW_HANDLER_KEYPRESS, &keypress_namepass_handler);
	        set_window_handler (namepass_win, ELW_HANDLER_CLICK, &click_namepass_handler);
    	} else {
        	show_window(namepass_win);
		select_window(namepass_win);
	}
}
#endif

//The character design window
void change_race(int new_race)
{
	if(our_actor.race_id==new_race)return;
	destroy_all_actors();
	our_actor.our_model = NULL;
	our_actor.race_id=new_race;
	our_actor.def=&races[new_race];
	our_actor.skin = our_actor.def->skin[find_pos_in_enum(our_actor.def->skin, our_actor.skin)];//Increment a random # of times.
	our_actor.hair = our_actor.def->hair[find_pos_in_enum(our_actor.def->hair, our_actor.hair)];
#ifdef NEW_EYES
	our_actor.eyes = our_actor.def->eyes[find_pos_in_enum(our_actor.def->eyes, our_actor.eyes)];
#endif //NEW_EYES
	our_actor.shirt = our_actor.def->shirts[find_pos_in_enum(our_actor.def->shirts, our_actor.shirt)];
	our_actor.pants = our_actor.def->pants[find_pos_in_enum(our_actor.def->pants, our_actor.pants)];
	our_actor.boots = our_actor.def->boots[find_pos_in_enum(our_actor.def->boots, our_actor.boots)];
	our_actor.head = our_actor.def->head[find_pos_in_enum(our_actor.def->head, our_actor.head)];
	our_actor.race = our_actor.def->type;
#ifdef FR_VERSION
	our_actor.scale = our_actor.def->scale[find_pos_in_enum(our_actor.def->scale, our_actor.scale)];
#endif //FR_VERSION
	our_actor.male = our_actor.race<gnome_female?our_actor.race%2:!(our_actor.race%2);

	change_actor();
}

int race_help=0;
int book_over=-1;

#ifdef FR_VERSION
int book_eldo=200000;
int book_haut_elfe=200001;
int book_dwarf=200002;
int book_orchan=200004;
int book_sinan=200005;
int book_elfe_noir=200006;
int book_gnome=200007;
int book_draegoni=200008;
#else //FR_VERSION
int book_human=200000;
int book_dwarf=200001;
int book_elf=200002;
int book_gnome=200003;
int book_orchan=200004;
int book_draegoni=200005;
#endif //FR_VERSION

void toggle_book(int id)
{
#ifdef FR_VERSION
	static int livre_ouvert = -1;
	if (livre_ouvert==id && book_win && windows_list.window[book_win].displayed)
    {
		ferme_livre(id);
        livre_ouvert = -1;
#else //FR_VERSION
	static int book_opened=-1;
	if(book_opened==id && book_win && windows_list.window[book_win].displayed){
		close_book(id);
		book_opened=-1;
#endif //FR_VERSION
	} else {
#ifdef FR_VERSION
        ouvre_livre(id);
        livre_ouvert = id;
#else //FR_VERSION
		open_book(id);
		book_opened=id;
#endif //FR_VERSION
	}
    // On force l'affichage du livre, m�me si la fen�tre doit rester en-dessous.
    move_window(book_win, -1, 0, windows_list.window[book_win].pos_x, windows_list.window[book_win].pos_y );
    show_window(book_win);
}

#ifndef NEW_NEW_CHAR_WINDOW
int display_color_race_handler (window_info *win)
{
	int x;

#ifdef ENGLISH
	glColor3f(1.00f,0.0f,0.f);
	// the red box around P2P races
	draw_box("P2P", 135, 70, 120, 88 , 0);
#endif //ENGLISH

	glColor3f(0.77f,0.57f,0.39f);

	//Gender
	draw_box(gender_str, 10, 10, 250, 45, 0);
	draw_smooth_button(male_str, DEFAULT_SMALL_RATIO, 40, 22, 60, 1, 0.77f, 0.57f ,0.39f, our_actor.male, 0.32f, 0.23f, 0.15f, 0.5f);
	draw_smooth_button(female_str, DEFAULT_SMALL_RATIO, 150, 22, 60, 1, 0.77f, 0.57f ,0.39f, !our_actor.male, 0.32f, 0.23f, 0.15f, 0.5f);

	//Race
	draw_box(race_str, 10, 65, 250, 98, 0);
	draw_smooth_button(human_str, DEFAULT_SMALL_RATIO, 20, 75, 60, 1, 0.77f, 0.57f ,0.39f, our_actor.race==human_female||our_actor.race==human_male, 0.32f, 0.23f, 0.15f, 0.5f);
	draw_smooth_button(elf_str, DEFAULT_SMALL_RATIO, 20, 103, 60, 1, 0.77f, 0.57f ,0.39f, our_actor.race==elf_female||our_actor.race==elf_male, 0.32f, 0.23f, 0.15f, 0.5f);
	draw_smooth_button(dwarf_str, DEFAULT_SMALL_RATIO, 20, 131, 60, 1, 0.77f, 0.57f ,0.39f, our_actor.race==dwarf_female||our_actor.race==dwarf_male, 0.32f, 0.23f, 0.15f, 0.5f);
	draw_smooth_button(gnome_str, DEFAULT_SMALL_RATIO, 140, 75, 60, 1, 0.77f, 0.57f ,0.39f, our_actor.race==gnome_female||our_actor.race==gnome_male, 0.32f, 0.23f, 0.15f, 0.5f);
	draw_smooth_button(orchan_str, DEFAULT_SMALL_RATIO, 140, 103, 60, 1, 0.77f, 0.57f ,0.39f, our_actor.race==orchan_female||our_actor.race==orchan_male, 0.32f, 0.23f, 0.15f, 0.5f);
	draw_smooth_button(draegoni_str, DEFAULT_SMALL_RATIO, 140, 131, 60, 1, 0.77f, 0.57f ,0.39f, our_actor.race==draegoni_female||our_actor.race==draegoni_male, 0.32f, 0.23f, 0.15f, 0.5f);

	//Appearance
	draw_box(appearance_str, 270, 10, 120, win->len_y-17, 0);
	x=330;
	draw_string_small(x-(get_string_width((unsigned char*)head_str)*8.0f/12.0f)/2.0f, 25, (unsigned char*)head_str, 1);
	draw_string_small(x-(get_string_width((unsigned char*)skin_str)*8.0f/12.0f)/2.0f, 48, (unsigned char*)skin_str, 1);
	draw_string_small(x-(get_string_width((unsigned char*)hair_str)*8.0f/12.0f)/2.0f, 71, (unsigned char*)hair_str, 1);
#ifdef NEW_EYES
	draw_string_small(x-(get_string_width((unsigned char*)eyes_str)*8.0f/12.0f)/2.0f, 94, (unsigned char*)eyes_str, 1);
	draw_string_small(x-(get_string_width((unsigned char*)shirt_str)*8.0f/12.0f)/2.0f, 117, (unsigned char*)shirt_str, 1);
	draw_string_small(x-(get_string_width((unsigned char*)pants_str)*8.0f/12.0f)/2.0f, 140, (unsigned char*)pants_str, 1);
	draw_string_small(x-(get_string_width((unsigned char*)boots_str)*8.0f/12.0f)/2.0f, 163, (unsigned char*)boots_str, 1);
#else //NEW_EYES
	draw_string_small(x-(get_string_width((unsigned char*)eyes_str)*8.0f/12.0f)/2.0f, 94, (unsigned char*)eyes_str, 1);
	draw_string_small(x-(get_string_width((unsigned char*)shirt_str)*8.0f/12.0f)/2.0f, 94, (unsigned char*)shirt_str, 1);
	draw_string_small(x-(get_string_width((unsigned char*)pants_str)*8.0f/12.0f)/2.0f, 117, (unsigned char*)pants_str, 1);
	draw_string_small(x-(get_string_width((unsigned char*)boots_str)*8.0f/12.0f)/2.0f, 140, (unsigned char*)boots_str, 1);
#endif //NEW_EYES

	//<<
	x=280;
	draw_string_small(x, 25, (unsigned char*)"<<", 1);
	draw_string_small(x, 48, (unsigned char*)"<<", 1);
	draw_string_small(x, 71, (unsigned char*)"<<", 1);
	draw_string_small(x, 94, (unsigned char*)"<<", 1);
	draw_string_small(x, 117, (unsigned char*)"<<", 1);
	draw_string_small(x, 140, (unsigned char*)"<<", 1);
#ifdef NEW_EYES
	draw_string_small(x, 163, (unsigned char*)"<<", 1);
#endif //NEW_EYES

	//>>
	x=364;
	draw_string_small(x, 25, (unsigned char*)">>", 1);
	draw_string_small(x, 48, (unsigned char*)">>", 1);
	draw_string_small(x, 71, (unsigned char*)">>", 1);
	draw_string_small(x, 94, (unsigned char*)">>", 1);
	draw_string_small(x, 117, (unsigned char*)">>", 1);
	draw_string_small(x, 140, (unsigned char*)">>", 1);
#ifdef NEW_EYES
	draw_string_small(x, 163, (unsigned char*)">>", 1);
#endif //NEW_EYES

#ifdef ENGLISH
	switch(race_help){
		case 1:
			show_help(p2p_race, 260, 80);
			break;
		case 2:
			show_help(p2p_race, 260, 110);
			break;
		case 3:
			show_help(p2p_race, 260, 140);
			break;
		case 0:
		default:
			break;
	}
#endif

	if(book_over==book_human){
		show_help(about_human, 142, 80);
	} else if(book_over==book_elf){
		show_help(about_elves, 142, 108);
	} else if(book_over==book_dwarf){
		show_help(about_dwarfs, 142, 136);
	} else if(book_over==book_gnome){
		show_help(about_gnomes, 260, 80);
	} else if(book_over==book_orchan){
		show_help(about_orchans, 262, 108);
	} else if(book_over==book_draegoni){
		show_help(about_draegoni, 262, 136);
	}

	glColor3f(1.0f,1.0f,1.0f);
	glEnable(GL_TEXTURE_2D);

#ifdef	NEW_TEXTURES
	bind_texture(icons_text);
#else	/* NEW_TEXTURES */
	get_and_set_texture_id(icons_text);
#endif	/* NEW_TEXTURES */
	glBegin(GL_QUADS);
#ifdef FR_VERSION
	if (livre_ouvert == book_human || book_over == book_human)
#else //FR_VERSION
	if(book_opened==book_human||book_over==book_human)
#endif //FR_VERSION
		draw_2d_thing((float)32/256,1.0f-(float)64/256,(float)63/256,1.0f-(float)95/256, 110, 75, 132, 97);
	else
		draw_2d_thing((float)0/256,1.0f-(float)64/256,(float)31/256,1.0f-(float)95/256, 110, 75, 132, 97);
#ifdef FR_VERSION
	if (livre_ouvert == book_elf || book_over == book_elf)
#else //FR_VERSION
	if(book_opened==book_elf||book_over==book_elf)
#endif //FR_VERSION
		draw_2d_thing((float)32/256,1.0f-(float)64/256,(float)63/256,1.0f-(float)95/256, 110, 103, 132, 125);
	else
		draw_2d_thing((float)0/256,1.0f-(float)64/256,(float)31/256,1.0f-(float)95/256, 110, 103, 132, 125);
#ifdef FR_VERSION
	if (livre_ouvert == book_dwarf || book_over == book_dwarf)
#else //FR_VERSION
	if(book_opened==book_dwarf||book_over==book_dwarf)
#endif //FR_VERSION
		draw_2d_thing((float)32/256,1.0f-(float)64/256,(float)63/256,1.0f-(float)95/256, 110, 131, 132, 153);
	else
		draw_2d_thing((float)0/256,1.0f-(float)64/256,(float)31/256,1.0f-(float)95/256, 110, 131, 132, 153);
#ifdef FR_VERSION
	if (livre_ouvert == book_gnome || book_over == book_gnome)
#else //FR_VERSION
	if(book_opened==book_gnome||book_over==book_gnome)
#endif //FR_VERSION
		draw_2d_thing((float)32/256,1.0f-(float)64/256,(float)63/256,1.0f-(float)95/256, 230, 75, 252, 97);
	else
		draw_2d_thing((float)0/256,1.0f-(float)64/256,(float)31/256,1.0f-(float)95/256, 230, 75, 252, 97);
#ifdef FR_VERSION
	if (livre_ouvert == book_orchan || book_over == book_orchan)
#else //FR_VERSION
	if(book_opened==book_orchan||book_over==book_orchan)
#endif //FR_VERSION
		draw_2d_thing((float)32/256,1.0f-(float)64/256,(float)63/256,1.0f-(float)95/256, 230, 103, 252, 125);
	else
		draw_2d_thing((float)0/256,1.0f-(float)64/256,(float)31/256,1.0f-(float)95/256, 230, 103, 252, 125);
#ifdef FR_VERSION
	if (livre_ouvert == book_draegoni || book_over == book_draegoni)
#else //FR_VERSION
	if(book_opened==book_draegoni||book_over==book_draegoni)
#endif //FR_VERSION
		draw_2d_thing((float)32/256,1.0f-(float)64/256,(float)63/256,1.0f-(float)95/256, 230, 131, 252, 153);
	else
		draw_2d_thing((float)0/256,1.0f-(float)64/256,(float)31/256,1.0f-(float)95/256, 230, 131, 252, 153);

	glEnd();
#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
	return 1;
}

int mouseover_color_race_handler (window_info *win, int mx, int my)
{
	if(mx>140 && mx<220){
		//P2P races
		if(my>75 && my<97){
			race_help=1;
		} else if(my>103 && my<125){
			race_help=2;
		} else if(my>131 && my<153){
			race_help=3;
		} else  race_help=0;
	} else race_help=0;

	if(mx>110 && mx<132) {
		if(my>75 && my<97){
			book_over=book_human;
		} else if(my>103 && my<125){
			book_over=book_elf;
		} else if(my>131 && my<153){
			book_over=book_dwarf;
		} else  book_over=-1;
	} else if(mx>230 && mx<252){
		if(my>75 && my<97){
			book_over=book_gnome;
		} else if(my>103 && my<125){
			book_over=book_orchan;
		} else if(my>131 && my<153){
			book_over=book_draegoni;
		} else  book_over=-1;
	} else book_over=-1;
	return 1;
}

int click_color_race_handler (window_info *win, int mx, int my, Uint32 flags)
{
	if ( (flags & ELW_MOUSE_BUTTON) == 0) return 0;


	if(my>22 && my<44){
		if(mx>40 && mx<120){
			change_race(our_actor.race_id+!our_actor.male);
		} else if(mx>150 && mx<210){
			change_race(our_actor.race_id-our_actor.male);
		}
	}

	if(mx>20 && mx<100){
		if(my>75 && my<97){
			change_race(0+our_actor.male);
		} else if(my>103 && my<125){
			change_race(2+our_actor.male);
		} else if(my>131 && my<153){
			change_race(4+our_actor.male);
		}
	} else if(mx>110 && mx<132) {
		if(my>75 && my<97){
			toggle_book(book_human);
			return 0;
		} else if(my>103 && my<125){
			toggle_book(book_elf);
			return 0;
		} else if(my>131 && my<153){
			toggle_book(book_dwarf);
			return 0;
		}
	} else if(mx>140 && mx<220) {
		if(my>75 && my<97){
			change_race(6+our_actor.male);
		} else if(my>103 && my<125){
			change_race(8+our_actor.male);
		} else if(my>131 && my<153){
			change_race(10+our_actor.male);
		}
	} else if(mx>230 && mx<252){
		if(my>75 && my<97){
			toggle_book(book_gnome);
			return 0;
		} else if(my>103 && my<125){
			toggle_book(book_orchan);
			return 0;
		} else if(my>131 && my<153){
			toggle_book(book_draegoni);
			return 0;
		}
	} else if(mx>280 && mx<295){
		if(my>25 && my<36){
			our_actor.head=dec(our_actor.def->head, our_actor.head, 1);

			// Detach the old head, and reattach and save the new one.
			model_detach_mesh(our_actor.our_model, our_actor.our_model->body_parts->head_meshindex);
			model_attach_mesh(our_actor.our_model, actors_defs[our_actor.race].head[our_actor.head].mesh_index);
			our_actor.our_model->body_parts->head_meshindex=actors_defs[our_actor.race].head[our_actor.head].mesh_index;
		} else if(my>48 && my<59){
			our_actor.skin=dec(our_actor.def->skin, our_actor.skin, 1);
			// Copy the skin texture names.
			my_strncp(our_actor.our_model->body_parts->hands_tex,actors_defs[our_actor.race].skin[our_actor.skin].hands_name,sizeof(our_actor.our_model->body_parts->hands_tex));
			my_strncp(our_actor.our_model->body_parts->head_tex,actors_defs[our_actor.race].skin[our_actor.skin].head_name,sizeof(our_actor.our_model->body_parts->head_tex));

#ifdef	NEW_TEXTURES
			change_enhanced_actor(our_actor.our_model->texture_id, our_actor.our_model->body_parts);
#else	/* NEW_TEXTURES */
			glDeleteTextures(1,&our_actor.our_model->texture_id); // Free the textures
			our_actor.our_model->texture_id = load_bmp8_enhanced_actor(our_actor.our_model->body_parts, 255);	// Rebuild the actor's textures.
#endif	/* NEW_TEXTURES */
		} else if(my>71 && my<82){
			our_actor.hair=dec(our_actor.def->hair, our_actor.hair, 1);

			// Copy the hair texture name.
			my_strncp(our_actor.our_model->body_parts->hair_tex,actors_defs[our_actor.race].hair[our_actor.hair].hair_name,sizeof(our_actor.our_model->body_parts->hair_tex));

#ifdef	NEW_TEXTURES
			change_enhanced_actor(our_actor.our_model->texture_id, our_actor.our_model->body_parts);
#else	/* NEW_TEXTURES */
			glDeleteTextures(1,&our_actor.our_model->texture_id); // Free the textures
			our_actor.our_model->texture_id = load_bmp8_enhanced_actor(our_actor.our_model->body_parts, 255);	// Rebuild the actor's textures.
#endif	/* NEW_TEXTURES */
		} else if(my>94 && my<105){
#ifdef NEW_EYES
			our_actor.eyes=dec(our_actor.def->eyes, our_actor.eyes, 1);

			// Copy the eyes texture name.
			my_strncp(our_actor.our_model->body_parts->eyes_tex,actors_defs[our_actor.race].eyes[our_actor.eyes].eyes_name,sizeof(our_actor.our_model->body_parts->eyes_tex));
#ifdef	NEW_TEXTURES
			change_enhanced_actor(our_actor.our_model->texture_id, our_actor.our_model->body_parts);
#else	/* NEW_TEXTURES */
			glDeleteTextures(1,&our_actor.our_model->texture_id); // Free the textures
			our_actor.our_model->texture_id = load_bmp8_enhanced_actor(our_actor.our_model->body_parts, 255);	// Rebuild the actor's textures.
#endif  /* NEW_TEXTURES */
#endif //NEW_EYES
			our_actor.shirt=dec(our_actor.def->shirts, our_actor.shirt, 1);

			// Copy the shirt and arms texture names.
			my_strncp(our_actor.our_model->body_parts->arms_tex,actors_defs[our_actor.race].shirt[our_actor.shirt].arms_name,sizeof(our_actor.our_model->body_parts->arms_tex));
			my_strncp(our_actor.our_model->body_parts->torso_tex,actors_defs[our_actor.race].shirt[our_actor.shirt].torso_name,sizeof(our_actor.our_model->body_parts->torso_tex));

			// If we need a new mesh, drop the old one and load it.
			if(actors_defs[our_actor.race].shirt[our_actor.shirt].mesh_index != our_actor.our_model->body_parts->torso_meshindex)
			{
				model_detach_mesh(our_actor.our_model, our_actor.our_model->body_parts->torso_meshindex);
				model_attach_mesh(our_actor.our_model, actors_defs[our_actor.race].shirt[our_actor.shirt].mesh_index);
				our_actor.our_model->body_parts->torso_meshindex=actors_defs[our_actor.race].shirt[our_actor.shirt].mesh_index;
			}

#ifdef	NEW_TEXTURES
			change_enhanced_actor(our_actor.our_model->texture_id, our_actor.our_model->body_parts);
#else	/* NEW_TEXTURES */
			glDeleteTextures(1,&our_actor.our_model->texture_id); // Free the textures
			our_actor.our_model->texture_id = load_bmp8_enhanced_actor(our_actor.our_model->body_parts, 255);	// Rebuild the actor's textures.
#endif	/* NEW_TEXTURES */
#ifdef NEW_EYES
		} else if(my>140 && my<151){
#else //NEW_EYES
		} else if(my>117 && my<128){
#endif //NEW_EYES
			our_actor.pants=dec(our_actor.def->pants, our_actor.pants, 1);

			// Copy the pants texture name.
			my_strncp(our_actor.our_model->body_parts->pants_tex,actors_defs[our_actor.race].legs[our_actor.pants].legs_name,sizeof(our_actor.our_model->body_parts->pants_tex));

			// If we need a new mesh, drop the old one and load it.
			if(actors_defs[our_actor.race].legs[our_actor.pants].mesh_index != our_actor.our_model->body_parts->legs_meshindex)
			{
				model_detach_mesh(our_actor.our_model, our_actor.our_model->body_parts->legs_meshindex);
				model_attach_mesh(our_actor.our_model, actors_defs[our_actor.race].legs[our_actor.pants].mesh_index);
				our_actor.our_model->body_parts->legs_meshindex=actors_defs[our_actor.race].legs[our_actor.pants].mesh_index;
			}

#ifdef	NEW_TEXTURES
			change_enhanced_actor(our_actor.our_model->texture_id, our_actor.our_model->body_parts);
#else	/* NEW_TEXTURES */
			glDeleteTextures(1,&our_actor.our_model->texture_id); // Free the textures
			our_actor.our_model->texture_id = load_bmp8_enhanced_actor(our_actor.our_model->body_parts, 255);	// Rebuild the actor's textures.
#endif	/* NEW_TEXTURES */
#ifdef NEW_EYES
		} else if(my>163 && my<174){
#else //NEW_EYES
		} else if(my>140 && my<151){
#endif //NEW_EYES
			our_actor.boots=dec(our_actor.def->boots, our_actor.boots, 1);

			// Copy the new boots texture name.
			my_strncp(our_actor.our_model->body_parts->boots_tex,actors_defs[our_actor.race].boots[our_actor.boots].boots_name,sizeof(our_actor.our_model->body_parts->boots_tex));

#ifdef	NEW_TEXTURES
			change_enhanced_actor(our_actor.our_model->texture_id, our_actor.our_model->body_parts);
#else	/* NEW_TEXTURES */
			glDeleteTextures(1,&our_actor.our_model->texture_id); // Free the textures
			our_actor.our_model->texture_id = load_bmp8_enhanced_actor(our_actor.our_model->body_parts, 255);	// Rebuild the actor's textures.
#endif	/* NEW_TEXTURES */
		}
	} else if(mx>364 && mx<379){
		if(my>25 && my<40){
			our_actor.head=inc(our_actor.def->head, our_actor.head, 1);

			// Detach the old head, and reattach and save the new one.
			model_detach_mesh(our_actor.our_model, our_actor.our_model->body_parts->head_meshindex);
			model_attach_mesh(our_actor.our_model, actors_defs[our_actor.race].head[our_actor.head].mesh_index);
			our_actor.our_model->body_parts->head_meshindex=actors_defs[our_actor.race].head[our_actor.head].mesh_index;
		} else if(my>48 && my<63){
			our_actor.skin=inc(our_actor.def->skin, our_actor.skin, 1);

			// Copy the skin texture names.
			my_strncp(our_actor.our_model->body_parts->hands_tex,actors_defs[our_actor.race].skin[our_actor.skin].hands_name,sizeof(our_actor.our_model->body_parts->hands_tex));
			my_strncp(our_actor.our_model->body_parts->head_tex,actors_defs[our_actor.race].skin[our_actor.skin].head_name,sizeof(our_actor.our_model->body_parts->head_tex));

#ifdef	NEW_TEXTURES
			change_enhanced_actor(our_actor.our_model->texture_id, our_actor.our_model->body_parts);
#else	/* NEW_TEXTURES */
			glDeleteTextures(1,&our_actor.our_model->texture_id); // Free the textures
			our_actor.our_model->texture_id = load_bmp8_enhanced_actor(our_actor.our_model->body_parts, 255);	// Rebuild the actor's textures.
#endif	/* NEW_TEXTURES */
		} else if(my>71 && my<86){
			our_actor.hair=inc(our_actor.def->hair, our_actor.hair, 1);

			// Copy the hair texture name.
			my_strncp(our_actor.our_model->body_parts->hair_tex,actors_defs[our_actor.race].hair[our_actor.hair].hair_name,sizeof(our_actor.our_model->body_parts->hair_tex));

#ifdef	NEW_TEXTURES
			change_enhanced_actor(our_actor.our_model->texture_id, our_actor.our_model->body_parts);
#else	/* NEW_TEXTURES */
			glDeleteTextures(1,&our_actor.our_model->texture_id); // Free the textures
			our_actor.our_model->texture_id = load_bmp8_enhanced_actor(our_actor.our_model->body_parts, 255);	// Rebuild the actor's textures.
#endif	/* NEW_TEXTURES */
		} else if(my>94 && my<109){
#ifdef NEW_EYES
			our_actor.eyes=inc(our_actor.def->eyes, our_actor.eyes, 1);

			// Copy the eyes texture name.
			my_strncp(our_actor.our_model->body_parts->eyes_tex,actors_defs[our_actor.race].eyes[our_actor.eyes].eyes_name,sizeof(our_actor.our_model->body_parts->eyes_tex));
#ifdef	NEW_TEXTURES
			change_enhanced_actor(our_actor.our_model->texture_id, our_actor.our_model->body_parts);
#else	/* NEW_TEXTURES */
			glDeleteTextures(1,&our_actor.our_model->texture_id); // Free the textures
			our_actor.our_model->texture_id = load_bmp8_enhanced_actor(our_actor.our_model->body_parts, 255);	// Rebuild the actor's textures.
#endif	/* NEW_TEXTURES */
		} else if(my>117 && my<132){
#endif //NEW_EYES
			our_actor.shirt=inc(our_actor.def->shirts, our_actor.shirt, 1);

			// Copy the shirt and arms texture names.
			my_strncp(our_actor.our_model->body_parts->arms_tex,actors_defs[our_actor.race].shirt[our_actor.shirt].arms_name,sizeof(our_actor.our_model->body_parts->arms_tex));
			my_strncp(our_actor.our_model->body_parts->torso_tex,actors_defs[our_actor.race].shirt[our_actor.shirt].torso_name,sizeof(our_actor.our_model->body_parts->torso_tex));

			// If we need a new mesh, drop the old one and load it.
			if(actors_defs[our_actor.race].shirt[our_actor.shirt].mesh_index != our_actor.our_model->body_parts->torso_meshindex)
			{
				model_detach_mesh(our_actor.our_model, our_actor.our_model->body_parts->torso_meshindex);
				model_attach_mesh(our_actor.our_model, actors_defs[our_actor.race].shirt[our_actor.shirt].mesh_index);
				our_actor.our_model->body_parts->torso_meshindex=actors_defs[our_actor.race].shirt[our_actor.shirt].mesh_index;
			}

#ifdef	NEW_TEXTURES
			change_enhanced_actor(our_actor.our_model->texture_id, our_actor.our_model->body_parts);
#else	/* NEW_TEXTURES */
			glDeleteTextures(1,&our_actor.our_model->texture_id); // Free the textures
			our_actor.our_model->texture_id = load_bmp8_enhanced_actor(our_actor.our_model->body_parts, 255);	// Rebuild the actor's textures.
#endif	/* NEW_TEXTURES */
#ifdef NEW_EYES
		} else if(my>140 && my<155){
#else //NEW_EYES
		} else if(my>117 && my<132){
#endif //NEW_EYES
			our_actor.pants=inc(our_actor.def->pants, our_actor.pants, 1);

			// Copy the pants texture name.
			my_strncp(our_actor.our_model->body_parts->pants_tex,actors_defs[our_actor.race].legs[our_actor.pants].legs_name,sizeof(our_actor.our_model->body_parts->pants_tex));

			// If we need a new mesh, drop the old one and load it.
			if(actors_defs[our_actor.race].legs[our_actor.pants].mesh_index != our_actor.our_model->body_parts->legs_meshindex)
			{
				model_detach_mesh(our_actor.our_model, our_actor.our_model->body_parts->legs_meshindex);
				model_attach_mesh(our_actor.our_model, actors_defs[our_actor.race].legs[our_actor.pants].mesh_index);
				our_actor.our_model->body_parts->legs_meshindex=actors_defs[our_actor.race].legs[our_actor.pants].mesh_index;
			}

#ifdef	NEW_TEXTURES
			change_enhanced_actor(our_actor.our_model->texture_id, our_actor.our_model->body_parts);
#else	/* NEW_TEXTURES */
			glDeleteTextures(1,&our_actor.our_model->texture_id); // Free the textures
			our_actor.our_model->texture_id = load_bmp8_enhanced_actor(our_actor.our_model->body_parts, 255);	// Rebuild the actor's textures.
#endif	/* NEW_TEXTURES */
#ifdef NEW_EYES
		} else if(my>163 && my<178){
#else //NEW_EYES
		} else if(my>140 && my<155){
#endif //NEW_EYES
			our_actor.boots=inc(our_actor.def->boots, our_actor.boots, 1);

			// Copy the new boots texture name.
			my_strncp(our_actor.our_model->body_parts->boots_tex,actors_defs[our_actor.race].boots[our_actor.boots].boots_name,sizeof(our_actor.our_model->body_parts->boots_tex));

#ifdef	NEW_TEXTURES
			change_enhanced_actor(our_actor.our_model->texture_id, our_actor.our_model->body_parts);
#else	/* NEW_TEXTURES */
			glDeleteTextures(1,&our_actor.our_model->texture_id); // Free the textures
			our_actor.our_model->texture_id = load_bmp8_enhanced_actor(our_actor.our_model->body_parts, 255);	// Rebuild the actor's textures.
#endif	/* NEW_TEXTURES */
		}
	}
	return 1;
}

void show_color_race_win(void)
{
	if(color_race_win < 0){
#ifdef NEW_EYES
		color_race_win = create_window (win_design, newchar_root_win, 0, 300, start_y, 420, 193, ELW_WIN_DEFAULT|ELW_CLICK_TRANSPARENT);
#else //NEW_EYES
		color_race_win = create_window (win_design, newchar_root_win, 0, 300, start_y, 420, 170, ELW_WIN_DEFAULT|ELW_CLICK_TRANSPARENT);
#endif //NEW_EYES
		set_window_handler (color_race_win, ELW_HANDLER_DISPLAY, &display_color_race_handler);
		set_window_handler (color_race_win, ELW_HANDLER_MOUSEOVER, &mouseover_color_race_handler);
		set_window_handler (color_race_win, ELW_HANDLER_CLICK, &click_color_race_handler);
    	} else {
        	show_window(color_race_win);
        	select_window(color_race_win);
    	}
}
#endif

#ifdef NEW_NEW_CHAR_WINDOW
int newchar_mouseover = 0; //book id if mouse is over a book 1 if mouse is over p2p race
int newchar_mouseover_time = 0;
char* tooltip;
int tooltip_x, tooltip_y;

int click_done_handler(widget_list *w, int mx, int my, Uint32 flags)
{
	if(w->window_id == color_race_win)
	{
		hide_window(color_race_win);
		show_window(namepass_win);
	}
	else
	{
#ifdef FR_VERSION
		/* Autant ne pas utiliser l'ancienne fonction create_character() qui
		 * effectue les controles et dispose de son propre syst�me de confirmation.
		 * Plus propre de tout refaire ici et de controler AVANT de demander
		 * la confirmation en recliquant sur [Suite]
		 */
		create_char_error_str[0] = '\0';
		if(!strncasecmp(inputs[1].str, actors_list[0]->actor_name, strlen(actors_list[0]->actor_name))){
			add_text_to_buffer(c_red2, error_bad_pass, 6000);
			return 0;
		} else if(strcmp(inputs[1].str, inputs[2].str)){
			add_text_to_buffer(c_red2, error_pass_no_match, 6000);
			return 0;
		} else if(inputs[0].pos<3){
			add_text_to_buffer(c_red2, error_username_length, 6000);
			return 0;
		} else if(inputs[1].pos<4){
			add_text_to_buffer(c_red2, error_password_length, 6000);
			return 0;
		}

		if(w->Flags)
		{
			creating_char=0;
#ifdef FR_VERSION
			send_new_char(inputs[0].str, inputs[1].str, our_actor.skin, our_actor.hair, our_actor.shirt, our_actor.pants, our_actor.boots, our_actor.head, our_actor.race, our_actor.scale);
#else //FR_VERSION
			send_new_char(inputs[0].str, inputs[1].str, our_actor.skin, our_actor.hair, our_actor.shirt, our_actor.pants, our_actor.boots, our_actor.head, our_actor.race);
#endif //FR_VERSION
		}
		else
		{
			w->Flags = BUTTON_ACTIVE;
			add_text_to_buffer(c_orange3, error_confirm_create_char, 6000);
		}
#else //FR_VERSION
		if(w->Flags)
		{
			create_character();
		}
		else
		{
			w->Flags = BUTTON_ACTIVE;
			add_text_to_buffer(c_orange3, error_confirm_create_char, 6000);
		}
#endif //FR_VERSION
	}
	return 1;
}

int click_back_handler(widget_list *w, int mx, int my, Uint32 flags)
{
	if(w->window_id == color_race_win)
	{
		destroy_all_actors();
		our_actor.our_model = NULL;
		use_windowed_chat = old_use_windowed_chat; //Restore use_windowed_chat
		hide_window(newchar_hud_win);
		hide_window(newchar_root_win);
		show_window(login_root_win);
		hide_hud_windows();
	}
	else
	{
		hide_window(namepass_win);
		show_window(color_race_win);
	}
	return 1;
}

int click_namepass_field(widget_list *w, int mx, int my, Uint32 flags)
{
	active = *(int*)w->spec;
	return 1;
}

int errorbox_draw(widget_list *w)
{
	draw_string_small(w->pos_x, w->pos_y, (unsigned char*)create_char_error_str, 8);
	return 1;
}

int name_draw(widget_list *w)
{
#ifdef FR_VERSION
	draw_smooth_button((char*)w->widget_info, w->size, w->pos_x, w->pos_y, w->len_x - 2*BUTTONRADIUS*w->size, 1, w->r, w->g, w->b, active == *(int*)w->spec, 0.16f, 0.12f, 0.08f, 0.0f);
#else //FR_VERSION
	draw_smooth_button((char*)w->widget_info, w->size, w->pos_x, w->pos_y, w->len_x - 2*BUTTONRADIUS*w->size, 1, w->r, w->g, w->b, active == *(int*)w->spec, 0.32f, 0.23f, 0.15f, 0.0f);
#endif //FR_VERSION
	return 1;
}

int password_draw(widget_list *w)
{
	if(!hidden)
	{
#ifdef FR_VERSION
		draw_smooth_button(get_pass_str(strlen((char*)w->widget_info)), w->size, w->pos_x, w->pos_y, w->len_x - 2*BUTTONRADIUS*w->size, 1, w->r, w->g, w->b, active == *(int*)w->spec, 0.16f, 0.12f, 0.08f, 0.0f);
#else //FR_VERSION
		draw_smooth_button(get_pass_str(strlen((char*)w->widget_info)), w->size, w->pos_x, w->pos_y, w->len_x - 2*BUTTONRADIUS*w->size, 1, w->r, w->g, w->b, active == *(int*)w->spec, 0.32f, 0.23f, 0.15f, 0.0f);
#endif //FR_VERSION
	}
	else
	{
#ifdef FR_VERSION
		draw_smooth_button((char*)w->widget_info, w->size, w->pos_x, w->pos_y, w->len_x - 2*BUTTONRADIUS*w->size, 1, w->r, w->g, w->b, active == *(int*)w->spec, 0.16f, 0.12f, 0.08f, 0.0f);
#else //FR_VERSION
		draw_smooth_button((char*)w->widget_info, w->size, w->pos_x, w->pos_y, w->len_x - 2*BUTTONRADIUS*w->size, 1, w->r, w->g, w->b, active == *(int*)w->spec, 0.32f, 0.23f, 0.15f, 0.0f);
#endif //FR_VERSION
	}
	return 1;
}

int keypress_namepass_handler (window_info *win, int mx, int my, Uint32 key, Uint32 unikey)
{
	Uint8 ch = key_to_char (unikey);
	int ret=0;
	struct input_text * t=&inputs[active];

#ifdef FR_VERSION
	// puisque le d�lai d'affichage des messages est d�sactiv� (cf display_namepass_handler)
	// alors on efface manuellement le message courant avant une nouvelle action utilisateur
	create_char_error_str[0] = '\0';
#endif //FR_VERSION
	if ((ret=check_character (active > 0, ch)))
	{
		if (ret==-1)
		{
			add_text_to_buffer (c_red1, error_max_digits, 6000);
		}
		else if (t->pos + 1 > MAX_USERNAME_LENGTH - 1)		// MAX_USERNAME_LENGTH includes the null terminator and t->pos counts from 0
		{
			add_text_to_buffer (c_red2, error_length, 6000);
		}
		else
		{
			// don't allow a character to start with player
			if(active == 0 && !strcasecmp(t->str, "player")) t->pos= 0;

			// add the character to the buffer
			char *p = t->str + t->pos++;
			*p++ = ch;
			*p = 0;
			ret=1;	//Reused to show that a letter has been added
		}
	}
	else if (unikey == SDLK_TAB || unikey == SDLK_RETURN)
	{
		active++;
		if(active>2) active=0;
	}
#ifdef FR_VERSION
#ifndef OSX
	else if (unikey == SDLK_BACKSPACE)
#else
	else if (key == SDLK_BACKSPACE)
#endif
	{
		// pour qu'un backspace superflu ne provoque pas l'erreur 'illegal character'
		if (t->pos > 0)
		{
			t->pos--;
			if (isdigit (t->str[t->pos])) numbers_in_name--;
			t->str[t->pos] = 0;
			ret = 1;	// Reused to show that a letter has been removed
		}
	}
#else //FR_VERSION
#ifndef OSX
	else if (unikey == SDLK_BACKSPACE && t->pos>0)
#else
	else if (key == SDLK_BACKSPACE && t->pos>0)
#endif
	{
		t->pos--;
		if (isdigit (t->str[t->pos])) numbers_in_name--;
		t->str[t->pos] = 0;
		ret = 1;	// Reused to show that a letter has been removed
	}
#endif //FR_VERSION
	else
	{
		//only send error messages on non-null characters
		if(ch != 0){
			add_text_to_buffer (c_red2, error_illegal_character, 6000);
		}
	}

	if(active>0){
		//Password/confirm
		if(ret){
			if(!strncasecmp(inputs[1].str, actors_list[0]->actor_name, strlen(actors_list[0]->actor_name))){
				add_text_to_buffer(c_red2, error_bad_pass, 6000);
			} else if(strcmp(inputs[1].str, inputs[2].str)){
				add_text_to_buffer(c_red2, error_pass_no_match, 6000);
			} else {
				add_text_to_buffer(c_green1, passwords_match, 6000);
			}
		}
	} else {
		safe_snprintf(actors_list[0]->actor_name, sizeof(actors_list[0]->actor_name), "%s", inputs[0].str);
	}

	return 1;
}

const struct WIDGET_TYPE name_type = {NULL, &name_draw, &click_namepass_field, NULL, NULL, NULL, NULL, NULL}; //custom widget for the name button
const struct WIDGET_TYPE password_type = {NULL, &password_draw, &click_namepass_field, NULL, NULL, NULL, NULL, NULL}; //custom widget for the password buttons
const struct WIDGET_TYPE errorbox_type = {NULL, &errorbox_draw, NULL, NULL, NULL, NULL, NULL, NULL}; //custom widget for displaying name/password errors
int specs[3] = {0, 1, 2};

int init_namepass_handler(window_info * win)
{
	float r = 0.77f, g = 0.57f, b = 0.39f; //widget color
	float very_small = DEFAULT_SMALL_RATIO; //font sizes
	float bit_small = 0.9f;
	float normal = 1.0f;
	int free_widget_id = 8;
	int widget_id;
	int y = 0, size;
	int center = 1;
#ifdef FR_VERSION
	// on adapte l'espacement vertical selon la taille de la fen�tre
	int sep = (window_height - HUD_MARGIN_Y - 280) / 25; // 280px + 25 sep
#else //FR_VERSION
	int sep = 6;
#endif //FR_VERSION

	//Choose your name and password
#ifdef FR_VERSION
	label_add_extended(win->window_id, free_widget_id++, 0, 10, y, 0, normal, 1.0f,0.5f,0.0f, "Nom du personnage :");
	y += 20 + sep;
	widget_id = widget_add(win->window_id, free_widget_id++, 0, win->len_x - 155, y, 145, 20, 0, very_small, r, g, b, &name_type, inputs[0].str, (void*)&specs[0]);
	y += 20 + sep;

	label_add_extended(win->window_id ,free_widget_id++, 0, 20, y, 0, very_small, 1.0f,0.8f,0.6f, "Il est conseill� de choisir");
	y+= DEFAULT_FONT_Y_LEN*very_small;
	label_add_extended(win->window_id ,free_widget_id++, 0, 20, y, 0, very_small, 1.0f,0.8f,0.6f, "un nom s'int�grant dans un");
	y+= DEFAULT_FONT_Y_LEN*very_small;
	label_add_extended(win->window_id ,free_widget_id++, 0, 20, y, 0, very_small, 1.0f,0.8f,0.6f, "monde m�di�val fantastique.");
	y+= DEFAULT_FONT_Y_LEN*very_small;

	y+= 8*sep;

	label_add_extended(win->window_id, free_widget_id++, 0, 10, y, 0, normal, 1.0f,0.5f,0.0f, "Mot de passe : ");
	y += 20 + sep;
	widget_id = widget_add(win->window_id, free_widget_id++, 0, win->len_x - 155, y, 145, 20, 0, very_small, r, g, b, &password_type, inputs[1].str, (void*)&specs[1]);
	y += 20 + 2*sep;
	label_add_extended(win->window_id, free_widget_id++, 0, 10, y, 0, very_small, 1.0f,0.5f,0.0f, "Confirmation : ");
	y += 18 + sep;
	widget_id = widget_add(win->window_id, free_widget_id++, 0, win->len_x - 155, y, 145, 20, 0, very_small, r, g, b, &password_type, inputs[2].str, (void*)&specs[2]);
	y += 20 + 2*sep;

	size = 15;
	label_add_extended(win->window_id, free_widget_id++, 0, win->len_x - 10 - size - 5 - strlen((char*)show_password)*DEFAULT_FONT_X_LEN*very_small, y + center, 0, very_small, r, g, b, (char*)show_password);
	widget_id = checkbox_add_extended(win->window_id, free_widget_id++, 0, win->len_x - 10 - size, y, size, size, 0, bit_small, r, g, b, &hidden);
	y += size + sep;

	label_add_extended(win->window_id ,free_widget_id++, 0, 20, y, 0, very_small, 1.0f,0.8f,0.6f, "(�vite de choisir un mot de");
	y+= DEFAULT_FONT_Y_LEN*very_small;
	label_add_extended(win->window_id ,free_widget_id++, 0, 20, y, 0, very_small, 1.0f,0.8f,0.6f, "passe trop facile � deviner)");
	y+= DEFAULT_FONT_Y_LEN*very_small;

	y+= sep;

	widget_add(win->window_id, free_widget_id++, 0, 35, y, 200, win->len_y - y, 0, very_small, r, g, b, &errorbox_type, NULL, NULL);

	//Done and Back buttons
	y = win->len_y - 30;
	widget_id = button_add_extended(win->window_id, free_widget_id++, 0, 30, y, 100, 20, 0, very_small, r, g, b, char_back);
	widget_set_OnClick(win->window_id, widget_id, &click_back_handler);
	widget_id = button_add_extended(win->window_id, free_widget_id++, 0, 140, y, 100, 20, 0, very_small, r, g, b, char_done);
	widget_set_OnClick(win->window_id, widget_id, &click_done_handler);

#else //FR_VERSION
	label_add_extended(win->window_id, free_widget_id++, 0, (win->len_x - strlen((char*)win_name_pass)*DEFAULT_FONT_X_LEN*bit_small)/2, y, 0, bit_small, r, g, b, (char*)win_name_pass);

	y += 15 + 2*sep;
	label_add_extended(win->window_id, free_widget_id++, 0, 15, y + center, 0, normal, r, g, b, (char*)login_username_str);
	widget_id = widget_add(win->window_id, free_widget_id++, 0, 115, y, 145, 20, 0, very_small, r, g, b, &name_type, inputs[0].str, (void*)&specs[0]);
	y += 10 + 20 + 2*sep;
	label_add_extended(win->window_id, free_widget_id++, 0, 15, y + center, 0, normal, r, g, b, (char*)login_password_str);
	widget_id = widget_add(win->window_id, free_widget_id++, 0, 115, y, 145, 20, 0, very_small, r, g, b, &password_type, inputs[1].str, (void*)&specs[1]);
	y += 20 + 2*sep;
	label_add_extended(win->window_id ,free_widget_id++, 0, 15, y + center, 0, normal, r, g, b, (char*)confirm_password);
	widget_id = widget_add(win->window_id, free_widget_id++, 0, 115, y, 145, 20, 0, very_small, r, g, b, &password_type, inputs[2].str, (void*)&specs[2]);
	y += 20 + 2*sep;
	size = 15;
	label_add_extended(win->window_id, free_widget_id++, 0, win->len_x - 20 - size - strlen((char*)show_password)*DEFAULT_FONT_X_LEN*bit_small, y, 0, bit_small, r, g, b, (char*)show_password);
	widget_id = checkbox_add_extended(win->window_id, free_widget_id++, 0, win->len_x - 10 - size, y, size, size, 0, bit_small, r, g, b, &hidden);
	y += 20 + sep;
	widget_add(win->window_id, free_widget_id++, 0, 35, y, 200, win->len_y - y, 0, very_small, r, g, b, &errorbox_type, NULL, NULL);
	//Done and Back buttons
	y = win->len_y -40;
	widget_id = button_add_extended(win->window_id, free_widget_id++, 0, 40, y, 80, 20, 0, very_small, r, g, b, char_done);
	widget_set_OnClick(win->window_id, widget_id, &click_done_handler);
	widget_id = button_add_extended(win->window_id, free_widget_id++, 0, 150, y, 80, 20, 0, very_small, r, g, b, char_back);
	widget_set_OnClick(win->window_id, widget_id, &click_back_handler);

#endif //FR_VERSION
	return 1;
}

int click_newchar_book_handler(widget_list *w, int mx, int my, Uint32 flags)
{
	if ( (flags & ELW_MOUSE_BUTTON) == 0) return 0;
	image_set_uv(w->window_id, w->id, (float)32/256,1.0f-(float)64/256,(float)63/256,1.0f-(float)95/256);
	toggle_book(w->id);
	newchar_mouseover = w->id;
	return 1;
}

int mouseover_newchar_book_handler(widget_list *w, int mx, int my)
{
	image_set_uv(w->window_id, w->id, (float)32/256,1.0f-(float)64/256,(float)63/256,1.0f-(float)95/256);
	newchar_mouseover = w->id;
#ifdef FR_VERSION
	if( newchar_mouseover == book_eldo) tooltip = about_eldo;
	else if( newchar_mouseover == book_haut_elfe) tooltip = about_haut_elfe;
	else if( newchar_mouseover == book_dwarf) tooltip = about_dwarfs;
	else if( newchar_mouseover == book_orchan) tooltip = about_orchans;
	else if( newchar_mouseover == book_gnome) tooltip = about_gnomes;
	else if( newchar_mouseover == book_sinan) tooltip = about_sinan;
	else if( newchar_mouseover == book_elfe_noir) tooltip = about_elfe_noir;
	else if( newchar_mouseover == book_draegoni) tooltip = about_draegoni;
	else if( newchar_mouseover == book_eldo) tooltip = about_eldo;
#else //FR_VERSION
	if( newchar_mouseover == book_human) tooltip = about_human;
	else if( newchar_mouseover == book_elf) tooltip = about_elves;
	else if( newchar_mouseover == book_dwarf) tooltip = about_dwarfs;
	else if( newchar_mouseover == book_gnome) tooltip = about_gnomes;
	else if( newchar_mouseover == book_orchan) tooltip = about_orchans;
	else if( newchar_mouseover == book_draegoni) tooltip = about_draegoni;
	else if( newchar_mouseover == book_human) tooltip = about_human;
#endif //FR_VERSION
	else return 1;
	newchar_mouseover_time = cur_time; //we are currently over something
	{
		int size = strlen(tooltip)*SMALL_FONT_X_LEN;
		tooltip_x = (mx + size <= 270) ? mx : 270 - size;
		tooltip_y = my + 20;
	}
	return 1;
}

int click_newchar_gender_handler(widget_list *w, int mx, int my, Uint32 flags)
{
	int i = multiselect_get_selected(w->window_id, w->id);
	switch(i)
	{
		case 0:
			change_race(our_actor.race_id + !our_actor.male);
		break;
		case 1:
			change_race(our_actor.race_id - our_actor.male);
	break;
	}
	return 1;
}

int mouseover_p2p_race_handler(widget_list *w, int mx, int my)
{
	int size = strlen(p2p_race)*SMALL_FONT_X_LEN;
	newchar_mouseover = 1; //we are over an p2p race
	newchar_mouseover_time = cur_time; //we are currently over something
	tooltip = p2p_race;
	tooltip_x = (mx + size <= 270) ? mx : 270 - size;
	tooltip_y = my + 20;
	return 1;
}

int click_newchar_race_handler(widget_list *w, int mx, int my, Uint32 flags)
{
	int i = multiselect_get_selected(w->window_id, w->id);
	switch(i)
	{
#ifdef FR_VERSION
        case 0:
            change_race(eldo_female + our_actor.male);
            break;
        case 1:
        change_race(haut_elfe_female + our_actor.male);
            break;
        case 2:
            change_race(dwarf_female + our_actor.male);
            break;
        case 3:
            change_race(orchan_female - 31 + our_actor.male);
            break;
        case 4:
            change_race(sinan_female - 99 + our_actor.male);
            break;
        case 5:
            change_race(elfe_noir_female - 99 + our_actor.male);
            break;
        case 6:
            change_race(gnome_female -31 + our_actor.male);
            break;
        case 7:
            change_race(draegoni_female - 31 + our_actor.male);
            break;
#else //FR_VERSION
		case 0:
			change_race(human_female + our_actor.male);
		break;
		case 1:
			change_race(elf_female + our_actor.male);
		break;
		case 2:
			change_race(dwarf_female + our_actor.male);
		break;
		case 3:
			change_race(gnome_female - 31 + our_actor.male);
		break;
		case 4:
			change_race(orchan_female - 31 + our_actor.male);
		break;
		case 5:
			change_race(draegoni_female - 31 + our_actor.male);
		break;
#endif //FR_VERSION
	}
#ifdef FR_VERSION
	recadrage_camera();
#endif //FR_VERSION
	return 0;
}

static void update_head(void)
{
	// Detach the old head, and reattach and save the new one.
	model_detach_mesh(our_actor.our_model,
		our_actor.our_model->body_parts->head_meshindex);
	model_attach_mesh(our_actor.our_model,
		actors_defs[our_actor.race].head[our_actor.head].mesh_index);
	our_actor.our_model->body_parts->head_meshindex =
		actors_defs[our_actor.race].head[our_actor.head].mesh_index;
}

static void update_skin(void)
{
	// Copy the skin texture names.
	my_strncp(our_actor.our_model->body_parts->hands_tex,
		actors_defs[our_actor.race].skin[our_actor.skin].hands_name,
		sizeof(our_actor.our_model->body_parts->hands_tex));
	my_strncp(our_actor.our_model->body_parts->head_tex,
		actors_defs[our_actor.race].skin[our_actor.skin].head_name,
		sizeof(our_actor.our_model->body_parts->head_tex));

#ifdef	NEW_TEXTURES
	change_enhanced_actor(our_actor.our_model->texture_id,
		our_actor.our_model->body_parts);
#else	/* NEW_TEXTURES */
	glDeleteTextures(1,&our_actor.our_model->texture_id); // Free the textures
	our_actor.our_model->texture_id = load_bmp8_enhanced_actor(our_actor.our_model->body_parts, 255);	// Rebuild the actor's textures.
#endif	/* NEW_TEXTURES */
}

static void update_hair(void)
{
	// Copy the hair texture name.
	my_strncp(our_actor.our_model->body_parts->hair_tex,
		actors_defs[our_actor.race].hair[our_actor.hair].hair_name,
		sizeof(our_actor.our_model->body_parts->hair_tex));

#ifdef	NEW_TEXTURES
	change_enhanced_actor(our_actor.our_model->texture_id,
		our_actor.our_model->body_parts);
#else	/* NEW_TEXTURES */
	glDeleteTextures(1,&our_actor.our_model->texture_id); // Free the textures
	our_actor.our_model->texture_id = load_bmp8_enhanced_actor(our_actor.our_model->body_parts, 255);	// Rebuild the actor's textures.
#endif	/* NEW_TEXTURES */
}

#ifdef NEW_EYES
static void update_eyes()
{
	// Copy the eyes texture name.
	my_strncp(our_actor.our_model->body_parts->eyes_tex,
		actors_defs[our_actor.race].eyes[our_actor.eyes].eyes_name,
		sizeof(our_actor.our_model->body_parts->eyes_tex));

#ifdef	NEW_TEXTURES
	change_enhanced_actor(our_actor.our_model->texture_id,
		our_actor.our_model->body_parts);
#else	/* NEW_TEXTURES */
	glDeleteTextures(1,&our_actor.our_model->texture_id); // Free the textures
	our_actor.our_model->texture_id = load_bmp8_enhanced_actor(our_actor.our_model->body_parts, 255);	// Rebuild the actor's textures.
#endif	/* NEW_TEXTURES */
}
#endif	/* NEW_EYES */

static void update_shirt()
{
	// Copy the shirt and arms texture names.
	my_strncp(our_actor.our_model->body_parts->arms_tex,
		actors_defs[our_actor.race].shirt[our_actor.shirt].arms_name,
		sizeof(our_actor.our_model->body_parts->arms_tex));
	my_strncp(our_actor.our_model->body_parts->torso_tex,
		actors_defs[our_actor.race].shirt[our_actor.shirt].torso_name,
		sizeof(our_actor.our_model->body_parts->torso_tex));

	// If we need a new mesh, drop the old one and load it.
	if (actors_defs[our_actor.race].shirt[our_actor.shirt].mesh_index !=
		our_actor.our_model->body_parts->torso_meshindex)
	{
		model_detach_mesh(our_actor.our_model,
			our_actor.our_model->body_parts->torso_meshindex);
		model_attach_mesh(our_actor.our_model,
			actors_defs[our_actor.race].shirt[our_actor.shirt].mesh_index);
		our_actor.our_model->body_parts->torso_meshindex =
			actors_defs[our_actor.race].shirt[our_actor.shirt].mesh_index;
	}

#ifdef	NEW_TEXTURES
	change_enhanced_actor(our_actor.our_model->texture_id, our_actor.our_model->body_parts);
#else	/* NEW_TEXTURES */
	glDeleteTextures(1,&our_actor.our_model->texture_id); // Free the textures
	our_actor.our_model->texture_id = load_bmp8_enhanced_actor(our_actor.our_model->body_parts, 255);	// Rebuild the actor's textures.
#endif	/* NEW_TEXTURES */
}

static void update_pants(void)
{
	// Copy the pants texture name.
	my_strncp(our_actor.our_model->body_parts->pants_tex,
		actors_defs[our_actor.race].legs[our_actor.pants].legs_name,
		sizeof(our_actor.our_model->body_parts->pants_tex));

	// If we need a new mesh, drop the old one and load it.
	if (actors_defs[our_actor.race].legs[our_actor.pants].mesh_index !=
		our_actor.our_model->body_parts->legs_meshindex)
	{
		model_detach_mesh(our_actor.our_model,
			our_actor.our_model->body_parts->legs_meshindex);
		model_attach_mesh(our_actor.our_model,
			actors_defs[our_actor.race].legs[our_actor.pants].mesh_index);
		our_actor.our_model->body_parts->legs_meshindex =
			actors_defs[our_actor.race].legs[our_actor.pants].mesh_index;
	}

#ifdef	NEW_TEXTURES
	change_enhanced_actor(our_actor.our_model->texture_id,
		our_actor.our_model->body_parts);
#else	/* NEW_TEXTURES */
	glDeleteTextures(1,&our_actor.our_model->texture_id); // Free the textures
	our_actor.our_model->texture_id = load_bmp8_enhanced_actor(our_actor.our_model->body_parts, 255);	// Rebuild the actor's textures.
#endif	/* NEW_TEXTURES */
}

static void update_boots(void)
{
	// Copy the new boots texture name.
	my_strncp(our_actor.our_model->body_parts->boots_tex,
		actors_defs[our_actor.race].boots[our_actor.boots].boots_name,
		sizeof(our_actor.our_model->body_parts->boots_tex));

	// If we need a new mesh, drop the old one and load it.
	if (actors_defs[our_actor.race].boots[our_actor.boots].mesh_index !=
		our_actor.our_model->body_parts->boots_meshindex)
	{
		model_detach_mesh(our_actor.our_model,
			our_actor.our_model->body_parts->boots_meshindex);
		model_attach_mesh(our_actor.our_model,
			actors_defs[our_actor.race].boots[our_actor.boots].mesh_index);
		our_actor.our_model->body_parts->boots_meshindex =
			actors_defs[our_actor.race].boots[our_actor.boots].mesh_index;
	}

#ifdef	NEW_TEXTURES
	change_enhanced_actor(our_actor.our_model->texture_id,
		our_actor.our_model->body_parts);
#else	/* NEW_TEXTURES */
	glDeleteTextures(1,&our_actor.our_model->texture_id); // Free the textures
	our_actor.our_model->texture_id = load_bmp8_enhanced_actor(our_actor.our_model->body_parts, 255);	// Rebuild the actor's textures.
#endif	/* NEW_TEXTURES */
}

#ifdef FR_VERSION
static void update_scales()
{
	our_actor.our_model->scale = 0.95f + our_actor.scale*0.01f;
#ifdef	NEW_TEXTURES
	change_enhanced_actor(our_actor.our_model->texture_id,
		our_actor.our_model->body_parts);
#else	/* NEW_TEXTURES */
	glDeleteTextures(1,&our_actor.our_model->texture_id); // Free the textures
	our_actor.our_model->texture_id = load_bmp8_enhanced_actor(our_actor.our_model->body_parts, 255);	// Rebuild the actor's textures.
#endif	/* NEW_TEXTURES */
}
#endif //FR_VERSION

int head_dec_handler(widget_list *w, int mx, int my, Uint32 flags)
{
	our_actor.head = dec(our_actor.def->head, our_actor.head, 1);

	update_head();

	return 1;
}

int head_inc_handler(widget_list *w, int mx, int my, Uint32 flags)
{
	our_actor.head = inc(our_actor.def->head, our_actor.head, 1);

	update_head();

	return 1;
}

int skin_dec_handler(widget_list *w, int mx, int my, Uint32 flags)
{
	our_actor.skin = dec(our_actor.def->skin, our_actor.skin, 1);

	update_skin();

	return 1;
}

int skin_inc_handler(widget_list *w, int mx, int my, Uint32 flags)
{
	our_actor.skin = inc(our_actor.def->skin, our_actor.skin, 1);

	update_skin();

	return 1;
}

int hair_dec_handler(widget_list *w, int mx, int my, Uint32 flags)
{
	our_actor.hair = dec(our_actor.def->hair, our_actor.hair, 1);

	update_hair();

	return 1;
}

int hair_inc_handler(widget_list *w, int mx, int my, Uint32 flags)
{
	our_actor.hair = inc(our_actor.def->hair, our_actor.hair, 1);

	update_hair();

	return 1;
}

#ifdef NEW_EYES
int eyes_dec_handler(widget_list *w, int mx, int my, Uint32 flags)
{
	our_actor.eyes = dec(our_actor.def->eyes, our_actor.eyes, 1);
#ifdef NEW_EYES
	update_eyes();
#endif
	return 1;
}

int eyes_inc_handler(widget_list *w, int mx, int my, Uint32 flags)
{
	our_actor.eyes = inc(our_actor.def->eyes, our_actor.eyes, 1);
#ifdef NEW_EYES
	update_eyes();
#endif
	return 1;
}
#endif //NEW_EYES

int shirt_dec_handler(widget_list *w, int mx, int my, Uint32 flags)
{
	our_actor.shirt = dec(our_actor.def->shirts, our_actor.shirt, 1);

	update_shirt();

	return 1;
}

int shirt_inc_handler(widget_list *w, int mx, int my, Uint32 flags)
{
	our_actor.shirt = inc(our_actor.def->shirts, our_actor.shirt, 1);

	update_shirt();

	return 1;
}

int pants_dec_handler(widget_list *w, int mx, int my, Uint32 flags)
{
	our_actor.pants=dec(our_actor.def->pants, our_actor.pants, 1);

	update_pants();

	return 1;
}

int pants_inc_handler(widget_list *w, int mx, int my, Uint32 flags)
{
	our_actor.pants=inc(our_actor.def->pants, our_actor.pants, 1);

	update_pants();

	return 1;
}

int boots_dec_handler(widget_list *w, int mx, int my, Uint32 flags)
{
	our_actor.boots = dec(our_actor.def->boots, our_actor.boots, 1);

	update_boots();

	return 1;
}

int boots_inc_handler(widget_list *w, int mx, int my, Uint32 flags)
{
	our_actor.boots = inc(our_actor.def->boots, our_actor.boots, 1);

	update_boots();

	return 1;
}

#ifdef FR_VERSION
int scale_dec_handler(widget_list *w, int mx, int my, Uint32 flags)
{
	our_actor.scale = dec(our_actor.def->scale, our_actor.scale, 1);

	update_scales();

	return 1;
}

int scale_inc_handler(widget_list *w, int mx, int my, Uint32 flags)
{
	our_actor.scale = inc(our_actor.def->scale, our_actor.scale, 1);

	update_scales();

	return 1;
}
#endif //FR_VERSION

int mouseover_color_race_handler(window_info *win, int mx, int my)
{
#ifdef FR_VERSION
	if(!(newchar_mouseover == book_eldo))      image_set_uv(win->window_id, book_eldo,      (float)0/256,1.0f-(float)64/256,(float)31/256,1.0f-(float)95/256);
	if(!(newchar_mouseover == book_haut_elfe)) image_set_uv(win->window_id, book_haut_elfe, (float)0/256,1.0f-(float)64/256,(float)31/256,1.0f-(float)95/256);
	if(!(newchar_mouseover == book_dwarf))     image_set_uv(win->window_id, book_dwarf,     (float)0/256,1.0f-(float)64/256,(float)31/256,1.0f-(float)95/256);
	if(!(newchar_mouseover == book_orchan))    image_set_uv(win->window_id, book_orchan,    (float)0/256,1.0f-(float)64/256,(float)31/256,1.0f-(float)95/256);
	if(!(newchar_mouseover == book_sinan))     image_set_uv(win->window_id, book_sinan,     (float)0/256,1.0f-(float)64/256,(float)31/256,1.0f-(float)95/256);
	if(!(newchar_mouseover == book_elfe_noir)) image_set_uv(win->window_id, book_elfe_noir, (float)0/256,1.0f-(float)64/256,(float)31/256,1.0f-(float)95/256);
	if(!(newchar_mouseover == book_gnome))     image_set_uv(win->window_id, book_gnome,     (float)0/256,1.0f-(float)64/256,(float)31/256,1.0f-(float)95/256);
	if(!(newchar_mouseover == book_draegoni))  image_set_uv(win->window_id, book_draegoni,  (float)0/256,1.0f-(float)64/256,(float)31/256,1.0f-(float)95/256);
#else //FR_VERSION
	if(!(newchar_mouseover == book_human)) image_set_uv(win->window_id, book_human, (float)0/256,1.0f-(float)64/256,(float)31/256,1.0f-(float)95/256);
	if(!(newchar_mouseover == book_elf)) image_set_uv(win->window_id, book_elf, (float)0/256,1.0f-(float)64/256,(float)31/256,1.0f-(float)95/256);
	if(!(newchar_mouseover == book_dwarf)) image_set_uv(win->window_id, book_dwarf, (float)0/256,1.0f-(float)64/256,(float)31/256,1.0f-(float)95/256);
	if(!(newchar_mouseover == book_gnome)) image_set_uv(win->window_id, book_gnome, (float)0/256,1.0f-(float)64/256,(float)31/256,1.0f-(float)95/256);
	if(!(newchar_mouseover == book_orchan)) image_set_uv(win->window_id, book_orchan, (float)0/256,1.0f-(float)64/256,(float)31/256,1.0f-(float)95/256);
	if(!(newchar_mouseover == book_draegoni)) image_set_uv(win->window_id, book_draegoni, (float)0/256,1.0f-(float)64/256,(float)31/256,1.0f-(float)95/256);
#endif //FR_VERSION
	if(!newchar_mouseover)
		newchar_mouseover_time = 0; //reset timer
	newchar_mouseover = 0;
	return 1;
}

int box_draw(widget_list *w)
{
	glColor3f(w->r, w->g, w->b); //draw a box
	draw_box(w->widget_info, w->pos_x, w->pos_y, w->len_x, w->len_y, 0);
#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
	return 1;
}

const struct WIDGET_TYPE box_type = {NULL, &box_draw, NULL, NULL, NULL, NULL, NULL, NULL}; //a custom box widget

int init_color_race_handler(window_info * win)
{
	float r = 0.77f, g = 0.57f, b = 0.39f; //widget color
	float rh = 0.32f, gh = 0.23f, bh = 0.15f; //highlighted color
	float very_small = DEFAULT_SMALL_RATIO; //font sizes
	float bit_small = 0.9f;
	float normal = 1.0f;
	int free_widget_id = 8; //next free widget id
	int widget_id;
	int i, x, y = 20, size;
	int sep = 5;
#ifdef FR_VERSION
	char* body_part_strs[7] = {head_str, skin_str, hair_str, shirt_str, pants_str, boots_str, "Taille"};
	void* body_handlers_dec[7] = {&head_dec_handler, &skin_dec_handler, &hair_dec_handler, &shirt_dec_handler, &pants_dec_handler, &boots_dec_handler, &scale_dec_handler};
	void* body_handlers_inc[7] = {&head_inc_handler, &skin_inc_handler, &hair_inc_handler, &shirt_inc_handler, &pants_inc_handler, &boots_inc_handler, &scale_inc_handler};
#else //FR_VERSION
	char* body_part_strs[7] = {head_str, skin_str, hair_str, eyes_str, shirt_str, pants_str, boots_str};
	void* body_handlers_dec[7] = {&head_dec_handler, &skin_dec_handler, &hair_dec_handler, &eyes_dec_handler, &shirt_dec_handler, &pants_dec_handler, &boots_dec_handler};
	void* body_handlers_inc[7] = {&head_inc_handler, &skin_inc_handler, &hair_inc_handler, &eyes_inc_handler, &shirt_inc_handler, &pants_inc_handler, &boots_inc_handler};
#endif //FR_VERSION
#ifdef FR_VERSION
	int book_ids[8] = {book_eldo, book_haut_elfe, book_dwarf, book_orchan, book_sinan, book_elfe_noir, book_gnome, book_draegoni};
#else //FR_VERSION
	int book_ids[6] = {book_human, book_elf, book_dwarf, book_gnome, book_orchan, book_draegoni};
#endif //FR_VERSION

#ifdef FR_VERSION
	// on adapte l'espacement vertical selon la taille de la fen�tre
	sep = (window_height - HUD_MARGIN_Y - 228) / 30; // 228px + 30 sep
	y = sep;
#endif //FR_VERSION

	//Design your character
#ifdef FR_VERSION
	label_add_extended(win->window_id, free_widget_id++, 0, (win->len_x - strlen((char*)win_design)*DEFAULT_FONT_X_LEN*bit_small)/2, 0, 0, bit_small, 1.0f,0.5f,0.0f, (char*)win_design);
#else //FR_VERSION
	label_add_extended(win->window_id, free_widget_id++, 0, (win->len_x - strlen((char*)win_design)*DEFAULT_FONT_X_LEN*bit_small)/2, 0, 0, bit_small, r, g, b, (char*)win_design);
#endif //FR_VERSION

	//Gender selection
#ifdef FR_VERSION
	y += DEFAULT_FONT_Y_LEN*bit_small + 2*sep;
#else //FR_VERSION
	y = DEFAULT_FONT_Y_LEN*bit_small + 8 + 2;
#endif //FR_VERSION
	widget_add(win->window_id, free_widget_id++, 0, 10, y, win->len_x - 20, 20 + 4*sep, 0, normal, r, g, b, &box_type, gender_str, NULL);
	y += 2*sep;
	widget_id = multiselect_add_extended(win->window_id, free_widget_id++, 0, 20 , y, win->len_x - 40, 25, normal, r, g, b, rh, gh, bh, 2);
	multiselect_button_add_extended(win->window_id, widget_id, 0, 0, 100, male_str, very_small, our_actor.male);
	multiselect_button_add_extended(win->window_id, widget_id, win->len_x - 140, 0, 100, female_str, very_small, !our_actor.male);
	widget_set_OnClick(win->window_id, widget_id, &click_newchar_gender_handler);

	//Races
#ifdef FR_VERSION
	y += 20 + 5*sep;
	widget_add(win->window_id, free_widget_id++, 0, 10, y, win->len_x - 20, 4*20 + 7*sep, 0, normal, r, g, b, &box_type, race_str, NULL);
#else //FR_VERSION
	y += 20 + 2*sep + 8;
	widget_add(win->window_id, free_widget_id++, 0, 10, y, win->len_x - 20, 3*22 + 8 + 6*sep, 0, normal, r, g, b, &box_type, race_str, NULL);
#endif //FR_VERSION
#ifdef ENGLISH
	widget_id = widget_add(win->window_id, free_widget_id++, 0, 136, y + 8, 120, 66 + 5*sep, 0, normal, 1.0f, 0.0f, 0.0f, &box_type, "P2P", NULL);
	widget_set_OnMouseover(win->window_id, widget_id, &mouseover_p2p_race_handler);
#endif //ENGLISH
#ifdef FR_VERSION
	y += 2*sep;
	widget_id = multiselect_add_extended(win->window_id, free_widget_id++,   0, 20, y, win->len_x-40 , 60+3*sep, normal, r, g, b, rh, gh, bh, 6);
	multiselect_button_add_extended(win->window_id, widget_id,   0,          0, 90, eldo_str,      very_small, our_actor.race == eldo_female || our_actor.race == eldo_male);
	multiselect_button_add_extended(win->window_id, widget_id,   0, 20 +   sep, 90, haut_elfe_str, very_small, our_actor.race == haut_elfe_female || our_actor.race == haut_elfe_male);
	multiselect_button_add_extended(win->window_id, widget_id,   0, 40 + 2*sep, 90, dwarf_str,     very_small, our_actor.race == dwarf_female || our_actor.race == dwarf_male);
	multiselect_button_add_extended(win->window_id, widget_id,   0, 60 + 3*sep, 90, orchan_str,    very_small, our_actor.race == orchan_female || our_actor.race == orchan_male);
	multiselect_button_add_extended(win->window_id, widget_id, 120,          0, 90, sinan_str,     very_small, our_actor.race == sinan_female || our_actor.race == sinan_male);
	multiselect_button_add_extended(win->window_id, widget_id, 120, 20 +   sep, 90, elfe_noir_str, very_small, our_actor.race == elfe_noir_female || our_actor.race == elfe_noir_male);
	multiselect_button_add_extended(win->window_id, widget_id, 120, 40 + 2*sep, 90, gnome_str,     very_small, our_actor.race == gnome_female || our_actor.race == gnome_male);
	multiselect_button_add_extended(win->window_id, widget_id, 120, 60 + 3*sep, 90, draegoni_str,  very_small, our_actor.race == draegoni_female || our_actor.race == draegoni_male);
#else //FR_VERSION
	y += 2*sep + 8;
	widget_id = multiselect_add_extended(win->window_id, free_widget_id++, 0, 20, y, win->len_x-40 , 100, normal, r, g, b, rh, gh, bh, 6);
	multiselect_button_add_extended(win->window_id, widget_id, 0, 0, 80, human_str, very_small, our_actor.race==human_female||our_actor.race==human_male);
	multiselect_button_add_extended(win->window_id, widget_id, 0, 22 + sep, 80, elf_str, very_small, our_actor.race==elf_female||our_actor.race==elf_male);
	multiselect_button_add_extended(win->window_id, widget_id, 0, 44 + 2*sep, 80, dwarf_str, very_small, our_actor.race==dwarf_female||our_actor.race==dwarf_male);
	multiselect_button_add_extended(win->window_id, widget_id, 120, 0, 80, gnome_str, very_small, our_actor.race==dwarf_female||our_actor.race==dwarf_male);
	multiselect_button_add_extended(win->window_id, widget_id, 120, 22 + sep, 80, orchan_str, very_small, our_actor.race==orchan_female||our_actor.race==orchan_male);
	multiselect_button_add_extended(win->window_id, widget_id, 120, 44 + 2*sep, 80, draegoni_str, very_small, our_actor.race==draegoni_female||our_actor.race==draegoni_male);
#endif //FR_VERSION
	widget_set_OnClick(win->window_id, widget_id, &click_newchar_race_handler);

	y--;
#ifdef FR_VERSION
	for(i = 0; i < 4; i++)
	{
		widget_id = image_add_extended(win->window_id, book_ids[i], 0, 110, y + (20 + sep)*i, 22, 22, 0, 1.0f, 1.0f, 1.0f, 1.0f, icons_text, (float)0/256,1.0f-(float)64/256,(float)31/256,1.0f-(float)95/256, -1);
		widget_set_OnClick(win->window_id, widget_id, &click_newchar_book_handler);
		widget_set_OnMouseover(win->window_id, widget_id, &mouseover_newchar_book_handler);
	}
	for(i = 0; i< 4; i++)
	{
		widget_id = image_add_extended(win->window_id, book_ids[i+4], 0, 230, y + (20 + sep)*i, 22, 22, 0, 1.0f, 1.0f, 1.0f, 1.0f, icons_text, (float)0/256,1.0f-(float)64/256,(float)31/256,1.0f-(float)95/256, -1);
		widget_set_OnClick(win->window_id, widget_id, &click_newchar_book_handler);
		widget_set_OnMouseover(win->window_id, widget_id, &mouseover_newchar_book_handler);
	}
#else //FR_VERSION
	for(i = 0; i < 3; i++)
	{
		widget_id = image_add_extended(win->window_id, book_ids[i], 0, 110, y + (22 + sep)*i, 22, 22, 0, 1.0f, 1.0f, 1.0f, 1.0f, icons_text, (float)0/256,1.0f-(float)64/256,(float)31/256,1.0f-(float)95/256, -1);
		widget_set_OnClick(win->window_id, widget_id, &click_newchar_book_handler);
		widget_set_OnMouseover(win->window_id, widget_id, &mouseover_newchar_book_handler);
	}
	for(i = 0; i< 3; i++)
	{
		widget_id = image_add_extended(win->window_id, book_ids[i+3], 0, 230, y + (22 + sep)*i, 22, 22, 0, 1.0f, 1.0f, 1.0f, 1.0f, icons_text, (float)0/256,1.0f-(float)64/256,(float)31/256,1.0f-(float)95/256, -1);
		widget_set_OnClick(win->window_id, widget_id, &click_newchar_book_handler);
		widget_set_OnMouseover(win->window_id, widget_id, &mouseover_newchar_book_handler);
	}
#endif //FR_VERSION
	y++;
#ifdef FR_VERSION
	y += 4*20 + 6*sep;
	label_add_extended(win->window_id ,free_widget_id++, 0, 15, y, 0, very_small, 1.0f,0.8f,0.6f, "Clique sur les icones � droite");
	y+= DEFAULT_FONT_Y_LEN*very_small;
	label_add_extended(win->window_id ,free_widget_id++, 0, 15, y, 0, very_small, 1.0f,0.8f,0.6f, "de chaque choix pour d�couvrir");
	y+= DEFAULT_FONT_Y_LEN*very_small;
	label_add_extended(win->window_id ,free_widget_id++, 0, 15, y, 0, very_small, 1.0f,0.8f,0.6f, "les particularit�s des peuples");
	y+= DEFAULT_FONT_Y_LEN*very_small + 3*sep;
#endif //FR_VERSION

	//Appereance
	size = 2*DEFAULT_FONT_X_LEN*bit_small;
#ifndef FR_VERSION
	y += 3*22 + 8 + 4*sep;
#endif //FR_VERSION
#ifdef FR_VERSION
	widget_add(win->window_id, free_widget_id++, 0, 10, y, win->len_x - 20, 3*DEFAULT_FONT_Y_LEN*bit_small + 7*sep, 0, normal, r, g, b, &box_type, appearance_str, NULL);
#else //FR_VERSION
	widget_add(win->window_id, free_widget_id++, 0, 10, y, win->len_x - 20, 4*DEFAULT_FONT_Y_LEN*bit_small + 6*sep, 0, normal, r, g, b, &box_type, appearance_str, NULL);
#endif //FR_VERSION
	y += 2*sep;
#ifdef NEW_EYES
	for(i = 0; i < 4; i++)//Head, Skin, Hair and Eyes
#else //NEW_EYES
	for(i = 0; i < 3; i++)//Head, Skin and Hair
#endif //NEW_EYES
	{
		widget_id = label_add_extended(win->window_id, free_widget_id++, 0, 20, y+(DEFAULT_FONT_Y_LEN*bit_small+sep)*i, 0, bit_small, r, g, b, "<<");
		widget_set_OnClick(win->window_id, widget_id, body_handlers_dec[i]);
#ifdef FR_VERSION
		x = 20 + size + (win->len_x/2 - 30 - 2*size - strlen((char*)body_part_strs[i])*DEFAULT_FONT_X_LEN*very_small)/2;
		label_add_extended(win->window_id, free_widget_id++, 0, x, y+(DEFAULT_FONT_Y_LEN*bit_small+sep)*i+(bit_small-very_small), 0, very_small, 1.0f,0.8f,0.6f, (char*)body_part_strs[i]);
#else //FR_VERSION
		x = 20 + size + (win->len_x/2 - 30 - 2*size - strlen((char*)body_part_strs[i])*DEFAULT_FONT_X_LEN*bit_small)/2;
		label_add_extended(win->window_id, free_widget_id++, 0, x, y+(DEFAULT_FONT_Y_LEN*bit_small+sep)*i, 0, bit_small, r, g, b, (char*)body_part_strs[i]);
#endif //FR_VERSION
		widget_id = label_add_extended(win->window_id, free_widget_id++, 0, win->len_x/2-10-size, y+(DEFAULT_FONT_Y_LEN*bit_small+sep)*i, 0, bit_small, r, g, b, ">>");
		widget_set_OnClick(win->window_id, widget_id, body_handlers_inc[i]);
	}
#ifdef NEW_EYES
	for(i = 4; i < 7; i++)//Shirt, Pants and Boots
#else //NEW_EYES
	for(i = 3; i < 6; i++)//Shirt, Pants and Boots
#endif //NEW_EYES
	{
#ifdef NEW_EYES
		widget_id = label_add_extended(win->window_id, free_widget_id++, 0, win->len_x/2+10, y+(DEFAULT_FONT_Y_LEN*bit_small+sep)*(i-4), 0, bit_small, r, g, b, "<<");
#else //NEW_EYES
		widget_id = label_add_extended(win->window_id, free_widget_id++, 0, win->len_x/2+10, y+(DEFAULT_FONT_Y_LEN*bit_small+sep)*(i-3), 0, bit_small, r, g, b, "<<");
#endif //NEW_EYES
		widget_set_OnClick(win->window_id, widget_id, body_handlers_dec[i]);
#ifdef FR_VERSION
		x = win->len_x/2 + 10 + size + (win->len_x/2 - 30 - 2*size - strlen((char*)body_part_strs[i])*DEFAULT_FONT_X_LEN*very_small)/2;
		label_add_extended(win->window_id, free_widget_id++, 0, x, y+(DEFAULT_FONT_Y_LEN*bit_small+sep)*(i-3)+(bit_small-very_small), 0, very_small, 1.0f,0.8f,0.6f, (char*)body_part_strs[i]);
		widget_id = label_add_extended(win->window_id, free_widget_id++, 0, win->len_x-20-size, y+(DEFAULT_FONT_Y_LEN*bit_small+sep)*(i-3), 0, bit_small, r, g, b, ">>");
#else //FR_VERSION
		x = win->len_x/2 + 10 + size + (win->len_x/2 - 30 - 2*size - strlen((char*)body_part_strs[i])*DEFAULT_FONT_X_LEN*bit_small)/2;
		label_add_extended(win->window_id, free_widget_id++, 0, x, y+(DEFAULT_FONT_Y_LEN*bit_small+sep)*(i-4), 0, bit_small, r, g, b, (char*)body_part_strs[i]);
		widget_id = label_add_extended(win->window_id, free_widget_id++, 0, win->len_x-20-size, y+(DEFAULT_FONT_Y_LEN*bit_small+sep)*(i-4), 0, bit_small, r, g, b, ">>");
#endif //FR_VERSION
		widget_set_OnClick(win->window_id, widget_id, body_handlers_inc[i]);
	}
#ifdef FR_VERSION
	// Taille du personnage
	i = 6;

	widget_id = label_add_extended(win->window_id, free_widget_id++, 0, 80, y+(DEFAULT_FONT_Y_LEN*bit_small+sep)*(i-3), 0, bit_small, r, g, b, "<<");
	widget_set_OnClick(win->window_id, widget_id, body_handlers_dec[i]);

	x = 77 + size + (win->len_x/2 - 30 - 2*size - strlen((char*)body_part_strs[i])*DEFAULT_FONT_X_LEN*very_small)/2;
	label_add_extended(win->window_id, free_widget_id++, 0, x, y+(DEFAULT_FONT_Y_LEN*bit_small+sep)*(i-3)+(bit_small-very_small), 0, very_small, 1.0f,0.8f,0.6f, (char*)body_part_strs[i]);

	widget_id = label_add_extended(win->window_id, free_widget_id++, 0, 160, y+(DEFAULT_FONT_Y_LEN*bit_small+sep)*(i-3), 0, bit_small, r, g, b, ">>");
	widget_set_OnClick(win->window_id, widget_id, body_handlers_inc[i]);
#endif //FR_VERSION
#ifdef FR_VERSION
	y = win->len_y - 30;

	//Done and Back
	widget_id = button_add_extended(win->window_id, free_widget_id++, 0, 30, y, 100, 20, 0, very_small, r, g, b, char_back);
	widget_set_OnClick(win->window_id, widget_id, &click_back_handler);
	widget_id = button_add_extended(win->window_id, free_widget_id++, 0, 140, y, 100, 20, 0, very_small, r, g, b, char_done);
	widget_set_OnClick(win->window_id, widget_id, &click_done_handler);
#else //FR_VERSION
	y = win->len_y -40;

	//Done and Back
	widget_id = button_add_extended(win->window_id, free_widget_id++, 0, 40, y, 80, 20, 0, very_small, r, g, b, char_done);
	widget_set_OnClick(win->window_id, widget_id, &click_done_handler);
	widget_id = button_add_extended(win->window_id, free_widget_id++, 0, 150, y, 80, 20, 0, very_small, r, g, b, char_back);
	widget_set_OnClick(win->window_id, widget_id, &click_back_handler);
#endif //FR_VERSION
	return 1;
}

int tooltip_win;

int display_tooltip_handler(window_info * win)
{
	if(newchar_mouseover_time == cur_time) //draw a help text if currently over something
		show_help(tooltip, tooltip_x, tooltip_y);
	return 1;
}

int display_newchar_hud_handler(window_info * win)
{
	glColor3f(0.0f, 0.0f, 0.0f); //Draw a black background
	glBegin(GL_QUADS);
	glVertex2i(0, 0);
	glVertex2i(0, win->len_y);
	glVertex2i(win->len_x, win->len_y);
	glVertex2i(win->len_x, 0);
	glEnd();
	glColor3f(0.77f, 0.57f, 0.39f);
#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
	return 1;
}

void create_newchar_hud_window(void)
{
	if(newchar_hud_win != -1) return;
	newchar_hud_win = create_window("Newchar_Hud", newchar_root_win, 0, window_width - 270, 0, 270, window_height - hud_y, ELW_USE_BORDER|ELW_SHOW|ELW_SHOW_LAST);
	set_window_handler(newchar_hud_win, ELW_HANDLER_DISPLAY, &display_newchar_hud_handler);
	tooltip_win = create_window("Help Text", newchar_hud_win, 0, 0, 0, 0, 0, ELW_SHOW);
	set_window_handler(tooltip_win, ELW_HANDLER_DISPLAY, &display_tooltip_handler);
	color_race_win = create_window(win_design, newchar_hud_win, 0, 0, 10, 270, window_height - hud_y - 10, ELW_SHOW|ELW_SHOW_LAST); //Design your character
	set_window_handler(color_race_win, ELW_HANDLER_INIT, &init_color_race_handler);
	set_window_handler(color_race_win, ELW_HANDLER_MOUSEOVER, &mouseover_color_race_handler);
	init_window(color_race_win, newchar_hud_win, 0, 0, 10, 270, window_height - hud_y - 10);
	namepass_win = create_window(win_name_pass, newchar_hud_win, 0, 0, 10, 270, window_height - hud_y - 10, ELW_SHOW_LAST); //Choose name and password
	set_window_handler(namepass_win, ELW_HANDLER_INIT, &init_namepass_handler);
	set_window_handler(namepass_win, ELW_HANDLER_KEYPRESS, &keypress_namepass_handler);
	init_window(namepass_win, newchar_hud_win, 0, 0, 10, 270, window_height - hud_y - 10);
}

void resize_newchar_hud_window(void)
{
	if(get_show_window(newchar_hud_win)) //Simply destroy and recreate everything
	{
		destroy_window(color_race_win);
		destroy_window(namepass_win);
		destroy_window(newchar_hud_win);
		destroy_window(tooltip_win);

		newchar_hud_win = -1;
		create_newchar_hud_window();
	}
}
#endif
