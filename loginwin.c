#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "loginwin.h"
#include "asc.h"
#include "books.h"
#include "draw_scene.h"
#include "elconfig.h"
#include "elwindows.h"
#include "gamewin.h"
#include "gl_init.h"
#include "hud.h"
#include "interface.h"
#include "multiplayer.h"
#include "new_character.h"
#include "rules.h"
#ifdef NEW_SOUND
#include "sound.h"
#endif // NEW_SOUND
#include "tabs.h"
#include "textures.h"
#include "translate.h"

int login_root_win = -1;
int login_text = -1;

static int login_screen_menus;

static char log_in_error_str[520] = {0};

int username_text_x;
int username_text_y;

int password_text_x;
int password_text_y;

int username_bar_x;
int username_bar_y;
int username_bar_x_len = 174;
int username_bar_y_len = 28;

int password_bar_x;
int password_bar_y;
int password_bar_x_len = 174;
int password_bar_y_len = 28;

int log_in_x;
int log_in_y;
#ifdef ENGLISH
int log_in_x_len = 87;
int log_in_y_len = 35;
#else //ENGLISH
int log_in_x_len = 82;
int log_in_y_len = 34;
#endif //ENGLISH

int new_char_x;
int new_char_y;
#ifdef ENGLISH
int new_char_x_len = 138;
int new_char_y_len = 35;
#else //ENGLISH
int new_char_x_len = 160;
int new_char_y_len = 34;
#endif //ENGLISH

int settings_x;
int settings_y;
#ifdef ENGLISH
int settings_x_len = 87;
int settings_y_len = 35;
#else //ENGLISH
int settings_x_len = 63;
int settings_y_len = 34;
#endif //ENGLISH

#ifndef ENGLISH
int cadre_x;
int cadre_y;
int cadre_x_len = 512;
int cadre_y_len = 447;
#endif //ENGLISH

char log_in_button_selected = 0;
char new_char_button_selected = 0;
char settings_button_selected = 0;

void init_login_screen ()
{
	CHECK_GL_ERRORS();
#ifdef	NEW_TEXTURES
	login_screen_menus = load_texture_cached("textures/login_menu.dds", tt_image);
	login_text = load_texture_cached("textures/login_back.dds", tt_image);
#else	/* NEW_TEXTURES */
	login_screen_menus = load_texture_cache ("./textures/login_menu.bmp",0);
	CHECK_GL_ERRORS();
	login_text = load_texture_cache ("./textures/login_back.bmp",255);
#endif	/* NEW_TEXTURES */
	CHECK_GL_ERRORS();
}

void set_login_error (const char *msg, int len, int print_err)
{
#ifdef NEW_SOUND
	int snd;
#endif // NEW_SOUND
	if (len <= 0)
	{
		// server didn't send a message, use the default
		safe_snprintf (log_in_error_str, sizeof(log_in_error_str), "%s: %s", reg_error_str, invalid_pass);
	}
	else if (print_err)
	{
#ifdef ENGLISH
		safe_snprintf (log_in_error_str, sizeof (log_in_error_str), "%s: %.*s", reg_error_str, len, msg);
#else //ENGLISH
		safe_snprintf (log_in_error_str, sizeof (log_in_error_str), "%.*s", len, msg);
#endif //ENGLISH
	}
	else
	{
		safe_strncpy2 (log_in_error_str, msg, sizeof (log_in_error_str), len);
	}
	reset_soft_breaks (log_in_error_str, strlen (log_in_error_str), sizeof (log_in_error_str), 1.0, window_width, NULL, NULL);

#ifdef NEW_SOUND
	if ((snd = get_index_for_sound_type_name("Login Error")) > -1)
		add_sound_object(snd, 0, 0, 1);
#endif // NEW_SOUND
}

