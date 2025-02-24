#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <float.h>
#include <errno.h>
//For stat() etc.. below
#include <sys/types.h>
#include <sys/stat.h>
#ifndef _MSC_VER
	#include <unistd.h>
#endif //_MSC_VER
#include "user_menus.h"
#ifdef MAP_EDITOR
 #include "../editeur_sources/global.h"
 #include "../editeur_sources/browser.h"
 #include "../editeur_sources/interface.h"
 #include "load_gl_extensions.h"
#else
#ifdef ACHIEVEMENTS
 #include "achievements.h"
#endif //ACHIEVEMENTS
 #include "alphamap.h"
 #include "bags.h"
 #include "buddy.h"
 #include "chat.h"
 #include "console.h"
 #include "counters.h"
 #include "dialogues.h"
 #include "draw_scene.h"
 #include "errors.h"
 #include "elwindows.h"
 #include "filter.h"
 #include "gamewin.h"
 #include "gl_init.h"
 #include "hud.h"
 #include "init.h"
 #include "interface.h"
 #include "items.h"
 #include "item_info.h"
 #include "manufacture.h"
 #include "map.h"
 #include "mapwin.h"
 #ifdef MISSILES
 #include "missiles.h"
 #endif //MISSILES
 #include "multiplayer.h"
 #include "new_character.h"
 #include "openingwin.h"
 #include "particles.h"
 #include "pm_log.h"
 #include "questlog.h"
 #include "reflection.h"
 #include "serverpopup.h"
 #include "session.h"
 #include "shadows.h"
 #include "sound.h"
 #include "spells.h"
 #include "stats.h"
 #include "storage.h"
 #include "tabs.h"
 #include "trade.h"
 #include "trade_log.h"
 #include "weather.h"
  #include "minimap.h"
 #ifdef NEW_ALPHA
  #include "3d_objects.h"
 #endif
  #include "io/elpathwrapper.h"
  #include "notepad.h"
  #include "sky.h"
  #ifdef OSX
   #include "events.h"
  #endif // OSX
#endif
#include "asc.h"
#include "elconfig.h"
#include "text.h"
#include "consolewin.h"
#include "queue.h"
#include "url.h"
#include "widgets.h"
#include "eye_candy_wrapper.h"
#include "sendvideoinfo.h"
#include "actor_init.h"
#include "io/elpathwrapper.h"
#ifdef	NEW_TEXTURES
#include "textures.h"
#endif	/* NEW_TEXTURES */
#ifdef	FSAA
#include "fsaa/fsaa.h"
#endif	/* FSAA */
#ifdef	CUSTOM_UPDATE
#include "custom_update.h"
#endif	/* CUSTOM_UPDATE */
#include "themes.h"
#include "fr_quickitems.h"
#include "trade.h"
#include "books.h"
#include "info_combat.h"
#ifdef WITHDRAW_LIST
#include "item_lists.h"
#endif //WITHDRAW_LIST
#define CONTROLS	0
#define HUD			1
#define CHAT		2
#define FONT		3
#define SERVER		4
#define AUDIO		5
#define VIDEO		6
#define GFX			7
#define CAMERA		8
#define TROUBLESHOOT	9
#ifdef DEBUG
#define DEBUGTAB	10
#define MAX_TABS 11
#else
#define MAX_TABS 10
#endif
#define CHECKBOX_SIZE		15
#define SPACING				5	//Space between widgets and labels and lines
#define LONG_DESC_SPACE		50	//Space to give to the long descriptions
#define MAX_LONG_DESC_LINES	3	//How many lines of text we can fit in LONG_DESC_SPACE
typedef char input_line[256];
Allcvars our_vars;
#define indexof_cvar(v) ((v) - our_vars.cvars)
int write_ini_on_exit = 1;
// Window Handling
int elconfig_win = -1;
int force_elconfig_win_ontop = 0;
int elconfig_tab_collection_id = 1;
unsigned char elconf_description_buffer[400] = {0};
typedef struct Tab { Uint32 tab; Uint16 x, y; } Tab;
static Tab elconfig_tabs[MAX_TABS];
int affichage_barres = 1 ;
int elconfig_menu_x = 10;
int elconfig_menu_y = 10;
int elconfig_menu_x_len = 640;
int elconfig_menu_y_len = 430;
int windows_on_top = 0;
int options_set = 0;
#ifdef	FSAA
int anti_aliasing = 0;
#endif	/* FSAA */
static int shadow_map_size;
int gx_adjust = 0;
int gy_adjust = 0;
int you_sit = 0;
int sit_lock = 0;
int use_keypress_dialog_boxes = 0, use_full_dialogue_window = 0;
int use_alpha_banner = 0;
int show_fps = 1;
#ifdef DEBUG
int render_skeleton = 0;
int render_mesh = 1;
int render_bones_id = 0;
int render_bones_orientation = 0;
#endif
#ifdef NEW_CURSOR
int big_cursors = 0;
int sdl_cursors = 0;
float pointer_size = 1.0;
#endif // NEW_CURSOR
float water_tiles_extension = 200.0;
int show_game_seconds = 0;
int skybox_update_delay = 10;
int skybox_local_weather = 0;
#ifdef OSX	// for probelem with rounded buttons on Intel graphics
int square_buttons = 0;
#endif
#ifdef	NEW_TEXTURES
int small_actor_texture_cache = 0;
#endif	/* NEW_TEXTURES */
int video_info_sent = 0;
int delai_sauve = 30;
int delai_sauve_ms = 30 * 60000;
int derniere_sauvegarde;
#if defined(MISSILES) && defined(DEBUG)
int enable_client_aiming = 0;
#endif // MISSILES & DEBUG
void options_loaded(void) {
	// find any options not marked as saved, excluding ones we marked on purpose
	for_cvars(v) {
		if (!v->saved && !(v->cvflags & CVF_INI)) {
			v->saved = 1;
		}
	}
	options_set = 1;
}

int int_zero_func(void) { return 0; }
float float_zero_func(void) { return 0.0f; }
int int_one_func(void) { return 1; }
float float_one_func(void) { return 1.0f; }

static inline void destroy_shadow_mapping(void) {
	if (gl_extensions_loaded)
	{
		CHECK_GL_ERRORS();
		if (have_extension(ext_framebuffer_object))
		{
#ifndef MAP_EDITOR
			free_shadow_framebuffer();
#endif //MAP_EDITOR
		}
		else
		{
			if (depth_map_id != 0)
			{
				glDeleteTextures(1, &depth_map_id);
				depth_map_id = 0;
			}
		}
		CHECK_GL_ERRORS();
	}
}

static inline void destroy_fbos(void) {
	if (gl_extensions_loaded)
	{
		CHECK_GL_ERRORS();
		if (have_extension(ext_framebuffer_object))
		{
#ifndef MAP_EDITOR
			destroy_shadow_mapping();
			free_reflection_framebuffer();
#endif //MAP_EDITOR
		}
		CHECK_GL_ERRORS();
	}
}

static inline void check_option_var(cstr name) {
	int i = find_var(name, VNK_GAME);
	if (i < 0) {
		LOG_ERROR("Can't find game var '%s'", name);
		return;
	}
	Cvar *v = our_vars.cvars + i;
	if (v->cvdtype == CVD_INT) {
		v->pfunc_seti(v->pint, *v->pint);
	} else if (v->cvdtype == CVD_BOOL) {
		*v->pint = !*v->pint;
		v->pfunc_setb(v->pint);
	} else if (v->cvdtype == CVD_STRING) {
		v->pfunc_sets(v->pstr, v->pstr, v->slen);
	} else if (v->cvdtype == CVD_FLOAT) {
		v->pfunc_setf(v->pfloat, *v->pfloat);
	}
}

static inline void build_fbos(void) {
	if (gl_extensions_loaded)
	{
#ifndef MAP_EDITOR
		if (have_extension(ext_framebuffer_object) && use_frame_buffer)
		{
			if ((water_shader_quality > 0) && show_reflection)
			{
				make_reflection_framebuffer(window_width, window_height);
			}
		}
#endif // MAP_EDITOR
		check_option_var("shadow_map_size");
	}
}

static inline void update_fbos(void) {
	destroy_fbos();
	build_fbos();
}

void change_var(int *var) {
	*var = !*var;
}

#ifndef MAP_EDITOR
void change_minimap_scale(float *var, float value)
{
	int shown = 0;
	*var = value;
	if (minimap_win >= 0) {
		shown = get_show_window(minimap_win);
		minimap_win_x = windows_list.window[minimap_win].cur_x;
		minimap_win_y = windows_list.window[minimap_win].cur_y;
		destroy_window(minimap_win);
		minimap_win = -1;
	}
	if (shown)
		display_minimap();
}

void change_sky_var(int *var)
{
	*var = !*var;
	skybox_update_colors();
}

void change_min_ec_framerate(float *var, float value) {
	if (value >= 0) {
		if (value < max_ec_framerate) {
			*var = value;
		} else if (!max_ec_framerate) {
			*var = value;
		} else {
			*var = max_ec_framerate - 1;
	  	}
	} else {
		*var = 0;
	}
}

void change_max_ec_framerate(float *var, float value) {
	if (value >= 1) {
		if (value > min_ec_framerate) {
			*var = value;
		} else if (!min_ec_framerate) {
			*var = value;
		} else {
			*var = min_ec_framerate + 1;
		}
	} else {
		*var = 1;
	}
}
#endif //!MAP_EDITOR

void change_int(int *var, int value) {
	if (value >= 0) {
		*var = value;
	}
}

void change_signed_int(int *var, int value) {
	*var = value;
}

void change_float(float *var, float value) {
#ifdef	ELC
	if (var == &name_text_size || var == &taille_titre_texte) {
		if (value > 2.0){
			value = 2.0;
		}
	}
#endif	//ELC
	*var = value;
}

void change_string(char *var, cstr s, int len) {
	while (*s && len--) {
		*var++= *s++;
	}
	*var = 0;
}

#ifdef ELC
void change_sound_level(float *var, float value)
{
	if (value >= 0.0f && value <= 1.0f+0.00001) {
		*var = value;
	} else {
		*var = 0;
	}
}

void change_password(char *passwd)
{
	int i = 0;
	char *str = password;

	while(*passwd) {
		*str++= *passwd++;
	}
	*str = 0;
	if (password[0]){	//We have a password
		for (; i < str-password; i++) {
			display_password_str[i] = '*';
		}
		display_password_str[i] = 0;
	}
}

#ifdef	NEW_TEXTURES
void update_max_actor_texture_handles(void) {
	if (poor_man == 1)
	{
		if (small_actor_texture_cache == 1)
		{
			max_actor_texture_handles = 1;
		}
		else
		{
			max_actor_texture_handles = 4;
		}
	}
	else
	{
		if (small_actor_texture_cache == 1)
		{
			max_actor_texture_handles = 16;
		}
		else
		{
			max_actor_texture_handles = 32;
		}
	}
}
#endif	/* NEW_TEXTURES */

void change_poor_man(int *poor_man)
{
	*poor_man = !*poor_man;
#ifdef	NEW_TEXTURES
	unload_texture_cache();
	update_max_actor_texture_handles();
#endif	/* NEW_TEXTURES */
	if (*poor_man) {
		show_reflection = 0;
		shadows_on = 0;
		clouds_shadows = 0;
		use_shadow_mapping = 0;
#ifndef MAP_EDITOR2
		special_effects = 0;
		use_eye_candy = 0;
		render_fog = 0;
		show_weather = 0;
#endif
#ifndef MAP_EDITOR
		use_frame_buffer = 0;
#endif
		update_fbos();
		skybox_show_clouds = 0;
		skybox_show_sun = 0;
		skybox_show_moons = 0;
		skybox_show_stars = 0;
	}
}

void change_compiled_vertex_array(int *value)
{
	if (*value) {
		*value = 0;
	}
	else if (!gl_extensions_loaded || have_extension(ext_compiled_vertex_array))
	{
		// don't check if we have hardware support when OpenGL
		// extensions are not initialized yet.
		*value = 1;
	}
	else LOG_TO_CONSOLE(c_green2,disabled_compiled_vertex_arrays);
}

void change_vertex_buffers(int *value)
{
	if (*value) {
		*value = 0;
	}
	else if (!gl_extensions_loaded || have_extension(arb_vertex_buffer_object))
	{
		// don't check if we have hardware support when OpenGL
		// extensions are not initialized yet.
		*value = 1;
	}
//	else LOG_TO_CONSOLE(c_green2,disabled_vertex_buffers);
}

void change_clouds_shadows(int *value)
{
	if (*value) {
		*value = 0;
	}
	else if (!gl_extensions_loaded || (get_texture_units() >= 2))
	{
		// don't check if we have hardware support when OpenGL
		// extensions are not initialized yet.
		*value = 1;
	}
//	else LOG_TO_CONSOLE(c_green2,disabled_clouds_shadows);
}

#ifdef	NEW_TEXTURES
void change_small_actor_texture_cache(int *value)
{
	if (*value)
	{
		*value = 0;
	}
	else
	{
		*value = 1;
	}

	update_max_actor_texture_handles();
}

