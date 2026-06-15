#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>
#include "session.h"
#include "actors.h"
#include "asc.h"
#include "elwindows.h"
#include "init.h"
#include "gamewin.h"
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

static Uint64 session_exp[NUM_SKILLS];
static Uint64 max_exp[NUM_SKILLS];
static Uint64 last_exp[NUM_SKILLS];

#ifndef ENGLISH
static Uint64 fullsession_exp[NUM_SKILLS];
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

void set_last_skill_exp(size_t skill, Sint64 exp) {
	if (skill >= NUM_SKILLS) return;
	if (exp < 0) {
		Sint64 f = fullsession_exp[skill], s = session_exp[skill], d = -exp;
		fullsession_exp[skill] = d > f ? 0 : f - d;
		session_exp[skill] = d > s ? 0 : s - d;
		return;
	}
	last_exp[skill] = exp;
	if (exp > max_exp[skill]) max_exp[skill] = exp;
	if (exp < 1000 && skill <= SI_DEF) ++exphits.n[skill];
	if (skill != SI_ALL && exp_log_threshold > 0 && exp >= exp_log_threshold) {
		char b[256];
		safe_snprintf(b, sizeof(b), "Tu as gagné %" PRId64 " exp en %s.", exp, statsinfo[skill].skillnames->name);
		LOG_TO_CONSOLE(c_green2, b);
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

static inline void fmt_xpm(char *b, u64 e, f64 t) {
    char d[128] = {}, *o = b;
    int l, m = sizeof(d) - 1, i, n;
    if (t) l = snprintf(d, m, "%.1f", e / t);
    else l = snprintf(d, m, "%" PRIu64, e);
    for (i = 0, n = l - 2*(t != 0); i < n; ++i) {
        *o++ = d[i];
        if (n > 3 && n - i > 1 && (n - i) % 3 == 1) *o++ = '\'';
    }
    for (i = n; i < l; *o++ = d[i++]);
    *o = 0;
}
int display_session_handler(window_info *win) {
	char b[512], totxp[128], mtotxp[128], sessxp[128], msessxp[128];
	int x = 10, y = 21, sb = sizeof(b) - 1;
	glColor3f(1.0f, 1.0f, 1.0f);
	y -= 16;
	if (affixp) {
		draw_string_small(x + 58, y, (u8 *)"Affichage: Aéré", 1);
	} else {
		draw_string_small(x + 58, y, (u8 *)"Affichage: Normal", 1);
	}
	y += 32;
	if (affixp) {
		safe_snprintf(b, sb, "%-14s%-12s%-16s%-12s%-16s", "Compétences   ", "       Total", "      Exp/Minute", "     Session", "      Exp/Minute" );
		draw_string_small(x, y, (u8 *)b, 1);
	} else {
		safe_snprintf(b, sb, "%-13s%10s%9s%10s%9s%9s%9s", "Compétences", "Total", "Exp/min", "Session", "Exp/min", "Max", "Dernier" );
		draw_string_small(x, y, (u8 *)b, 1);
	}
	glDisable(GL_TEXTURE_2D);
	glColor3f(0.77f, 0.57f, 0.39f);
	glBegin(GL_LINES);
	glVertex3i(0, 53, 0);
	glVertex3i(win->len_x, 53, 0);
	glEnd();
	glEnable(GL_TEXTURE_2D);
	glColor3f(1.0f, 1.0f, 1.0f);
	y = 55;
	int fulltimediff = max2i(1, cur_time - fullsession_start_time), timediff = max2i(1, cur_time - session_start_time);
	f32 ft = fulltimediff / 60000.0f, st = timediff / 60000.0f;
	for (int i = 0; i < NUM_SKILLS; ++i) {
		if (last_mouse_click_y >= y && last_mouse_click_y < y + 16) {
			elglColourN("global.mouseselected");
		} else if (last_mouse_over_y >= y && last_mouse_over_y < y + 16) {
			elglColourN("global.mousehighlight");
		} else if (i & 1) {
			elglColourN("global.row.odd");
		} else {
			glColor3f(1.0f, 1.0f, 1.0f);
		}
		cstr skname = (cstr)statsinfo[i].skillnames->name;
		u64 e = *statsinfo[i].exp, df = min2q(999999999, subgt_u64(e, fullsession_exp[i])), ds = min2q(999999999, subgt_u64(e, session_exp[i]));
		if (affixp) {
			fmt_xpm(totxp, df, 0);
			fmt_xpm(mtotxp, df, ft);
			fmt_xpm(sessxp, ds, 0);
			fmt_xpm(msessxp, ds, st);
			safe_snprintf(b, sb, "%-14s%12s%16s%12s%16s", skname, totxp, mtotxp, sessxp, msessxp);
		} else {
			safe_snprintf(b, sb, "%-13s%10" PRIu64 "%9.1f%10" PRIu64 "%9.1f%9" PRIu64 "%9" PRIu64, skname, df, min2f(999999, df / ft), ds, min2f(999999, ds / st), min2q(99999999, max_exp[i]), min2q(99999999, last_exp[i]));
		}
		draw_string_small(x, y, (u8 *)b, 1);
		y += 16;
	}
	y += 16;
	glColor3f(1.0f, 1.0f, 1.0f);
	draw_string_small(x, y, (u8 *)"Durée de la session", 1);
	safe_snprintf(b, sb, "%02d:%02d:%02d", fulltimediff / 3600000, (fulltimediff / 60000) % 60, (fulltimediff / 1000) % 60);
	draw_string_small(x + 214, y, (u8 *)b, 1);
	safe_snprintf(b, sb, "%02d:%02d:%02d", timediff / 3600000, (timediff / 60000) % 60, (timediff / 1000) % 60);
	draw_string_small(x + 444, y, (u8 *)b, 1);
	y += 16;
	draw_string_small(x, y, (u8 *)"Distance", 1);
	safe_snprintf(b, sb, "%d", distance_moved < 0 ? 0 : distance_moved);
	draw_string_small(x + 200, y, (u8 *)b, 1);
	if (show_reset_help) {
		show_help(session_reset_help, 0, win->len_y+10);
		show_reset_help = 0;
	}
#ifdef OPENGL_TRACE
	CHECK_GL_ERRORS();
#endif
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