int resize_login_handler (window_info *win, Uint32 w, Uint32 h)
{
	int half_screen_x = w / 2;
	int half_screen_y = h / 2;
	int len1 = strlen (login_username_str);
	int len2 = strlen (login_password_str);
#ifdef ENGLISH
	int offset = 20 + (len1 > len2 ? (len1+1) * 16 : (len2+1) * 16);
#else //ENGLISH
	int offset = 60 + (len1 > len2 ? (len1+1) * 16 : (len2+1) * 16);
#endif //ENGLISH

	username_text_x = half_screen_x - offset;
#ifdef ENGLISH
	username_text_y = half_screen_y - 130;
#else //ENGLISH
	username_text_y = half_screen_y - 107;
#endif //ENGLISH

	password_text_x = half_screen_x - offset;
#ifdef ENGLISH
	password_text_y = half_screen_y - 100;
#else //ENGLISH
	password_text_y = half_screen_y - 75;
#endif //ENGLISH

#ifdef ENGLISH
	username_bar_x = half_screen_x;
	username_bar_y = username_text_y - 7;
#else //ENGLISH
	username_bar_x = half_screen_x - username_bar_x_len/2;
	username_bar_y = half_screen_y - username_bar_y_len/2 - 96;
#endif //ENGLISH

#ifdef ENGLISH
	password_bar_x = half_screen_x;
	password_bar_y = password_text_y - 7;
#else //ENGLISH
	password_bar_x = half_screen_x - password_bar_x_len/2;
	password_bar_y = half_screen_y - password_bar_y_len/2 - 67;
#endif //ENGLISH

#ifdef ENGLISH
	log_in_x = username_text_x;
	log_in_y = half_screen_y - 50;
#else //ENGLISH
	log_in_x = half_screen_x - log_in_x_len/2 - 1;
	log_in_y = half_screen_y - 45;
#endif //ENGLISH

#ifdef ENGLISH
	settings_x = username_bar_x + username_bar_x_len - settings_x_len;
	settings_y = half_screen_y - 50;
#else //ENGLISH
	settings_x = half_screen_x - settings_x_len/2;
	settings_y = half_screen_y + 34;
#endif //ENGLISH

#ifdef ENGLISH
	new_char_x = log_in_x + ((settings_x + settings_x_len) - log_in_x)/2 - new_char_x_len/2;
	new_char_y = half_screen_y - 50;
#else //ENGLISH
	new_char_x = half_screen_x - new_char_x_len/2;
	new_char_y = half_screen_y - 3;
#endif //ENGLISH

#ifndef ENGLISH
    cadre_x = half_screen_x - cadre_x_len/2;
    cadre_y = half_screen_y - cadre_y_len/2;
#endif //ENGLISH

	return 1;
}

