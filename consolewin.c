#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <SDL.h>
#include "consolewin.h"
#include "asc.h"
#include "books.h"
#include "chat.h"
#include "console.h"
#include "draw_scene.h"
#include "elwindows.h"
#include "gamewin.h"
#include "gl_init.h"
#include "global.h"
#include "hud.h"
#include "init.h"
#include "interface.h"
#include "mapwin.h"
#include "new_character.h"
#if !defined OSX && !defined WINDOWS
#include "paste.h"
#endif
#include "spells.h"
#include "text.h"
#include "special_effects.h"
#include "eye_candy_wrapper.h"
#define CONSOLE_SCROLLBAR_ID 2222
#define CONSOLE_SCROLLBAR_WIDTH 10
int console_root_win = -1;
int locked_to_console = 0;
int console_out_id = 40;
int console_in_id = 41;
int nr_console_lines = 0;
static int total_nr_lines = 0;
static int scroll_up_lines = 0;
int console_text_changed = 0;
int console_text_width = -1;
int console_scroll_drag(widget_list *widget, int mx, int my, Uint32 flags, int dx, int dy) {
	int line = vscrollbar_get_pos(console_root_win, widget->id);
	if (line != get_total_nr_lines() - nr_console_lines - scroll_up_lines) {
		scroll_up_lines = total_nr_lines - nr_console_lines - line;
		console_text_changed = 1;
	}
	return 0;
}
int console_scroll_click(widget_list *widget, int mx, int my, Uint32 flags) {
	int line = vscrollbar_get_pos(console_root_win, widget->id);
	if (line != total_nr_lines - nr_console_lines - scroll_up_lines) {
		scroll_up_lines = total_nr_lines - nr_console_lines - line;
		console_text_changed = 1;
	}
	return 0;
}
int display_console_handler(window_info *win) {
	static int msg = 0, offset = 0;
	// are we actively drawing things?
	if (SDL_GetAppState() & SDL_APPACTIVE) {
		set_font(chat_font);    // switch to the chat font
		if (console_text_changed) {
			find_line_nr(total_nr_lines, total_nr_lines - nr_console_lines - scroll_up_lines, FILTER_ALL, &msg, &offset, chat_zoom, console_text_width);
			text_field_set_buf_pos(console_root_win, console_out_id, msg, offset);
			console_text_changed = 0;
			vscrollbar_set_bar_len(console_root_win, CONSOLE_SCROLLBAR_ID, total_nr_lines);
			widget_set_size(console_root_win, CONSOLE_SCROLLBAR_ID, nr_console_lines);
			vscrollbar_set_pos(console_root_win, CONSOLE_SCROLLBAR_ID, total_nr_lines - nr_console_lines - scroll_up_lines);
		}
		draw_console_pic(cons_text);
		// afficher les sépérateurs ("^^...") avec la fonte système
		set_font(0);    // switch to fixed
		if (scroll_up_lines != 0) {
			const unsigned char *sep_string = (unsigned char *)"^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^ ^^";
			glColor3f(1.0, 1.0, 1.0);
			// séparateurs avec zoom=1 (car CONSOLE_SEP_HEIGHT reste une constante à 18 !) et au dessus de l'input (quelque soit le HUD !)
//			draw_string_width(10, win->len_y - input_widget->len_y - CONSOLE_SEP_HEIGHT - HUD_MARGIN_Y, sep_string, win->len_x - hud_x - 20, 1);
			draw_string_zoomed_clipped(10, win->len_y - input_widget->len_y - HUD_MARGIN_Y - 10, sep_string, -1, win->len_x - hud_x - 20, DEFAULT_FONT_Y_LEN, 1.0f);
		}
		//ttlanhil: disabled, until the scrolling in console is adusted to work with filtering properly
		//if the users prefer that console not be filtered, the following line can be removed.
		//if they want it filtered, then more work can be done until it works properly
		//((text_field*)((widget_find(console_root_win, console_out_id))->widget_info))->chan_nr = current_filter;
		draw_hud_interface();
	}
	if (special_effects) {
		display_special_effects(0);
	}
	// remember the time stamp to improve FPS quality when switching modes
	next_fps_time = cur_time + 1000;
	last_count = 0;
	ec_idle();
	update_camera();
	draw_delay = 20;
	if ((input_widget != NULL) && (input_widget->window_id != win->window_id)) {
		input_widget_move_to_win(win->window_id);
	}
	return 1;
}
static int keypress_console_handler(window_info *win, int mx, int my, Uint32 key, Uint32 unikey) {
	Uint16 keysym = key & 0xffff;
	/* TonyFlow: on transmet ces touches directement au widget de saisie du tchat
	 * avant qu'elles ne soient prises par l'interface pour bouger la camera ou le perso
	 */
	if ((keysym == SDLK_LEFT) || (keysym == SDLK_RIGHT) || (keysym == SDLK_HOME) || (keysym == SDLK_END) || (keysym == SDLK_DELETE) || (keysym == SDLK_INSERT)) {
		chat_input_key(input_widget, 0, 0, key, unikey);
		return 1;
	}
	// first try the keypress handler for all root windows
	if ( keypress_root_common(key, unikey)) {
		return 1;
	} else if (keysym == SDLK_UP) {
		if (total_nr_lines > nr_console_lines + scroll_up_lines) {
			scroll_up_lines++;
			console_text_changed = 1;
		}
	} else if (keysym == SDLK_DOWN) {
		if (scroll_up_lines > 0) {
			scroll_up_lines--;
			console_text_changed = 1;
		}
	} else if (key == K_TABCOMPLETE && input_text_line.len > 0) {
		do_tab_complete(&input_text_line);
	} else if (key & ELW_ALT && keysym == SDLK_PAGEUP && total_nr_lines > nr_console_lines + scroll_up_lines) {
		scroll_up_lines = total_nr_lines - nr_console_lines;
		console_text_changed = 1;
	} else if (key & ELW_ALT && keysym == SDLK_PAGEDOWN && scroll_up_lines > 0) {
		scroll_up_lines = 0;
		console_text_changed = 1;
	} else if (keysym == SDLK_PAGEUP && total_nr_lines > nr_console_lines + scroll_up_lines) {
		scroll_up_lines += nr_console_lines - 1;
		if (nr_console_lines + scroll_up_lines > total_nr_lines) {
			scroll_up_lines = total_nr_lines - nr_console_lines;
		}
		console_text_changed = 1;
	} else if (keysym == SDLK_PAGEDOWN && scroll_up_lines > 0) {
		scroll_up_lines -= nr_console_lines - 1;
		if (scroll_up_lines < 0) {
			scroll_up_lines = 0;
		}
		console_text_changed = 1;
	} else if ((key == K_MAP) || (key == K_MARKFILTER)) {
		if (!locked_to_console && switch_to_game_map()) {
			// if K_MARKFILTER pressed, open the map window with the filter active
			if (key == K_MARKFILTER) {
				mark_filter_active = 1;
			}
			hide_window(console_root_win);
			show_window(map_root_win);
		}
	} else {
		Uint8 ch = key_to_char(unikey);
		reset_tab_completer();
		if ((ch == '`' || key == K_CONSOLE) && !locked_to_console) {
			if (keep_grabbing_mouse) {
				toggle_have_mouse();
				keep_grabbing_mouse = 0;
			}
			hide_window(console_root_win);
			show_window(game_root_win);
			// Undo stupid quickbar hack
			if ( !get_show_window(quickbar_win)) {
				show_window(quickbar_win);
			}
			if ( !get_show_window(quickspell_win)) {
				show_window(quickspell_win);
			}
		} else if ( !text_input_handler(key, unikey)) {
			// nothing we can handle
			return 0;
		}
	}
	// we handled it, return 1 to let the window manager know
	return 1;
}
static int resize_console_handler(window_info *win, int width, int height) {
	widget_resize(console_root_win, console_out_id, width - hud_x - 20, height - HUD_MARGIN_Y - input_widget->len_y - 20 - nb_ligne_tabs * tab_bar_height);
	widget_resize(console_root_win, input_widget->id, width - hud_x, input_widget->len_y);
	widget_move(console_root_win, input_widget->id, 0, height - input_widget->len_y - HUD_MARGIN_Y);
	nr_console_lines = (height - input_widget->len_y - HUD_MARGIN_Y - 20 - nb_ligne_tabs * tab_bar_height - 1) / (int)(DEFAULT_FONT_Y_LEN * chat_zoom);
	console_text_width = (int)(width - hud_x - 20);
	return 1;
}
static int click_console_handler(window_info *win, int mx, int my, Uint32 flags) {
#if !defined OSX && !defined WINDOWS
	if ((flags & ELW_MID_MOUSE)) {
		start_paste_from_primary(NULL);
	} else
#endif
	if ((flags & ELW_WHEEL_UP) && total_nr_lines > nr_console_lines + scroll_up_lines ) {
		scroll_up_lines++;
		console_text_changed = 1;
	} else if ((flags & ELW_WHEEL_DOWN) && scroll_up_lines > 0 ) {
		scroll_up_lines--;
		console_text_changed = 1;
	} else {
		return 0; // we didn't handle it
	}
	return 1;
}
static int show_console_handler(window_info *win) {
	int i;
	for (i = 0; i < MAX_CHAT_TABS; i++) {
		if (channels[i].open) {
			tab_set_label_color_by_id(chat_win, chat_tabcollection_id, channels[i].tab_id, -1.0f, -1.0f, -1.0f);
		}
	}
	hide_window(color_race_win);
	if (use_windowed_chat == 1) {
		display_tab_bar();
	}
	return 1;
}
int get_console_text_width(void) {
	return console_text_width;
}
int get_total_nr_lines(void) {
	return total_nr_lines;
}
void clear_console() {
	console_text_changed = 1;
	lines_to_show = 0;
	not_from_the_end_console = 0;
	scroll_up_lines = 0;
	total_nr_lines = 0;
}
void update_console_win(text_message *msg) {
	int nlines = rewrap_message(msg, chat_zoom, chat_font, console_text_width, NULL);
	if (msg->deleted) {
		if (scroll_up_lines > nlines) {
			scroll_up_lines -= nlines;
		} else {
			scroll_up_lines = 0;
			console_text_changed = 1;
		}
	} else {
		if (scroll_up_lines == 0) {
			console_text_changed = 1;
		} else {
			scroll_up_lines += nlines;
			if (scroll_up_lines > DISPLAY_TEXT_BUFFER_SIZE) {
				scroll_up_lines = DISPLAY_TEXT_BUFFER_SIZE;
			}
		}
		total_nr_lines += nlines;
	}
}
void create_console_root_window(int width, int height) {
	if (console_root_win < 0) {
		console_root_win = create_window("Console", -1, -1, 0, 0, width, height, ELW_TITLE_NONE | ELW_SHOW_LAST);
		set_window_handler(console_root_win, ELW_HANDLER_DISPLAY, &display_console_handler);
		set_window_handler(console_root_win, ELW_HANDLER_KEYPRESS, &keypress_console_handler);
		set_window_handler(console_root_win, ELW_HANDLER_RESIZE, &resize_console_handler);
		set_window_handler(console_root_win, ELW_HANDLER_CLICK, &click_console_handler);
		set_window_handler(console_root_win, ELW_HANDLER_SHOW, &show_console_handler);
		console_out_id = text_field_add_extended(console_root_win, console_out_id, NULL, 10, 10 + nb_ligne_tabs * tab_bar_height, width - hud_x - 20, height - INPUT_HEIGHT - HUD_MARGIN_Y - 20 - nb_ligne_tabs * tab_bar_height, 0, chat_zoom, chat_font, -1.0f, -1.0f, -1.0f, display_text_buffer, DISPLAY_TEXT_BUFFER_SIZE, CHAT_ALL, 0, 0);
		if (input_widget == NULL) {
			Uint32 id;
			id = text_field_add_extended(console_root_win, 0, NULL, 0, height - INPUT_HEIGHT - HUD_MARGIN_Y, width - hud_x, INPUT_HEIGHT, (INPUT_DEFAULT_FLAGS | TEXT_FIELD_BORDER) ^ WIDGET_CLICK_TRANSPARENT, chat_zoom, chat_font, 0.77f, 0.57f, 0.39f, &input_text_line, 1, FILTER_ALL, INPUT_MARGIN, INPUT_MARGIN);
			input_widget = widget_find(console_root_win, id);
			input_widget->OnResize = input_field_resize;
		} else {
			input_widget_move_to_win(console_root_win);
		}
		widget_set_OnKey(input_widget->window_id, input_widget->id, chat_input_key);
		nr_console_lines = (height - input_widget->len_y - HUD_MARGIN_Y - 20 - nb_ligne_tabs * tab_bar_height - 1) / (int)(DEFAULT_FONT_Y_LEN * chat_zoom);
		console_text_width = (int)(width - hud_x - 20);
		vscrollbar_add_extended(console_root_win, CONSOLE_SCROLLBAR_ID, NULL, 0, 0, CONSOLE_SCROLLBAR_WIDTH, height - INPUT_HEIGHT - HUD_MARGIN_Y, 0, nr_console_lines, 0.77f, 0.57f, 0.39f, 0, 1, 0);
		widget_set_OnDrag(console_root_win, CONSOLE_SCROLLBAR_ID, console_scroll_drag);
		widget_set_OnClick(console_root_win, CONSOLE_SCROLLBAR_ID, console_scroll_click);
	}
}
int input_field_resize(widget_list *w, Uint32 x, Uint32 y) {
	window_info *console_win = &windows_list.window[console_root_win];
	widget_list *console_out_w = widget_find(console_root_win, console_out_id);
	text_field *tf = w->widget_info;
	text_message *msg = &(tf->buffer[tf->msg]);
	int tmp_chan = msg->chan_idx;
	// set invalid width to force rewrap
	msg->wrap_width = 0;
	//Set to CHAT_NONE so rewrap_message doesn't mess with total_nr_lines.
	msg->chan_idx = CHAT_NONE;
	tf->nr_lines = rewrap_message(msg, w->size, tf->font_num, w->len_x - 2 * tf->x_space, &tf->cursor);
	msg->chan_idx = tmp_chan;
	if (use_windowed_chat != 2 || !get_show_window(chat_win)) {
		window_info *win = &windows_list.window[w->window_id];
		widget_move(input_widget->window_id, input_widget->id, 0, win->len_y - input_widget->len_y - HUD_MARGIN_Y);
	}
	// redimensionnement de la console out en conséquence
	widget_resize(console_root_win, console_out_id, console_out_w->len_x, console_win->len_y - HUD_MARGIN_Y - input_widget->len_y - 20 - nb_ligne_tabs * tab_bar_height);
	widget_resize(console_root_win, CONSOLE_SCROLLBAR_ID, CONSOLE_SCROLLBAR_WIDTH, console_win->len_y - HUD_MARGIN_Y - input_widget->len_y);
	nr_console_lines = (console_out_w->len_y - 1) / (int)(DEFAULT_FONT_Y_LEN * chat_zoom);
	console_text_changed = 1;
	return 1;
}
int history_grep(const char *text, int len) {
	unsigned int i = 0, wraps = 1;
	int idx = last_message;
	int skip;
	for (skip = 0; skip < len; skip++) {
		if (text[skip] != ' ') {
			break;
		}
	}
	if (skip >= len) {
		return 1;
	}
	text += skip;
	len -= skip;
	for (i = 0; i <= total_nr_lines; ++i) {
		if (++wraps >= display_text_buffer[idx].wrap_lines) {
			wraps = 1;
			if (--idx < 0) {
				break;
			}
		}
		if (i <= scroll_up_lines || display_text_buffer[idx].len < len) {
			// line is already visible, or the message is too
			// short to contain the search term
			continue;
		}
		if (safe_strcasestr(display_text_buffer[idx].data, display_text_buffer[idx].len, text, len)) {
			if (i > total_nr_lines - nr_console_lines) {
				scroll_up_lines = total_nr_lines - nr_console_lines;
			} else {
				scroll_up_lines = i + 1;
			}
			console_text_changed = 1;
			break;
		}
	}
	return 1;
}