void change_eye_candy(int *value)
{
	if (*value)
	{
		*value = 0;
	}
	else if (!gl_extensions_loaded || ((get_texture_units() >= 2) &&
		supports_gl_version(1, 5)))
	{
		// don't check if we have hardware support when OpenGL
		// extensions are not initialized yet.
		*value = 1;
	}
}
#else	/* NEW_TEXTURES */
void change_mipmaps(int *value)
{
	if (*value) {
		*value = 0;
	}
	else if (!gl_extensions_loaded || have_extension(sgis_generate_mipmap))
	{
		// don't check if we have hardware support when OpenGL
		// extensions are not initialized yet.
		*value = 1;
	}
//	else LOG_TO_CONSOLE(c_green2,disabled_mipmaps);
}
#endif	/* NEW_TEXTURES */

void change_point_particles(int *value)
{
	if (*value) {
		*value = 0;
	}
	else if (!gl_extensions_loaded || have_extension(arb_point_sprite))
	{
		// don't check if we have hardware support when OpenGL
		// extensions are not initialized yet.
		*value = 1;
	}
	else
	{
		LOG_TO_CONSOLE(c_green2, disabled_point_particles);
	}

#ifndef	NEW_TEXTURES
	ec_set_draw_method();
#endif	/* NEW_TEXTURES */
}

void change_particles_percentage(int *pointer, int value)
{
	if (value>0 && value <= 100) {
		particles_percentage = value;
	}
	else
	{
		particles_percentage = 0;
		LOG_TO_CONSOLE(c_green2, disabled_particles_str);
	}
}

void change_new_selection(int *value)
{
	if (*value)
	{
		*value = 0;
	}
	else
	{
		if (gl_extensions_loaded)
		{
			if ((supports_gl_version(1, 3) ||
				have_extension(arb_texture_env_combine)) &&
				(get_texture_units() > 1) && (bpp == 32))
			{
				*value = 1;
			}
		}
		else
		{
			*value = 1;
		}
	}
}

int switch_video(int mode, int full_screen)
{
	int win_width,
		win_height,
		win_bpp;

	int index = mode - 1;

	int flags = SDL_OPENGL;
	if (full_screen)
		flags |= SDL_FULLSCREEN;

	if (mode == 0 && !full_screen) {
		win_width = video_width;
		win_height = video_height;
		win_bpp = bpp;
	} else if (index < 0 || index >= video_modes_count) {
		//warn about this error
		LOG_TO_CONSOLE(c_red2,invalid_video_mode);
		return 0;
	} else {
		/* Check if the video mode is supported. */
		win_width = video_modes[index].width;
		win_height = video_modes[index].height;
		win_bpp = video_modes[index].bpp;
	}

#ifndef LINUX
	LOG_TO_CONSOLE(c_green2, video_restart_str);
	video_mode = mode;
	set_var_unsaved("video_mode", VNK_INI);
	return 1;
#endif

	destroy_fbos();

	if (!SDL_VideoModeOK(win_width, win_height, win_bpp, flags)) {
		LOG_TO_CONSOLE(c_red2, invalid_video_mode);
		return 0;
	} else {
		set_new_video_mode(full_screen, mode);
#ifndef MAP_EDITOR2
		if (items_win >= 0) {
			windows_list.window[items_win].show_handler(&windows_list.window[items_win]);
		}
#endif
	}
	build_fbos();
#ifdef NEW_NEW_CHAR_WINDOW
	resize_newchar_hud_window(); //This window needs resizing
#endif

	return 1;
}
void switch_vidmode(int *pointer, int mode)
{
	if (!video_mode_set) {
		/* Video isn't ready yet, just remember the mode */
		video_mode = mode;
	} else {
		switch_video(mode, full_screen);
	}
}

void toggle_full_screen_mode(int *fs)
{
	if (!video_mode_set) {
		*fs = !*fs;
	} else {
		toggle_full_screen();
	}
}

#ifdef NEW_CURSOR
void change_sdl_cursor(int *fs)
{
	if (!*fs) {
		SDL_ShowCursor(1);
	} else {
		SDL_ShowCursor(0);
	}
	*fs = !*fs;
}
#endif // NEW_CURSOR

void change_quickitems_size(int *dest, int value)
{
	if (value > FR_QUICKITEMS_MAXSIZE) value = FR_QUICKITEMS_MAXSIZE;
	else if (value < 1) value = 1;
	*dest = value;
	if (quickbar_win >= 0) resize_fr_quickitems();
}

void change_quickspells_size(int *dest, int value)
{
	if (value > QUICKSPELLS_MAXSIZE) value = QUICKSPELLS_MAXSIZE;
	else if (value < 1) value = 1;
	*dest = value;
	if (quickspell_win >= 0) resize_quickspells(0);
}

void toggle_follow_cam(int *fc)
{
	last_kludge = camera_kludge;
	if (*fc)
		hold_camera = rz;
	else
		hold_camera+= camera_kludge;
	change_var(fc);
}

void toggle_follow_cam_behind(int *fc)
{
	if (*fc)
	{
		last_kludge = camera_kludge;
		hold_camera += camera_kludge;
	}
	else
	{
		last_kludge = -rz;
	}
	change_var(fc);
}

void toggle_ext_cam(int *ec)
{
	change_var(ec);
	if (*ec)
	{
		isometric = 0;
		if (video_mode_set)
		{
			resize_root_window();
			set_all_intersect_update_needed(main_bbox_tree);
		}
	}
}

void change_tilt_float(float *var, float value)
{
	*var = value;
	if (rx > -min_tilt_angle) rx = -min_tilt_angle;
	else if (rx < -max_tilt_angle) rx = -max_tilt_angle;
}

void change_shadow_map_size(int *pointer, int value)
{
	const int array[10] = {256, 512, 768, 1024, 1280, 1536, 1792, 2048, 3072, 4096};
	int index, size, i, max_size, error;
	char error_str[1024];

	if (value >= array[0])
	{
		index = 0;
		for (i = 0; i < 10; i++)
		{
			/* Check if we can set the multiselect widget to this */
			if (array[i] == value)
			{
				index = i;
				break;
			}
		}
	}
	else
	{
		index = min2i(max2i(0, value), 9);
	}

	size = array[index];

	if (gl_extensions_loaded && use_shadow_mapping)
	{
		error = 0;

		if (use_frame_buffer)
		{
			glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE_EXT, &max_size);
		}
		else
		{
			max_size = min2i(window_width, window_height);
		}

		if (size > max_size)
		{
			while ((size > max_size) && (index > 0))
			{
				index--;
				size = array[index];
			}
			error = 1;
		}

		if (!(have_extension(arb_texture_non_power_of_two) || supports_gl_version(2, 0)))
		{
			switch (index)
			{
				case 2:
					index = 1;
					error = 1;
					break;
				case 4:
				case 5:
				case 6:
					index = 3;
					error = 1;
					break;
				case 8:
					index = 7;
					error = 1;
					break;
			}
			size = array[index];
		}
		if (error == 1)
		{
			memset(error_str, 0, sizeof(error_str));
			safe_snprintf(error_str, sizeof(error_str),
				shadow_map_size_not_supported_str, size);
			LOG_TO_CONSOLE(c_yellow2, error_str);
		}
		smsize = size;

		destroy_shadow_mapping();
		if (have_extension(ext_framebuffer_object) && use_frame_buffer)
		{
			make_shadow_framebuffer();
		}
	}
	else
	{
		smsize = size;
	}

	if (pointer)
	{
		*pointer = index;
	}
}

#ifdef	FSAA
void change_fsaa(int *pointer, int value)
{
	unsigned int i, index, fsaa_value;

	index = 0;
	fsaa_value = 0;

	if (value > 0)
	{
		for (i = 1; i < get_fsaa_mode_count(); i++)
		{
			if (get_fsaa_mode(i))
			{
				index++;
				fsaa_value = i;
			}
			if (value == index)
			{
				break;
			}
		}
	}

	fsaa = fsaa_value;

	if (pointer != 0)
	{
			*pointer = index;
	}

	LOG_TO_CONSOLE(c_green2, video_restart_str);
}
#endif	/* FSAA */

#ifdef CUSTOM_UPDATE
void change_custom_update(int *var)
{
	*var = !*var;

	if (*var)
	{
		start_custom_update();
	}
}

void change_custom_clothing(int *var)
{
	*var = !*var;
#ifdef	NEW_TEXTURES
	unload_actor_texture_cache();
#endif	/* NEW_TEXTURES */
}
#endif    //CUSTOM_UPDATE

void change_delai_sauvegarde (int *pointeur, int temps)
{
	delai_sauve_ms = temps * 60000; // Conversion en millisecondes
	*pointeur = temps;
}

#ifndef MAP_EDITOR2
void set_afk_time(int *pointer, int time) {
	if (time > 0) {
		afk_time = time*60000;
		*pointer = time;
	} else {
		afk_time = 0;
		*pointer = 0;
	}
}

void set_buff_icon_size(int *pointer, int value)
{
	/* The value is actually set in the widget code so attempting so controlling the
		range here does not work.  Instead, use the built in max/min code of the widget.
		We still need to set the value here for the initial read from the config file. */
	*pointer = value;
	/* Turn off icons when the size is zero (or at least low). */
	view_buffs = (value < 5) ?0: 1;
}

static void change_lines_to_show(int *p, int v) {
	*p = v;
	lines_to_show = max_lines_to_show;
}

void change_dark_channeltext(int *dct, int value)
{
	*dct = value;
	if (*dct == 1)
		set_text_message_color (&input_text_line, 0.6f, 0.6f, 0.6f);
	else if (*dct == 2)
		set_text_message_color (&input_text_line, 0.16f, 0.16f, 0.16f);
	else
		set_text_message_color (&input_text_line, 1.0f, 1.0f, 1.0f);
}

void change_windowed_chat(int *wc, int val) {
	int old_wc = *wc;
	*wc = val;
	if (*wc == 1) {
		if (game_root_win >= 0) {
			display_tab_bar ();
		}
	} else if (tab_bar_win_1 >= 0) {
		hide_window (tab_bar_win_1);
	}
	if (*wc == 2) {
		if (game_root_win >= 0) {
			display_chat();
		}
	} else if (chat_win >= 0) {
		hide_window (chat_win);
	}
	if (old_wc != *wc && (old_wc == 1 || old_wc == 2)) {
		convert_tabs (*wc);
	}
}

void change_affichage_barre (int *affich_barres)
{
	*affich_barres = ! *affich_barres;
	if (! hud_x)
	{
		hud_x = HUD_MARGIN_X;
		hud_y = HUD_MARGIN_Y;
	}
	else
	{
		hud_x = 0;
		hud_y = 0;
	}
	resize_root_window ();
}

void change_chat_zoom(float *dest, float value) {
	// valeur minimale pour avoir une hauteur de ligne > 0 (évite des divisions par 0)
	if (value * DEFAULT_FONT_Y_LEN < 1) value = 1 / DEFAULT_FONT_Y_LEN;
	if (value < 0.0f) {
		return;
	}
	*dest = value;
	if (opening_root_win >= 0 || console_root_win >= 0 || chat_win >= 0 || game_root_win >= 0) {
		if (opening_root_win >= 0) {
			opening_win_update_zoom();
		}
		if (console_root_win >= 0) {
			nr_console_lines = (window_height - input_widget->len_y - HUD_MARGIN_Y - 20 - nb_ligne_tabs*tab_bar_height - 1) / (int)(DEFAULT_FONT_Y_LEN * chat_text_size);
			widget_set_size(console_root_win, console_out_id, value);
		}
		if (chat_win >= 0) {
			chat_win_update_zoom();
		}
	}
	if (input_widget) {
		text_field *tf = input_widget->widget_info;
		widget_set_size(input_widget->window_id, input_widget->id, value);
		if (windowed_chat != 2) {
			widget_resize(input_widget->window_id, input_widget->id, input_widget->len_x, tf->y_space*2 + ceilf(DEFAULT_FONT_Y_LEN*input_widget->size*tf->nr_lines));
		}
	}
}

void change_chat_font(int *var, int value)
{
	if (value < 0) return;
	*var = value;

	if (input_widget) {
		((text_field*)(input_widget->widget_info))->font_num = value;
	}
	if (console_root_win >= 0) {
		((text_field*)((widget_find(console_root_win, console_out_id))->widget_info))->font_num = value;
	}
//	change_chat_zoom(&chat_text_size, &chat_text_size);
}

void change_book_zoom(float *dest, float value) {
	// valeur minimale pour avoir une hauteur de ligne > 0 (évite des divisions par 0)
	if (value * DEFAULT_FONT_Y_LEN < 1) value = 1 / DEFAULT_FONT_Y_LEN;
	*dest = value;
	book_reload = 1;
}

void change_note_zoom(float *dest, float value) {
	if (value < 0.0f)
		return;
	*dest = value;
	if (notepad_win >= 0)
		notepad_win_update_zoom ();
}

#endif
#endif // def ELC

void change_dir_name(char *var, cstr str, int len)
{
	int idx;

	for (idx = 0; idx < len && str[idx]; idx++) {
		var[idx] = str[idx];
	}
	if (var[idx-1] != '/') {
		var[idx++] = '/';
	}
	var[idx] = '\0';
}

