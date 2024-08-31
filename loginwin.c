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
#include "sound.h"
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
int log_in_x_len = 82;
int log_in_y_len = 34;

int new_char_x;
int new_char_y;
int new_char_x_len = 160;
int new_char_y_len = 34;

int settings_x;
int settings_y;
int settings_x_len = 63;
int settings_y_len = 34;

int cadre_x;
int cadre_y;
int cadre_x_len = 512;
int cadre_y_len = 447;

char log_in_button_selected = 0;
char new_char_button_selected = 0;
char settings_button_selected = 0;

void init_login_screen ()
{
	CHECK_GL_ERRORS();
	login_screen_menus = load_texture_cached("textures/login_menu.dds", tt_image);
	login_text = load_texture_cached("textures/login_back.dds", tt_image);
	CHECK_GL_ERRORS();
}

void set_login_error (const char *msg, int len, int print_err)
{
	int snd;
	if (len <= 0)
	{
		// server didn't send a message, use the default
		safe_snprintf (log_in_error_str, sizeof(log_in_error_str), "%s: %s", reg_error_str, invalid_pass);
	}
	else if (print_err)
	{
		safe_snprintf (log_in_error_str, sizeof (log_in_error_str), "%.*s", len, msg);
	}
	else
	{
		safe_strncpy2 (log_in_error_str, msg, sizeof (log_in_error_str), len);
	}
	reset_soft_breaks (log_in_error_str, strlen (log_in_error_str), sizeof (log_in_error_str), 1.0, window_width, NULL, NULL);

	if ((snd = get_index_for_sound_type_name("Login Error")) > -1)
		add_sound_object(snd, 0, 0, 1);
}

int resize_login_handler (window_info *win, Uint32 w, Uint32 h)
{
	int half_screen_x = w / 2;
	int half_screen_y = h / 2;
	int len1 = strlen (login_username_str);
	int len2 = strlen (login_password_str);
	int offset = 60 + (len1 > len2 ? (len1+1) * 16 : (len2+1) * 16);

	username_text_x = half_screen_x - offset;
	username_text_y = half_screen_y - 107;

	password_text_x = half_screen_x - offset;
	password_text_y = half_screen_y - 75;

	username_bar_x = half_screen_x - username_bar_x_len/2;
	username_bar_y = half_screen_y - username_bar_y_len/2 - 96;

	password_bar_x = half_screen_x - password_bar_x_len/2;
	password_bar_y = half_screen_y - password_bar_y_len/2 - 67;

	log_in_x = half_screen_x - log_in_x_len/2 - 1;
	log_in_y = half_screen_y - 45;

	settings_x = half_screen_x - settings_x_len/2;
	settings_y = half_screen_y + 34;

	new_char_x = half_screen_x - new_char_x_len/2;
	new_char_y = half_screen_y - 3;

    cadre_x = half_screen_x - cadre_x_len/2;
    cadre_y = half_screen_y - cadre_y_len/2;

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

	draw_console_pic(login_text);


	// start drawing the actual interface pieces
	bind_texture(login_screen_menus);
	glColor3f (1.0f,1.0f,1.0f);
	glBegin (GL_QUADS);

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


	glEnd();

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
	glColor3f (1.0f, 0.0f, 0.0f);

	// print the current error, if any

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
