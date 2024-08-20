#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>
#include "session.h"
#include "actors.h"
#include "asc.h"
#include "elwindows.h"
#include "init.h"
#include "global.h"
#include "hud.h"
#ifdef MISSILES
#include "missiles.h"
#endif //MISSILES
#include "multiplayer.h"
#include "named_colours.h"
#include "platform.h"
#include "sound.h"
#include "stats.h"
#include "translate.h"
#include "counters.h"
#ifdef OPENGL_TRACE
#include "gl_init.h"
#endif
#include "widgets.h"

int affixp = 0;
int session_win = -1;
int exp_log_threshold = 5000;
static int reconnecting = 0;
static int last_port = -1;
static unsigned char last_server_address[60];
static int show_reset_help = 0;
static int last_mouse_click_y = -1;
static int last_mouse_over_y = -1;
static int distance_moved = -1;

static Uint32 session_exp[NUM_SKILLS];
static Uint32 max_exp[NUM_SKILLS];
static Uint32 last_exp[NUM_SKILLS];

#ifndef ENGLISH
static Uint32 fullsession_exp[NUM_SKILLS];
Uint32 fullsession_start_time;
#endif //ENGLISH
Uint32 session_start_time;

int display_session_handler(window_info *win);

#ifdef MISSILES
int get_session_exp_ranging(void)
{
	return *(statsinfo[SI_RAN].exp) - session_exp[SI_RAN];
}
#endif //MISSILES

static int mouseover_session_reset_handler(void)
{
	if (!disable_double_click && show_help_text)
		show_reset_help = 1;
	return 0;
}

static int click_session_handler(window_info *win, int mx, int my, Uint32 flags)
{
	if (flags & (ELW_WHEEL_UP|ELW_WHEEL_DOWN))
		return 0;
	last_mouse_click_y = my;
	do_click_sound();
	return 1;
}

static int mouseover_session_handler(window_info *win, int mx, int my)
{
	last_mouse_over_y = my;
	return 1;
}

void fill_session_win(void)
{
	int reset_button_id = -1;
#ifdef FR_VERSION
	int affichagexp_button_id = -2;
#endif //FR_VERSION
	set_window_handler(session_win, ELW_HANDLER_DISPLAY, &display_session_handler);
	set_window_handler(session_win, ELW_HANDLER_CLICK, &click_session_handler );
	set_window_handler(session_win, ELW_HANDLER_MOUSEOVER, &mouseover_session_handler );
	reset_button_id=button_add_extended(session_win, reset_button_id, NULL, 450, 280, 0, 0, 0, 1.0f, 0.77f, 0.57f, 0.39f, reset_str);
	widget_set_OnClick(session_win, reset_button_id, session_reset_handler);
	widget_set_OnMouseover(session_win, reset_button_id, mouseover_session_reset_handler);
#ifdef FR_VERSION
	affichagexp_button_id=button_add_extended(session_win, affichagexp_button_id, NULL, 10, 5, 0, 0, 0, 0.5f, 0.77f, 0.57f, 0.39f, "Changer");
	widget_set_OnClick(session_win, affichagexp_button_id, session_affichagexp);
#endif //FR_VERSION
}

void set_last_skill_exp(size_t skill, int exp)
{
	if (skill < NUM_SKILLS)
	{
		last_exp[skill] = exp;
		if (exp > max_exp[skill])
			max_exp[skill] = exp;
		if ((skill != SI_ALL) && (exp >= exp_log_threshold) && (exp_log_threshold > 0))
		{
			char str[80];
#ifdef FR_VERSION
			safe_snprintf(str, sizeof(str), "Tu as gagné %d exp en %s.", exp, statsinfo[skill].skillnames->name);
#else //FR_VERSION
			safe_snprintf(str, sizeof(str), "You gained %d exp for %s.", exp, statsinfo[skill].skillnames->name);
#endif //FR_VERSION
			LOG_TO_CONSOLE(c_green2,str);
		}
	}
}