#ifdef ANTI_ALIAS
void change_aa(int *pointer) {
	change_var(pointer);
	if (anti_alias) {
		glHint(GL_POINT_SMOOTH_HINT,   GL_NICEST);
		glHint(GL_LINE_SMOOTH_HINT,    GL_NICEST);
		glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
		glEnable(GL_POINT_SMOOTH);
		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_POLYGON_SMOOTH);
	} else {
		glHint(GL_POINT_SMOOTH_HINT,   GL_FASTEST);
		glHint(GL_LINE_SMOOTH_HINT,    GL_FASTEST);
		glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);
		glDisable(GL_POINT_SMOOTH);
		glDisable(GL_LINE_SMOOTH);
		glDisable(GL_POLYGON_SMOOTH);
	}
#ifdef OPENGL_TRACE
	CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
}
#endif // ANTI_ALIAS
#ifdef ELC
#ifdef OSX
void change_projection_float_init(float *var, float value) {
	change_float(var, value);
}

void change_projection_bool_init(int *pointer) {
	change_var(pointer);
}
#endif //OSX
void change_projection_float(float *var, float value) {
	change_float(var, value);
	if (video_mode_set)
	{
		resize_root_window ();
		set_all_intersect_update_needed (main_bbox_tree);
	}
}

void change_projection_bool(int *pointer) {
	change_var(pointer);
	if (video_mode_set)
	{
		resize_root_window ();
		if (pointer == &isometric && isometric) extended_cam = 0;
		set_all_intersect_update_needed (main_bbox_tree);
	}
}

void change_gamma(float *pointer, float value)
{
	*pointer = value;
	if (video_mode_set && !disable_gamma_adjust) {
		SDL_SetGamma(value, value, value);
	}
}

#ifndef MAP_EDITOR2
void change_windows_on_top(int *var)
{
	int winid_list[] = { storage_win, manufacture_win, items_win, buddy_win, ground_items_win, sigil_win, elconfig_win, tab_stats_win, minimap_win, trade_win, book_win, paper_win };
	int i;
	*var =!*var;
	if (*var) {
		for (i = 0; i<sizeof(winid_list)/sizeof(int); i++)
		{
			if (winid_list[i] >= 0)
			{
				window_info *win = &windows_list.window[winid_list[i]];
				/* Change the root windows */
				move_window(winid_list[i], -1, 0, win->pos_x, win->pos_y );
				/* Display any open windows */
				if (win->displayed != 0 || win->reinstate != 0)
					show_window(winid_list[i]);
			}
		}
	}
	else
	{
		// Change the root windows
		for (i = 0; i<sizeof(winid_list)/sizeof(int); i++)
			if (winid_list[i] >= 0)
			{
				window_info *win = &windows_list.window[winid_list[i]];
				move_window(winid_list[i], game_root_win, 0, win->pos_x, win->pos_y );
			}

		// Hide all the windows if needed
		if (windows_list.window[game_root_win].displayed == 0) {
			hide_window(game_root_win);
		}
	}
}
#endif

#ifndef MAP_EDITOR2
void change_separate_flag(int *pointer) {
	change_var(pointer);

	if (chat_win >= 0) {
		update_chat_win_buffers();
	}
}
#endif

void change_shadow_mapping (int *sm)
{
	if (*sm)
	{
		*sm = 0;
	}
	else
	{
		// don't check if we have hardware support when OpenGL
		// extensions are not initialized yet.
		if (!gl_extensions_loaded || ((get_texture_units() >= 3) &&
			have_extension(arb_shadow) && have_extension(arb_texture_env_combine)))
		{
			*sm = 1;
		}
		else
		{
			LOG_TO_CONSOLE (c_red1, disabled_shadow_mapping);
		}
	}
	update_fbos();
}

#ifndef MAP_EDITOR2
void change_global_filters (int *use)
{
	*use = !*use;
	// load global filters when new value is true, but only when changed
	// in game, not on startup
	if (options_set && *use) {
		// Permet de recharger plus proprement la liste.
		load_filters();
	}
}
#endif //ndef MAP_EDITOR2

#endif // ELC

void change_reflection(int *rf)
{
	*rf = !*rf;
	update_fbos();
}

#ifndef MAP_EDITOR
void change_frame_buffer(int *fb)
{
	if (*fb)
	{
		*fb = 0;
	}
	else
	{
		if (!gl_extensions_loaded || have_extension(ext_framebuffer_object))
		{
			*fb = 1;
		}
		else
		{
			LOG_TO_CONSOLE (c_red1, disabled_framebuffer);
		}
	}
	update_fbos();
}
#endif

void change_shadows(int *sh)
{
	*sh = !*sh;
#ifndef MAP_EDITOR
	if (*sh)
		use_shadow_mapping = 0;
	else
		use_shadow_mapping = 1;

	change_shadow_mapping (&use_shadow_mapping);
#endif
	update_fbos();
}

#ifndef MAP_EDITOR
int int_max_water_shader_quality(void) {
	if (gl_extensions_loaded)
	{
		return get_max_supported_water_shader_quality();
	}
	else
	{
		return 2;
	}
}

void change_water_shader_quality(int *wsq, int value)
{
	if (gl_extensions_loaded)
	{
		*wsq = min2i(max2i(value, 0), get_max_supported_water_shader_quality());
	}
	else
	{
		*wsq = value;
	}
	update_fbos();
}
#endif

#ifdef MAP_EDITOR

void set_auto_save_interval (int *save_time, int time)
{
	if (time>0) {
		*save_time = time*60000;
	} else {
		*save_time = 0;
	}
}

void switch_vidmode(int *pointer, int mode)
{
	switch(mode)
		{
			case 1: window_width = 640;
				window_height = 480;
				return;
			case 2: window_width = 780;
				window_height = 550;
				return;
			case 3:
				window_width = 990;
				window_height = 720;
				return;
			case 4:
				window_width = 1070;
				window_height = 785;
				return;
			case 5:
				window_width = 1250;
				window_height = 990;
				return;
			case 6:
				window_width = 1600;
				window_height = 1200;
			case 7:
				window_width = 1280;
				window_height = 800;
			default:
				return;
		}
}

#endif

int find_var(cstr name, vnkind_t k) {
	cstr p = name;
	for (; *p && *p != ' ' && *p != '='; ++p);
	int n = p - name, full = k != VNK_CMD_SHORT;
	for_cvars(v) {
		if (!strncmp(name, full ? v->name : v->shortname, n)) {
			return indexof_cvar(v);
		}
	}
	return -1;
}

cstr get_option_description(cstr name, vnkind_t k) {
	int var_index = find_var(name, k);
	if (var_index == -1) {
		LOG_ERROR("Can't find var '%s', kind %d", name, k);
		return NULL;
	}
	return (cstr)our_vars.cvars[var_index].display.desc;
}

int set_var_unsaved(cstr name, vnkind_t k) {
	int var_index = find_var(name, k);
	if (var_index == -1) {
		LOG_ERROR("Can't find var '%s', kind %d", name, k);
		return 0;
	}
	our_vars.cvars[var_index].saved = 0;
	return 1;
}

int toggle_bool_var(cstr name) {
	int i = find_var(name, VNK_INI);
	Cvar *v = our_vars.cvars + i;
	if (i == -1 || v->cvdtype != CVD_BOOL) {
		LOG_ERROR("Invalid bool option '%s'", name);
		return 0;
	}
	v->pfunc_setb(v->pint);
	v->saved = 0;
	return 1;
}

#ifdef	ELC
int set_int_var(cstr name, int new_value) {
	int i = find_var(name, VNK_INI);
	Cvar *v = our_vars.cvars + i;
	if (i != -1 && v->cvdtype == CVD_INT) {
		int tab_win_id = elconfig_tabs[v->widgets.tab_id].tab;
		int widget_id = v->widgets.widget_id;
		// This bit belongs in the widgets module
		widget_list *widget = widget_find(tab_win_id, widget_id);
		v->saved = 0;
		if (widget && widget->widget_info) {
			spinbutton *button = widget->widget_info;
			*(int *)button->data = new_value;
			safe_snprintf(button->input_buffer, sizeof(button->input_buffer), "%i", *(int *)button->data);
			return 1;
		}
		return 0;
	}
	LOG_ERROR("Can't find int var '%s'", name);
	return 0;
}
#endif

void change_language(cstr new_lang)
{
	int var_index;

	LOG_DEBUG("Language changed, was [%s] now [%s]\n",  language, new_lang);
	/* guard against being the same string */
	if (strcmp(language, new_lang) != 0)
		safe_strncpy(language, new_lang, sizeof(language));

	var_index = find_var("language", VNK_INI);

	if (var_index != -1)
	{
		our_vars.cvars[var_index].saved = 0;
	}
	else
	{
		LOG_ERROR("Can't find var '%s', type 'OPT_STRING'", "language");
	}
}

void check_options(void) {
	check_option_var("use_compiled_vertex_array");
	check_option_var("use_vertex_buffers");
	check_option_var("clouds_shadows");
#ifdef	NEW_TEXTURES
	check_option_var("small_actor_texture_cache");
	check_option_var("use_eye_candy");
#else	/* NEW_TEXTURES */
	check_option_var("use_mipmaps");
#endif	/* NEW_TEXTURES */
	check_option_var("use_point_particles");
	check_option_var("use_frame_buffer");
	check_option_var("use_shadow_mapping");
	check_option_var("shadow_map_size");
	check_option_var("water_shader_quality");
}

int check_var(char *name, vnkind_t k) {
	int new_val;
	char our_string[256];
	char *ptr = name, *tptr;
	int i = find_var(name, k);
	if (i < 0) {
		LOG_WARNING("Can't find var '%s', kind %d", name, k);
		return -1;
	}
	Cvar *v = our_vars.cvars + i;
	ptr += (k != VNK_CMD_SHORT) ? v->name_len : v->shortname_len;
	while (*ptr && (*ptr == ' ' || *ptr == '='))
		ptr++;	// go to the string occurence
	if (!*ptr || *ptr == 0x0d || *ptr == 0x0a)
		return -1;	// hmm, why would you do such a stupid thing?
	if (*ptr == '"')
	{
		//Accurate quoting
		tptr = ++ptr;
		while (*tptr && *tptr != '"')
		{
			if (*tptr == 0x0a || *tptr == 0x0d)
			{
#ifdef ELC
				char str[200];
				safe_snprintf(str, sizeof(str), "Reached newline without an ending \" in %s", v->name);
				LOG_TO_CONSOLE(c_red2,str);
#endif // ELC
				break;
			}
			tptr++;
		}
		*tptr = 0;
	} else {
		// Strip it
		tptr = our_string;
		while (*ptr && *ptr != 0x0a && *ptr != 0x0d) {
			if (*ptr != ' ')
				*tptr++= *ptr++; //Strip all spaces
			else
				ptr++;
		}
		*tptr = 0;
		ptr = our_string;
	}
	if (k == VNK_INI) {
		v->saved = 1;
	} else if (k == VNK_GAME) {
		// make sure in-game changes are stored in el.ini
		v->saved = 0;
	}
	if (v->cvflags & CVF_INI) {
		v->saved = 0;
	}
	if (v->cvdtype == CVD_INT) {
		v->pfunc_seti(v->pint, atoi(ptr));
	} else if (v->cvdtype == CVD_BOOL) {
		if (ptr && ptr[0] =='!')
			new_val = !*v->pint;
		else
			new_val = (atoi(ptr) > 0);
		if (new_val != *v->pint)
			v->pfunc_setb(v->pint);
	} else if (v->cvdtype == CVD_STRING) {
		v->pfunc_sets(v->pstr, ptr, v->slen);
	} else if (v->cvdtype == CVD_FLOAT) {
		float f = atof(ptr);
		v->pfunc_setf(v->pfloat, f);
	} else {
		return -1;
	}
	return 1;
}
void free_vars(void) {
	our_vars.n_cvars = 0;
	our_vars.n_snodes = 0;
}
static inline Cvar *lastcv(void) {
	assert(our_vars.n_cvars > 0);
	return our_vars.cvars + our_vars.n_cvars - 1;
}
static inline void cvset_flags(Uint8 f) { lastcv()->cvflags |= f; }
static inline void cvset_immfuncs(ival_func pfunc_imin, ival_func pfunc_imax) {
	Cvar *v = lastcv();
	v->pfunc_imin = pfunc_imin;
	v->pfunc_imax = pfunc_imax;
	v->cvflags |= CVF_FUNCS;
}
static inline void cvset_fminmaxstep(float fmin, float fmax, float fstep) {
	Cvar *v = lastcv();
	v->fmin = fmin;
	v->fmax = fmax;
	v->fstep = fstep;
}
static inline void cvset_fmmsfuncs(fval_func pfunc_fmin, fval_func pfunc_fmax, fval_func pfunc_fstep) {
	Cvar *v = lastcv();
	v->pfunc_fmin = pfunc_fmin;
	v->pfunc_fmax = pfunc_fmax;
	v->pfunc_fstep = pfunc_fstep;
}
static inline void append_label(Cvar *v, cstr l) {
	assert(our_vars.n_snodes < countof(our_vars.snodes));
	Snode *s = our_vars.snodes + our_vars.n_snodes++;
	s->sval = l;
	s->snext = 0;
	*v->labels_ptail = s;
	v->labels_ptail = &s->snext;
	v->cvflags |= CVF_MULTI;
}
#define cvset_labels(...) _cvset_labels((cstr []){__VA_ARGS__, 0})
static inline void _cvset_labels(cstr *l) {
	Cvar *v = lastcv();
	for (cstr *p = l; *p; ++p) {
		append_label(v, *p);
	}
}
#define cvar_bool(v, sn, pfunc_set, vdef, sdesc, ldesc, tab) _init_cvar(&(Cvar){.cvdtype=CVD_BOOL, .name=#v, .shortname=sn, .pint=&v, .pfunc_setb=pfunc_set}, vdef, sdesc, ldesc, tab)
#define cvar_int(v, sn, pfunc_set, vdef, vmin, vmax, sdesc, ldesc, tab) _init_cvar(&(Cvar){.cvdtype=CVD_INT, .name=#v, .shortname=sn, .pint=&v, .imin=vmin, .imax=vmax, .pfunc_seti=pfunc_set}, vdef, sdesc, ldesc, tab)
#define cvar_string(v, sn, pfunc_set, vdef, sdesc, ldesc, tab) _init_cvar(&(Cvar){.cvdtype=CVD_STRING, .name=#v, .shortname=sn, .pstr=v, .pfunc_sets=pfunc_set}, vdef, sdesc, ldesc, tab)
#define cvar_float(v, sn, pfunc_set, vdef, vmin, vmax, vstep, sdesc, ldesc, tab) _init_cvar(&(Cvar){.cvdtype=CVD_FLOAT, .name=#v, .shortname=sn, .pfloat=&v, .fmin=vmin, .fmax=vmax, .fstep=vstep, .pfunc_setf=pfunc_set}, vdef, sdesc, ldesc, tab)
static void _init_cvar(Cvar *d, float vdef, char *short_desc, char *long_desc, int tab_id) {
	assert(our_vars.n_cvars < countof(our_vars.cvars));
	Cvar *v = our_vars.cvars + our_vars.n_cvars++;
	*v = *d;
	v->name_len = strlen(v->name);
	v->shortname_len = strlen(v->shortname);
	add_options_distringid(v->name, &v->display, short_desc, long_desc);
	v->widgets.tab_id = tab_id;
	int t = v->cvdtype;
	if (t == CVD_INT || t == CVD_BOOL) {
		*v->pint = vdef;
	} else if (t == CVD_STRING) {
		v->slen = vdef;
	} else if (t == CVD_FLOAT) {
		*v->pfloat = vdef;
	}
	v->labels_ptail = &v->labels;
}
static inline void add_multi_option_lastcv(cstr s) {
	append_label(lastcv(), s);
}
void add_multi_option(cstr name, cstr label) {
	int i = find_var(name, VNK_INI);
	if (i == -1) {
		LOG_ERROR("Can't find ini var '%s'", name);
	} else {
		append_label(our_vars.cvars + i, label);
	}
}