// the code was removed from draw_login_screen () in interface.c since I don't
// want to introduce new global variables, but the mouseover and click handlers
// need to know the positions of the buttons and input fields. The other option
// was to pass (a struct of) 24 integers to draw_login_screen, which seemed a
// bit excessive.
int display_login_handler (window_info *win)
{
	int num_lines;
#ifdef ENGLISH
#ifdef	NEW_TEXTURES
	float selected_bar_u_start = (float)0/256;
	float selected_bar_v_start = (float)0/256;

	float selected_bar_u_end = (float)174/256;
	float selected_bar_v_end = (float)28/256;

	float unselected_bar_u_start = (float)0/256;
	float unselected_bar_v_start = (float)40/256;

	float unselected_bar_u_end = (float)170/256;
	float unselected_bar_v_end = (float)63/256;
	/////////////////////////
	float log_in_unselected_start_u = (float)0/256;
	float log_in_unselected_start_v = (float)80/256;

	float log_in_unselected_end_u = (float)87/256;
	float log_in_unselected_end_v = (float)115/256;

	float log_in_selected_start_u = (float)0/256;
	float log_in_selected_start_v = (float)120/256;

	float log_in_selected_end_u = (float)87/256;
	float log_in_selected_end_v = (float)155/256;
	/////////////////////////
	float new_char_unselected_start_u = (float)100/256;
	float new_char_unselected_start_v = (float)80/256;

	float new_char_unselected_end_u = (float)238/256;
	float new_char_unselected_end_v = (float)115/256;

	float new_char_selected_start_u = (float)100/256;
	float new_char_selected_start_v = (float)120/256;

	float new_char_selected_end_u = (float)238/256;
	float new_char_selected_end_v = (float)155/256;
	/////////////////////////
	float settings_unselected_start_u = (float)0/256;
	float settings_unselected_start_v = (float)160/256;

	float settings_unselected_end_u = (float)87/256;
	float settings_unselected_end_v = (float)195/256;

	float settings_selected_start_u = (float)0/256;
	float settings_selected_start_v = (float)200/256;

	float settings_selected_end_u = (float)87/256;
	float settings_selected_end_v = (float)235/256;
#else	/* NEW_TEXTURES */
	float selected_bar_u_start = (float)0/256;
	float selected_bar_v_start = 1.0f - (float)0/256;

	float selected_bar_u_end = (float)174/256;
	float selected_bar_v_end = 1.0f - (float)28/256;

	float unselected_bar_u_start = (float)0/256;
	float unselected_bar_v_start = 1.0f - (float)40/256;

	float unselected_bar_u_end = (float)170/256;
	float unselected_bar_v_end = 1.0f - (float)63/256;
	/////////////////////////
	float log_in_unselected_start_u = (float)0/256;
	float log_in_unselected_start_v = 1.0f - (float)80/256;

	float log_in_unselected_end_u = (float)87/256;
	float log_in_unselected_end_v = 1.0f - (float)115/256;

	float log_in_selected_start_u = (float)0/256;
	float log_in_selected_start_v = 1.0f - (float)120/256;

	float log_in_selected_end_u = (float)87/256;
	float log_in_selected_end_v = 1.0f-(float)155/256;
	/////////////////////////
	float new_char_unselected_start_u = (float)100/256;
	float new_char_unselected_start_v = 1.0f-(float)80/256;

	float new_char_unselected_end_u = (float)238/256;
	float new_char_unselected_end_v = 1.0f-(float)115/256;

	float new_char_selected_start_u = (float)100/256;
	float new_char_selected_start_v = 1.0f-(float)120/256;

	float new_char_selected_end_u = (float)238/256;
	float new_char_selected_end_v = 1.0f-(float)155/256;
	/////////////////////////
	float settings_unselected_start_u = (float)0/256;
	float settings_unselected_start_v = 1.0f - (float)160/256;

	float settings_unselected_end_u = (float)87/256;
	float settings_unselected_end_v = 1.0f - (float)195/256;

	float settings_selected_start_u = (float)0/256;
	float settings_selected_start_v = 1.0f - (float)200/256;

	float settings_selected_end_u = (float)87/256;
	float settings_selected_end_v = 1.0f-(float)235/256;
#endif	/* NEW_TEXTURES */
#else //ENGLISH
#ifdef NEW_TEXTURES
    float cadre_u_start = 0.0f;
    float cadre_v_start = 0.0f;
    float cadre_u_end   = 1.0f;
    float cadre_v_end   = (float)447/512;

	float selected_log_bar_u_start = (float)8/512;
	float selected_log_bar_u_end = (float)182/512;
	float selected_log_bar_v_start = (float)450/512;
	float selected_log_bar_v_end = (float)477/512;

	float selected_password_bar_u_start = (float)8/512;
	float selected_password_bar_u_end = (float)182/512;
	float selected_password_bar_v_start = (float)479/512;
	float selected_password_bar_v_end = (float)506/512;

	float log_in_selected_start_u = (float)193/512;
	float log_in_selected_end_u = (float)275/512;
	float log_in_selected_start_v = (float)450/512;
	float log_in_selected_end_v = (float)484/512;

	float new_char_selected_start_u = (float)277/512;
	float new_char_selected_end_u = (float)437/512;
	float new_char_selected_start_v = (float)450/512;
	float new_char_selected_end_v = (float)484/512;

	float settings_selected_start_u = (float)439/512;
	float settings_selected_end_u = (float)502/512;
	float settings_selected_start_v = (float)450/512;
	float settings_selected_end_v = (float)484/512;
#else //NEW_TEXTURES
    float cadre_u_start = 0.0f;
    float cadre_v_start = 1.0f;
    float cadre_u_end   = 1.0f;
    float cadre_v_end   = 1.0f - (float)447/512;

	float selected_log_bar_u_start = (float)8/512;
	float selected_log_bar_u_end = (float)182/512;
	float selected_log_bar_v_start = 1.0f-(float)450/512;
	float selected_log_bar_v_end = 1.0f-(float)477/512;

	float selected_password_bar_u_start = (float)8/512;
	float selected_password_bar_u_end = (float)182/512;
	float selected_password_bar_v_start = 1.0f-(float)479/512;
	float selected_password_bar_v_end = 1.0f-(float)506/512;

	float log_in_selected_start_u = (float)193/512;
	float log_in_selected_end_u = (float)275/512;
	float log_in_selected_start_v = 1.0f - (float)450/512;
	float log_in_selected_end_v = 1.0f-(float)484/512;

	float new_char_selected_start_u = (float)277/512;
	float new_char_selected_end_u = (float)437/512;
	float new_char_selected_start_v = 1.0f - (float)450/512;
	float new_char_selected_end_v = 1.0f-(float)484/512;

	float settings_selected_start_u = (float)439/512;
	float settings_selected_end_u = (float)502/512;
	float settings_selected_start_v = 1.0f - (float)450/512;
	float settings_selected_end_v = 1.0f-(float)484/512;
#endif //NEW_TEXTURES
#endif //ENGLISH

	draw_console_pic(login_text);

#ifdef ENGLISH
	// ok, start drawing the interface...
	draw_string (username_text_x, username_text_y, (unsigned char*)login_username_str, 1);
	draw_string (password_text_x, password_text_y, (unsigned char*)login_password_str, 1);

	num_lines = reset_soft_breaks(login_rules_str, strlen(login_rules_str), sizeof(login_rules_str), 1, settings_x + settings_x_len - username_text_x, NULL, NULL);
	draw_string_zoomed(username_text_x, log_in_y + 60, (unsigned char*)login_rules_str, num_lines, 1);
#endif //ENGLISH

	// start drawing the actual interface pieces
#ifdef	NEW_TEXTURES
	bind_texture(login_screen_menus);
#else	/* NEW_TEXTURES */
	get_and_set_texture_id(login_screen_menus);
#endif	/* NEW_TEXTURES */
	glColor3f (1.0f,1.0f,1.0f);
	glBegin (GL_QUADS);

#ifdef ENGLISH
	// username box
	if (username_box_selected)
		draw_2d_thing (selected_bar_u_start, selected_bar_v_start, selected_bar_u_end, selected_bar_v_end, username_bar_x, username_bar_y, username_bar_x + username_bar_x_len, username_bar_y + username_bar_y_len);
	else
		draw_2d_thing (unselected_bar_u_start, unselected_bar_v_start, unselected_bar_u_end, unselected_bar_v_end, username_bar_x, username_bar_y, username_bar_x + username_bar_x_len, username_bar_y + username_bar_y_len);

	// password box
	if (password_box_selected)
		draw_2d_thing (selected_bar_u_start, selected_bar_v_start, selected_bar_u_end, selected_bar_v_end, password_bar_x, password_bar_y, password_bar_x + password_bar_x_len, password_bar_y + password_bar_y_len);
	else
		draw_2d_thing (unselected_bar_u_start, unselected_bar_v_start, unselected_bar_u_end, unselected_bar_v_end, password_bar_x, password_bar_y, password_bar_x + password_bar_x_len, password_bar_y + password_bar_y_len);

	// log in button
	if (log_in_button_selected)
		draw_2d_thing (log_in_selected_start_u, log_in_selected_start_v, log_in_selected_end_u, log_in_selected_end_v, log_in_x, log_in_y, log_in_x + log_in_x_len, log_in_y + log_in_y_len);
	else
		draw_2d_thing (log_in_unselected_start_u, log_in_unselected_start_v, log_in_unselected_end_u, log_in_unselected_end_v, log_in_x, log_in_y, log_in_x + log_in_x_len, log_in_y + log_in_y_len);

	// new char button
	if (new_char_button_selected)
		draw_2d_thing (new_char_selected_start_u, new_char_selected_start_v, new_char_selected_end_u, new_char_selected_end_v, new_char_x, new_char_y, new_char_x + new_char_x_len, new_char_y + new_char_y_len);
	else
		draw_2d_thing (new_char_unselected_start_u, new_char_unselected_start_v, new_char_unselected_end_u, new_char_unselected_end_v, new_char_x, new_char_y, new_char_x + new_char_x_len, new_char_y + new_char_y_len);

	// settings button
	if (settings_button_selected)
		draw_2d_thing (settings_selected_start_u, settings_selected_start_v, settings_selected_end_u, settings_selected_end_v, settings_x, settings_y, settings_x + settings_x_len, settings_y + settings_y_len);
	else
		draw_2d_thing (settings_unselected_start_u, settings_unselected_start_v, settings_unselected_end_u, settings_unselected_end_v, settings_x, settings_y, settings_x + settings_x_len, settings_y + settings_y_len);
#else //ENGLISH
    // Dessine le cadre
	draw_2d_thing (cadre_u_start, cadre_v_start, cadre_u_end, cadre_v_end, cadre_x, cadre_y, cadre_x+cadre_x_len, cadre_y+cadre_y_len);

	// Dessine la zone de saisie pour le pseudo
	if (username_box_selected)
    {
		draw_2d_thing (selected_log_bar_u_start, selected_log_bar_v_start, selected_log_bar_u_end, selected_log_bar_v_end, username_bar_x, username_bar_y, username_bar_x + username_bar_x_len, username_bar_y + username_bar_y_len);
    }

    // Dessine la zone de saisie pour le mot de passe
	if (password_box_selected)
    {
		draw_2d_thing (selected_password_bar_u_start, selected_password_bar_v_start, selected_password_bar_u_end, selected_password_bar_v_end, password_bar_x, password_bar_y, password_bar_x + password_bar_x_len, password_bar_y + password_bar_y_len);
    }

    // Dessine le bouton pour se connecter
	if (log_in_button_selected)
    {
		draw_2d_thing (log_in_selected_start_u, log_in_selected_start_v, log_in_selected_end_u, log_in_selected_end_v, log_in_x, log_in_y, log_in_x + log_in_x_len, log_in_y + log_in_y_len);
    }

    // Dessine le bouton pour creer un nouveau personnage
	if (new_char_button_selected)
    {
		draw_2d_thing (new_char_selected_start_u, new_char_selected_start_v, new_char_selected_end_u, new_char_selected_end_v, new_char_x, new_char_y, new_char_x + new_char_x_len, new_char_y + new_char_y_len);
    }

    // Dessine le bouton pour activer les options
	if (settings_button_selected)
    {
		draw_2d_thing (settings_selected_start_u, settings_selected_start_v, settings_selected_end_u, settings_selected_end_v, settings_x, settings_y, settings_x + settings_x_len, settings_y + settings_y_len);
    }

#endif //ENGLISH

	glEnd();

#ifndef ENGLISH
	draw_string_shadowed(username_text_x, username_text_y, (unsigned char*)login_username_str, 1, 1.0f,0.9f,0.5f, 0.3f,0.2f,0.0f);
	draw_string_shadowed(password_text_x, password_text_y, (unsigned char*)login_password_str, 1, 1.0f,0.9f,0.5f, 0.3f,0.2f,0.0f);

    // On a un message d'erreur, on l'affiche a la place du message d'information
    if (strlen(log_in_error_str) > 0)
    {
    	num_lines = reset_soft_breaks(log_in_error_str, strlen(log_in_error_str), sizeof(log_in_error_str), 1, cadre_x_len-80, NULL, NULL);
		draw_string_shadowed(cadre_x+40, cadre_y+cadre_y_len-100, (unsigned char*)log_in_error_str, num_lines, 1.0f,0.2f,0.2f, 0.3f,0.0f,0.0f);
    }
    else
    {
    	num_lines = reset_soft_breaks(login_rules_str, strlen(login_rules_str), sizeof(login_rules_str), 1, cadre_x_len-80, NULL, NULL);
		draw_string_shadowed(cadre_x+40, cadre_y+cadre_y_len-100, (unsigned char*)login_rules_str, num_lines, 1.0f,1.0f,1.0f, 0.0f,0.0f,0.0f);
    }

	draw_string_shadowed(username_bar_x + 4, username_text_y, (unsigned char*)username_str, 1, 1.0f,0.5f,0.1f, 0.3f,0.0f,0.0f);
	draw_string_shadowed(password_bar_x + 4, password_text_y, (unsigned char*)display_password_str, 1, 1.0f,0.5f,0.1f, 0.3f,0.0f,0.0f);
#else //ENGLISH
	glColor3f (0.0f, 0.9f, 1.0f);
	draw_string (username_bar_x + 4, username_text_y, (unsigned char*)username_str, 1);
	draw_string (password_bar_x + 4, password_text_y, (unsigned char*)display_password_str, 1);
#endif //ENGLISH
	glColor3f (1.0f, 0.0f, 0.0f);

	// print the current error, if any
#ifdef ENGLISH
	draw_string (0, log_in_y + 40, (unsigned char*)log_in_error_str, 5);
#endif //ENGLISH

	CHECK_GL_ERRORS ();
	draw_delay = 20;
	return 1;
}