void set_session_exp_to_current(void)
{
	int i;
	for (i=0; i<NUM_SKILLS; i++)
	{
		max_exp[i] = last_exp[i] = 0;
		session_exp[i] = *(statsinfo[i].exp);
	}
}

void update_session_distance(void)
{
	static int last_x = -1, last_y = -1;
	actor *me = get_our_actor ();
	if (me == NULL)
		return;
	if ((me->x_tile_pos != last_x) || (me->y_tile_pos != last_y))
	{
		last_x = me->x_tile_pos;
		last_y = me->y_tile_pos;
		distance_moved++;
	}
}

int display_session_handler(window_info *win)
{
#ifndef ENGLISH
	int fulltimediff;
#endif //ENGLISH
	int i, x, y, timediff;
	char buffer[128];
	float oa_exp;

	char correcwarning[30];
	char provi[17];
	char *finxpm = NULL;
	char totxp[13];
	char mtotxp[17];
	char sessxp[13];
	char msessxp[17];
	char compe[15];
	char lignecomplete[72];
	char chmilliers[4];
	char chcentaines[4];
	int millions, milliers, nbmillions, nbmilliers, centaines;
	float xpm;
	int esp;

	x = 10;
	y = 21;
	timediff = 0;
	oa_exp = 0.0f;

	glColor3f(1.0f, 1.0f, 1.0f);
#ifdef ENGLISH
	safe_snprintf(buffer, sizeof(buffer), "%-20s%-17s%-17s%-17s", "Skill", "Total Exp", "Max Exp", "Last Exp" );
	draw_string_small(x, y, (unsigned char*)buffer, 1);
	glDisable(GL_TEXTURE_2D);
	glColor3f(0.77f, 0.57f, 0.39f);
	glBegin(GL_LINES);
	glVertex3i(0, 37, 0);
	glVertex3i(win->len_x, 37, 0);
#else //ENGLISH
	y -= 16;
	if (affixp == 0) 
	{
		draw_string_small(x + 58, y, (unsigned char*)"Affichage: Normal", 1);
	} else draw_string_small(x + 58, y, (unsigned char*)"Affichage: Aéré", 1);
	y += 32;
	if(affixp == 0)
	{
		safe_snprintf(buffer, sizeof(buffer), "%-20s%-13s%-13s%-13s%-13s", "Compétences", "Total Exp", "Session Exp", "Max Exp", "Dernier Exp" );
		draw_string_small(x, y, (unsigned char*)buffer, 1);
	} else {
		safe_snprintf(buffer, sizeof(buffer), "%-14s%-12s%-16s%-12s%-16s", "Compétences   ", "       Total", "      Exp/Minute", "     Session", "      Exp/Minute" );
		draw_string_small(x, y, (unsigned char*)buffer, 1);
	}
	glDisable(GL_TEXTURE_2D);
	glColor3f(0.77f, 0.57f, 0.39f);
	glBegin(GL_LINES);
	glVertex3i(0, 53, 0);
	glVertex3i(win->len_x, 53, 0);
#endif //ENGLISH
	glEnd();
	glEnable(GL_TEXTURE_2D);
	glColor3f(1.0f, 1.0f, 1.0f);

	y = 55;
#ifndef ENGLISH
	fulltimediff = cur_time - fullsession_start_time;
	if(fulltimediff<=0) fulltimediff=1;
	timediff = cur_time - session_start_time;
	if(timediff<=0) timediff=1;
#endif //ENGLISH

	for (i=0; i<NUM_SKILLS; i++)
	{
		if ((last_mouse_click_y >= y) && (last_mouse_click_y < y+16))
			elglColourN("global.mouseselected");
		else if ((last_mouse_over_y >= y) && (last_mouse_over_y < y+16))
			elglColourN("global.mousehighlight");
		else
			glColor3f(1.0f, 1.0f, 1.0f);
#ifdef FR_VERSION
		if(affixp == 0)
		{
			safe_snprintf(buffer, sizeof(buffer), "%-20s%-7u%-6.1f%-7u%-6.1f%-13u%-13u", statsinfo[i].skillnames->name, *(statsinfo[i].exp) - fullsession_exp[i], (float)(*(statsinfo[i].exp) - fullsession_exp[i])/((float)fulltimediff/60000.0f), *(statsinfo[i].exp) - session_exp[i], (float)(*(statsinfo[i].exp) - session_exp[i])/((float)timediff/60000.0f), max_exp[i], last_exp[i]);
		} else {
			if ((*(statsinfo[i].exp) - fullsession_exp[i]) >= 1000000) {
				millions = (*(statsinfo[i].exp) - fullsession_exp[i]);
				nbmillions = millions / 1000000;	
				milliers = millions % 1000000;
				nbmilliers = milliers / 1000;
				centaines = milliers % 1000;
				if (nbmilliers == 0) {
					sprintf(chmilliers, "000");
				} else if (nbmilliers < 10) {
					sprintf(chmilliers, "00%d", nbmilliers);
				} else if (nbmilliers < 100) {
					sprintf(chmilliers, "0%d", nbmilliers);
				} else sprintf(chmilliers, "%d", nbmilliers);
				if (centaines == 0) {
					sprintf(chcentaines, "000");
				} else if (centaines < 10) {
					sprintf(chcentaines, "00%d", centaines);
				} else if (centaines < 100) {
					sprintf(chcentaines, "0%d", centaines);
				} else sprintf(chcentaines, "%d", centaines);
				sprintf(totxp, "%d'%s'%s", nbmillions, chmilliers, chcentaines);
			} else if ((*(statsinfo[i].exp) - fullsession_exp[i]) >= 1000) {
				milliers = (*(statsinfo[i].exp) - fullsession_exp[i]);
				nbmilliers = milliers / 1000;
				centaines = milliers % 1000;
				if (centaines == 0) {
					sprintf(chcentaines, "000");
				} else if (centaines < 10) {
					sprintf(chcentaines, "00%d", centaines);
				} else if (centaines < 100) {
					sprintf(chcentaines, "0%d", centaines);
				} else sprintf(chcentaines, "%d", centaines);
				sprintf(totxp, "%d'%s", nbmilliers, chcentaines);
			} else {
				sprintf(totxp, "%d", *(statsinfo[i].exp) - fullsession_exp[i]);			
			}
			if ((float)(*(statsinfo[i].exp) - fullsession_exp[i])/((float)fulltimediff/60000.0f) >= 1000){
				xpm = (float)(*(statsinfo[i].exp) - fullsession_exp[i])/((float)fulltimediff/60000.0f);
				sprintf(provi, "%-.1f", xpm);
				finxpm = strchr(provi, '.');
				milliers = (int)xpm;
				nbmilliers = milliers / 1000;
				centaines = milliers % 1000;
				if (centaines == 0) {
					sprintf(chcentaines, "000");
				} else if (centaines < 10) {
					sprintf(chcentaines, "00%d", centaines);
				} else if (centaines < 100) {
					sprintf(chcentaines, "0%d", centaines);
				} else sprintf(chcentaines, "%d", centaines);
				sprintf(mtotxp, "%d'%s%s xp/mn", nbmilliers, chcentaines, finxpm);
			} else	{
				sprintf(mtotxp, "%.1lf xp/mn", (float)(*(statsinfo[i].exp)-fullsession_exp[i])/((float)fulltimediff/60000.0f));
			}
			if ((*(statsinfo[i].exp) - session_exp[i]) >= 1000000) {
				millions = (*(statsinfo[i].exp) - session_exp[i]);
				nbmillions = millions / 1000000;	
				milliers = millions % 1000000;
				nbmilliers = milliers / 1000;
				centaines = milliers % 1000;
				if (nbmilliers == 0) {
					sprintf(chmilliers, "000");
				} else if (nbmilliers < 10) {
					sprintf(chmilliers, "00%d", nbmilliers);
				} else if (nbmilliers < 100) {
					sprintf(chmilliers, "0%d", nbmilliers);
				} else sprintf(chmilliers, "%d", nbmilliers);
				if (centaines == 0) {
					sprintf(chcentaines, "000");
				} else if (centaines < 10) {
					sprintf(chcentaines, "00%d", centaines);
				} else if (centaines < 100) {
					sprintf(chcentaines, "0%d", centaines);
				} else sprintf(chcentaines, "%d", centaines);
				sprintf(sessxp, "%d'%s'%s", nbmillions, chmilliers, chcentaines);
			} else if ((*(statsinfo[i].exp) - session_exp[i]) >= 1000) {
				milliers = (*(statsinfo[i].exp) - session_exp[i]);
				nbmilliers = milliers / 1000;
				centaines = milliers % 1000;
				if (centaines == 0) {
					sprintf(chcentaines, "000");
				} else if (centaines < 10) {
					sprintf(chcentaines, "00%d", centaines);
				} else if (centaines < 100) {
					sprintf(chcentaines, "0%d", centaines);
				} else sprintf(chcentaines, "%d", centaines);
				sprintf(sessxp, "%d'%s", nbmilliers, chcentaines);
			} else	{
				sprintf(sessxp, "%d", *(statsinfo[i].exp) - session_exp[i]);
			}
			if ((float)(*(statsinfo[i].exp) - session_exp[i])/((float)timediff/60000.0f) >= 1000){
				xpm = (float)(*(statsinfo[i].exp) - session_exp[i])/((float)timediff/60000.0f);
				sprintf(provi, "%-.1f", xpm);
				finxpm = strchr(provi, '.');
				milliers = (int)xpm;
				nbmilliers = milliers / 1000;
				centaines = milliers % 1000;
				if (centaines == 0) {
					sprintf(chcentaines, "000");
				} else if (centaines < 10) {
					sprintf(chcentaines, "00%d", centaines);
				} else if (centaines < 100) {
					sprintf(chcentaines, "0%d", centaines);
				} else sprintf(chcentaines, "%d", centaines);
				sprintf(msessxp, "%d'%s%s xp/mn", nbmilliers, chcentaines, finxpm);
			} else	{
				sprintf(msessxp, "%.1lf xp/mn", (float)(*(statsinfo[i].exp) - session_exp[i])/((float)timediff/60000.0f));
			}
			sprintf(correcwarning, "%s", statsinfo[i].skillnames->name);
			strcpy(compe, correcwarning);
			strcpy(correcwarning, "");
			if (strlen(compe) < 14)
			{
			    for (esp = strlen(compe); esp != 14; esp++)
			    {
				strcat(correcwarning, " ");
			    }
			    strcat(compe, correcwarning);
			    strcpy(correcwarning, "");
			}
			if (strlen(totxp) < 12)
			{
			    for (esp = strlen(totxp); esp != 12; esp++)
			    {
				strcat(correcwarning, " ");
			    }
			    strcat(correcwarning, totxp);
			    strcpy(totxp, correcwarning);
			    strcpy(correcwarning, "");
			}
			if (strlen(mtotxp) < 16)
			{
			    for (esp = strlen(mtotxp); esp != 16; esp++)
			    {
				strcat(correcwarning, " ");
			    }
			    strcat(correcwarning, mtotxp);
			    strcpy(mtotxp, correcwarning);
			    strcpy(correcwarning, "");
			}
			if (strlen(sessxp) < 12)
			{
			    for (esp = strlen(sessxp); esp != 12; esp++)
			    {
				strcat(correcwarning, " ");
			    }
			    strcat(correcwarning, sessxp);
			    strcpy(sessxp, correcwarning);
			    strcpy(correcwarning, "");
			}
			if (strlen(msessxp) < 16)
			{
			    for (esp = strlen(msessxp); esp != 16; esp++)
			    {
				strcat(correcwarning, " ");
			    }
			    strcat(correcwarning, msessxp);
			    strcpy(msessxp, correcwarning);
			    strcpy(correcwarning, "");
			}
			sprintf(lignecomplete, "%s%s%s%s%s", compe, totxp, mtotxp, sessxp, msessxp);
			safe_snprintf(buffer, sizeof(buffer), "%s", lignecomplete);
		}
#else //FR_VERSION
		safe_snprintf(buffer, sizeof(buffer), "%-20s%-17u%-17u%-17u", statsinfo[i].skillnames->name, *(statsinfo[i].exp) - session_exp[i], max_exp[i], last_exp[i]);
#endif //FR_VERSION
		draw_string_small(x, y, (unsigned char*)buffer, 1);
		y += 16;
		if(i < NUM_SKILLS-1)
			oa_exp += *(statsinfo[i].exp) - session_exp[i];
	}

	y += 16;

	glColor3f(1.0f, 1.0f, 1.0f);

#ifdef ENGLISH
	draw_string_small(x, y, (unsigned char*)"Session Time", 1);
	timediff = cur_time - session_start_time;
	safe_snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", timediff/3600000, (timediff/60000)%60, (timediff/1000)%60);
	draw_string_small(x + 200, y, (unsigned char*)buffer, 1);
#else //ENGLISH
	draw_string_small(x, y, (unsigned char*)"Durée de la session", 1);
	safe_snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", fulltimediff/3600000, (fulltimediff/60000)%60, (fulltimediff/1000)%60);
	draw_string_small(x + 214, y, (unsigned char*)buffer, 1);
	safe_snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", timediff/3600000, (timediff/60000)%60, (timediff/1000)%60);
	draw_string_small(x + 444, y, (unsigned char*)buffer, 1);
#endif //ENGLISH

#ifdef ENGLISH
	y += 16;

	draw_string_small(x, y, (unsigned char*)"Exp/Min", 1);

	if(timediff<=0){
		timediff=1;
	}
	safe_snprintf(buffer, sizeof(buffer), "%2.2f", oa_exp/((float)timediff/60000.0f));
	draw_string_small(x + 200, y, (unsigned char*)buffer, 1);
#endif //ENGLISH

	y += 16;
	draw_string_small(x, y, (unsigned char*)"Distance", 1);

	safe_snprintf(buffer, sizeof(buffer), "%d", (distance_moved<0) ?0: distance_moved);
	draw_string_small(x + 200, y, (unsigned char*)buffer, 1);
	
	if (show_reset_help)
	{
		show_help(session_reset_help, 0, win->len_y+10);
		show_reset_help = 0;
	}

#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE

	return 1;
}