//ELC specific variables
#ifdef ELC
static void init_ELC_vars(void) {
	int i;
	// CONTROLS TAB
	cvar_bool(sit_lock,"sl",change_var,0,"Sit Lock","Enable this to prevent your character from moving by accident when you are sitting.",CONTROLS);
	cvar_bool(always_pathfinding,"apf", change_var, 0, "Extend the range of the walk cursor", "Extends the range of the walk cursor to as far as you can see.  Using this option, movement may be slightly less responsive on larger maps.", CONTROLS);
	cvar_bool(use_floating_messages,"fmsg", change_var, 1, "Floating Messages", "Toggles the use of floating experience messages and other graphical enhancements", CONTROLS);
	cvar_bool(floating_session_counters,"fsc", change_var, 0, "Floating Session Counters", "Toggles the display of floating session counters.  Configure each type using the context menu of the counter category.", CONTROLS);
	cvar_bool(info_combat_console,"icc", change_var, 0, "Info-combat console", "Activer les messages d'infos en combat, dans la console", CONTROLS);
	cvar_bool(info_combat_float_msg,"icfm", change_var, 0, "Info-combat messages flottants", "Voir les messages flottants sur les informations au combat.", CONTROLS);
	cvar_bool(exphits.show,"ehs", change_var, 1, "Info-combat compteurs d'expérience", "Montrer le nombre de coups qui donnent de l'expérience durant le combat.", CONTROLS);
#ifdef SELECT_WITH_MOUSE_ON_BANNER
	cvar_bool(select_with_mouse_on_banner,"smb", change_var, 0, "Sélection au survol de la bannière", "Permet d'attaquer un monstre ou de lancer un dialogue avec un pnj en cliquant sur son nom ou sa barre de point de vie", CONTROLS);
#endif //SELECT_WITH_MOUSE_ON_BANNER
#ifdef WALK_AFTER_SPELL_FR
	cvar_bool(walk_after_spell,"walksp", change_var, 0, "Lancer un sort ne vous fait pas vous arréter", "Permet de lancer un sort et de continuer son chemin, peut causer des troubles durant le combat (fuite non désirée)", CONTROLS);
#endif //WALK_AFTER_SPELL_FR
	cvar_bool(use_keypress_dialog_boxes,"keydiag", change_var, 0, "Keypresses in dialogue boxes", "Toggles the ability to press a key to select a menu option in dialogue boxes (eg The Wraith)", CONTROLS);
	cvar_bool(use_full_dialogue_window,"fulldiag", change_var, 0, "Keypresses allowed anywhere in dialogue boxes", "If set, the above will work anywhere in the Dialogue Window, if unset only on the NPC's face", CONTROLS);
	cvar_bool(disable_double_click,"ddc", change_var, 0, "Disable double-click button safety", "Some buttons are protected from mis-click by requiring you to double-click them.  This option disables that protection.", CONTROLS);
#ifdef ACHIEVEMENTS
	cvar_bool(achievements_ctrl_click,"acc", change_var, 0, "Control click required to view achievements", "To view a players achievements, you click on them with the eye cursor.  With this option enabled, you must use Ctrl+click.", CONTROLS);
#endif //ACHIEVEMENTS
	cvar_int(mouse_limit,"ml",change_int,15,1, INT_MAX,"Mouse Limit","You can increase the mouse sensitivity and cursor changing by adjusting this number to lower numbers, but usually the FPS will drop as well!",CONTROLS);
#ifdef OSX
	cvar_bool(osx_right_mouse_cam,"osxrmc", change_var,0,"Rotate Camera with right mouse button", "Allows to rotate the camera by pressing the right mouse button and dragging the cursor", CONTROLS);
	cvar_bool(emulate_3_button_mouse,"e3bm", change_var,0,"Emulate a 3 Button Mouse", "If you have a 1 Button Mouse you can use <apple> click to emulate a rightclick. Needs client restart.", CONTROLS);
#endif // OSX
#ifdef NEW_CURSOR
	cvar_bool(sdl_cursors,"scur", change_sdl_cursor,1,"Old Style Pointers", "Use default SDL cursor.", CONTROLS);
	cvar_bool(big_cursors,"bcur", change_var,0,"Big Pointers", "Use 32x32 graphics for pointer. Only works with SDL cursor turned off.", CONTROLS);
	cvar_float(pointer_size,"psz", change_float,1.0,"Pointer Size", "Scale the pointer. 1.0 is 1:1 scale with pointer graphic. Only works with SDL cursor turned off.", CONTROLS,0.25,4.0,0.05);
#endif // NEW_CURSOR
	cvar_int(trade_log_mode,"tlm",change_int, TRADE_LOG_NONE,0,0,"Trade log","Set how successful trades are logged.",CONTROLS);
		cvset_labels("Aucun enregistrement", "Log en console", "Log dans un fichier", "Log en console et fichier");
	// CONTROLS TAB

	// HUD TAB
	cvar_bool(show_fps,"fps",change_var,1,"Show FPS","Show the current frames per second in the corner of the window",TROUBLESHOOT);
	cvar_bool(view_analog_clock,"aclk",change_var,1,"Analog Clock","Toggle the analog clock",HUD);
	cvar_bool(view_digital_clock,"dclk",change_var,1,"Digital Clock","Toggle the digital clock",HUD);
	cvar_bool(view_knowledge_bar,"kbar",change_var,1,"Knowledge Bar","Toggle the knowledge bar",HUD);
	cvar_bool(view_hud_timer,"hudtime",change_var,1,"Countdown/Stopwatch Timer","Timer controls: Right-click for menu. Shift-left-click to toggle mode. Left-click to start/stop. Mouse wheel to reset, up/down to change countdown start time (+ctrl/alt to change step).",HUD);
	cvar_bool(show_game_seconds,"gsecs",change_var,0,"Show Game Seconds","Show seconds on the digital clock. Note: the seconds displayed are computed on client side and synchronized with the server at each new minute.",HUD);
	cvar_bool(show_stats_in_hud,"hudstat",change_var,0,"Stats In HUD","Toggle showing stats in the HUD",HUD);
	cvar_bool(show_statbars_in_hud,"statbars",change_var,0,"StatBars In HUD","Toggle showing statbars in the HUD. Needs Stats in HUD",HUD);
	cvar_bool(logo_click_to_url,"logoclk",change_var,0,"Logo Click To URL","Toggle clicking the LOGO opening a browser window",HUD);
	cvar_string(logo_link, "logolink", change_string, 128, "Logo Link", "URL when clicking the logo", HUD);
	cvar_bool(show_help_text,"help",change_var,1,"Help Text","Enable tooltips.",HUD);
	cvar_bool(always_enlarge_text,"aetext",change_var,1,"Always Enlarge Text","Some text can be enlarged by pressing ALT or CTRL, often only while the mouse is over it.  Setting this option effectively locks the ALT/CTRL state to on.",HUD);
	cvar_bool(show_item_desc_text,"itemdesc",change_var,1,"Item Description Text","Enable item description tooltips. Needs item_info.txt file.",HUD);
	cvar_bool(use_alpha_border,"alphabord", change_var, 1,"Alpha Border","Toggle the use of alpha borders",HUD);	//ADVVID);
	cvar_string(titre_theme,"theme",change_string,20,"Thème","Sélection du thème de l'interface (relancer le client pour appliquer)",HUD);
		cvset_flags(CVF_INI);
	cvar_float(chat_alpha_background,"alphachat", change_float, 0.5,0.0, 1.0, 0.01, "Fond transparent des messages", "Sélection du niveau de transparence du fond derrière les messages en vue 3D", HUD);
	cvar_bool(use_alpha_banner,"alphabanner", change_var, 0,"Alpha Behind Name/Health Text","Toggle the use of an alpha background to name/health banners",HUD);
	cvar_bool(cm_banner_disabled,"cmbanner", change_var, 0,"Disable Name/Health Text Context Menu","Disable the context menu on your players name/health banner.",HUD);
	cvar_bool(windows_on_top,"wot", change_windows_on_top, 0, "Windows On Top","Allows the Manufacture, Storage and Inventory windows to appear above the map and console.", HUD);
	cvar_bool(opaque_window_backgrounds,"opaquewin", change_var, 0,"Use Opaque Window Backgrounds","Toggle the current state of all windows between transparent and opaque background. Use CTRL+D to toggle the current state of an individual window.",HUD);
	cvar_int(exp_log_threshold,"explt",change_int,5000,0, INT_MAX,"Log exp gain to console", "If you gain experience of this value or over, then a console message will be written.  Set the value to zero to disable completely.",HUD);
	cvar_bool(map_3d_markers,"marks3d",change_3d_marks,1,"Enable 3D Map Markers","Shows user map markers in the game window",HUD);
	cvar_bool(item_window_on_drop,"itemdrop",change_var,1,"Item Window On Drop","Toggle whether the item window shows when you drop items",HUD);
	cvar_float(minimap_scale,"mmapscale", change_minimap_scale, 0.7,0.5, 1.5, 0.1, "Minimap Scale", "Adjust the overall size of the minimap", CONTROLS);
	cvar_bool(minimap_lancement,"mmapstart",change_var,0,"Minicarte au lancement","Lors du lancement du jeu, affichage de la minicarte.",CONTROLS);
	cvar_bool(pin_minimap,"pinmmap",change_var,0,"Pin Minimap","Toggle whether the minimap ignores close-all-windows.",CONTROLS);
	cvar_bool(rotate_minimap,"rmmap",change_var,1,"Rotate Minimap","Toggle whether the minimap should rotate.",CONTROLS);
	cvar_bool(rot_boussole,"rcomp", change_var, 0, "Rotation boussole", "Permet de changer le type de rotation de la boussole.", CONTROLS);
	cvar_bool(continent_map_boundaries,"cmb", change_var, 1, "Map Boundaries On Continent Map", "Show map boundaries on the continent map", HUD);
	cvar_bool(enable_user_menus,"usermenus", toggle_user_menus, 0, "Enable User Menus","Create .menu files in your config directory.  First line is the menu name. After that, each line is a command using the format \"Menus Text || command || command\".  Prompt for input using \"command text <prompt text>\".",HUD);
#ifdef WITHDRAW_LIST
	cvar_int(min_time_between_withdraw,"mtwithdraw",change_int,100,0,500,"Temps entre deux récupérations d'item","Change la valeur entre deux récupération d'item dans la fenêtre de liste objet",HUD);
#endif //WITHDRAW_LIST
#ifdef SHOW_ATTR_BOOSTED
	cvar_bool(show_attr_boosted,"saboost", change_var, 0, "Afficher les attributs", "Affiche les attributs dans le HUD", HUD);
#endif //SHOW_ATTR_BOOSTED
#ifdef DISPLAY_MANAPOINT
	cvar_bool(view_mp,"mp", change_var, 0, "Afficher les points de mana", "Affiche les points de mana au dessus de ton personnage", HUD);
	cvar_bool(view_mana_bar,"mbar", change_var, 0, "Afficher la barre de mana", "Affiche la barre de mana au dessus de ton personnage", HUD);
#endif //DISPLAY_MANAPOINT
#ifdef SHOW_COORD_SETTER
	cvar_bool(show_coord_2,"sc2", change_var, 0, "Afficher les coordonées", "Affiche vos coordonées dans le HUD", HUD);
#endif //SHOW_COORD_SETTER
#if !defined(WINDOWS) && !defined(OSX)
	cvar_bool(use_clipboard,"uclb", change_var, 1, "Use Clipboard For Pasting", "Use CLIPBOARD for pasting (as e.g. GNOME does) or use PRIMARY cutbuffer (as xterm does)",HUD);
#endif
	// HUD TAB

	cvar_int(quickitems_size,"qisz",change_quickitems_size,6,1, FR_QUICKITEMS_MAXSIZE,"Taille de la barre d'objets","Permet d'augmenter la longueur de votre barre d'accès rapide aux objets",HUD);
	cvar_int(quickspells_size,"qssz",change_quickspells_size,6,1, QUICKSPELLS_MAXSIZE,"Taille de la barre de sorts","Permet d'augmenter la longueur de votre barre de lancement rapide de 1 jusqu'à 12 sorts",HUD);
	cvar_bool(affichage_barres,"hudbord",change_affichage_barre,1,"Affichage des bordures","Permet d'afficher ou non les bordures sur le bord de l'écran (Touche F6)",HUD);
		cvset_flags(CVF_INI);

	// CHAT TAB
	cvar_int(windowed_chat, "winchat", change_windowed_chat, 1,0,2, "Gestion des messages", "Sélection du mode d'affichage des messages.", CHAT);
		cvset_labels("Ancien comportement", "Onglets", "Fenêtre");
	cvar_bool(local_chat_separate,"locsep", change_separate_flag, 0, "Separate Local Chat", "Should local chat be separate?", CHAT);
	cvar_bool(guild_chat_separate,"gmsep", change_separate_flag, 1, "Separate Guild Chat", "Should guild chat be separate?", CHAT);
	cvar_bool(server_chat_separate,"scsep", change_separate_flag, 0, "Separate Server Messages", "Should the messages from the server be separate?", CHAT);
	cvar_bool(mod_chat_separate,"modsep", change_separate_flag, 0, "Separate Moderator Chat", "Should moderator chat be separated from the rest?", CHAT);
	cvar_bool(dev_chat_separate,"devsep", change_var, 0, "Separate Dev Chat", "Should dev chat be separated from the rest?", CHAT);
	cvar_bool(coord_chat_separate,"coordsep", change_var, 0, "Separate Coord Chat", "Should coord chat be separated from the rest?", CHAT);
	cvar_bool(highlight_tab_on_nick,"highlight", change_var, 1, "Highlight Tabs On Name", "Should tabs be highlighted when someone mentions your name?", CHAT);
	cvar_int(time_warning_hour,"warnh",change_int,-1,-1, 30,"Time warning for new hour","If set to -1, there will be no warning given. Otherwise, you will get a notification in console this many minutes before the new hour",CHAT);
	cvar_int(time_warning_sun,"warns",change_int,-1,-1, 30,"Time warning for dawn/dusk","If set to -1, there will be no warning given. Otherwise, you will get a notification in console this many minutes before sunrise/sunset",CHAT);
	cvar_int(time_warning_day,"warnd",change_int,-1,-1, 30,"Time warning for new #day","If set to -1, there will be no warning given. Otherwise, you will get a notification in console this many minutes before the new day",CHAT);
	cvar_int(auto_afk_time,"afkt",set_afk_time,5,0, INT_MAX,"AFK Time","The idle time in minutes before the AFK auto message",CHAT);
	cvar_string(afk_message,"afkm",change_string,127,"AFK Message","Set the AFK message",CHAT);
	cvar_bool(afk_local,"afkl", change_var, 0, "Save Local Chat Messages When AFK", "When you go AFK, local chat messages are counted and saved as well as PMs", CHAT);
#ifdef NEW_SOUND
	cvar_bool(afk_snd_warning,"afks", change_var, 0, "Play AFK Message Sound", "When you go AFK, a sound is played when you receive a message or trade request", CHAT);
#endif	//NEW_SOUND
	cvar_bool(use_global_ignores,"gign",change_var,1,"Global Ignores","Global ignores is a list with people that are well known for being nasty, so we put them into a list (global_ignores.txt). Enable this to load that list on startup.",CHAT);
	cvar_bool(save_ignores,"sign",change_var,1,"Save Ignores","Toggle saving of the local ignores list on exit.",CHAT);
	cvar_bool(use_global_filters,"gfil", change_global_filters, 1, "Global Filter", "Toggle the use of global text filters.", CHAT);
	cvar_bool(caps_filter,"caps",change_var,1,"Caps Filter","Toggle the caps filter",CHAT);
	cvar_bool(show_timestamp,"ts",change_var,0,"Horodatage des messages","Voir les heures sur les canaux de discussions",CHAT);
	cvar_int(max_lines_to_show,"mls",change_lines_to_show,10,0,100,"Lines de l'historique en vue 3D","Nombre maximal de lines de l'historique à afficher en haut de la vue 3D.",CHAT);
	cvar_float(scroll_off_secs,"sos",change_float,3,0,3600,0.1,"Défilement automatique des messages","Secondes entre le défilement automatique des messages de l'historique affichés en haut de la vue 3D. Si zéro, les messages restent toujours visibles.",CHAT);
	cvar_int(dark_channeltext,"dctext",change_dark_channeltext,0,0,2,"Couleur texte console","Affiche le texte dans une couleur noire sur les canaux pour un meilleur affichage sur les cartes claires (peut-être difficilement visible en mode consolle F1)",CHAT);
		cvset_labels("Normal", "Moyen", "Noir");
		cvset_flags(CVF_HORIZ);
	cvar_int(dedup_lookback,"dedup",change_int,10,0,100,"Fusionnement des messages similaires","Nombre de messages passés à tester pour la similarité. Zéro désactive. Messages similaires ne sont pas affichés; ils reçoivent un suffixe de répétitions.",CHAT);
	// CHAT TAB

	// FONT TAB
	cvar_float(chat_text_size,"csize",change_chat_zoom,1,0.0, FLT_MAX, 0.01,"Chat Text Size","Sets the size of the normal text",FONT);
	cvar_float(name_text_size,"nsize",change_float,1,0.0, 2.0, 0.01,"Name Text Size","Set the size of the players name text",FONT);
	cvar_float(taille_titre_texte,"titlesz", change_float, 1,0.0, 2.0, 0.01, "Taille des titres", "Permet de modifier la taille des titres", FONT);
	cvar_float(book_text_size,"booksz", change_book_zoom, 1,0.0, FLT_MAX, 0.01, "Taille des textes des livres", "Permet de modifier le taille des textes des livres.", FONT);
	cvar_float(note_text_size,"notesz", change_note_zoom, 0.8,0.0, FLT_MAX, 0.01, "Notepad Text Size","Sets the size of the text in the notepad", FONT);
	cvar_float(mapmark_text_size,"marksz", change_float, 0.3,0.0, FLT_MAX, 0.01, "Mapmark Text Size","Sets the size of the mapmark text", FONT);
	cvar_int(chat_font,"cfont",change_chat_font,0,0,INT_MAX,"Chat Font","Set the type of font used for normal text",FONT);
	cvar_int(name_font,"nfont",change_int,0,0,INT_MAX,"Name Font","Change the type of font used for the name",FONT);
	// FONT TAB

	// SERVER TAB
	cvar_string(username,"u",change_string,MAX_USERNAME_LENGTH,"Username","Your user name here",SERVER);
	cvar_string(password,"p",change_string,MAX_USERNAME_LENGTH,"Password","Put your password here",SERVER);
		cvset_flags(CVF_PASSWORD);
	cvar_int(log_chat,"log",change_int,LOG_SERVER,0,3,"Log Messages","Log messages from the server (chat, harvesting events, GMs, etc)",SERVER);
		cvset_labels("Pas discussions", "Seulement discussions", "Messages serveur", "Serveur dans srv_log.txt");
	cvar_bool(buddy_log_notice,"budlog", change_var, 1, "Log Buddy Sign On/Off", "Toggle whether to display notices when people on your buddy list log on or off", SERVER);
	cvar_string(language,"lang",change_string,8,"Language","Wah?",SERVER);
	cvar_string(browser,"b",change_string,70,"Browser","Location of your web browser (Windows users leave blank to use default browser)",SERVER);
	cvar_bool(write_ini_on_exit,"wini", change_var, 1,"Save INI","Save options when you quit",SERVER);
	cvar_string(data_dir,"dir",change_dir_name,90,"Data Directory","Place were we keep our data. Can only be changed with a Client restart.",SERVER);
	cvar_bool(serverpopup,"spu",change_var,1,"Use Special Text Window","Toggles whether server messages from channel 255 are displayed in a pop up window.",SERVER);
	cvar_int(delai_sauve,"savedly", change_delai_sauvegarde, 30,10, INT_MAX, "Délai pour la sauvegarde", "Durée en minutes entre 2 sauvegardes automatiques", SERVER);
	 /* Note: We don't take any action on the already-running thread, as that wouldn't necessarily be good. */
	cvar_bool(autoupdate,"aup",change_var,1,"Automatic Updates","Toggles whether updates are automatically downloaded.",SERVER);
#ifdef CUSTOM_UPDATE
	cvar_bool(custom_update,"cup",change_custom_update,1,"Custom Looks Updates","Toggles whether custom look updates are automatically downloaded.",SERVER);
	cvar_bool(custom_clothing,"scc",change_custom_clothing,1,"Show Custom clothing","Toggles whether custom clothing is shown.",SERVER);
#endif	//CUSTOM_UPDATE
	// SERVER TAB

	// AUDIO TAB
#ifdef NEW_SOUND
	cvar_bool(disable_sound,"nosnd", stop_all_sound, 0, "Disable Sound & Music System", "Disable all of the sound effects and music processing", AUDIO);
	cvar_string(sound_device, "snddev", change_string, 30, "Sound Device", "Device used for playing sounds & music", AUDIO);
	cvar_bool(enable_sound,"sndeff", toggle_sounds, 0, "Enable Sound Effects", "Turn sound effects on/off", AUDIO);
	cvar_float(sound_gain,"sgain", change_sound_level, 1,0.0, 1.0, 0.1, "Overall Sound Effects Volume", "Adjust the overall sound effects volume", AUDIO);
	cvar_float(crowd_gain,"crgain", change_sound_level, 1,0.0, 1.0, 0.1, "Crowd Sounds Volume", "Adjust the crowd sound effects volume", AUDIO);
	cvar_float(enviro_gain,"envgain", change_sound_level, 1,0.0, 1.0, 0.1, "Environmental Sounds Volume", "Adjust the environmental sound effects volume", AUDIO);
	cvar_float(actor_gain,"again", change_sound_level, 1,0.0, 1.0, 0.1, "Character Sounds Volume", "Adjust the sound effects volume for fighting, magic and other character sounds", AUDIO);
	cvar_float(walking_gain,"wgain", change_sound_level, 1,0.0, 1.0, 0.1, "Walking Sounds Volume", "Adjust the walking sound effects volume", AUDIO);
	cvar_float(gamewin_gain,"gwgain", change_sound_level, 1,0.0, 1.0, 0.1, "Item and Inventory Sounds Volume", "Adjust the item and inventory sound effects volume", AUDIO);
	cvar_float(client_gain,"clgain", change_sound_level, 1,0.0, 1.0, 0.1, "Misc Client Sounds Volume", "Adjust the client sound effects volume (warnings, hud/button clicks)", AUDIO);
	cvar_float(warn_gain,"wrngain", change_sound_level, 1,0.0, 1.0, 0.1, "Text Warning Sounds Volume", "Adjust the user configured text warning sound effects volume", AUDIO);
	cvar_bool(enable_music,"music",toggle_music,0,"Enable Music","Turn music on/off",AUDIO);
	cvar_float(music_gain,"mgain",change_sound_level,1,0.0, 1.0, 0.1,"Music Volume","Adjust the music volume",AUDIO);
#endif	//NEW_SOUND
	// AUDIO TAB

	// VIDEO TAB
	cvar_bool(full_screen,"fs",toggle_full_screen_mode,0,"Full Screen","Changes between full screen and windowed mode",VIDEO);
	cvar_int(video_mode,"vid",switch_vidmode,4,0,INT_MAX,"Video Mode","The video mode you wish to use",VIDEO);
		cvset_labels("Préférence utilisateur");
	for (i = 0; i < video_modes_count; i++) {
		char s[100];
		safe_snprintf(s, sizeof(s), "%dx%dx%d", video_modes[i].width, video_modes[i].height, video_modes[i].bpp);
		if (video_modes[i].name)
			free(video_modes[i].name);
		video_modes[i].name = strdup(s);
		add_multi_option_lastcv(video_modes[i].name);
	}
	cvar_bool(disable_window_adjustment,"nowinadj", change_var, 0, "Disable window size adjustment","Disables the window size adjustment on video mode changes in windowed mode.", VIDEO);
	cvar_int(video_width,"width",change_int, 640,640, INT_MAX,"Userdefined width","Userdefined window width",VIDEO);
	cvar_int(video_height,"height",change_int, 480,480, INT_MAX,"Userdefined height","Userdefined window height",VIDEO);
	cvar_int(limit_fps,"lfps",change_int,0,0, INT_MAX,"Limit FPS","Limit the frame rate to reduce load on the system",VIDEO);
	cvar_float(video_gamma,"g",change_gamma,1,0.10, 3.00, 0.05,"Luminosité (gamma)","Règle la luminosité de l'écran (gamma).",VIDEO);
	cvar_bool(disable_gamma_adjust,"dga",change_var,0,"Disable Gamma Adjustment","Stop the client from adjusting the display video_gamma.",VIDEO);
#ifdef ANTI_ALIAS
	cvar_bool(anti_alias,"aa", change_aa, 0, "Toggle Anti-Aliasing", "Anti-aliasing makes edges look smoother", VIDEO);
#endif //ANTI_ALIAS
#ifdef	FSAA
	cvar_int(anti_aliasing, "fsaa", change_fsaa, 0, 0, INT_MAX, "Anti-Aliasing", "Full Scene Anti-Aliasing", VIDEO);
		cvset_flags(CVF_HORIZ);
		cvset_labels(get_fsaa_mode_str(0));
	for (i = 1; i < get_fsaa_mode_count(); i++) {
		if (get_fsaa_mode(i) == 1) {
			add_multi_option_lastcv(get_fsaa_mode_str(i));
		}
	}
#endif	/* FSAA */
	cvar_bool(use_frame_buffer,"fb", change_frame_buffer, 0, "Toggle Frame Buffer Support", "Toggle frame buffer support. Used for reflection and shadow mapping.", VIDEO);
	cvar_int(water_shader_quality,"wsq",change_water_shader_quality,1,0,INT_MAX,"  water shader quality","Defines what shader is used for water rendering. Higher values are slower but look better. Needs \"toggle frame buffer support\" to be turned on.",VIDEO);
		cvset_immfuncs(int_zero_func, int_max_water_shader_quality);
#ifdef	NEW_TEXTURES
	cvar_bool(small_actor_texture_cache,"satc",change_small_actor_texture_cache,0,"Small actor texture cache","A small Actor texture cache uses less video memory, but actor loading can be slower.",VIDEO);
#else	/* NEW_TEXTURES */
	cvar_bool(use_mipmaps,"mm",change_mipmaps,0,"Mipmaps","Mipmaps is a texture effect that blurs the texture a bit - it may look smoother and better, or it may look worse depending on your graphics driver settings and the like.",VIDEO);
#endif	/* NEW_TEXTURES */
	cvar_bool(use_vertex_buffers,"vbo",change_vertex_buffers,0,"Vertex Buffer Objects","Toggle the use of the vertex buffer objects, restart required to activate it",VIDEO);
	cvar_bool(video_info_sent, "svi", change_var, 0, "Video info sent", "Video information are sent to the server (like OpenGL version and OpenGL extentions)", VIDEO);
		cvset_flags(CVF_INI);
	// VIDEO TAB

	// GFX TAB
	cvar_bool(shadows_on,"shad",change_shadows,0,"Shadows","Toggles the shadows", GFX);
	cvar_bool(use_shadow_mapping,"sm", change_shadow_mapping, 0, "Shadow Mapping", "If you want to use some better quality shadows, enable this. It will use more resources, but look prettier.", GFX);
	cvar_int(shadow_map_size,"smsize",change_shadow_map_size,1024,256,4096,"Shadow Map Size","This parameter determines the quality of the shadow maps. You should as minimum set it to 512.",GFX);
		cvset_labels("256","512","768","1024","1280","1536","1792","2048","3072","4096");
	cvar_bool(no_adjust_shadows,"noadj",change_var,0,"Don't Adjust Shadows","If enabled, tell the engine not to disable the shadows if the frame rate is too low.",GFX);
	cvar_bool(clouds_shadows,"cshad",change_clouds_shadows,1,"Cloud Shadows","The clouds shadows are projected on the ground, and the game looks nicer with them on.",GFX);
	cvar_bool(show_reflection,"refl",change_reflection,1,"Show Reflections","Toggle the reflections",GFX);
	cvar_bool(render_fog,"fog",change_var,1,"Render Fog","Toggles fog rendering.",GFX);
	cvar_bool(show_weather,"wthr",change_var,1,"Show Weather Effects","Toggles thunder, lightning and rain effects.",GFX);
	cvar_bool(skybox_show_sky,"sky", change_sky_var,1,"Show Sky", "Enable the sky box.", GFX);
/* 	cvar_bool(reflect_sky,"reflect_sky", change_var,1,"Reflect Sky", "Sky Performance Option. Disable these from top to bottom until you're happy", GFX); */
	cvar_bool(skybox_show_clouds,"clds", change_sky_var,1,"Show Clouds", "Sky Performance Option. Disable these from top to bottom until you're happy", GFX);
/*	cvar_bool(horizon_fog,"horizon_fog", change_sky_var,1,"Show Horizon Fog", "Sky Performance Option. Disable these from top to bottom until you're happy", GFX); */
	cvar_bool(skybox_show_sun,"sun", change_sky_var,1,"Show Sun", "Sky Performance Option. Disable these from top to bottom until you're happy", GFX);
	cvar_bool(skybox_show_moons,"moons", change_sky_var,1,"Show Moons", "Sky Performance Option. Disable these from top to bottom until you're happy", GFX);
	cvar_bool(skybox_show_stars,"stars", change_sky_var,1,"Show Stars", "Sky Performance Option. Disable these from top to bottom until you're happy", GFX);
	cvar_int(skybox_update_delay,"skyup", change_int, skybox_update_delay,0, 60, "Sky Update Delay", "Specifies the delay in seconds between 2 updates of the sky and the environment. A value of 0 corresponds to an update at every frame.", GFX);
	cvar_int(particles_percentage,"pp",change_particles_percentage,100,0, 100,"Particle Percentage","If you experience a significant slowdown when particles are nearby, you should consider lowering this number.",GFX);
	cvar_bool(special_effects,"sfx", change_var, 1, "Toggle Special Effects", "Special spell effects", GFX);
#ifdef	NEW_TEXTURES
	cvar_bool(use_eye_candy,"ec", change_eye_candy, 1, "Enable Eye Candy", "Toggles most visual effects, like spells' and harvesting events'. Needs OpenGL 1.5", GFX);
#else	/* NEW_TEXTURES */
	cvar_bool(use_eye_candy,"ec", change_var, 1, "Enable Eye Candy", "Toggles most visual effects, like spells' and harvesting events'", GFX);
#endif	/* NEW_TEXTURES */
	cvar_bool(enable_blood,"eb",change_var,0,"Enable Blood","Enable blood special effects during combat.",GFX);
	cvar_bool(use_harvesting_eye_candy,"uharvec",change_var,0,"Enable harvesting effect","This effect shows that you're harvesting. Only you can see it!",GFX);
	cvar_bool(use_lamp_halo,"ulh",change_var,0,"Use Lamp Halos","Enable halos for torches, candles, etc.",GFX);
	cvar_bool(use_fancy_smoke,"ufs",change_var,0,"Use Fancy Smoke","If your system has performance problems around chimney smoke, turn this option off.",GFX);
	cvar_float(max_ec_framerate,"ecmaxf",change_max_ec_framerate,45,2.0, FLT_MAX, 1.0,"Max Effects Framerate","If your framerate is above this amount, eye candy will use maximum detail.",GFX);
	cvar_float(min_ec_framerate,"ecminf",change_min_ec_framerate,15,1.0, FLT_MAX, 1.0,"Min Effects Framerate","If your framerate is below this amount, eye candy will use minimum detail.",GFX);
	cvar_int(light_columns_threshold,"lct",change_int,5,0, INT_MAX,"Light columns threshold","If your framerate is below this amount, you will not get columns of light around teleportation effects (useful for slow systems).",GFX);
	cvar_int(max_idle_cycles_per_second,"micps",change_int,40,1, INT_MAX,"Max Idle Cycles Per Second","The eye candy 'idle' function, which moves particles around, will run no more than this often.  If your CPU is your limiting factor, lowering this can give you a higher framerate.  Raising it gives smoother particle motion (up to the limit of your framerate).",GFX);
#ifdef	NEW_ALPHA
	cvar_bool(use_3d_alpha_blend,"3dalpha",change_var,1,"3D Alpha Blending","Toggle the use of the alpha blending on 3D objects",GFX);
#endif	//NEW_ALPHA
	// GFX TAB

	// CAMERA TAB
	cvar_float(far_plane,"fp", change_projection_float, 100.0,40.0, 200.0, 1.0, "Maximum Viewing Distance", "Adjusts how far you can see.", CAMERA);
	cvar_float(far_reflection_plane,"frp", change_projection_float, 100.0,0.0, 200.0, 1.0, "Maximum Reflection Distance", "Adjusts how far the reflections are displayed.", CAMERA);
	cvar_float(max_zoom_level,"mzl",change_float,max_zoom_level,4.0, 8.0, 0.5,"Maximum Camera Zoom Out","Sets the maxiumum value that the camera can zoom out",CAMERA);
	cvar_float(perspective,"persp", change_projection_float, 0.15f,0.01, 0.80, 0.01, "Perspective", "The degree of perspective distortion. Change if your view looks odd.", CAMERA);
	cvar_bool(isometric,"isom", change_projection_bool, 1, "Use Isometric View", "Toggle the use of isometric (instead of perspective) view", CAMERA);
	cvar_bool(follow_cam,"folcam", toggle_follow_cam,0,"Follow Camera", "Causes the camera to stay fixed relative to YOU and not the world", CAMERA);
	cvar_bool(fol_cam_behind,"fcb", toggle_follow_cam_behind,0,"Keep the camera behind the char", "Causes the camera to stay behind you while walking (works only in follow camera mode)", CAMERA);
	cvar_bool(extended_cam,"extcam", toggle_ext_cam,0,"Extended Camera", "Camera range of motion extended and adjusted to allow overhead and first person style camera.", CAMERA);
	cvar_bool(ext_cam_auto_zoom,"autozoom", change_var,0,"Auto zoom", "Allows the camera to zoom automatically when getting close to the ground (works only in extended camera mode and with a max tilt angle over 90.0).", CAMERA);
	cvar_float(normal_camera_rotation_speed,"nrot",change_float,15,1.0,FLT_MAX,0.5,"Camera Rotation Speed","Set the speed the camera rotates",CAMERA);
	cvar_float(fine_camera_rotation_speed,"frot",change_float,1,1.0,FLT_MAX,0.5,"Fine Rotation Speed","Set the fine camera rotation speed (when holding shift+arrow key)",CAMERA);
	cvar_float(normal_camera_deceleration,"ncd",change_float,normal_camera_deceleration,0.01,1.0,0.01,"Camera Rotation Deceleration","Set the camera rotation deceleration",CAMERA);
	cvar_float(min_tilt_angle,"mintilt", change_tilt_float,30.0,20.0, 45.0, 1.0,"Minimum tilt angle", "Minimum angle that the camera can reach when raising it (works only in extended camera mode).", CAMERA);
	cvar_float(max_tilt_angle,"maxtilt", change_tilt_float,90.0,60.0, 150.0, 1.0,"Maximum tilt angle", "Maximum angle that the camera can reach when lowering it (works only in extended camera mode).", CAMERA);
	cvar_float(follow_strength,"fstrn",change_float,0.1,0.0,1.00,0.01,"Follow Camera Snapiness","Adjust how responsive the follow camera is. 0 is stopped, 1 is fastest. Use the three numbers below to tweak the feel. Try them one at a time, then mix them to find a ratio you like.",CAMERA);
	cvar_float(const_speed,"fcon",change_float,7,0.0,10.00,1.0,"Constant Speed","The basic rate that the camera rotates at to keep up with you.",CAMERA);
	cvar_float(lin_speed,"flin",change_float,1,0.0,10.00,1.0,"Linear Decel.","A hit of speed that drops off as the camera gets near its set point.",CAMERA);
	cvar_float(quad_speed,"fquad",change_float,1,0.0,10.00,1.0,"Quadratic Decel.","A hit of speed that drops off faster as it nears the set point.",CAMERA);
	// CAMERA TAB

	// TROUBLESHOOT TAB
	//cvar_bool(shadows_on,"shad",change_shadows,0,"Shadow Bug","Some video cards have trouble with the shadows. Uncheck this if everything you see is white.", TROUBLESHOOT);
	// Grum: attempt to work around bug in Ati linux drivers.
	cvar_bool(ati_click_workaround,"atibug", change_var, 0, "ATI Bug", "If you are using an ATI graphics card and don't move when you click, try this option to work around a bug in their drivers.", TROUBLESHOOT);
	cvar_bool(use_old_clicker,"oldmc", change_var, 0, "Mouse Bug", "Unrelated to ATI graphics cards, if clicking to walk doesn't move you, try toggling this option.", TROUBLESHOOT);
	cvar_bool(use_new_selection,"uns", change_new_selection, 1, "New selection", "Using new selection can give you a higher framerate.  However, if your cursor does not change when over characters or items, try disabling this option.", TROUBLESHOOT);
	cvar_bool(use_compiled_vertex_array,"cva",change_compiled_vertex_array,1,"Compiled Vertex Array","Some systems will not support the new compiled vertex array in EL. Disable this if some 3D objects do not display correctly.",TROUBLESHOOT);
	cvar_bool(use_draw_range_elements,"dre",change_var,1,"Draw Range Elements","Disable this if objects appear partially stretched.",TROUBLESHOOT);
	cvar_bool(use_point_particles,"upp",change_point_particles,1,"Point Particles","Some systems will not support the new point based particles in EL. Disable this if your client complains about not having the point based particles extension.",TROUBLESHOOT);
	cvar_bool(use_loading_snapshot,"uls", change_var, 1, "Capture d'écran lors de chargement", "Utiliser une capture d'écran en arrière-plan lors de chargement d'une carte. À désactiver s'il ne s'affiche qu'une image blanche.", TROUBLESHOOT);
#ifndef	NEW_TEXTURES
	cvar_bool(transparency_resolution_fix,"trf",change_var,0,"Transparency Resolution Fix","Use this if your video card or driver has problems with rendering highly blended effects, like teleportation.",TROUBLESHOOT);
#endif	/* NEW_TEXTURES */
	cvar_int(gx_adjust,"gxa", change_signed_int, 0,-3,3, "Adjust graphics X","Fine adjustment for text/line positioning - X direction.",TROUBLESHOOT);
	cvar_int(gy_adjust,"gxa", change_signed_int, 0,-3,3, "Adjust graphics Y","Fine adjustment for text/line positioning - Y direction.",TROUBLESHOOT);
#ifdef OSX
	cvar_bool(square_buttons,"sqbut",change_var,1,"Square Buttons","Use square buttons rather than rounded",TROUBLESHOOT);
#endif
	cvar_bool(poor_man,"poor",change_poor_man,0,"Poor Man","If the game is running very slow for you, toggle this setting.",TROUBLESHOOT);
	// TROUBLESHOOT TAB

	// DEBUGTAB TAB
#ifdef DEBUG
	cvar_float(sunny_sky_bias,"ssbias", change_float,0.0,"Sunny sky bias", "Change the radius of the sun effect on the sky.", DEBUGTAB, -1.0, 1.0, 0.01);
	cvar_float(sunny_clouds_bias,"scbias", change_float,-0.1,"Sunny clouds bias", "Change the radius of the sun effect on the clouds.", DEBUGTAB, -1.0, 1.0, 0.01);
	cvar_float(sunny_fog_bias,"sfbias", change_float,0.0,"Sunny fog bias", "Change the radius of the sun effect on the fog.", DEBUGTAB, -1.0, 1.0, 0.01);
	cvar_float(water_tiles_extension,"wtext", change_float,200.0,"Water tiles extension", "Extends the water tiles upto the specified distance.", DEBUGTAB, 0.0, 1000.0, 1.0);
#ifdef MISSILES
	cvar_bool(enable_client_aiming,"eca",change_var,0,"Enable client aiming","Allow to aim at something by holding CTRL key. This aim is only done on client side and is used only for debugging purposes. Warning: enabling this code can produce server resyncs or locks when playing with missiles...",DEBUGTAB);
#endif //MISSILES
	cvar_bool(render_skeleton,"rskel",change_var,0,"Render Skeleton", "Render the Cal3d skeletons.", DEBUGTAB);
	cvar_bool(render_mesh,"rmesh",change_var,1,"Render Mesh", "Render the meshes", DEBUGTAB);
	cvar_bool(render_bones_id,"rbid",change_var,0,"Render bones ID", "Render the bones ID", DEBUGTAB);
	cvar_bool(render_bones_orientation,"rbor",change_var,0,"Render bones orientation", "Render the bones orientation", DEBUGTAB);
	cvar_float(near_plane,"np", change_projection_float, 0.1, "Minimum Viewing Distance", "Adjusts how near you can see.", DEBUGTAB, 0.1, 10.0, 0.1);
	cvar_bool(skybox_local_weather,"slocal", change_var,0,"Local Weather", "Show local weather areas on the sky. It allows to see distant weather but can reduce performance.", DEBUGTAB);
#endif // DEBUG
	// DEBUGTAB TAB
}
#endif // def ELC