int mouseover_login_handler (window_info *win, int mx, int my)
{
	// check to see if the log in button is active, or not
	if (mx >= log_in_x && mx <= log_in_x + log_in_x_len && my >= log_in_y && my <= log_in_y + log_in_y_len && username_str[0] && password_str[0])
		log_in_button_selected = 1;
	else
		log_in_button_selected = 0;

	// check to see if the new char button is active, or not
	if (mx >= new_char_x && mx <= new_char_x + new_char_x_len && my >= new_char_y && my <= new_char_y + new_char_y_len)
		new_char_button_selected = 1;
	else
		new_char_button_selected = 0;

	// check to see if the settings button is active, or not
	if (mx >= settings_x && mx <= settings_x + settings_x_len && my >= settings_y && my <= settings_y + settings_y_len)
		settings_button_selected = 1;
	else
		settings_button_selected = 0;

	return 1;
}

int click_login_handler (window_info *win, int mx, int my, Uint32 flags)
{
	int left_click = flags & ELW_LEFT_MOUSE;
	extern int force_elconfig_win_ontop;
	force_elconfig_win_ontop = 0;

	if (left_click == 0) return 0;

	// check to see if we clicked on the username box
	if (mx >= username_bar_x && mx <= username_bar_x + username_bar_x_len && my >= username_bar_y && my <= username_bar_y + username_bar_y_len)
	{
		username_box_selected = 1;
		password_box_selected = 0;
	}
	// check to see if we clicked on the password box
	else if (mx >= password_bar_x && mx <= password_bar_x + password_bar_x_len && my >= password_bar_y && my <= password_bar_y + password_bar_y_len)
	{
		username_box_selected = 0;
		password_box_selected = 1;
	}
	// check to see if we clicked on the ACTIVE Log In button
	if (log_in_button_selected)
	{
		log_in_error_str[0] = '\0';
		send_login_info ();
	}
	//check to see if we clicked on the ACTIVE New Char button
	else if (new_char_button_selected)
	{
		// don't destroy the login window just yet, the user might
		// click the back button
		hide_window (login_root_win);
		create_newchar_root_window ();
		if (last_display == -1)
		{
			create_rules_root_window (win->len_x, win->len_y, newchar_root_win, 15);
			show_window (rules_root_win);
		}
		else
		{
			show_window (newchar_root_win);
		}
	}
	// to see if we clicked on the ACTIVE settings button
	else if (settings_button_selected)
	{
		force_elconfig_win_ontop = 1;
		view_window (&elconfig_win, 0);
	}
	return 1;
}