void init_session(void)
{
	int save_server = 1;

	/* if we have server info saved, compare with current */
	if (last_port > 0)
	{
		/* if changed, we need to reset the session stats */
		if ((last_port != port) || (strcmp((char *)last_server_address, (char *)server_address)))
		{
			LOG_TO_CONSOLE(c_red2,"Server changed so resetting session stats");
			reconnecting = 0;
		}
		/* else if the same, no need */
		else
			save_server = 0;
	}

	/* save the server info if first time or changed */
	if (save_server)
	{
		last_port = port;
		safe_strncpy((char *)last_server_address, (char *)server_address, sizeof(last_server_address));
	}

	if (!reconnecting){
#ifndef ENGLISH
		int i;
		fullsession_start_time = cur_time;
		for (i=0; i<NUM_SKILLS; i++)
		{
			fullsession_exp[i] = *(statsinfo[i].exp);
		}
#endif //ENGLISH
		set_session_exp_to_current();
		session_start_time = cur_time;
		reconnecting = 1;
	}
	else if ( disconnect_time != 0 ) {
		session_start_time += (cur_time-disconnect_time);
		disconnect_time = 0;
	}
}

int session_reset_handler(void)
{
	static Uint32 last_click = 0;
	/* provide some protection for inadvertent pressing (double click that can be disabled) */
	if (safe_button_click(&last_click))
	{
		init_session();
		set_session_exp_to_current();
		session_start_time = cur_time;
		reset_session_counters();
#ifdef MISSILES
		range_critical_hits = 0;
		range_success_hits = 0;
		range_total_shots = 0;
#endif //MISSILES
		distance_moved = 0;
	}
	return 0;
}

int session_affichagexp(void)
{
	if(affixp == 0)
	{
		affixp = 1;
	} else {
		affixp = 0;
	}
	return 0;
}