void init_vars(void) {
#ifdef ELC
	init_ELC_vars();

#else
	// NOTE !!!!
	// some repeated in init_ELC_vars() so that we can control the order showin in the tabs

	//Global vars...
	// Only possible to do at startup - this could of course be changed by using a special function for this purpose. I just don't see why you'd want to change the directory whilst running the game...
	cvar_string(data_dir,"dir",change_dir_name,90,"Data Directory","Place were we keep our data. Can only be changed with a Client restart.",SERVER);
	cvar_int(limit_fps,"lfps",change_int,0,0,INT_MAX,"Limit FPS","Limit the frame rate to reduce load on the system",VIDEO);
#ifdef MAP_EDITOR
	cvar_int(video_mode,"vid",switch_vidmode,4,1,7,"Video Mode","The video mode you wish to use",VIDEO);
	cvar_bool(close_browser_on_select,"cbos", change_var, 0,"Close Browser","Close the browser on select",HUD);
	cvar_bool(show_position_on_minimap,"spos", change_var, 0,"Show Pos","Show position on the minimap",HUD);
	cvar_int(auto_save,"asv", set_auto_save_interval, 0,0,INT_MAX,"Auto Save","Auto Save",HUD);
	cvar_bool(show_grid,"sgrid", change_var, 0, "Show Grid", "Show grid",HUD);
#endif
#ifndef MAP_EDITOR
	cvar_bool(use_frame_buffer,"fb", change_frame_buffer, 0, "Toggle Frame Buffer Support", "Toggle frame buffer support. Used for reflection and shadow mapping.", VIDEO);
	cvar_bool(use_animation_program,"uap", change_use_animation_program, 1, "Use animation program", "Use GL_ARB_vertex_program for actor animation", VIDEO);
#endif //MAP_EDITOR
#ifdef OSX
	cvar_bool(square_buttons,"sqbut",change_var,1,"Square Buttons","Use square buttons rather than rounded",HUD);
#endif

#endif // ELC

}