int keypress_login_handler (window_info *win, int mx, int my, Uint32 key, Uint32 unikey)
{
	Uint8 ch = key_to_char (unikey);

	// First check key presses common to all root windows. Many of these
	// don't make sense at this point, but it should be harmless.
	if ( keypress_root_common (key, unikey) )
	{
		return 1;
	}
	else if (ch == SDLK_RETURN && username_str[0] && password_str[0])
	{
		log_in_error_str[0] = '\0';
		send_login_info();
	}
	else if (ch == SDLK_TAB)
	{
		username_box_selected = !username_box_selected;
		password_box_selected = !password_box_selected;
	}
	else if (username_box_selected)
	{
		add_char_to_username (ch);
	}
	else
	{
		add_char_to_password (ch);
	}

	return 1;
}

int show_login_handler(window_info * win)
{
	hide_window(book_win);
	hide_window(paper_win);
	hide_window(color_race_win);
	hide_window(elconfig_win);
	hide_window(tab_help_win);
	return 1;
}

void create_login_root_window (int width, int height)
{
	if (login_root_win < 0)
	{
		login_root_win = create_window ("Login", -1, -1, 0, 0, width, height, ELW_TITLE_NONE|ELW_SHOW_LAST);

		set_window_handler (login_root_win, ELW_HANDLER_DISPLAY, &display_login_handler);
		set_window_handler (login_root_win, ELW_HANDLER_MOUSEOVER, &mouseover_login_handler);
		set_window_handler (login_root_win, ELW_HANDLER_CLICK, &click_login_handler);
		set_window_handler (login_root_win, ELW_HANDLER_KEYPRESS, &keypress_login_handler);
		set_window_handler (login_root_win, ELW_HANDLER_RESIZE, &resize_login_handler);
		set_window_handler (login_root_win, ELW_HANDLER_SHOW, &show_login_handler);

		resize_window (login_root_win, width, height);
	}
}