void write_var(FILE *fout, int ivar) {
	if (!fout) {
		return;
	}
	Cvar *v = our_vars.cvars + ivar;
	int t = v->cvdtype, f = v->cvflags;
	if (t == CVD_INT || t == CVD_BOOL) {
		fprintf(fout, "#%s = %d\n", v->name, *v->pint);
	} else if (t == CVD_STRING) {
		cstr s = v->pstr;
		if ((f & CVF_PASSWORD) || !strcmp(v->name, "password")) {
			s = "";
		}
		fprintf(fout, "#%s = \"%s\"\n", v->name, s);
	} else if (t == CVD_FLOAT) {
		fprintf(fout, "#%s = %g\n", v->name, *v->pfloat);
	}
	v->saved = 1;	// keep only one copy of this setting
}

int read_el_ini(void) {
	cstr path = "le.ini";
#ifdef MAP_EDITOR
	path = "mapedit.ini";
#endif
	FILE *fin = open_file_config(path, "r");
	if (fin == NULL){
		LOG_ERROR("%s: %s \"%s\": %s\n", reg_error_str, cant_open_file, path, strerror(errno));
		return 0;
	}
	input_line line;
	while (fgets(line, sizeof(input_line), fin)) {
		if (*line == '#') {
			check_var(line + 1, VNK_INI);
		}
	}
	fclose(fin);
	return 1;
}

int write_el_ini(void) {
#if !defined(WINDOWS)
	int fd;
	struct stat statbuff;
#endif // !WINDOWS
	int nlines = 0, maxlines = 0, iline, ivar, all_saved = 1;
	input_line *cont = NULL;
	input_line last_line;
	FILE *file;
	Uint8 written[countof(our_vars.cvars)] = {0};
	// first check if we need to change anything
	//
	// The advantage of skipping this check is that a new el.ini would be
	// created in the users $HOME/.elc for Unix users, even if nothing
	// changed. However, most of the time it's pointless to update an
	// unchanged file.
	for_cvars(v) {
		all_saved &= v->saved;
	}
	if (all_saved) {
		return 1;
	}
	// Consolidate changes for any items featured more than once - on different tabs for example.
	for_cvars(v) {
		if (!v->saved) {
			for_cvars(w) {
				if (w->saved && !strcmp(v->name, w->name)) {
					w->saved = 0;
				}
			}
		}
	}
	// read the ini file
	cstr path = "le.ini";
	file = open_file_config(path, "r");
	if (!file) {
		LOG_ERROR("%s: %s \"%s\": %s\n", reg_error_str, cant_open_file, path, strerror(errno));
	} else {
		maxlines = 300;
		cont = malloc(maxlines * sizeof(input_line));
		while (fgets(cont[nlines], sizeof(input_line), file)) {
			if (++nlines >= maxlines) {
				maxlines *= 2;
				cont = realloc(cont, maxlines * sizeof(input_line));
			}
		}
		fclose(file);
	}
	// Now write the contents of the file, updating those variables that have been changed
	file = open_file_config(path, "w");
	if (!file){
		LOG_ERROR("%s: %s \"%s\": %s\n", reg_error_str, cant_open_file, path, strerror(errno));
		if (cont) {
			free(cont);
		}
		return 0;
	}
	last_line[0] = 0;
	for (iline = 0; iline < nlines; ++iline) {
		if (cont[iline][0] != '#') {
			if (strcmp(cont[iline], last_line)) {
				fprintf(file, "%s", cont[iline]);
			}
		} else {
			ivar = find_var(cont[iline] + 1, VNK_INI);
			if (ivar >= 0 && written[ivar])
				continue;
			if (ivar < 0 || our_vars.cvars[ivar].saved)
				fprintf(file, "%s", cont[iline]);
			else
				write_var(file, ivar);
			if (ivar >= 0)
				written[ivar] = 1;
		}
		strcpy(last_line, cont[iline]);
	}

	// now write all variables that still haven't been saved yet
	for_cvars(v) {
		// check if we already wrote a var with the same name
		int c = find_var(v->name, VNK_INI), i = indexof_cvar(v);
		if (c >= 0 && written[c])
			continue;
		if (!v->saved) {
			fprintf(file, "\n");
			write_var(file, i);
			written[i] = 1;
		}
	}
#if !defined(WINDOWS)
	fd = fileno (file);
	fstat(fd, &statbuff);
	/* Set perms to 600 on el_ini if they are anything else */
	if (statbuff.st_mode != (S_IRUSR|S_IWUSR)){
		fchmod(fd, S_IRUSR|S_IWUSR);
	}
#endif // !WINDOWS

	fclose(file);
	free(cont);
	return 1;
}

/* ------ ELConfig Window functions start here ------ */
#ifdef ELC
int display_elconfig_handler(window_info *win) {
	for_cvars(v) {
		if (v->cvflags & CVF_MULTI) {
			multiselect_set_selected(elconfig_tabs[v->widgets.tab_id].tab, v->widgets.widget_id, *v->pint);
		}
	}
	// Draw the long description of an option
	draw_string_small(TAB_MARGIN, elconfig_menu_y_len-LONG_DESC_SPACE, elconf_description_buffer, MAX_LONG_DESC_LINES);
	return 1;
}

static inline void update_from_spinbutton(Cvar *v, spinbutton *b) {
	switch (b->type) {
	case SPIN_FLOAT:
		v->pfunc_setf(v->pfloat, *(float *)b->data);
		break;
	case SPIN_INT:
		v->pfunc_seti(v->pint, *(int *)b->data);
		break;
	}
	v->saved = 0;
}

int spinbutton_onkey_handler(widget_list *widget, int mx, int my, Uint32 key, Uint32 unikey) {
	if (widget && !(key & ELW_ALT) && !(key & ELW_CTRL)) {
		for_cvars(v) {
			if (v->widgets.widget_id == widget->id) {
				update_from_spinbutton(v, widget->widget_info);
				return 1;
			}
		}
	}
	return 0;
}

int spinbutton_onclick_handler(widget_list *widget, int mx, int my, Uint32 flags) {
	if (widget) {
		for_cvars(v) {
			if (v->widgets.widget_id == widget->id) {
				update_from_spinbutton(v, widget->widget_info);
				return 1;
			}
		}
	}
	return 0;
}

int multiselect_click_handler(widget_list *widget, int mx, int my, Uint32 flags)
{
	if (flags&ELW_LEFT_MOUSE || flags&ELW_RIGHT_MOUSE) {
		for_cvars(v) {
			if (v->widgets.widget_id == widget->id) {
				v->pfunc_seti(v->pint, multiselect_get_selected(elconfig_tabs[v->widgets.tab_id].tab, v->widgets.widget_id));
				v->saved = 0;
				return 1;
			}
		}
	}
	return 0;
}

int mouseover_option_handler(widget_list *widget, int mx, int my)
{
	int i;

	//Find the label in our_vars
	for (i = 0; i < our_vars.n_cvars; i++) {
		if (our_vars.cvars[i].widgets.label_id == widget->id || widget->id == our_vars.cvars[i].widgets.widget_id) {
			break;
		}
	}
	if (i == our_vars.n_cvars) {
		//We didn't find anything, abort
		return 0;
	}
	put_small_text_in_box(our_vars.cvars[i].display.desc, strlen((char *)our_vars.cvars[i].display.desc),
								elconfig_menu_x_len-TAB_MARGIN*2, (char *)elconf_description_buffer);
	return 1;
}
int onclick_label_handler(widget_list *w, int mx, int my, Uint32 f) {
	if (f & ELW_LEFT_MOUSE) {
		for_cvars(v) {
			if (v->widgets.label_id == w->id) {
				assert(v->cvdtype == CVD_BOOL);
				v->pfunc_setb(v->pint);
				v->saved = 0;
				do_click_sound();
				return 1;
			}
		}
	}
	return 0;
}
int onclick_checkbox_handler(widget_list *w, int mx, int my, Uint32 f) {
	if (f & ELW_LEFT_MOUSE) {
		for_cvars(v) {
			if (v->widgets.widget_id == w->id) {
				assert(v->cvdtype == CVD_BOOL);
				*v->pint ^= 1;
				v->pfunc_setb(v->pint);
				v->saved = 0;
				do_click_sound();
				return 1;
			}
		}
	}
	return 0;
}
int string_onkey_handler(widget_list *widget) {
	// dummy key handler that marks the appropriate variable as changed
	if (widget) {
		for_cvars(v) {
			if (v->widgets.widget_id == widget->id) {
				v->saved = 0;
				return 1;
			}
		}
	}
	return 0;
}

void elconfig_populate_tabs(void) {
	int x, y, label_id = -1, widget_id = -1;
	int widget_height, label_height, next_id = 2;
	for (Tab *t = elconfig_tabs, *te = t + MAX_TABS; t < te; ++t) {
		t->x = t->y = TAB_MARGIN;
	}
	for_cvars(v) {
		if (v->cvflags & (CVF_INI | CVF_PASSWORD)) {
			continue;
		}
		Tab *t = elconfig_tabs + v->widgets.tab_id;
		int usefuncs = (v->cvflags & CVF_FUNCS);
		if (v->cvdtype == CVD_BOOL) {
			widget_id = checkbox_add_extended(t->tab, next_id++, NULL, t->x, t->y, CHECKBOX_SIZE, CHECKBOX_SIZE, 0, 1.0, 0.77f, 0.59f, 0.39f, v->pint);
			label_id = label_add(t->tab, NULL, (char *)v->display.str, t->x+CHECKBOX_SIZE+SPACING, t->y);
			widget_set_OnClick(t->tab, label_id, onclick_label_handler);
			widget_set_OnClick(t->tab, widget_id, onclick_checkbox_handler);
		} else if (v->cvdtype == CVD_INT) {
			label_id = label_add_extended(t->tab, next_id++, NULL, t->x, t->y, 0, 1.0, 0.77f, 0.59f, 0.39f, (char *)v->display.str);
			if (v->cvflags & CVF_MULTI) {
				Snode *l = v->labels;
				if (v->cvflags & CVF_HORIZ) {
					widget_id = multiselect_add_extended(t->tab, next_id++, NULL, t->x+SPACING+get_string_width(v->display.str), t->y, 350, 80, 1.0f, 0.77f, 0.59f, 0.39f, 0.32f, 0.23f, 0.15f, 0);
					x = 0;
					for (y = 0; l; ++y, l = l->snext) {
						cstr label = l->sval;
						int radius = BUTTONRADIUS;
						float width_ratio = DEFAULT_FONT_X_LEN/12.0f;
						int width = 0;
						width = 2 * radius+(get_string_width((unsigned char*)label)*width_ratio);
						multiselect_button_add_extended(t->tab, widget_id, x, 0, width, label, DEFAULT_SMALL_RATIO, y == *v->pint);
						if (!*label) {
							--y;
						} else {
							x += width + SPACING;
						}
					}
				} else {
					widget_id = multiselect_add_extended(t->tab, next_id++, NULL, t->x+SPACING+get_string_width(v->display.str), t->y, 250, 80, 1.0f, 0.77f, 0.59f, 0.39f, 0.32f, 0.23f, 0.15f, 0);
					for (y = 0; l; ++y, l = l->snext) {
						cstr label = l->sval;
						int n = strlen(label), width = n > 0 ? 0 : -1;

						multiselect_button_add_extended(t->tab, widget_id, 0, y*(22+SPACING), width, label, DEFAULT_SMALL_RATIO, y == *v->pint);
						if (!n) {
							--y;
						}
					}
				}
				widget_set_OnClick(t->tab, widget_id, multiselect_click_handler);
			} else {
				int vmin = usefuncs ? v->pfunc_imin() : v->imin, vmax = usefuncs ? v->pfunc_imax() : v->imax;
				widget_id = spinbutton_add(t->tab, NULL, elconfig_menu_x_len/4*3, t->y, 100, 20, SPIN_INT, v->pint, vmin, vmax, 1.0);
				widget_set_OnKey(t->tab, widget_id, spinbutton_onkey_handler);
				widget_set_OnClick(t->tab, widget_id, spinbutton_onclick_handler);
			}
		} else if (v->cvdtype == CVD_FLOAT) {
			label_id = label_add_extended(t->tab, next_id++, NULL, t->x, t->y, 0, 1.0, 0.77f, 0.59f, 0.39f, (char *)v->display.str);
			float vfmin = usefuncs ? v->pfunc_fmin() : v->fmin, vfmax = usefuncs ? v->pfunc_fmax() : v->fmax;
			widget_id = spinbutton_add(t->tab, NULL, elconfig_menu_x_len/4*3, t->y, 100, 20, SPIN_FLOAT, v->pfloat, vfmin, vfmax, v->fstep);
			widget_set_OnKey(t->tab, widget_id, spinbutton_onkey_handler);
			widget_set_OnClick(t->tab, widget_id, spinbutton_onclick_handler);
		} else if (v->cvdtype == CVD_STRING) {
			label_id = label_add_extended(t->tab, next_id++, NULL, t->x, t->y, 0, 1.0, 0.77f, 0.59f, 0.39f, (char *)v->display.str);
			widget_id = pword_field_add_extended(t->tab, next_id++, NULL, elconfig_menu_x_len/2, t->y, 280, 20, P_TEXT, 1.0f, 0.77f, 0.59f, 0.39f, (Uint8 *)v->pstr, v->slen);
			widget_set_OnKey(t->tab, widget_id, string_onkey_handler);
		}
		//Calculate y position of the next option.
		label_height = widget_find(t->tab, label_id)->len_y;
		widget_height = widget_find(t->tab, widget_id)->len_y;
		t->y += (widget_height > label_height ? widget_height : label_height)+SPACING;
		if (t->y > widget_get_height(elconfig_win, elconfig_tab_collection_id)-TAB_TAG_HEIGHT) {
			/* Expand the scrollbar to fit all our widgets. */
			set_window_scroll_len(t->tab, t->y);
			set_window_scroll_inc(t->tab, widget_height+SPACING);
		}
		v->widgets.label_id = label_id;
		v->widgets.widget_id = widget_id;
		widget_set_OnMouseover(t->tab, label_id, mouseover_option_handler);
		widget_set_OnMouseover(t->tab, widget_id, mouseover_option_handler);
	}
}

// TODO: replace this hack by something clean.
int show_elconfig_handler(window_info *win) {
	int pwinx, pwiny;
	window_info *pwin;
	if (win->pos_id != -1) {
		pwin = &windows_list.window[win->pos_id];
		pwinx = pwin->cur_x;
		pwiny = pwin->cur_y;
	} else {
		pwinx = 0;
		pwiny = 0;
	}
#ifndef MAP_EDITOR2
	if (get_show_window(newchar_root_win)) {
		init_window(win->window_id, newchar_root_win, 0, win->pos_x - pwinx, win->pos_y - pwiny, win->len_x, win->len_y);
	} else {
		int our_root_win = -1;
		if (!force_elconfig_win_ontop && !windows_on_top) {
			our_root_win = game_root_win;
		}
		init_window(win->window_id, our_root_win, 0, win->pos_x - pwinx, win->pos_y - pwiny, win->len_x, win->len_y);
	}
#else
	init_window(win->window_id, game_root_win, 0, win->pos_x - pwinx, win->pos_y - pwiny, win->len_x, win->len_y);
#endif
	return 1;
}

void display_elconfig_win(void) {
	if (elconfig_win < 0) {
		int our_root_win = -1;
		int i;
		if (!windows_on_top) {
			our_root_win = game_root_win;
		}
		/* Set up the window */
		//@tosh : j'enlève pour l'instant le flag ELW_RESIZEABLE, qui est à l'origine de quelques bugs.
		elconfig_win = create_window(win_configuration, our_root_win, 0, elconfig_menu_x, elconfig_menu_y, elconfig_menu_x_len, elconfig_menu_y_len, ELW_WIN_DEFAULT);

		set_window_color(elconfig_win, ELW_COLOR_BORDER, 0.77f, 0.59f, 0.39f, 0.0f);
		set_window_handler(elconfig_win, ELW_HANDLER_DISPLAY, &display_elconfig_handler );
		// TODO: replace this hack by something clean.
		set_window_handler(elconfig_win, ELW_HANDLER_SHOW, &show_elconfig_handler);
		/* Create tabs */
		elconfig_tab_collection_id = tab_collection_add_extended (elconfig_win, elconfig_tab_collection_id, NULL, TAB_MARGIN, TAB_MARGIN, elconfig_menu_x_len-TAB_MARGIN*2, elconfig_menu_y_len-TAB_MARGIN*2-LONG_DESC_SPACE, 0, 0.7, 0.77f, 0.57f, 0.39f, MAX_TABS, TAB_TAG_HEIGHT);
		/* Pass ELW_SCROLLABLE as the final argument to tab_add() if you want
		 * to put more widgets in the tab than the size of the window allows.*/
		elconfig_tabs[CONTROLS].tab = tab_add(elconfig_win, elconfig_tab_collection_id, ttab_controls, 0, 0, ELW_SCROLLABLE);
		elconfig_tabs[HUD].tab = tab_add(elconfig_win, elconfig_tab_collection_id, ttab_hud, 0, 0, ELW_SCROLLABLE);
		elconfig_tabs[CHAT].tab = tab_add(elconfig_win, elconfig_tab_collection_id, ttab_chat, 0, 0, ELW_SCROLLABLE);
		elconfig_tabs[FONT].tab = tab_add(elconfig_win, elconfig_tab_collection_id, ttab_font, 0, 0, ELW_SCROLLABLE);
		elconfig_tabs[SERVER].tab = tab_add(elconfig_win, elconfig_tab_collection_id, ttab_server, 0, 0, ELW_SCROLLABLE);
		elconfig_tabs[AUDIO].tab = tab_add(elconfig_win, elconfig_tab_collection_id, ttab_audio, 0, 0, ELW_SCROLLABLE);
		elconfig_tabs[VIDEO].tab = tab_add(elconfig_win, elconfig_tab_collection_id, ttab_video, 0, 0, ELW_SCROLLABLE);
		elconfig_tabs[GFX].tab = tab_add(elconfig_win, elconfig_tab_collection_id, ttab_gfx, 0, 0, ELW_SCROLLABLE);
		elconfig_tabs[CAMERA].tab = tab_add(elconfig_win, elconfig_tab_collection_id, ttab_camera, 0, 0, ELW_SCROLLABLE);
		elconfig_tabs[TROUBLESHOOT].tab = tab_add(elconfig_win, elconfig_tab_collection_id, ttab_troubleshoot, 0, 0, ELW_SCROLLABLE);
#ifdef DEBUG
		elconfig_tabs[DEBUGTAB].tab = tab_add(elconfig_win, elconfig_tab_collection_id, "Debug", 0, 0, ELW_SCROLLABLE);
#endif
		elconfig_populate_tabs();

		/* configure scrolling for tabs */
		for (i = 0; i<MAX_TABS; i++) {
			/* configure scrolling for any tabs that exceed the window length */
			if (elconfig_tabs[i].y > (widget_get_height(elconfig_win, elconfig_tab_collection_id)-TAB_TAG_HEIGHT)) {
				set_window_scroll_len(elconfig_tabs[i].tab, elconfig_tabs[i].y);
				set_window_scroll_inc(elconfig_tabs[i].tab, TAB_TAG_HEIGHT);
			}
			/* otherwise disable scrolling */
			else {
			set_window_scroll_inc(elconfig_tabs[i].tab, 0);
				widget_set_flags(elconfig_tabs[i].tab, windows_list.window[elconfig_tabs[i].tab].scroll_id, WIDGET_DISABLED);
			}
		}
	}
	show_window(elconfig_win);
	select_window(elconfig_win);
}
#endif //ELC
