#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <libxml/parser.h>
#include "chat.h"
#include "asc.h"
#include "colors.h"
#include "console.h"
#include "consolewin.h"
#include "elconfig.h"
#include "errors.h"
#include "gamewin.h"
#include "global.h"
#include "hud.h"
#include "init.h"
#include "interface.h"
#include "mapwin.h"
#include "multiplayer.h"
#include "queue.h"
#include "text.h"
#include "translate.h"
#ifdef ENGLISH
#ifdef OPENGL_TRACE
#include "gl_init.h"
#endif
#else //ENGLISH
#include "gl_init.h"
#endif //ENGLISH
#include "io/elfilewrapper.h"
#include "io/elpathwrapper.h"
#include "sound.h"

int chat_win = -1;

void remove_chat_tab (Uint8 channel);
int add_chat_tab (int nlines, Uint8 channel);
void update_chat_tab_idx (Uint8 old_ix, Uint8 new_idx);
#ifndef ENGLISH
int changement_barre = 0;
void remove_tab_button (Uint8 channel, int ligne);
// Nombre de bouton sur la barre 1
int nb_tab_button_1 = 0;
// Nombre de bouton sur la barre 2
int nb_tab_button_2 = 0;
#else //ENGLISH
void remove_tab_button (Uint8 channel);
#endif //ENGLISH
int add_tab_button (Uint8 channel);
#ifndef ENGLISH
void update_tab_button_idx (Uint8 old_idx, Uint8 new_idx, int niveau_tab);
#else //ENGLISH
void update_tab_button_idx (Uint8 old_idx, Uint8 new_idx);
#endif //ENGLISH
void convert_tabs (int new_wc);
int display_channel_color_win(Uint32 channel_number);

Uint32 active_channels[MAX_ACTIVE_CHANNELS];
Uint8 current_channel = 0;
queue_t *chan_name_queue;
chan_name * pseudo_chans[SPEC_CHANS];

widget_list *input_widget = NULL;

void input_widget_move_to_win(int window_id)
{
	window_info *win = NULL;
	if ((window_id >= 0) && (window_id < windows_list.num_windows))
		win = &windows_list.window[window_id];
	if ((input_widget == NULL) || (win == NULL))
		return;

	widget_move_win(input_widget->window_id, input_widget->id, window_id);
	if(window_id == chat_win) {
		widget_set_flags(input_widget->window_id, input_widget->id, TEXT_FIELD_BORDER|TEXT_FIELD_EDITABLE|TEXT_FIELD_NO_KEYPRESS);
		input_widget->OnResize = NULL;
		resize_chat_handler(win, win->len_x, win->len_y);
	} else {
		text_field *tf = input_widget->widget_info;
		Uint32 flags;

		input_widget->OnResize = input_field_resize;
		if(window_id == console_root_win) {
			flags = (TEXT_FIELD_BORDER|INPUT_DEFAULT_FLAGS)^WIDGET_CLICK_TRANSPARENT;
		} else if(window_id == game_root_win && input_text_line.len == 0) {
			flags = INPUT_DEFAULT_FLAGS|WIDGET_DISABLED;
		} else if(window_id == map_root_win) {
			flags = INPUT_DEFAULT_FLAGS|WIDGET_INVISIBLE;
		} else {
			flags = INPUT_DEFAULT_FLAGS;
		}
		widget_set_flags(input_widget->window_id, input_widget->id, flags);
		widget_resize(input_widget->window_id, input_widget->id, win->len_x-HUD_MARGIN_X, tf->y_space*2+ceilf(DEFAULT_FONT_Y_LEN*input_widget->size*tf->nr_lines));
		widget_move(input_widget->window_id, input_widget->id, 0, win->len_y-input_widget->len_y-HUD_MARGIN_Y);
	}
}

void add_tab (Uint8 channel)
{
#ifndef ENGLISH
	if (tab_bar_win_1 != -1) add_tab_button (channel);
#else //ENGLISH
	if (tab_bar_win != -1) add_tab_button (channel);
#endif //ENGLISH
	if (chat_win != -1) add_chat_tab (0, channel);
}

#ifndef ENGLISH
// Suppression de l'un des boutons
void remove_tab (Uint8 channel, int niveau_tab)
{
	recolour_messages(display_text_buffer);
    if (niveau_tab == 1)
    {
        if (tab_bar_win_1 != -1) remove_tab_button (channel,1);
    }
    else if (niveau_tab == 2)
    {
        if (tab_bar_win_2 != -1) remove_tab_button (channel,2);
    }
	if (chat_win != -1) remove_chat_tab (channel);
}
#else //ENGLISH
void remove_tab (Uint8 channel)
{
	recolour_messages(display_text_buffer);
	if (tab_bar_win != -1) remove_tab_button (channel);
	if (chat_win != -1) remove_chat_tab (channel);
}
#endif //ENGLISH

#ifndef ENGLISH
void update_tab_idx (Uint8 old_idx, Uint8 new_idx, int niveau_tab)
#else //ENGLISH
void update_tab_idx (Uint8 old_idx, Uint8 new_idx)
#endif //ENGLISH
{
	// XXX: CAUTION
	// Since this function simply replaces old_idx y new_idx, it could
	// potentially cause trouble when new_idx is already in use by
	// another tab. However, as the code is now, successive calls to
	// update_tab_idx are in increasing order of old_idx, and new_idx
	// is lower than old_idx, so we should be safe.

#ifndef ENGLISH
	if (tab_bar_win_1 != -1 && niveau_tab == 1) update_tab_button_idx (old_idx, new_idx,1);
	if (tab_bar_win_2 != -1 && niveau_tab == 2) update_tab_button_idx (old_idx, new_idx,2);
#else //ENGLISH
	if (tab_bar_win != -1) update_tab_button_idx (old_idx, new_idx);
#endif //ENGLISH
	if (chat_win != -1) update_chat_tab_idx (old_idx, new_idx);
}

void set_channel_tabs (const Uint32 *chans)
{
#ifndef ENGLISH
	int nmax = CHAT_CHANNEL5-CHAT_CHANNEL1+1;
#else //ENGLISH
	int nmax = CHAT_CHANNEL3-CHAT_CHANNEL1+1;
#endif //ENGLISH
	Uint32 chan;
	Uint8 chan_nr, chan_nrp;

	for (chan_nr = 0; chan_nr < nmax; chan_nr++)
	{
		chan = chans[chan_nr];
		if (chan == 0) continue;

		for (chan_nrp = 0; chan_nrp < nmax; chan_nrp++)
		{
			if (active_channels[chan_nrp] == chan) break;
		}

		if (chan_nrp >= nmax)
		{
			// we left this channel
#ifndef ENGLISH
            int i;
            for (i=0; i<nb_tab_button_1; i++)
            {
                if (tabs_1[i].channel == chan_nr+CHAT_CHANNEL1)
                {
                    remove_tab (chan_nr+CHAT_CHANNEL1,1);
                }
            }
            for (i=0; i<nb_tab_button_2; i++)
            {
                if (tabs_2[i].channel == chan_nr+CHAT_CHANNEL1)
                {
                    remove_tab (chan_nr+CHAT_CHANNEL1,2);
                }
            }
#else //ENGLISH
			remove_tab (chan_nr+CHAT_CHANNEL1);
#endif //ENGLISH
		}
		else
		{
#ifndef ENGLISH
            int i;
            for (i=0; i<nb_tab_button_1; i++)
            {
                if (tabs_1[i].channel == chan_nr+CHAT_CHANNEL1)
                {
        			update_tab_idx (chan_nr+CHAT_CHANNEL1, chan_nrp+CHAT_CHANNEL1,1);
                }
            }
            for (i=0; i<nb_tab_button_2; i++)
            {
                if (tabs_2[i].channel == chan_nr+CHAT_CHANNEL1)
                {
        			update_tab_idx (chan_nr+CHAT_CHANNEL1, chan_nrp+CHAT_CHANNEL1,2);
                }
            }
#else //ENGLISH
			update_tab_idx (chan_nr+CHAT_CHANNEL1, chan_nrp+CHAT_CHANNEL1);
#endif //ENGLISH
		}
	}

	for (chan_nrp = 0; chan_nrp < nmax; chan_nrp++)
	{
		chan = active_channels[chan_nrp];

		if (chan == 0) continue;

		for (chan_nr = 0; chan_nr < nmax; chan_nr++)
		{
			if (chans[chan_nr] == chan) break;
		}

		if (chan_nr >= nmax)
		{
			// we have a new channel
			add_tab (chan_nrp+CHAT_CHANNEL1);
		}
	}
}

void set_active_channels (Uint8 active, const Uint32 *channels, int nchan)
{
	Uint32 tmp[MAX_ACTIVE_CHANNELS];
	int i;

	for (i = 0; i < MAX_ACTIVE_CHANNELS; i++)
    {
		tmp[i] = active_channels[i];
    }

	for (i = 0; i < nchan; i++)
		active_channels[i] = SDL_SwapLE32(channels[i]);
	for ( ; i < MAX_ACTIVE_CHANNELS; i++)
		active_channels[i] = 0;

	set_channel_tabs (tmp);

	current_channel = active;
}

void send_active_channel (Uint8 chan)
{
	Uint8 msg[2];

#ifndef ENGLISH
	if (chan >= CHAT_CHANNEL1 && chan <= CHAT_CHANNEL5)
#else //ENGLISH
	if (chan >= CHAT_CHANNEL1 && chan <= CHAT_CHANNEL3)
#endif //ENGLISH
	{
		msg[0] = SET_ACTIVE_CHANNEL;
		msg[1] = chan;
		my_tcp_send (my_socket, msg, 2);

		current_channel = chan - CHAT_CHANNEL1;
	}
}

Uint32 get_active_channel (Uint8 idx)
{
#ifndef ENGLISH
	if (idx >= CHAT_CHANNEL1 && idx <= CHAT_CHANNEL5)
#else //ENGLISH
	if (idx >= CHAT_CHANNEL1 && idx <= CHAT_CHANNEL3)
#endif //ENGLISH
		return active_channels[idx-CHAT_CHANNEL1];
	return 0;
}

#define CHAT_WIN_SPACE		4
#define CHAT_WIN_TAG_HEIGHT	20
#define CHAT_WIN_TAG_SPACE	3
#define CHAT_WIN_TEXT_WIDTH  	500
#define CHAT_OUT_TEXT_HEIGHT 	(18*8)
#define CHAT_IN_TEXT_HEIGHT 	(18*3)
#define CHAT_WIN_SCROLL_WIDTH	20

int local_chat_separate = 0;
int personal_chat_separate = 0;
int guild_chat_separate = 1;
int server_chat_separate = 0;
int mod_chat_separate = 0;
#ifndef ENGLISH
int dev_chat_separate = 0;
int coord_chat_separate = 0;
#endif //ENGLISH

/*
 * use_windowed_chat == 0: old behaviour, all text is printed
 * use_windowed_chat == 1: channel selection bar
 * use_windowed_chat == 2: chat window
 */
int use_windowed_chat = 1;
int highlight_tab_on_nick = 1;

////////////////////////////////////////////////////////////////////////
// Chat window variables

int chat_scroll_id = 15;
int chat_tabcollection_id = 20;
int chat_out_start_id = 21;

int chat_win_x = 0; // upper left corner by default
int chat_win_y = 0;

int chat_win_text_width = CHAT_WIN_TEXT_WIDTH;
int chat_out_text_height = CHAT_OUT_TEXT_HEIGHT;

int current_line = 0;
int text_changed = 1;
int nr_displayed_lines;

chat_channel channels[MAX_CHAT_TABS];
int active_tab = -1;

chan_name *tab_label (Uint8 chan);//Forward declaration


void clear_chat_wins (void)
{
	int i = 0;
	if(use_windowed_chat != 2){return;}

	for (;i < MAX_CHAT_TABS; ++i){
		channels[i].nr_lines = 0;
	}

	vscrollbar_set_bar_len (chat_win, chat_scroll_id, 0);
	vscrollbar_set_pos (chat_win, chat_scroll_id, 0);
	current_line = 0;
	text_changed = 1;
}


void init_chat_channels(void)
{
	int itab;

	for (itab = 0; itab < MAX_CHAT_TABS; itab++)
	{
		channels[itab].tab_id = -1;
		channels[itab].out_id = chat_out_start_id + itab;
		channels[itab].chan_nr = CHAT_ALL;
		channels[itab].nr_lines = 0;
		channels[itab].open = 0;
		channels[itab].newchan = 0;
	}
}

void clear_input_line (void)
{
	input_text_line.data[0] = '\0';
	input_text_line.len = 0;
	if(input_widget != NULL) {
		text_field *field = input_widget->widget_info;
		field->cursor = 0;
		field->cursor_line = 0;
		field->nr_lines = 1;
		if(use_windowed_chat != 2) {
			widget_resize(input_widget->window_id, input_widget->id, input_widget->len_x, field->y_space*2+DEFAULT_FONT_Y_LEN*input_widget->size);
		}
	}
	/* Hide the game win input widget */
	if(input_widget->window_id == game_root_win) {
		widget_set_flags(game_root_win, input_widget->id, INPUT_DEFAULT_FLAGS|WIDGET_DISABLED);
	}
	history_reset();
}

int close_channel (window_info *win)
{
	int id = win->window_id;
	int ichan;

	for (ichan = 0; ichan < MAX_CHAT_TABS; ichan++)
	{
		if (channels[ichan].tab_id == id)
		{
			int idx = channels[ichan].chan_nr - CHAT_CHANNEL1;

			if (idx >= 0 && idx < MAX_ACTIVE_CHANNELS)
			{
				char str[256];
				safe_snprintf(str, sizeof(str), "%c#lc %d", RAW_TEXT, active_channels[idx]);
#ifdef ENGLISH
			safe_snprintf(str, sizeof(str), "%c#lc %d", RAW_TEXT, active_channels[idx]);
#else
			safe_snprintf(str, sizeof(str), "%c#qc %d", RAW_TEXT, active_channels[idx]);
#endif
				my_tcp_send(my_socket, (Uint8*)str, strlen(str+1)+1);
			}

			// Safe to remove?
#ifndef ENGLISH
			if (tab_bar_win_1 != -1) remove_tab_button(channels[ichan].chan_nr,1);
#else //ENGLISH
			if (tab_bar_win != -1) remove_tab_button(channels[ichan].chan_nr);
#endif //ENGLISH

			return 1;
		}
	}

	// we shouldn't get here
	LOG_ERROR ("Trying to close non-existant channel\n");
	return 0;
}

void remove_chat_tab (Uint8 channel)
{
	int ichan;

	for (ichan = 0; ichan < MAX_CHAT_TABS; ichan++)
	{
		if (channels[ichan].chan_nr == channel && channels[ichan].open)
		{
			int nr = tab_collection_get_tab_nr (chat_win, chat_tabcollection_id, channels[ichan].tab_id);
			tab_collection_close_tab (chat_win, chat_tabcollection_id, nr);

			channels[ichan].tab_id = -1;
			channels[ichan].chan_nr = CHAT_ALL;
			channels[ichan].nr_lines = 0;
			channels[ichan].open = 0;
			channels[ichan].newchan = 0;
			channels[ichan].highlighted = 0;

			return;
		}
	}
}

int add_chat_tab(int nlines, Uint8 channel)
{
	int ichan;

	for (ichan = 0; ichan < MAX_CHAT_TABS; ichan++)
	{
		if (!channels[ichan].open)
		{
			// yay, found an empty slot
			char title[64];
			int inout_width = chat_win_text_width + 2 * CHAT_WIN_SPACE;
			int output_height = chat_out_text_height + 2 * CHAT_WIN_SPACE;

			channels[ichan].chan_nr = channel;
			channels[ichan].nr_lines = nlines;
			channels[ichan].open = 1;
			channels[ichan].newchan = 1;
			channels[ichan].highlighted = 0;

			my_strncp(title,(tab_label(channel))->name, sizeof(title));

			channels[ichan].tab_id = tab_add (chat_win, chat_tabcollection_id, title, 0, 1, 0);
			set_window_flag (channels[ichan].tab_id, ELW_CLICK_TRANSPARENT);

			set_window_min_size (channels[ichan].tab_id, 0, 0);
#ifdef FR_VERSION
			channels[ichan].out_id = text_field_add_extended (channels[ichan].tab_id, channels[ichan].out_id, NULL, 0, 0, inout_width, output_height, 0, chat_zoom, chat_font, 0.77f, 0.57f, 0.39f, display_text_buffer, DISPLAY_TEXT_BUFFER_SIZE, channel, CHAT_WIN_SPACE, CHAT_WIN_SPACE);
#else //FR_VERSION
			channels[ichan].out_id = text_field_add_extended (channels[ichan].tab_id, channels[ichan].out_id, NULL, 0, 0, inout_width, output_height, 0, chat_zoom, 0.77f, 0.57f, 0.39f, display_text_buffer, DISPLAY_TEXT_BUFFER_SIZE, channel, CHAT_WIN_SPACE, CHAT_WIN_SPACE);
#endif //FR_VERSION

			set_window_handler (channels[ichan].tab_id, ELW_HANDLER_DESTROY, close_channel);

			if(!channels[ichan].highlighted && channels[active_tab].chan_nr != CHAT_ALL)
			{
				tab_set_label_color_by_id (chat_win, chat_tabcollection_id, channels[ichan].tab_id, 1.0, 1.0, 0.0);
			}

			return ichan;
		}
	}
	//no empty slot found
	return -1;
}

void update_chat_tab_idx (Uint8 old_idx, Uint8 new_idx)
{
	int itab;

	if (old_idx == new_idx) return;

	for (itab = 0; itab < MAX_CHAT_TABS; itab++)
	{
		if (channels[itab].chan_nr == old_idx && channels[itab].open)
		{
			channels[itab].chan_nr = new_idx;
			return;
		}
	}
}

Uint8 get_tab_channel (Uint8 channel)
{
	switch (channel)
	{
		case CHAT_LOCAL:
			if (!local_chat_separate) return CHAT_ALL;
			break;
		case CHAT_PERSONAL:
			if (!personal_chat_separate) return CHAT_ALL;
			break;
		case CHAT_GM:
			if (!guild_chat_separate) return CHAT_ALL;
			break;
		case CHAT_SERVER:
			if (!server_chat_separate) return CHAT_ALL;
			break;
		case CHAT_MOD:
			if (!mod_chat_separate) return CHAT_ALL;
			break;
		case CHAT_MODPM:
			// always display moderator PMs in all tabs
			return CHAT_ALL;
#ifndef ENGLISH
		case CHAT_DEV:
			if (!dev_chat_separate) return CHAT_ALL;
			break;
		case CHAT_COORD:
			if (!coord_chat_separate) return CHAT_ALL;
			break;
#endif //ENGLISH
	}

	return channel;
}

void update_chat_window (text_message *msg, char highlight)
{
#ifdef FR_VERSION
	int ichan, nlines, width, channel;
#else //FR_VERSION
	int ichan, len, nlines, width, channel;
#endif //FR_VERSION
	char found;

	// don't bother if there's no chat window
	if (chat_win < 0) return;

	// rewrap message to get correct # of lines
	width = windows_list.window[chat_win].len_x;
#ifdef FR_VERSION
	nlines = rewrap_message(msg, chat_zoom, chat_font, width, NULL);
#else //FR_VERSION
	nlines = rewrap_message(msg, chat_zoom, width, NULL);
#endif //FR_VERSION

	// first check if we need to display in all open channels
	channel = get_tab_channel (msg->chan_idx);

	if (channel == CHAT_ALL || channel == CHAT_MODPM)
	{
		for (ichan = 0; ichan < MAX_CHAT_TABS; ichan++)
		{
			if (channels[ichan].open) {
				if (msg->deleted) {
					channels[ichan].nr_lines -= nlines;
				} else {
					channels[ichan].nr_lines += nlines;
				}
			}
		}

#ifdef FR_VERSION
		vscrollbar_set_bar_len (chat_win, chat_scroll_id, channels[active_tab].nr_lines);
		vscrollbar_set_pos (chat_win, chat_scroll_id, channels[active_tab].nr_lines);
#else //FR_VERSION
		len = channels[active_tab].nr_lines - nr_displayed_lines;
		if (len < 0) len = 0;
		vscrollbar_set_bar_len (chat_win, chat_scroll_id, len);
		vscrollbar_set_pos (chat_win, chat_scroll_id, len);
#endif //FR_VERSION

		current_line = channels[active_tab].nr_lines;
		text_changed = 1;
		return;
	}

	// message not for all channels, see if this channel is already open
	found = 0;
	for (ichan = 0; ichan < MAX_CHAT_TABS; ichan++)
	{
		if (channels[ichan].open && (channels[ichan].chan_nr == channel || channels[ichan].chan_nr == CHAT_ALL))
		{
			if (msg->deleted) {
				channels[ichan].nr_lines -= nlines;
			} else {
				channels[ichan].nr_lines += nlines;
			}
			channels[ichan].newchan = 1;

			if (ichan == active_tab)
			{
#ifdef FR_VERSION
				vscrollbar_set_bar_len (chat_win, chat_scroll_id, channels[ichan].nr_lines);
				vscrollbar_set_pos (chat_win, chat_scroll_id, channels[ichan].nr_lines);
#else //FR_VERSION
				len = channels[ichan].nr_lines - nr_displayed_lines;
				if (len < 0) len = 0;

				vscrollbar_set_bar_len (chat_win, chat_scroll_id, len);
				vscrollbar_set_pos (chat_win, chat_scroll_id, len);
#endif //FR_VERSION
				current_line = channels[ichan].nr_lines;
				text_changed = 1;
			}
			else if (highlight && !channels[ichan].highlighted && channels[active_tab].chan_nr != CHAT_ALL && channels[ichan].chan_nr != CHAT_ALL && !get_show_window(console_root_win)) //Make sure we don't change the color of a highlighted tab
			{
				tab_set_label_color_by_id (chat_win, chat_tabcollection_id, channels[ichan].tab_id, 1.0, 1.0, 0.0);
			}
			if (found) return; // we found the respective tab and the "all" tab now
			found++;
		}
	}

	// nothing to delete from
	if (msg->deleted) return;

	// channel not found, try to create a new one
	if(add_chat_tab(nlines, channel) == -1)
	{
		// uh oh, no empty slot found. this shouldn't really be happening...
		// log in general channel
		channels[0].nr_lines += nlines;
		channels[0].newchan = 1;
		if (0 == active_tab)
		{
#ifdef FR_VERSION
			vscrollbar_set_bar_len (chat_win, chat_scroll_id, channels[0].nr_lines);
			vscrollbar_set_pos (chat_win, chat_scroll_id, channels[0].nr_lines);
#else //FR_VERSION
			len = channels[0].nr_lines - nr_displayed_lines;
			if (len < 0) len = 0;

			vscrollbar_set_bar_len (chat_win, chat_scroll_id, len);
			vscrollbar_set_pos (chat_win, chat_scroll_id, len);
#endif //FR_VERSION
			current_line = channels[active_tab].nr_lines;
			text_changed = 1;
		}
		else if (highlight && !channels[0].highlighted) //Make sure we don't change the color of a highlighted tab
		{
			tab_set_label_color_by_id (chat_win, chat_tabcollection_id, channels[0].tab_id, 1.0, 1.0, 0.0);
		}
	}
}

int display_chat_handler (window_info *win)
{
	static int msg_start = 0, offset_start = 0;
	if (text_changed)
	{
		int line = vscrollbar_get_pos (chat_win, chat_scroll_id);

		find_line_nr (channels[active_tab].nr_lines, line, channels[active_tab].chan_nr, &msg_start, &offset_start, chat_zoom, chat_win_text_width);
		text_field_set_buf_pos (channels[active_tab].tab_id, channels[active_tab].out_id, msg_start, offset_start);
		text_changed = 0;
	}

	if ((input_widget!= NULL) && (input_widget->window_id != win->window_id))
		input_widget_move_to_win(win->window_id);

	return 1;
}

void switch_to_chat_tab(int id, char click)
{
	if(!click)
	{
		int itab;
		//Do what a mouse click would do
		widget_list *widget = widget_find(chat_win, chat_tabcollection_id);
		tab_collection *collection = widget->widget_info;

		for(itab = 0; itab < collection->nr_tabs; itab++)
		{
			if(collection->tabs[itab].content_id == id)
			{
				break;
			}
		}
		tab_collection_select_tab(chat_win, chat_tabcollection_id, itab);
	}
	tab_set_label_color_by_id (chat_win, chat_tabcollection_id, id, -1.0, -1.0, -1.0);

	//set active_tab
	for (active_tab = 0; active_tab < MAX_CHAT_TABS; active_tab++)
	{
		if (channels[active_tab].tab_id == id && channels[active_tab].open)
		{
			break;
		}
	}
	if (active_tab >= MAX_CHAT_TABS)
	{
		// This shouldn't be happening
		LOG_ERROR ("Trying to switch to non-existant channel");
		active_tab = 0;
	}
	current_line = channels[active_tab].nr_lines - nr_displayed_lines;
	if (current_line < 0)
	{
		current_line = 0;
	}
#ifdef FR_VERSION
	vscrollbar_set_bar_len(chat_win, chat_scroll_id, channels[active_tab].nr_lines);
	vscrollbar_set_pos(chat_win, chat_scroll_id, channels[active_tab].nr_lines);
#else //FR_VERSION
	vscrollbar_set_bar_len(chat_win, chat_scroll_id, current_line);
	vscrollbar_set_pos(chat_win, chat_scroll_id, current_line);
#endif //FR_VERSION
	text_changed = 1;
	channels[active_tab].highlighted = 0;

#ifndef ENGLISH
	if (channels[active_tab].chan_nr >= CHAT_CHANNEL1 && channels[active_tab].chan_nr <= CHAT_CHANNEL5)
#else //ENGLISH
	if (channels[active_tab].chan_nr >= CHAT_CHANNEL1 && channels[active_tab].chan_nr <= CHAT_CHANNEL3)
#endif //ENGLISH
	{
		send_active_channel (channels[active_tab].chan_nr);
	}
	recolour_messages(display_text_buffer);
}

void change_to_current_chat_tab(const char *input)
{
	Uint8 channel;
	int ichan;
	int itab;
	int input_len = strlen(input);

	if(input[0] == '@' || input[0] == char_at_str[0])
	{
		channel = CHAT_CHANNEL1 + current_channel;
	}
	else if(my_strncompare(input, "#gm ", 4) || (my_strncompare(input, gm_cmd_str, strlen(gm_cmd_str)) && input_len > strlen(gm_cmd_str)+1 && input[strlen(gm_cmd_str)] == ' '))
	{
		channel = CHAT_GM;
	}
	else if(my_strncompare(input, "#mod ", 5) || (my_strncompare(input, mod_cmd_str, strlen(mod_cmd_str)) && input_len > strlen(mod_cmd_str)+1 && input[strlen(mod_cmd_str)] == ' '))
	{
		channel = CHAT_MOD;
	}
#ifndef ENGLISH
	else if(my_strncompare(input, "#dev ", 7) || (my_strncompare(input, dev_cmd_str, strlen(dev_cmd_str)) && input_len > strlen(dev_cmd_str)+1 && input[strlen(dev_cmd_str)] == ' '))
	{
		channel = CHAT_DEV;
	}
	else if(my_strncompare(input, "#coord ", 7) || (my_strncompare(input, coord_cmd_str, strlen(coord_cmd_str)) && input_len > strlen(coord_cmd_str)+1 && input[strlen(coord_cmd_str)] == ' '))
	{
		channel = CHAT_COORD;
	}
#endif	//ENGLISH
	else if(my_strncompare(input, "#bc ", 4) || (my_strncompare(input, bc_cmd_str, strlen(bc_cmd_str)) && input_len > strlen(bc_cmd_str)+1 && input[strlen(bc_cmd_str)] == ' '))
	{
		channel = CHAT_SERVER;
	}
	else if(input[0] == '/' || input[0] == char_slash_str[0])
	{
		channel = CHAT_PERSONAL;
	}
	else if(input[0] == '#' || input[0] == char_cmd_str[0]) {
		//We don't want to switch tab on commands.
		channel = CHAT_ALL;
	}
	else
	{
		channel = CHAT_LOCAL;
	}
	channel = get_tab_channel (channel);

	if(channel != CHAT_ALL)
	{
		for(ichan = 0; ichan < MAX_CHAT_TABS; ichan++)
		{
			if(channels[ichan].chan_nr == channel && channels[ichan].open)
			{
				if(ichan != active_tab) //We don't want to switch to the tab we're already in
				{
					switch_to_chat_tab(channels[ichan].tab_id, 0);
				}
				return;
			}
		}
		//We didn't find any tab to switch to, create new
		itab = add_chat_tab(0, channel);
		if(itab == -1)
		{
			//Eek, it failed, switch to general
			switch_to_chat_tab(channels[0].tab_id, 0);
		}
		else
		{
			switch_to_chat_tab(channels[itab].tab_id, 0);
		}
	}
}

int chat_tabs_click (widget_list *widget, int mx, int my, Uint32 flags)
{
	int id;

	id = tab_collection_get_tab_id (chat_win, widget->id);

	if (flags&ELW_RIGHT_MOUSE)
	{
		int i;
		for(i=0; i < MAX_CHAT_TABS; i++)
		{
			if(channels[i].tab_id == id)
			{
				display_channel_color_win(get_active_channel(channels[i].chan_nr));
				return 1;
			}
		}
	}
	else
	{
	if (id != channels[active_tab].tab_id)
	{
		//We're not looking at the tab we clicked
		switch_to_chat_tab(id, 1);
		return 1;
	}
	}
	return 0;
}

int chat_scroll_drag (widget_list *widget, int mx, int my, Uint32 flags, int dx, int dy)
{
	int line = vscrollbar_get_pos (chat_win, widget->id);
	if (line != current_line)
	{
		text_changed = 1;
		current_line = line;
	}
        return 0;
}

int chat_scroll_click (widget_list *widget, int mx, int my, Uint32 flags)
{
	int line = vscrollbar_get_pos (chat_win, widget->id);
	if (line != current_line)
	{
		text_changed = 1;
		current_line = line;
	}
        return 0;
}

int chat_input_key (widget_list *widget, int mx, int my, Uint32 key, Uint32 unikey)
{
	Uint16 keysym = key & 0xffff;
	text_field *tf;
	text_message *msg;

	if (widget == NULL) {
		return 0;
	}
	tf = (text_field *) widget->widget_info;
	msg = tf->buffer;

	if ( (!(key & ELW_CTRL) && ( (keysym == SDLK_UP) || (keysym == SDLK_DOWN) ) ) ||
		(keysym == SDLK_LEFT) || (keysym == SDLK_RIGHT) || (keysym == SDLK_HOME) ||
		(keysym == SDLK_END) || (keysym == SDLK_DELETE && tf->cursor < msg->len) ) {
		//pass it along. the defaults are good enough
		widget->Flags &= ~TEXT_FIELD_NO_KEYPRESS;
		text_field_keypress (widget, mx, my, key, unikey);
		widget->Flags |= TEXT_FIELD_NO_KEYPRESS;
	}
	else
	{
		return 0;
	}
	/* Key was handled, stop blinking on input */
	tf->next_blink = cur_time + TF_BLINK_DELAY;
	return 1;
}

int resize_chat_handler(window_info *win, int width, int height)
{
	int itab;
	int scroll_x = width - CHAT_WIN_SCROLL_WIDTH;
	int scroll_height = height - 2*ELW_BOX_SIZE;
	int inout_width = width - CHAT_WIN_SCROLL_WIDTH - 2 * CHAT_WIN_SPACE;
	int input_height = CHAT_IN_TEXT_HEIGHT + 2 * CHAT_WIN_SPACE;
	int input_y = height - input_height - CHAT_WIN_SPACE;
	int tabcol_height = input_y - 2 * CHAT_WIN_SPACE;
	int output_height = tabcol_height - CHAT_WIN_TAG_HEIGHT;
	int line_height = DEFAULT_FONT_Y_LEN*chat_zoom;

	if (output_height < 5*line_height + 2 * CHAT_WIN_SPACE && input_height > 3*line_height + 2 * CHAT_WIN_SPACE)
	{
		input_height -= 2*line_height;
		input_y += 2*line_height;
		output_height += 2*line_height;
		tabcol_height += 2*line_height;
	}
	else if (output_height < 8*line_height + 2 * CHAT_WIN_SPACE && input_height > 2*line_height + 2 * CHAT_WIN_SPACE)
	{
		input_height -= line_height;
		input_y += line_height;
		output_height += line_height;
		tabcol_height += line_height;
	}

	chat_win_text_width = inout_width - 2 * CHAT_WIN_SPACE;
	chat_out_text_height = output_height - 2 * CHAT_WIN_SPACE;

	widget_resize (chat_win, chat_scroll_id, CHAT_WIN_SCROLL_WIDTH, scroll_height);
	widget_move (chat_win, chat_scroll_id, scroll_x, ELW_BOX_SIZE);

	widget_resize (chat_win, chat_tabcollection_id, inout_width, tabcol_height);

	for (itab = 0; itab < MAX_CHAT_TABS; itab++)
		if (channels[itab].tab_id >= 0)
			widget_resize (channels[itab].tab_id, channels[itab].out_id, inout_width, output_height);

	widget_resize (chat_win, input_widget->id, inout_width, input_height);
	widget_move (chat_win, input_widget->id, CHAT_WIN_SPACE, input_y);

	update_chat_win_buffers();

	return 0;
}


void update_chat_win_buffers(void)
{
	int itab, imsg;
	// recompute line breaks
	for (itab = 0; itab < MAX_CHAT_TABS; itab++)
	{
		channels[itab].nr_lines = 0;
	}

		imsg = 0;
	while (1)
	{
		update_chat_window (&display_text_buffer[imsg], 0);
		if (imsg == last_message || last_message < 0)
			break;
		if (++imsg >= DISPLAY_TEXT_BUFFER_SIZE)
			imsg = 0;
	}

	// adjust the text position and scroll bar
	nr_displayed_lines = (int) (chat_out_text_height / (18.0f * chat_zoom));
	current_line = channels[active_tab].nr_lines - nr_displayed_lines;
	if (current_line < 0)
		current_line = 0;
#ifdef FR_VERSION
	widget_set_size (chat_win, chat_scroll_id, nr_displayed_lines);
	vscrollbar_set_bar_len (chat_win, chat_scroll_id, channels[active_tab].nr_lines);
	vscrollbar_set_pos (chat_win, chat_scroll_id, channels[active_tab].nr_lines);
#else //FR_VERSION
	vscrollbar_set_bar_len (chat_win, chat_scroll_id, current_line);
	vscrollbar_set_pos (chat_win, chat_scroll_id, current_line);
#endif //FR_VERSION
	text_changed = 1;
}

void parse_input(char *data, int len)
{
	if (len > MAX_TEXT_MESSAGE_LENGTH)
	{
		LOG_TO_CONSOLE(c_red2, command_too_long_str);
		return;
	}

	if (data[0] == '%' && len > 1)
	{
		if ( (check_var ((char*)&(data[1]), IN_GAME_VAR) ) < 0)
		{
			send_input_text_line ((char*)data, len);
		}
	}
	else if ( data[0] == '#' || data[0] == char_cmd_str[0] )
	{
		test_for_console_command ((char*)data, len);
	}
	else if (data[0] == '/' && len > 1)
	{
		// Forum #58898: Please the server when sending player messages;
		// remove all but one space between name and message start.
		// do not assume data is null terminated
		size_t dx = 0;
		char *rebuf = (char *)malloc(len+1);
		for (dx=0; (dx < len) && (data[dx] != ' '); dx++)
			rebuf[dx] = data[dx];
		rebuf[dx] = '\0';
		while ((dx < len) && (data[dx] == ' '))
			dx++;
		if (dx < len)
		{
			size_t rebuf_len = 0;
			safe_strcat(rebuf, " ", len+1);
			rebuf_len = strlen(rebuf);
			safe_strncpy2(&rebuf[rebuf_len], &data[dx], len+1-rebuf_len, len-dx);
		}
		send_input_text_line (rebuf, strlen(rebuf));
		free(rebuf);
	}
	else
	{
		if(data[0] == char_at_str[0])
			data[0] = '@';
		send_input_text_line ((char*)data, len);
	}
}


int root_key_to_input_field (Uint32 key, Uint32 unikey)
{
	Uint16 keysym = key & 0xffff;
	Uint8 ch = key_to_char (unikey);
	text_field *tf;
	text_message *msg;
	int alt_on = key & ELW_ALT, ctrl_on = key & ELW_CTRL;

	if(input_widget == NULL || (input_widget->Flags & TEXT_FIELD_EDITABLE) == 0) {
		return 0;
	}

	tf = input_widget->widget_info;
	msg = &(tf->buffer[tf->msg]);

	if (keysym == SDLK_ESCAPE)
	{
		clear_input_line();
	}
	else if (ch == SDLK_RETURN && msg->len > 0)
	{
		parse_input(msg->data, msg->len);
		add_line_to_history((char*)msg->data, msg->len);
		clear_input_line();
	}
	else if (tf->cursor == 1 && (ch == '/' || ch == char_slash_str[0])
	             && (msg->data[0] == '/' || msg->data[0]==char_slash_str[0])
	             && last_pm_from[0])
	{
		// watch for the '//' shortcut
		tf->cursor += put_string_in_buffer (msg, (unsigned char*)last_pm_from, 1);
		tf->cursor += put_char_in_buffer (msg, ' ', tf->cursor);

		// set invalid width to force rewrap
		msg->wrap_width = 0;
#ifdef FR_VERSION
		tf->nr_lines = rewrap_message (msg, input_widget->size, tf->font_num, input_widget->len_x - 2 * tf->x_space, &tf->cursor);
#else //FR_VERSION
		tf->nr_lines = rewrap_message (msg, input_widget->size, input_widget->len_x - 2 * tf->x_space, &tf->cursor);
#endif //FR_VERSION
	}
	else if (ch == SDLK_BACKSPACE || ch == SDLK_DELETE
#ifdef OSX
	             || ch == 127
#endif
	             || (!alt_on && !ctrl_on && is_printable (ch) && ch != '`')
	        )
	{
		if (is_printable (ch) && !get_show_window(map_root_win)) {
			//Make sure the widget is visible.
			widget_unset_flags (input_widget->window_id, input_widget->id, WIDGET_DISABLED);
			widget_unset_flags (input_widget->window_id, input_widget->id, WIDGET_INVISIBLE);
		}
		// XXX FIXME: we've set the input widget with the
		// TEXT_FIELD_NO_KEYPRESS flag so that the default key
		// handler for a text field isn't called, but now
		// we do want to call it directly. So we clear the flag,
		// and reset it afterwards.
		input_widget->Flags &= ~TEXT_FIELD_NO_KEYPRESS;
		text_field_keypress (input_widget, 0, 0, key, unikey);
		input_widget->Flags |= TEXT_FIELD_NO_KEYPRESS;
	}
	else if (key == K_TABCOMPLETE && input_text_line.len > 0)
	{
		do_tab_complete(&input_text_line);
	}
	else if (get_show_window(console_root_win))
	{
		chat_input_key (input_widget, 0, 0, key, unikey);
	}
	else
	{
		return 0;
	}
	tf->next_blink = cur_time + TF_BLINK_DELAY;
	if(input_widget->window_id != chat_win && tf->nr_lines != floorf((input_widget->len_y-2*tf->y_space)/(DEFAULT_FONT_Y_LEN*input_widget->size))) {
		/* Resize the input widget if needed */
		widget_resize(input_widget->window_id, input_widget->id, input_widget->len_x, tf->y_space*2 + ceilf(DEFAULT_FONT_Y_LEN*input_widget->size*tf->nr_lines));
	}
	while(tf->buffer->data[tf->cursor] == '\r' && tf->cursor < tf->buffer->len)
	{
		tf->cursor++;
	}
	return 1;
}

void paste_in_input_field (const Uint8 *text)
{
	text_field *tf;
	text_message *msg;

	if (input_widget == NULL) {
		return;
	} else if (input_widget->window_id == game_root_win) {
		widget_unset_flags (game_root_win, input_widget->id, WIDGET_DISABLED);
	}

	tf = input_widget->widget_info;
	msg = &(tf->buffer[tf->msg]);

	tf->cursor += put_string_in_buffer(msg, text, tf->cursor);

	// set invalid width to force rewrap
	msg->wrap_width = 0;
#ifdef FR_VERSION
		tf->nr_lines = rewrap_message(msg, input_widget->size, tf->font_num, input_widget->len_x - 2 * tf->x_space, &tf->cursor);
#else //FR_VERSION
	tf->nr_lines = rewrap_message(msg, input_widget->size, input_widget->len_x - 2 * tf->x_space, &tf->cursor);
#endif //FR_VERSION
	if(use_windowed_chat != 2) {
		widget_resize(input_widget->window_id, input_widget->id, input_widget->len_x, tf->y_space*2 + ceilf(DEFAULT_FONT_Y_LEN*input_widget->size*tf->nr_lines));
	}
}

void put_string_in_input_field(const Uint8 *text)
{
	text_field *tf = input_widget->widget_info;
	text_message *msg = &(tf->buffer[tf->msg]);

	if(text != NULL) {
		tf->cursor = msg->len = safe_snprintf((char*)msg->data, msg->size, "%s", text);
		// set invalid width to force rewrap
		msg->wrap_width = 0;
#ifdef FR_VERSION
		tf->nr_lines = rewrap_message(msg, input_widget->size, tf->font_num, input_widget->len_x - 2 * tf->x_space, &tf->cursor);
#else //FR_VERSION
		tf->nr_lines = rewrap_message(msg, input_widget->size, input_widget->len_x - 2 * tf->x_space, &tf->cursor);
#endif //FR_VERSION
		if(use_windowed_chat != 2) {
			widget_resize(input_widget->window_id, input_widget->id, input_widget->len_x, tf->y_space*2 + ceilf(DEFAULT_FONT_Y_LEN*input_widget->size*tf->nr_lines));
		}
		if(input_widget->window_id == game_root_win) {
			widget_unset_flags (input_widget->window_id, input_widget->id, WIDGET_DISABLED);
		}
	}
}

int close_chat_handler (window_info *win)
{
	// revert to using the tab bar
	// call the config function to make sure it's done properly
	change_windowed_chat(&use_windowed_chat, 1);
	set_var_unsaved("windowed_chat", INI_FILE_VAR);

	return 1;
}

void create_chat_window(void)
{
	int chat_win_width = CHAT_WIN_TEXT_WIDTH + 4 * CHAT_WIN_SPACE + CHAT_WIN_SCROLL_WIDTH;
	int chat_win_height = CHAT_OUT_TEXT_HEIGHT + CHAT_IN_TEXT_HEIGHT + 7 * CHAT_WIN_SPACE + CHAT_WIN_TAG_HEIGHT;
	int inout_width = CHAT_WIN_TEXT_WIDTH + 2 * CHAT_WIN_SPACE;
	int output_height = CHAT_OUT_TEXT_HEIGHT + 2 * CHAT_WIN_SPACE;
	int tabcol_height = output_height + CHAT_WIN_TAG_HEIGHT;
	int input_y = tabcol_height + 2 * CHAT_WIN_SPACE;
	int input_height = CHAT_IN_TEXT_HEIGHT + 2 * CHAT_WIN_SPACE;

	int min_width = CHAT_WIN_SCROLL_WIDTH + 2 * CHAT_WIN_SPACE + (int)(CHAT_WIN_TEXT_WIDTH * chat_zoom);
	int min_height = 7 * CHAT_WIN_SPACE + CHAT_WIN_TAG_HEIGHT + (int) ((2+5) * 18.0 * chat_zoom);

	nr_displayed_lines = (int) ((CHAT_OUT_TEXT_HEIGHT-1) / (18.0 * chat_zoom));

	chat_win = create_window ("Chat", game_root_win, 0, chat_win_x, chat_win_y, chat_win_width, chat_win_height, ELW_WIN_DEFAULT|ELW_RESIZEABLE|ELW_CLICK_TRANSPARENT);

	set_window_handler (chat_win, ELW_HANDLER_DISPLAY, &display_chat_handler);
	set_window_handler (chat_win, ELW_HANDLER_RESIZE, &resize_chat_handler);
	set_window_handler (chat_win, ELW_HANDLER_CLOSE, &close_chat_handler);

#ifdef FR_VERSION
	chat_scroll_id = vscrollbar_add_extended (chat_win, chat_scroll_id, NULL, chat_win_width - CHAT_WIN_SCROLL_WIDTH, ELW_BOX_SIZE, CHAT_WIN_SCROLL_WIDTH, chat_win_height - 2*ELW_BOX_SIZE, 0, nr_displayed_lines, 0.77f, 0.57f, 0.39f, 0, 1, 0);
#else //FR_VERSION
	chat_scroll_id = vscrollbar_add_extended (chat_win, chat_scroll_id, NULL, chat_win_width - CHAT_WIN_SCROLL_WIDTH, ELW_BOX_SIZE, CHAT_WIN_SCROLL_WIDTH, chat_win_height - 2*ELW_BOX_SIZE, 0, 1.0f, 0.77f, 0.57f, 0.39f, 0, 1, 0);
#endif //FR_VERSION
	widget_set_OnDrag (chat_win, chat_scroll_id, chat_scroll_drag);
	widget_set_OnClick (chat_win, chat_scroll_id, chat_scroll_click);

	chat_tabcollection_id = tab_collection_add_extended (chat_win, chat_tabcollection_id, NULL, CHAT_WIN_SPACE, CHAT_WIN_SPACE, inout_width, tabcol_height, 0, 0.7, 0.77f, 0.57f, 0.39f, MAX_CHAT_TABS, CHAT_WIN_TAG_HEIGHT);
	widget_set_OnClick (chat_win, chat_tabcollection_id, chat_tabs_click);

	channels[0].tab_id = tab_add (chat_win, chat_tabcollection_id, (tab_label(CHAT_ALL))->name, 0, 0, 0);
	set_window_flag (channels[0].tab_id, ELW_CLICK_TRANSPARENT);
	set_window_min_size (channels[0].tab_id, 0, 0);
#ifdef FR_VERSION
	channels[0].out_id = text_field_add_extended (channels[0].tab_id, channels[0].out_id, NULL, 0, 0, inout_width, output_height, 0, chat_zoom, chat_font, 0.77f, 0.57f, 0.39f, display_text_buffer, DISPLAY_TEXT_BUFFER_SIZE, FILTER_ALL, CHAT_WIN_SPACE, CHAT_WIN_SPACE);
#else //FR_VERSION
	channels[0].out_id = text_field_add_extended (channels[0].tab_id, channels[0].out_id, NULL, 0, 0, inout_width, output_height, 0, chat_zoom, 0.77f, 0.57f, 0.39f, display_text_buffer, DISPLAY_TEXT_BUFFER_SIZE, FILTER_ALL, CHAT_WIN_SPACE, CHAT_WIN_SPACE);
#endif //FR_VERSION
	channels[0].chan_nr = CHAT_ALL;
	channels[0].nr_lines = 0;
	channels[0].open = 1;
	channels[0].newchan = 0;
	active_tab = 0;

	if(input_widget == NULL) {
		Uint32 id;
		set_text_message_color (&input_text_line, 1.0f, 1.0f, 1.0f);
#ifdef FR_VERSION
		id = text_field_add_extended (chat_win, 19, NULL, CHAT_WIN_SPACE, input_y, inout_width, input_height, TEXT_FIELD_BORDER|TEXT_FIELD_EDITABLE|TEXT_FIELD_NO_KEYPRESS, chat_zoom, chat_font, 0.77f, 0.57f, 0.39f, &input_text_line, 1, FILTER_ALL, CHAT_WIN_SPACE, CHAT_WIN_SPACE);
#else //FR_VERSION
		id = text_field_add_extended (chat_win, 19, NULL, CHAT_WIN_SPACE, input_y, inout_width, input_height, TEXT_FIELD_BORDER|TEXT_FIELD_EDITABLE|TEXT_FIELD_NO_KEYPRESS, chat_zoom, 0.77f, 0.57f, 0.39f, &input_text_line, 1, FILTER_ALL, CHAT_WIN_SPACE, CHAT_WIN_SPACE);
#endif //FR_VERSION
		widget_set_OnKey (chat_win, id, chat_input_key);
		input_widget = widget_find(chat_win, id);
	}
	set_window_min_size (chat_win, min_width, min_height);
}

void display_chat(void)
{
	if (chat_win < 0)
	{
		create_chat_window ();
	}
	else
	{
		if(input_widget != NULL) {
			input_widget->OnResize = NULL;
		}
		show_window (chat_win);
		select_window (chat_win);
	}
	update_chat_win_buffers();
}

void chat_win_update_zoom(void)
{
	int itab;

	widget_set_size(chat_win, input_widget->id, chat_zoom);
	for (itab = 0; itab < MAX_CHAT_TABS; itab++) {
		if (channels[itab].open) {
			widget_set_size(channels[itab].tab_id, channels[itab].out_id, chat_zoom);
		}
	}
	text_changed = 1;
}

////////////////////////////////////////////////////////////////////////

#define CS_MAX_DISPLAY_CHANS 10

#ifndef ENGLISH
int tab_bar_win_1 = -1;
#else //ENGLISH
int tab_bar_win = -1;
#endif //ENGLISH
int chan_sel_win = -1;
int chan_sel_scroll_id = -1;
#ifndef ENGLISH
chat_tab tabs_1[MAX_CHAT_TABS];
int current_bar = 0;
int cur_button_id_1 = 0;
#else //ENGLISH
chat_tab tabs[MAX_CHAT_TABS];
int cur_button_id = 0;
#endif //ENGLISH
int tabs_in_use = 0;
int current_tab = 0;

#ifndef ENGLISH
chat_tab tabs_2[MAX_CHAT_TABS];
int nb_ligne_tabs = 0;
int tab_bar_win_2 = -1;
int cur_button_id_2 = 0;

int tab_bar_width_1 = 0;
int tab_bar_width_2 = 0;
#else //ENGLISH
int tab_bar_width = 0;
#endif //ENGLISH
int tab_bar_height = 18;

void add_chan_name(int no, char * name, char * desc)
{
	chan_name *entry;
	int len;

	if(((entry = malloc(sizeof(*entry))) == NULL)
		||((entry->description = malloc(strlen(desc)+1)) == NULL)
		||((entry->name = malloc(strlen(name)+1)) == NULL)) {
		LOG_ERROR("Memory allocation error reading channel list");
		return;
	}
	entry->channel = no;
	safe_strncpy(entry->name, name, strlen(name) + 1);
	safe_strncpy(entry->description, desc, strlen(desc) + 1);
	queue_push(chan_name_queue, entry);
	len = chan_name_queue->nodes-CS_MAX_DISPLAY_CHANS;
#ifdef FR_VERSION
	// si la barre de défilement existe déjà : il faut dans tous les cas la mettre à jour !
	if (chan_sel_scroll_id != -1)
		vscrollbar_set_bar_len(chan_sel_win, chan_sel_scroll_id, chan_name_queue->nodes);
	else if (len > 0 && chan_sel_win != -1)
		chan_sel_scroll_id = vscrollbar_add_extended (chan_sel_win, 0, NULL, 165, 20, 20, 163, 0, CS_MAX_DISPLAY_CHANS, 0.77f, 0.57f, 0.39f, 0, 1, chan_name_queue->nodes);
#else //FR_VERSION
	if(len > 0 && chan_sel_scroll_id == -1 && chan_sel_win != -1) {
		chan_sel_scroll_id = vscrollbar_add_extended (chan_sel_win, 0, NULL, 165, 20, 20, 163, 0, 1.0, 0.77f, 0.57f, 0.39f, 0, 1, len);
	}
#endif //FR_VERSION
}

void add_spec_chan_name(int no, char * name, char * desc)
{
	chan_name *entry;
	if(((entry = malloc(sizeof(*entry))) == NULL)
		||((entry->description = malloc(strlen(desc)+1)) == NULL)
		||((entry->name = malloc(strlen(name)+1)) == NULL)){
			LOG_ERROR("Memory allocation error reading channel list");
			return;
		}
	entry->channel = no;
	safe_strncpy(entry->name, name, strlen(name) + 1);
	safe_strncpy(entry->description, desc, strlen(desc) + 1);
	pseudo_chans[no]=entry;
}

void generic_chans(void)
{	//the channel list file is missing. We'll use hard-coded values
	//remake the queue, just in case we got half way through the file
	queue_destroy(chan_name_queue);
	queue_initialise(&chan_name_queue);
	add_spec_chan_name(0, "Channel %d", "Channel %d");
	add_spec_chan_name(1, "Guild", "Your guild's chat channel");
	add_spec_chan_name(2, "All", "Display chat in all channels");
	add_spec_chan_name(3, "None", "Messages not on any channel");
	add_spec_chan_name(4, "Options", "Select which channels to join");
	add_spec_chan_name(5, "History", "View all previous chat in all channels you have been on");
	add_spec_chan_name(6, "Local", "Chat in your local area");
	add_spec_chan_name(7, "PMs", "Private messages");
	add_spec_chan_name(8, "GMs", "Guild Messages");
	add_spec_chan_name(9, "Server", "Messages from the server");
	add_spec_chan_name(10, "Mod", "Mod chat");
#ifdef FR_VERSION
	add_spec_chan_name(11, "Dev", "Dev chat");
	add_spec_chan_name(12, "Coord", "Coord chat");
	add_spec_chan_name(13, "Combat", "Combat chat");
#endif //FR_VERSION
	add_chan_name(1, "Newbie", "Newbie Q&A about the game");
	add_chan_name(3, "Market", "Trading, hiring, and price checks");
	add_chan_name(4, "EL Gen Chat", "Chat about EL topics");
	add_chan_name(5, "Roleplay", "Discussion about, and Roleplaying");
	add_chan_name(6, "Contests", "Contest information and sometimes chat");
}

void init_channel_names(void)
{
	char file[256];
	xmlDocPtr doc;
	xmlNodePtr cur;

	// Per-channel info
	char *channelname;
	char *channeldesc;
	int channelno;

	// Temp info
	xmlChar *attrib;
	int attriblen;

	queue_initialise(&chan_name_queue);

	// Load the file, depending on WINDOWS = def|undef
	// Then parse it. If that fails, fallback onto the english one. If that fails, use builtins.
	safe_snprintf (file, sizeof (file), "languages/%s/strings/channels.xml", lang);

	doc = xmlParseFile (file);
	if (doc == NULL ) {
		doc = xmlParseFile("languages/en/strings/channels.xml");
		if (doc == NULL) { //darn, don't have that either?
			LOG_ERROR (using_builtin_chanlist);
			generic_chans();
			return;
		}
		//well the localised version didn't load, but the 'en' version did
		LOG_ERROR (using_eng_chanlist, lang);
	}

	// Get the root element, if it exists.
	cur = xmlDocGetRootElement (doc);
	if (cur == NULL) {
		// Use generics. Defaulting to english, then using the fallbacks makes obfuscated, messy code.
		LOG_ERROR (using_builtin_chanlist);
		generic_chans();
		xmlFreeDoc(doc);
		return;
	}

	// Check the root element.
	if (xmlStrcasecmp (cur->name, (const xmlChar *) "CHANNELS")) {
		LOG_ERROR (xml_bad_root_node, file);
		xmlFreeDoc(doc);
		generic_chans();
		return;
	}

	// Load first child node
	cur = cur->xmlChildrenNode;

	// Loop while we have a node, copying ATTRIBS, etc
	while (cur != NULL)	{
		if(cur->type != XML_ELEMENT_NODE) {
			/* NO-OP. better performance to check now than later */
		} else if ((!xmlStrcmp (cur->name, (const xmlChar *)"label"))) {
			// Get the name.
			attrib = xmlGetProp (cur, (xmlChar*)"name");
			if (attrib == NULL) {
				LOG_ERROR (xml_bad_node);
				xmlFree (attrib);
				continue;
			}
			attriblen = strlen ((char*)attrib);
			if (attriblen < 1) {
				LOG_ERROR (xml_bad_node);
				xmlFree (attrib);
				continue;
			}
#ifdef ENGLISH
			/*channelname = malloc (attriblen)+1;
			my_xmlStrncopy (&channelname, attrib, attriblen);*/
			channelname = (char*)xmlStrdup(attrib);
#else //ENGLISH
			channelname = malloc (attriblen+1);
			my_xmlStrncopy (&channelname, (char*)attrib, attriblen);
#endif //ENGLISH
			xmlFree (attrib);

			// Get the index number
			attrib = xmlGetProp (cur, (xmlChar*)"index");
			if (attrib == NULL) {
				LOG_ERROR (xml_bad_node);
				xmlFree (attrib);
				continue;
			}
			attriblen = strlen ((char*)attrib);
			if (attriblen < 1) {
				LOG_ERROR (xml_bad_node);
				xmlFree (attrib);
				continue;
			}
			channelno = atoi ((char*)attrib);
			xmlFree (attrib);

			// Get the description.
			if ((cur->children == NULL) || (strlen ((char*)cur->children->content) < 1)) {
				free (channelname);
				LOG_ERROR (xml_bad_node);
				continue;
			}
			attrib = cur->children->content;
			attriblen = strlen ((char*)attrib);
#ifdef ENGLISH
			/*channeldesc = malloc (attriblen)+1;
			my_xmlStrncopy (&channeldesc, attrib, attriblen);*/
			channeldesc = (char*)xmlStrdup(attrib);
#else //ENGLISH
			channeldesc = malloc (attriblen+1);
			my_xmlStrncopy (&channeldesc, (char*)attrib, attriblen);
#endif //ENGLISH
			xmlFree (attrib);

			// Add it.
			add_spec_chan_name(channelno, channelname, channeldesc);
			free(channelname);
			free(channeldesc);
		} else if ((!xmlStrcmp (cur->name, (const xmlChar *)"channel"))) {
			// Get the channel.
			attrib = xmlGetProp (cur, (xmlChar*)"number");
			if (attrib == NULL){
				LOG_ERROR (xml_bad_node);
				xmlFree (attrib);
				continue;
			}
			attriblen = strlen ((char*)attrib);
			if (attriblen < 1){
				LOG_ERROR (xml_bad_node);
				xmlFree (attrib);
				continue;
			}
			channelno = atoi ((char*)attrib);
			xmlFree (attrib);

			// Get the name.
			attrib = xmlGetProp (cur, (xmlChar*)"name");
			if (attrib == NULL){
				LOG_ERROR (xml_bad_node);
				xmlFree (attrib);
				continue;
			}
			attriblen = strlen ((char*)attrib);
			if (attriblen < 1){
				LOG_ERROR (xml_bad_node);
				xmlFree (attrib);
				continue;
			}
#ifdef ENGLISH
			/*channelname = malloc (attriblen)+1;
			my_xmlStrncopy (&channelname, attrib, attriblen);*/
			channelname = (char*)xmlStrdup(attrib);
#else //ENGLISH
			/*channelname = xmlStrdup(attrib);*/
			channelname = malloc (attriblen+1);
			my_xmlStrncopy (&channelname, (char*)attrib, attriblen);
#endif //ENGLISH
			xmlFree (attrib);

			// Get the description.
			if (cur->children == NULL) {
				free (channelname);
				LOG_ERROR (xml_bad_node);
				continue;
			} else if (strlen ((char*)cur->children->content) < 1) {
				free (channelname);
				LOG_ERROR (xml_bad_node);
				continue;
			}
			attrib = cur->children->content;
#ifdef ENGLISH
			/*attriblen = strlen (attrib);
			channeldesc = malloc (attriblen);
			my_xmlStrncopy (&channeldesc, attrib, attriblen);*/
			channeldesc = (char*)xmlStrdup(attrib);
#else //ENGLISH
			attriblen = strlen ((char*)attrib);
			channeldesc = malloc (attriblen+1);
			my_xmlStrncopy (&channeldesc, (char*)attrib, attriblen);
#endif //ENGLISH
			xmlFree (attrib);

			// Add it.
			add_chan_name(channelno, channelname, channeldesc);
			free(channelname);
			free(channeldesc);
		} else {
			LOG_ERROR (xml_undefined_node, file, (cur->name != NULL && strlen((char*)cur->name) < 100) ? cur->name	: (const xmlChar *)"not a string");
		}
		cur = cur->next;         // Advance to the next node.
	}
	if(queue_isempty(chan_name_queue)) {
		//how did we not get any channels from it?
		LOG_ERROR(using_builtin_chanlist);
		generic_chans();
	}
}

void cleanup_chan_names(void)
{
	//don't call this command unless the client is closing
	int i=0;
	node_t *temp_node, *step = queue_front_node(chan_name_queue);
	chan_name *temp_cn;
	for (;i<SPEC_CHANS;++i) {
		if(pseudo_chans[i] == NULL) {
			continue;
		}
		if(pseudo_chans[i]->name != NULL) {
			free(pseudo_chans[i]->name);
		}
		if(pseudo_chans[i]->description != NULL) {
			free(pseudo_chans[i]->description);
		}
		free(pseudo_chans[i]);
	}
	while(step != NULL) {
		temp_node = step;
		step = step->next;
		temp_cn = queue_delete_node(chan_name_queue, temp_node);
		if(temp_cn == NULL || temp_cn->name == NULL || strlen(temp_cn->name) < 1) {
			continue;
		}
		if(temp_cn->name != NULL) {
			free(temp_cn->name);
		}
		if(temp_cn->description != NULL) {
			free(temp_cn->description);
		}
		free(temp_cn);
	}
	queue_destroy(chan_name_queue);
}

#ifndef ENGLISH
int test_channel_race (int no_channel)
{
    // On essaie de joindre un canal de race
    if ((no_channel >= 11) && (no_channel <= 19))
    {
        // On verifie que le joueur a bien le droit de joindre le canal de race
        if (((your_info.race == HAUT_ELFE) && (no_channel == 11)) ||
				((your_info.race == GALDUR) && (no_channel == 12)) ||
				((your_info.race == HOMME_BLEU) && (no_channel == 13)) ||
				((your_info.race == HUMAIN_ELDORIAN) && (no_channel == 14)) ||
				((your_info.race == KULTAR) && (no_channel == 15)) ||
				((your_info.race == NAIN) && (no_channel == 16)) ||
				((your_info.race == ELFE_NOIR) && (no_channel == 17)) ||
				((your_info.race == HUMAIN_SINAN) && (no_channel == 18)) ||
				((your_info.race == HUMAIN_INDEFINI) && (no_channel == 19)))
        {
            return 1;
        }
        // Le joueur n'a pas l'autorisation
        else
        {
            return 0;
        }
    }
    else
    {
        return 1;
    }
}

void cleanup_chan_race_names()
{
	//don't call this command unless the client is closing
	node_t *temp_node, *step = queue_front_node(chan_name_queue);
	chan_name *temp_cn;
    do
    {
		temp_node = step;
		step = step->next;
        // Le joueur n'a pas l'autorisation de joindre le canal de race
        if (!test_channel_race((int) ((chan_name*)(temp_node->data))->channel))
        {
           temp_cn = queue_delete_node(chan_name_queue, temp_node);
	 	   if(temp_cn == NULL || temp_cn->name == NULL || strlen(temp_cn->name) < 1)
           {
		      continue;
		   }
		   if(temp_cn->name != NULL)
           {
			  free(temp_cn->name);
		   }
		   if(temp_cn->description != NULL)
           {
			  free(temp_cn->description);
		   }
		   free(temp_cn);
		}
	}
	while(step != NULL);
}
#endif //ENGLISH

int highlight_tab(const Uint8 channel)
{
	int i;

	if(!highlight_tab_on_nick || channel == CHAT_ALL)
	{
		//We don't want to highlight
		return 0;
	}
	switch(use_windowed_chat)
	{
		case 1:
#ifndef ENGLISH
			if (tab_bar_win_1 < 0)
#else //ENGLISH
			if (tab_bar_win < 0)
#endif //ENGLISH
			{
				//it doesn't exist
				return 0;
			}
#ifndef ENGLISH
			if(tabs_1[current_tab].channel != CHAT_ALL) {
				/* If we're in the All tab, we have already seen this message */
				for (i = 0; i < nb_tab_button_1; i++)
				{
					if (tabs_1[i].channel == channel)
					{
						if (current_tab != i && !tabs_1[i].highlighted)
						{
							widget_set_color (tab_bar_win_1, tabs_1[i].button, 1.0f, 0.0f, 0.0f);
							tabs_1[i].highlighted = 1;
						}
						break;
                    }
                }
			}
			if(tabs_2[current_tab].channel != CHAT_ALL) {
				/* If we're in the All tab, we have already seen this message */
				for (i = 0; i < nb_tab_button_2; i++)
				{
					if (tabs_2[i].channel == channel)
					{
						if (current_tab != i && !tabs_2[i].highlighted)
						{
							widget_set_color (tab_bar_win_2, tabs_2[i].button, 1.0f, 0.0f, 0.0f);
							tabs_2[i].highlighted = 1;
						}
						break;
                    }
                }
			}
#else //ENGLISH
			if(tabs[current_tab].channel != CHAT_ALL) {
				/* If we're in the All tab, we have already seen this message */
				for (i = 0; i < tabs_in_use; i++)
				{
					if (tabs[i].channel == channel)
					{
						if (current_tab != i && !tabs[i].highlighted)
						{
							widget_set_color (tab_bar_win, tabs[i].button, 1.0f, 0.0f, 0.0f);
							tabs[i].highlighted = 1;
						}
						break;
					}
				}
			}
#endif //ENGLISH
		break;
		case 2:
			if (chat_win < 0)
			{
				//it doesn't exist
				return 0;
			}
			if(channels[active_tab].chan_nr != CHAT_ALL) {
				/* If we're in the All tab, we have already seen this message */
				for (i = 0; i < MAX_CHAT_TABS; i++)
				{
					if (channels[i].open && channels[i].chan_nr == channel)
					{
						if (i != active_tab && !channels[i].highlighted)
						{
							tab_set_label_color_by_id (chat_win, chat_tabcollection_id, channels[i].tab_id, 1.0, 0.0, 0.0);
							channels[i].highlighted = 1;
						}
						break;
					}
				}
			}
		break;
		default:
			return 0;
		break;
	}
	return 1;
}

#ifndef ENGLISH
void switch_to_tab(int id, int niveau_tab)
{
    int i, j;
    if (niveau_tab == 1)
    {
	    i=2;
    	for(j=0;j < MAX_CHAT_TABS; ++j)
        {
            widget_set_color (tab_bar_win_1, tabs_1[j].button, 0.77f, 0.57f, 0.39f);
        }

    	widget_set_color (tab_bar_win_1, tabs_1[current_tab].button, 0.77f, 0.57f, 0.39f);
    	widget_set_color (tab_bar_win_1, tabs_1[0].button,  0.5f, 0.75f, 1.0f);
    	widget_set_color (tab_bar_win_1, tabs_1[1].button,  0.5f, 0.75f, 1.0f);
    	for(;i < MAX_CHAT_TABS; ++i) {
    		if(tabs_1[i].button <= 0) {
    			continue;
    		} else if(tabs_1[i].highlighted) {
    			continue;
    		}
    		widget_set_color (tab_bar_win_1, tabs_1[i].button, 0.77f, 0.57f, 0.39f);
    	}
        if (current_bar == 1)
        {
        	tabs_1[current_tab].highlighted = 0;
    	    widget_set_color (tab_bar_win_1, tabs_1[current_tab].button, 0.77f, 0.57f, 0.39f);
        }
        else if (current_bar == 2)
        {
        	tabs_2[current_tab].highlighted = 0;
    	    widget_set_color (tab_bar_win_2, tabs_2[current_tab].button, 0.77f, 0.57f, 0.39f);
        }
    	current_tab = id;
    	widget_set_color (tab_bar_win_1, tabs_1[current_tab].button, 0.57f, 1.0f, 0.59f);
	    current_filter = tabs_1[current_tab].channel;
        current_bar = 1;

    	if(tabs_1[current_tab].channel >= CHAT_CHANNEL1 && tabs_1[current_tab].channel <= CHAT_CHANNEL5) {
    		send_active_channel (tabs_1[current_tab].channel);
		    recolour_messages(display_text_buffer);
	    }
    }
    else if (niveau_tab == 2)
    {
	    i=0;
    	for(j=0;j < MAX_CHAT_TABS; ++j)
        {
            widget_set_color (tab_bar_win_2, tabs_2[j].button, 0.77f, 0.57f, 0.39f);
        }
    	widget_set_color (tab_bar_win_2, tabs_2[current_tab].button, 0.77f, 0.57f, 0.39f);
    	widget_set_color (tab_bar_win_2, tabs_2[current_tab].button, 0.77f, 0.57f, 0.39f);
    	for(;i < MAX_CHAT_TABS; ++i) {
    		if(tabs_2[i].button <= 0) {
    			continue;
    		} else if(tabs_2[i].highlighted) {
    			continue;
    		}
    		widget_set_color (tab_bar_win_2, tabs_2[i].button, 0.77f, 0.57f, 0.39f);
    	}
        if (current_bar == 1)
        {
        	tabs_1[current_tab].highlighted = 0;
    	    widget_set_color (tab_bar_win_1, tabs_1[current_tab].button, 0.77f, 0.57f, 0.39f);
        }
        else if (current_bar == 2)
        {
        	tabs_2[current_tab].highlighted = 0;
    	    widget_set_color (tab_bar_win_2, tabs_2[current_tab].button, 0.77f, 0.57f, 0.39f);
        }
    	current_tab = id;
    	widget_set_color (tab_bar_win_2, tabs_2[current_tab].button, 0.57f, 1.0f, 0.59f);
	    current_filter = tabs_2[current_tab].channel;
        current_bar = 2;

    	if(tabs_2[current_tab].channel >= CHAT_CHANNEL1 && tabs_2[current_tab].channel <= CHAT_CHANNEL5) {
    		send_active_channel (tabs_2[current_tab].channel);
		    recolour_messages(display_text_buffer);
	    }
    }
}
#else //ENGLISH
void switch_to_tab(int id)
{
	int i=2;
	widget_set_color (tab_bar_win, tabs[current_tab].button, 0.77f, 0.57f, 0.39f);
	widget_set_color (tab_bar_win, tabs[0].button,  0.5f, 0.75f, 1.0f);
	widget_set_color (tab_bar_win, tabs[1].button,  0.5f, 0.75f, 1.0f);
	for(;i < MAX_CHAT_TABS; ++i) {
		if(tabs[i].button <= 0) {
			continue;
		} else if(tabs[i].highlighted) {
			continue;
		}
		widget_set_color (tab_bar_win, tabs[i].button, 0.77f, 0.57f, 0.39f);
	}
	current_tab = id;
	widget_set_color (tab_bar_win, tabs[current_tab].button, 0.57f, 1.0f, 0.59f);
	current_filter = tabs[current_tab].channel;
	tabs[current_tab].highlighted = 0;
	if(tabs[current_tab].channel >= CHAT_CHANNEL1 && tabs[current_tab].channel <= CHAT_CHANNEL3) {
		send_active_channel (tabs[current_tab].channel);
		recolour_messages(display_text_buffer);
	}
}
#endif //ENGLISH


#ifndef ENGLISH
int tab_bar_button_click (widget_list *w, int mx, int my, Uint32 flags, int ligne)
{
	int itab;

    for (itab = 0; itab < tabs_in_use; itab++)
   	{
        if (ligne == 1)
        {
   		    if (w->id == tabs_1[itab].button)
   			    break;
        }
        if (ligne == 2)
        {
   		    if (w->id == tabs_2[itab].button)
   			    break;
        }
   	}


    if (itab >= tabs_in_use)
	    // shouldn't happen
		return 0;

	if (flags&ELW_RIGHT_MOUSE)
	{
		if (ligne == 1)
		{
			display_channel_color_win(get_active_channel(tabs_1[itab].channel));
		}
		else if (ligne == 2)
		{
			display_channel_color_win(get_active_channel(tabs_2[itab].channel));
		}
		return 1;
	}
	else
	{

		// NOTE: This is an optimization, instead of redefining a "Tab/Button" type.
		//		 Further use of this would be best served be a new definition.
		// Detect clicking on 'x'
		if (ligne == 1)
		{
			if(tabs_1[itab].channel == CHAT_CHANNEL1 || tabs_1[itab].channel == CHAT_CHANNEL2 ||
					tabs_1[itab].channel == CHAT_CHANNEL3 || tabs_1[itab].channel == CHAT_CHANNEL4 ||
					tabs_1[itab].channel == CHAT_CHANNEL5)
			{
				int x = w->len_x - 6;
				int y = 5;
				char str[256];

				// 'x' was clicked?
				if(mx > x-4 && mx < x+3 && my > y-4 && my < y+3)
				{
					// Drop this channel via #lc
					snprintf(str, sizeof(str), "%c#qc %d", RAW_TEXT, active_channels[tabs_1[itab].channel-CHAT_CHANNEL1]);
					my_tcp_send(my_socket, (Uint8*)str, strlen(str+1)+1);
					// Can I remove this?
					remove_tab(tabs_1[itab].channel,1);
					if(current_tab == itab) {
						int i;
						//We're closing the current tab, switch to the all-tab
						for(i = 0; i < tabs_in_use; i++)
						{
							if(tabs_1[i].channel == CHAT_ALL)
							{
								switch_to_tab(i,1);
								break;
							}
						}
					}
					return 1; //The click was handled, no need to continue
				}
			}
			if (current_tab != itab)
			{
				switch_to_tab(itab,1);
			}
		}
		else if (ligne == 2)
		{
			if(tabs_2[itab].channel == CHAT_CHANNEL1 || tabs_2[itab].channel == CHAT_CHANNEL2 ||
					tabs_2[itab].channel == CHAT_CHANNEL3 || tabs_2[itab].channel == CHAT_CHANNEL4 ||
					tabs_2[itab].channel == CHAT_CHANNEL5)
			{
				int x = w->len_x - 6;
				int y = 5;
				char str[256];
				// 'x' was clicked?
				if(mx > x-4 && mx < x+3 && my > y-4 && my < y+3)
				{
					// Drop this channel via #lc
					snprintf(str, sizeof(str), "%c#qc %d", RAW_TEXT, active_channels[tabs_2[itab].channel-CHAT_CHANNEL1]);
					my_tcp_send(my_socket, (Uint8*)str, strlen(str+1)+1);
					// Can I remove this?
					remove_tab(tabs_2[itab].channel,2);
					if(current_tab == itab) {
						int i;
						//We're closing the current tab, switch to the all-tab
						for(i = 0; i < nb_tab_button_1; i++)
						{
							if(tabs_1[i].channel == CHAT_ALL)
							{
								switch_to_tab(i,1);
								break;
							}
						}
					}
					return 1; //The click was handled, no need to continue
				}
			}
			if (current_tab != itab)
			{
				switch_to_tab(itab,2);
			}
		}
	}

	lines_to_show = 10;

	return 1;
}
int tab_bar_button_click_1 (widget_list *w, int mx, int my, Uint32 flags)
{
   return tab_bar_button_click(w, mx, my, flags, 1);
}
int tab_bar_button_click_2 (widget_list *w, int mx, int my, Uint32 flags)
{
   return tab_bar_button_click(w, mx, my, flags, 2);
}

#else //ENGLISH
int tab_bar_button_click (widget_list *w, int mx, int my, Uint32 flags)
{
	int itab;

	for (itab = 0; itab < tabs_in_use; itab++)
	{
		if (w->id == tabs[itab].button)
			break;
	}

	if (itab >= tabs_in_use)
		// shouldn't happen
		return 0;


	if (flags&ELW_RIGHT_MOUSE)
	{
		display_channel_color_win(get_active_channel(tabs[itab].channel));
		return 1;
	}
	else
	{
		// NOTE: This is an optimization, instead of redefining a "Tab/Button" type.
		//		 Further use of this would be best served be a new definition.
		// Detect clicking on 'x'
		if(tabs[itab].channel == CHAT_CHANNEL1 || tabs[itab].channel == CHAT_CHANNEL2 ||
				tabs[itab].channel == CHAT_CHANNEL3)
		{
			int x = w->len_x - 6;
			int y = 5;
			char str[256];

			// 'x' was clicked?
			if(mx > x-4 && mx < x+3 && my > y-4 && my < y+3)
			{
				// Drop this channel via #lc
				safe_snprintf(str, sizeof(str), "%c#lc %d", RAW_TEXT, active_channels[tabs[itab].channel-CHAT_CHANNEL1]);
				my_tcp_send(my_socket, (Uint8*)str, strlen(str+1)+1);
				// Can I remove this?
				remove_tab(tabs[itab].channel);
				if(current_tab == itab) {
					int i;
					//We're closing the current tab, switch to the all-tab
					for(i = 0; i < tabs_in_use; i++)
					{
						if(tabs[i].channel == CHAT_ALL)
						{
							switch_to_tab(i);
							break;
						}
					}
				}
				do_click_sound();
				return 1; //The click was handled, no need to continue
			}
		}


		if (current_tab != itab)
		{
			switch_to_tab(itab);
			do_click_sound();
		}
		lines_to_show = 10;
	}
	return 1;
}
#endif //ENGLISH

char tmp_tab_label[20];

chan_name *tab_label (Uint8 chan)
{
	//return pointer after stepping through chan_name_queue
	int cnr, steps=0;
	node_t *step = queue_front_node(chan_name_queue);
	char name[255];
	char desc[255];
	switch (chan)
	{
		case CHAT_ALL:	return pseudo_chans[2];
		case CHAT_NONE:	return pseudo_chans[3];
		case CHAT_LIST:	return pseudo_chans[4];
		case CHAT_HIST:	return pseudo_chans[5];
		case CHAT_LOCAL:	return pseudo_chans[6];
		case CHAT_PERSONAL:	return pseudo_chans[7];
		case CHAT_GM:	return pseudo_chans[8];
		case CHAT_SERVER:	return pseudo_chans[9];
		case CHAT_MOD:	return pseudo_chans[10];
#ifdef FR_VERSION
		case CHAT_DEV:	return pseudo_chans[11];
		case CHAT_COORD:	return pseudo_chans[12];
		case CHAT_COMBAT: return pseudo_chans[13];
#endif //FR_VERSION
	}

#ifndef ENGLISH
	if(chan < CHAT_CHANNEL1 || chan > CHAT_CHANNEL5 ){
#else //ENGLISH
	if(chan < CHAT_CHANNEL1 || chan > CHAT_CHANNEL3 ){
#endif //ENGLISH
		// shouldn't get here...
		return NULL;
	}
	cnr = active_channels[chan-CHAT_CHANNEL1];
	if(cnr >= 1000000000){//ooh, guild channel!
		return pseudo_chans[1];
	}
	if(step == NULL){
		//say what? we don't know _any_ channels? something is very wrong...
		return NULL;
	}
	for (; step != NULL && step->data != NULL; step = step->next, steps++){
		if(((chan_name*)(step->data))->channel == cnr){
			return step->data;
		}
		if(step->next == NULL){
			break;
		}
	}
	//we didn't find it, so we use the generic version
	safe_snprintf (name, sizeof(name), pseudo_chans[0]->name, cnr);
	safe_snprintf (desc, sizeof(desc), pseudo_chans[0]->description, cnr);
	add_chan_name(cnr,name,desc);

#ifdef FR_VERSION
	// inutile puisque add_chan_name() se charge de mettre à jour la barre de défilement
#else //FR_VERSION
	if(chan_sel_scroll_id >= 0 && steps > 8) {
		vscrollbar_set_bar_len(chan_sel_win, chan_sel_scroll_id, steps-8);
		//we're adding another name to the queue, so the window scrollbar needs to be adusted
	}
#endif //FR_VERSION
	return step->next->data;
}

unsigned int chan_int_from_name(char * name, int * return_length)
{
	node_t *step = queue_front_node(chan_name_queue);
	char * cname = name;

	while(*cname && isspace(*cname)) {	//should there be a space at the front,
		cname++;						//we can handle that.
	}
	while(step->next != NULL) {
		step = step->next;
		if(my_strncompare(((chan_name*)(step->data))->name, cname, strlen(((chan_name*)(step->data))->name))) {
			if(return_length != NULL) {
				*return_length = strlen(((chan_name*)(step->data))->name);
			}
			return ((chan_name*)(step->data))->channel;
		}
	}
	return 0;
}

#ifndef ENGLISH
void update_tab_button_idx (Uint8 old_idx, Uint8 new_idx, int niveau_tab)
#else //ENGLISH
void update_tab_button_idx (Uint8 old_idx, Uint8 new_idx)
#endif //ENGLISH
{
	int itab;

	if (old_idx == new_idx) return;

#ifndef ENGLISH
    if (niveau_tab == 1)
    {
    	for (itab = 0; itab < nb_tab_button_1; itab++)
    	{
    		if (tabs_1[itab].channel == old_idx)
    		{
    			tabs_1[itab].channel = new_idx;
    			return;
            }
        }
    }
    if (niveau_tab == 2)
    {
    	for (itab = 0; itab < tabs_in_use-nb_tab_button_1; itab++)
    	{
    		if (tabs_2[itab].channel == old_idx)
    		{
    			tabs_2[itab].channel = new_idx;
    			return;
            }
        }
    }
#else //ENGLISH
	for (itab = 0; itab < tabs_in_use; itab++)
	{
		if (tabs[itab].channel == old_idx)
		{
			tabs[itab].channel = new_idx;
			return;
		}
	}
#endif //ENGLISH
}

#ifndef ENGLISH
int chan_tab_mouseover_handler(widget_list *widget, int niveau_tab)
{
	int itab = 0;
	if(!show_help_text){return 0;}
	for (itab = 0; itab < tabs_in_use; itab++){
        if (niveau_tab == 1)
        {
    		if ((tabs_1[itab].button) == (widget->id))
            {
    			show_help(tabs_1[itab].description, widget->pos_x,widget->pos_y+widget->len_y);
    			return 1;
    		}
        }
        if (niveau_tab == 2)
        {
    		if ((tabs_2[itab].button) == (widget->id))
            {
    			show_help(tabs_2[itab].description, widget->pos_x,widget->pos_y+widget->len_y);
    			return 1;
    		}
        }
	}
	return 0;
}
#else //ENGLISH
int chan_tab_mouseover_handler(widget_list *widget)
{
	int itab = 0;
	if(!show_help_text){return 0;}
	for (itab = 0; itab < tabs_in_use; itab++){
		if ((tabs[itab].button) == (widget->id)){
			show_help(tabs[itab].description, widget->pos_x,widget->pos_y+widget->len_y);
			return 1;
		}
	}
	return 0;
}
#endif //ENGLISH

#ifndef ENGLISH
int chan_tab_mouseover_handler_1(widget_list *widget)
{
    return chan_tab_mouseover_handler(widget, 1);
}
int chan_tab_mouseover_handler_2(widget_list *widget)
{
    return chan_tab_mouseover_handler(widget, 2);
}
#endif //ENGLISH

int display_chan_sel_handler(window_info *win)
{
	int i = 0, y = 5, x = 5, t = 0, num_lines = 0;
	float local_zoom = 0.75f;

	node_t *step = queue_front_node(chan_name_queue);

#ifdef FR_VERSION
    cleanup_chan_race_names();
	if (chan_sel_scroll_id != -1) vscrollbar_set_bar_len(chan_sel_win, chan_sel_scroll_id, chan_name_queue->nodes);
#endif //FR_VERSION

	if(mouse_x >= win->pos_x+win->len_x || mouse_y >= win->pos_y+win->len_y) {
		win->displayed = 0;
		return 0;//auto close when you mouseout
	}
	t = vscrollbar_get_pos(chan_sel_win, chan_sel_scroll_id);
	if(t > 0) {
		for (; i<t ; ++i) {
			if(step->next == NULL) {
				break;
			}
			step = step->next;
		}
	}
	for (i = 0; i < CS_MAX_DISPLAY_CHANS; ++i) {//loathe not having auto-moving widgets...
		glColor3f(0.5f, 0.75f, 1.0f);
		draw_string_zoomed(x, y, (unsigned char*)((chan_name*)(step->data))->name, 1, local_zoom);
		if(mouse_y > win->pos_y+y && mouse_y < win->pos_y+y+20 && mouse_x >= win->pos_x+5
			&& mouse_x-5 <= win->pos_x + 8*((signed)strlen(((chan_name*)(step->data))->name))) {
			show_help(((chan_name*)(step->data))->description, mouse_x-win->pos_x,mouse_y-win->pos_y-15);
		}
		y += 18;
		step = step->next;
		if(step == NULL) {
			y += (18*(10-i-1));
			break;
		}
	}
	glDisable(GL_TEXTURE_2D);
	glColor3f(0.77f, 0.57f, 0.39f);
	glBegin(GL_LINES);
		glVertex2i(1, y-2);
		glVertex2i(win->len_x, y-2);
	glEnd();
	glEnable(GL_TEXTURE_2D);
	num_lines = reset_soft_breaks(channel_help_str, strlen(channel_help_str), sizeof(channel_help_str), local_zoom, win->len_x - 5, NULL, NULL);
	draw_string_zoomed(x, y+=5, (unsigned char*)channel_help_str, num_lines, local_zoom);
	win->len_y = 187 + num_lines * DEFAULT_FONT_Y_LEN * local_zoom + 2;
#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
	return 0;
}

int click_chan_sel_handler(window_info *win, int mx, int my, Uint32 flags)
{
	int i = 0, y = my-5;
	node_t *step = queue_front_node(chan_name_queue);
	y /= 18;
	i = vscrollbar_get_pos(chan_sel_win, chan_sel_scroll_id);
	if(i>0){
		y+=i;
	}

	if(flags&ELW_WHEEL_UP) {
		vscrollbar_scroll_up(chan_sel_win, chan_sel_scroll_id);
	} else if(flags&ELW_WHEEL_DOWN) {
		vscrollbar_scroll_down(chan_sel_win, chan_sel_scroll_id);
	} else {
		for (i = 0; i < y ; ++i) {
			if(step->next == NULL) {
				return 0;
			}
			step = step->next;
		}
		if(mouse_x >= win->pos_x+5 && mouse_x-5 <= win->pos_x + 8*((signed)strlen(((chan_name*)(step->data))->name))) {
			char tmp[20];
			safe_snprintf(tmp, sizeof(tmp), "#jc %d", ((chan_name*)(step->data))->channel);
			send_input_text_line(tmp, strlen(tmp));
			do_click_sound();
		}
	}
	return 1;
}

int tab_special_click(widget_list *w, int mx, int my, Uint32 flags)
{
	int itab = 0;
#ifndef ENGLISH
	for (itab = 0; itab < nb_tab_button_1; itab++) {
		if (tabs_1[itab].button == w->id){
			switch(tabs_1[itab].channel) {
#else //ENGLISH
	for (itab = 0; itab < tabs_in_use; itab++) {
		if (tabs[itab].button == w->id){
			switch(tabs[itab].channel) {
#endif //ENGLISH
				case CHAT_HIST:
#ifdef FR_VERSION
					/* On inverse l'ordre des 2 fonctions pour éviter les soucis avec le texte de chat qui ne s'affiche plus */
					toggle_window(console_root_win);
					toggle_window(game_root_win);
#else //FR_VERSION
					toggle_window(game_root_win);
					toggle_window(console_root_win);
#endif //FR_VERSION
					do_click_sound();
					break;
				case CHAT_LIST:
					if(chan_sel_win >= 0) {
						toggle_window(chan_sel_win);
					} else {
#ifndef ENGLISH
						chan_sel_win = create_window ("Channel Selection", tab_bar_win_1, 0, w->pos_x,w->pos_y+w->len_y+1, 185, 187, (ELW_USE_BACKGROUND|ELW_USE_BORDER|ELW_SHOW|ELW_ALPHA_BORDER|ELW_CLOSE_BOX));
#else //ENGLISH
						chan_sel_win = create_window ("Channel Selection", tab_bar_win, 0, w->pos_x,w->pos_y+w->len_y+1, 185, 187, (ELW_USE_BACKGROUND|ELW_USE_BORDER|ELW_SHOW|ELW_ALPHA_BORDER|ELW_CLOSE_BOX));
#endif //ENGLISH
						windows_list.window[chan_sel_win].back_color[3]= 0.25f;
						set_window_handler (chan_sel_win, ELW_HANDLER_DISPLAY, &display_chan_sel_handler);
						set_window_handler (chan_sel_win, ELW_HANDLER_CLICK, &click_chan_sel_handler);
						if(chan_name_queue->nodes >= CS_MAX_DISPLAY_CHANS && chan_sel_scroll_id == -1) {
#ifdef FR_VERSION
							chan_sel_scroll_id = vscrollbar_add_extended (chan_sel_win, 0, NULL, 165, 20, 20, 163, 0, CS_MAX_DISPLAY_CHANS, 0.77f, 0.57f, 0.39f, 0, 1, chan_name_queue->nodes);
#else //FR_VERSION
							int len = chan_name_queue->nodes-CS_MAX_DISPLAY_CHANS;
							chan_sel_scroll_id = vscrollbar_add_extended (chan_sel_win, 0, NULL, 165, 20, 20, 163, 0, 1.0, 0.77f, 0.57f, 0.39f, 0, 1, len);
#endif //FR_VERSION
						}
					}
					do_click_sound();
					break;
				default:
#ifndef ENGLISH
					return tab_bar_button_click(w, mx, my, flags,1);	//grumble. this shouldn't happen
#else //ENGLISH
					return tab_bar_button_click(w, mx, my, flags);	//grumble. this shouldn't happen
#endif //ENGLISH
			}
			return 1;
		}
	}
	return 0;
}


// Draw details on the channel buttons.

static int draw_tab_details (widget_list *W)
{
	int x = W->pos_x + W->len_x - 6;
	int y = W->pos_y+5;
#ifdef ENGLISH
	int itab;
#else //ENGLSIH
	int itab_1, itab_2;
#endif //ENGLISH

	glColor3f(0.77f,0.57f,0.39f);

	glDisable(GL_TEXTURE_2D);

	/* check for an active "#jc" channel */
#ifdef ENGLISH
	for (itab = 0; itab < tabs_in_use; itab++)
		if ((tabs[itab].button == W->id) && (tabs[itab].channel == CHAT_CHANNEL1 + current_channel))
		{
			int x = W->pos_x+2;
			int y = W->pos_y+1;
			int i, color;
			/* draw the "+" for the active channel */
			for(i=0; i<MAX_CHANNEL_COLORS; i++)
			{
				if(channel_colors[i].nr == get_active_channel(tabs[itab].channel) && channel_colors[i].color > -1)
				break;
			}
			if(i < MAX_CHANNEL_COLORS)
			{
				color = channel_colors[i].color;
				glColor3ub(colors_list[color].r1, colors_list[color].g1, colors_list[color].b1);
			}
			glBegin(GL_LINES);
				glVertex2i(x+gx_adjust,y+4);
				glVertex2i(x+7+gx_adjust,y+4);
				glVertex2i(x+3,y+gy_adjust);
				glVertex2i(x+3,y+7+gy_adjust);
			glEnd();
			glColor3f(0.77f,0.57f,0.39f);
			/* draw a dotted underline if input would go to this channel */
			if ((input_text_line.len > 0) && (input_text_line.data[0] == '@') && !((input_text_line.len > 1) && (input_text_line.data[1] == '@')))
			{
				glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(1, 0xCCCC);
				glLineWidth(3.0);
				glBegin(GL_LINES);
					glVertex2i(W->pos_x, W->pos_y + W->len_y + 4);
					glVertex2i(W->pos_x + W->len_x, W->pos_y + W->len_y + 4);
				glEnd();
				glPopAttrib();
			}
			break;
		}
#else //ENGLISH
	for (itab_1 = 0; itab_1 < nb_tab_button_1; itab_1++)
		if ((tabs_1[itab_1].button == W->id) && (tabs_1[itab_1].channel == CHAT_CHANNEL1 + current_channel))
		{
			int i, color;
			/* draw the "+" for the active channel */
			for(i=0; i<MAX_CHANNEL_COLORS; i++)
			{
				if(channel_colors[i].nr == get_active_channel(tabs_1[itab_1].channel) && channel_colors[i].color > -1)
				break;
			}
			if(i < MAX_CHANNEL_COLORS)
			{
				color = channel_colors[i].color;
				glColor3ub(colors_list[color].r1, colors_list[color].g1, colors_list[color].b1);
			}
			glBegin(GL_LINES);
				glVertex2i(x+gx_adjust,y+4);
				glVertex2i(x+7+gx_adjust,y+4);
				glVertex2i(x+3,y+gy_adjust);
				glVertex2i(x+3,y+7+gy_adjust);
			glEnd();
			glColor3f(0.77f,0.57f,0.39f);
			/* draw a dotted underline if input would go to this channel */
			if ((input_text_line.len > 0) && (input_text_line.data[0] == '!') && !((input_text_line.len > 1) && (input_text_line.data[1] == '!')))
			{
				glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(1, 0xCCCC);
				glLineWidth(3.0);
				glBegin(GL_LINES);
					glVertex2i(W->pos_x, W->pos_y + W->len_y + 4);
					glVertex2i(W->pos_x + W->len_x, W->pos_y + W->len_y + 4);
				glEnd();
				glPopAttrib();
			}
			break;
		}
	for (itab_2 = 0; itab_2 < nb_tab_button_2; itab_2++)
		if ((tabs_2[itab_2].button == W->id) && (tabs_2[itab_2].channel == CHAT_CHANNEL1 + current_channel))
		{
			int i, color;
			/* draw the "+" for the active channel */
			for(i=0; i<MAX_CHANNEL_COLORS; i++)
			{
				if(channel_colors[i].nr == get_active_channel(tabs_2[itab_2].channel) && channel_colors[i].color > -1)
				break;
			}
			if(i < MAX_CHANNEL_COLORS)
			{
				color = channel_colors[i].color;
				glColor3ub(colors_list[color].r1, colors_list[color].g1, colors_list[color].b1);
			}
			glBegin(GL_LINES);
				glVertex2i(x+gx_adjust,y+4);
				glVertex2i(x+7+gx_adjust,y+4);
				glVertex2i(x+3,y+gy_adjust);
				glVertex2i(x+3,y+7+gy_adjust);
			glEnd();
			glColor3f(0.77f,0.57f,0.39f);
			/* draw a dotted underline if input would go to this channel */
			if ((input_text_line.len > 0) && (input_text_line.data[0] == '@') && !((input_text_line.len > 1) && (input_text_line.data[1] == '@')))
			{
				glPushAttrib(GL_ENABLE_BIT|GL_LINE_BIT);
				glEnable(GL_LINE_STIPPLE);
				glLineStipple(1, 0xCCCC);
				glLineWidth(3.0);
				glBegin(GL_LINES);
					glVertex2i(W->pos_x, W->pos_y + W->len_y + 4);
					glVertex2i(W->pos_x + W->len_x, W->pos_y + W->len_y + 4);
				glEnd();
				glPopAttrib();
			}
			break;
		}
#endif //ENGLISH


	/* draw the closing x */
	glBegin(GL_LINES);
		glVertex2i(x-4,y-4);
		glVertex2i(x+3,y+3);

		glVertex2i(x-4,y+3);
		glVertex2i(x+3,y-4);
	glEnd();
	glEnable(GL_TEXTURE_2D);
#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
	return 1;
}

#ifndef ENGLISH
int add_tab_button (Uint8 channel)
{
	int itab_1, itab_2;
	const char *label;
	chan_name *chan;

	for (itab_1 = 0; itab_1 < nb_tab_button_1; itab_1++)
	{
		if (tabs_1[itab_1].channel == channel)
        {
			// already there
			return itab_1;
        }
	}
	for (itab_2 = 0; itab_2 < nb_tab_button_2; itab_2++)
	{
		if (tabs_2[itab_2].channel == channel)
        {
			// already there
			return itab_2;
        }
	}
	if (tabs_in_use >= MAX_CHAT_TABS)
		// no more room. Shouldn't happen anyway.
		return -1;

    // La taille des canaux est plus grande que la largeur de l'affichage
    // on passe sur la deuxieme ligne
    if ((window_width-hud_x < tab_bar_width_1+(Uint16)(strlen(tab_label(channel)->name) * 11 * 0.75) + 30*0.75) && changement_barre == 0)
    {
    	tabs_2[nb_tab_button_2].channel = channel;
    	tabs_2[nb_tab_button_2].highlighted = 0;
    	chan = tab_label (channel);
    	if(chan == NULL){
    		return -1;
    	}
    	label = chan->name;
    	tabs_2[nb_tab_button_2].description = chan->description;
    }
    else
    {
    	tabs_1[nb_tab_button_1].channel = channel;
    	tabs_1[nb_tab_button_1].highlighted = 0;
    	chan = tab_label (channel);
    	if(chan == NULL){
    		return -1;
    	}
    	label = chan->name;
    	tabs_1[nb_tab_button_1].description = chan->description;
        changement_barre = 1;
    }

    // La taille des canaux est plus grande que la largeur de l'affichage
    // on passe sur la deuxieme ligne
    if ((window_width-hud_x < tab_bar_width_1+(Uint16)(strlen(tab_label(channel)->name) * 11 * 0.75) + 30*0.75) && changement_barre == 0)
    {
        // La deuxieme ligne n'est pas encore cree
	    if (tab_bar_win_2 < 0)
    	{
    		create_tab_bar ();
    	}
	    tabs_2[nb_tab_button_2].button = button_add_extended (tab_bar_win_2, cur_button_id_2++, NULL, tab_bar_width_2, 0, 0, tab_bar_height, 0, 0.75, 0.77f, 0.57f, 0.39f, label);
    }
    else // La taille est inferieur, on contiue sur la premiere ligne
    {
	    tabs_1[nb_tab_button_1].button = button_add_extended (tab_bar_win_1, cur_button_id_1++, NULL, tab_bar_width_1, 0, 0, tab_bar_height, 0, 0.75, 0.77f, 0.57f, 0.39f, label);
    }

    if(channel == CHAT_HIST || channel == CHAT_LIST) {
		//a couple of special cases
		widget_set_OnClick (tab_bar_win_1, tabs_1[itab_1].button, tab_special_click);
		widget_set_color (tab_bar_win_1, tabs_1[itab_1].button, 0.5f, 0.75f, 1.0f);
	} else {
		//general case
        if (nb_ligne_tabs == 1 || changement_barre == 1)
        {
		    widget_set_OnClick (tab_bar_win_1, tabs_1[itab_1].button, tab_bar_button_click_1);
        }
        else if (nb_ligne_tabs == 2 && changement_barre == 0)
        {
		    widget_set_OnClick (tab_bar_win_2, tabs_2[itab_2].button, tab_bar_button_click_2);
        }
	}

    if (nb_ligne_tabs == 1 || changement_barre == 1)
    {
	    widget_set_OnMouseover (tab_bar_win_1, tabs_1[itab_1].button, chan_tab_mouseover_handler_1);
    	widget_set_type(tab_bar_win_1, tabs_1[itab_1].button, &square_button_type);
    }
    else if (nb_ligne_tabs == 2 && changement_barre == 0)
    {
	    widget_set_OnMouseover (tab_bar_win_2, tabs_2[itab_2].button, chan_tab_mouseover_handler_2);
    	widget_set_type(tab_bar_win_2, tabs_2[itab_2].button, &square_button_type);
    }
 	// Handlers for the 'x'
 	// Make sure it's a CHANNEL first

    if (nb_ligne_tabs == 1 || changement_barre == 1)
    {
        if(tabs_1[itab_1].channel == CHAT_CHANNEL1 || tabs_1[itab_1].channel == CHAT_CHANNEL2 ||
     	   tabs_1[itab_1].channel == CHAT_CHANNEL3 || tabs_1[itab_1].channel == CHAT_CHANNEL4 ||
           tabs_1[itab_1].channel == CHAT_CHANNEL5)
        {
     		widget_set_OnDraw (tab_bar_win_1, tabs_1[itab_1].button, draw_tab_details);
        }
    }
    else if (nb_ligne_tabs == 2 && changement_barre == 0)
    {
        if(tabs_2[itab_2].channel == CHAT_CHANNEL1 || tabs_2[itab_2].channel == CHAT_CHANNEL2 ||
     	   tabs_2[itab_2].channel == CHAT_CHANNEL3 || tabs_2[itab_2].channel == CHAT_CHANNEL4 ||
           tabs_2[itab_2].channel == CHAT_CHANNEL5)
        {
     		widget_set_OnDraw (tab_bar_win_2, tabs_2[itab_2].button, draw_tab_details);
        }
    }

    if (nb_ligne_tabs == 1 || changement_barre == 1)
    {
	    tab_bar_width_1 += widget_get_width (tab_bar_win_1, tabs_1[nb_tab_button_1].button)+1;
    	resize_window (tab_bar_win_1, tab_bar_width_1, tab_bar_height);
        nb_tab_button_1++;
    }
    else if (nb_ligne_tabs == 2 && changement_barre == 0)
    {
    	tab_bar_width_2 += widget_get_width (tab_bar_win_2, tabs_2[nb_tab_button_2].button)+1;
    	resize_window (tab_bar_win_2, tab_bar_width_2, tab_bar_height);
        nb_tab_button_2++;
    }

    changement_barre = 0;
	tabs_in_use++;
	return tabs_in_use - 1;
}

#else //ENGLISH
int add_tab_button (Uint8 channel)
{
	int itab;
	const char *label;
	chan_name *chan;

	for (itab = 0; itab < tabs_in_use; itab++)
	{
		if (tabs[itab].channel == channel)
			// already there
			return itab;
	}

	if (tabs_in_use >= MAX_CHAT_TABS)
		// no more room. Shouldn't happen anyway.
		return -1;

	tabs[tabs_in_use].channel = channel;
	tabs[tabs_in_use].highlighted = 0;
	chan = tab_label (channel);
	if(chan == NULL){
		return -1;
	}
	label = chan->name;
	tabs[tabs_in_use].description = chan->description;

	tabs[tabs_in_use].button = button_add_extended (tab_bar_win, cur_button_id++, NULL, tab_bar_width, 0, 0, tab_bar_height, 0, 0.75, 0.77f, 0.57f, 0.39f, label);
	if(channel == CHAT_HIST || channel == CHAT_LIST) {
		//a couple of special cases
		widget_set_OnClick (tab_bar_win, tabs[itab].button, tab_special_click);
		widget_set_color (tab_bar_win, tabs[itab].button, 0.5f, 0.75f, 1.0f);
	} else {
		//general case
		widget_set_OnClick (tab_bar_win, tabs[itab].button, tab_bar_button_click);
	}
	widget_set_OnMouseover (tab_bar_win, tabs[itab].button, chan_tab_mouseover_handler);
	widget_set_type(tab_bar_win, tabs[itab].button, &square_button_type);
 	// Handlers for the 'x'
 	// Make sure it's a CHANNEL first
 	if(tabs[itab].channel == CHAT_CHANNEL1 || tabs[itab].channel == CHAT_CHANNEL2 ||
 	   tabs[itab].channel == CHAT_CHANNEL3)
 	{
 		widget_set_OnDraw (tab_bar_win, tabs[itab].button, draw_tab_details);
 	}
	tab_bar_width += widget_get_width (tab_bar_win, tabs[tabs_in_use].button)+1;
	resize_window (tab_bar_win, tab_bar_width, tab_bar_height);

	tabs_in_use++;
	return tabs_in_use - 1;
}
#endif //ENGLISH

#ifndef ENGLISH
void remove_tab_button (Uint8 channel, int ligne)
{
	int itab, w;

	for (itab = 0; itab < tabs_in_use; itab++)
	{
        if (ligne == 1)
        {
		    if (tabs_1[itab].channel == channel)
    			break;
        }
        else if (ligne == 2)
        {
		    if (tabs_2[itab].channel == channel)
			    break;
        }
	}
	if (itab >= tabs_in_use) return;

    if (ligne == 1)
    {
	    w = widget_get_width (tab_bar_win_1, tabs_1[itab].button)+1;
    	widget_destroy (tab_bar_win_1, tabs_1[itab].button);
    	for (++itab; itab < nb_tab_button_1; itab++)
    	{
    		widget_move_rel (tab_bar_win_1, tabs_1[itab].button, -w, 0);
    		tabs_1[itab-1] = tabs_1[itab];
    	}
    	tabs_in_use--;
        nb_tab_button_1--;

    	tab_bar_width_1 -= w;
    	resize_window (tab_bar_win_1, tab_bar_width_1, tab_bar_height);

        // Il y a assez de place pour passer le premier canal de la deuxieme ligne sur la premiere
		//TODO (TonyFlow): tester aussi si les onglets suivants de la ligne 2 peuvent aller en ligne 1
        if (nb_ligne_tabs == 2 && (window_width-hud_x > tab_bar_width_1 + widget_get_width (tab_bar_win_2, tabs_2[0].button)+ 1))
        {
            Uint8 temp_channel;
            changement_barre = 1;
            temp_channel = tabs_2[0].channel;
            remove_tab_button (tabs_2[0].channel, 2);
            add_tab_button(temp_channel);
        }
   	}
    else if (ligne == 2)
    {
        w = widget_get_width (tab_bar_win_2, tabs_2[itab].button)+1;
    	widget_destroy (tab_bar_win_2, tabs_2[itab].button);
    	for (++itab; itab < nb_tab_button_2; itab++)
    	{
    		widget_move_rel (tab_bar_win_2, tabs_2[itab].button, -w, 0);
    		tabs_2[itab-1] = tabs_2[itab];
    	}
    	tabs_in_use--;
        nb_tab_button_2--;

    	tab_bar_width_2 -= w;
    	resize_window (tab_bar_win_2, tab_bar_width_2, tab_bar_height);

        // Il n'y a plus rien sur la 2eme barre, on la supprime donc
		//TODO (TonyFlow): tester si maintenant le (ou les) premier(s) onglet(s) peuvent aller en ligne 1
        if (tabs_in_use == nb_tab_button_1)
        {
			window_info *console_win = &windows_list.window[console_root_win];
    		widget_list *console_out_w = widget_find(console_root_win, console_out_id);

            destroy_window (tab_bar_win_2);
            nb_ligne_tabs = 1;
            tab_bar_win_2 = -1;
            cur_button_id_2 = 0;
            tab_bar_width_2 = 0;

			// On décalle et on change la taille de la console_out
			widget_move(console_root_win, console_out_id, 10, 10 + nb_ligne_tabs*tab_bar_height);
			widget_resize(console_root_win, console_out_id, console_out_w->len_x, console_win->len_y - input_widget->len_y - CONSOLE_SEP_HEIGHT - HUD_MARGIN_Y - 10 - nb_ligne_tabs*tab_bar_height);
			nr_console_lines = (console_out_w->len_y - 1) / (int)(DEFAULT_FONT_Y_LEN * chat_zoom);
			display_console_handler((widget_find(console_root_win, console_out_id))->widget_info);
        }
    }
}
#else //ENGLISH
void remove_tab_button (Uint8 channel)
{
	int itab, w;

	for (itab = 0; itab < tabs_in_use; itab++)
	{
		if (tabs[itab].channel == channel)
			break;
	}
	if (itab >= tabs_in_use) return;

	w = widget_get_width (tab_bar_win, tabs[itab].button)+1;
	widget_destroy (tab_bar_win, tabs[itab].button);
	for (++itab; itab < tabs_in_use; itab++)
	{
		widget_move_rel (tab_bar_win, tabs[itab].button, -w, 0);
		tabs[itab-1] = tabs[itab];
	}
	tabs_in_use--;

	tab_bar_width -= w;
	resize_window (tab_bar_win, tab_bar_width, tab_bar_height);
}
#endif //ENGLISH

void update_tab_bar (text_message * msg)
{
	int itab, new_button;
	Uint8 channel;

	// dont need to care for deleted messages
	if (msg->deleted) return;

	// don't bother if there's no tab bar
#ifndef ENGLISH
	if (tab_bar_win_1 < 0 && tab_bar_win_2 < 0) return;
#else //ENGLISH
	if (tab_bar_win < 0) return;
#endif //ENGLISH

	// Only update specific channels
	channel = get_tab_channel (msg->chan_idx);
	if (channel == CHAT_ALL || channel == CHAT_MODPM) {
#ifdef FR_VERSION
		lines_to_show += rewrap_message(msg, chat_zoom, chat_font, get_console_text_width(), NULL);
#else //FR_VERSION
		lines_to_show += rewrap_message(msg, chat_zoom, get_console_text_width(), NULL);
#endif //FR_VERSION
		if (lines_to_show >= 10) lines_to_show = 10;
		return;
	}

#ifdef FR_VERSION
	if (tabs_1[current_tab].channel == CHAT_ALL && msg->chan_idx != CHAT_COMBAT) {
#else //FR_VERSION
	if (tabs[current_tab].channel == CHAT_ALL) {
#endif //FR_VERSION
#ifdef FR_VERSION
		lines_to_show += rewrap_message(msg, chat_zoom, chat_font, get_console_text_width(), NULL);
#else //FR_VERSION
		lines_to_show += rewrap_message(msg, chat_zoom, get_console_text_width(), NULL);
#endif //FR_VERSION
		if (lines_to_show >= 10) lines_to_show = 10;
	}

#ifndef ENGLISH
	for (itab = 2; itab < nb_tab_button_1; itab++)
	{
		if (tabs_1[itab].channel == channel)
		{
			if (current_tab != itab && !tabs_1[itab].highlighted && tabs_1[current_tab].channel != CHAT_ALL && !get_show_window(console_root_win) && current_bar == 1)
				widget_set_color (tab_bar_win_1, tabs_1[itab].button, 1.0f, 1.0f, 0.0f);
			if (current_tab == itab && current_bar == 1) {
#else //ENGLISH
	for (itab = 2; itab < tabs_in_use; itab++)
	{
		if (tabs[itab].channel == channel)
		{
			if (current_tab != itab && !tabs[itab].highlighted && tabs[current_tab].channel != CHAT_ALL && !get_show_window(console_root_win))
				widget_set_color (tab_bar_win, tabs[itab].button, 1.0f, 1.0f, 0.0f);
			if (current_tab == itab) {
#endif //ENGLISH
#ifdef FR_VERSION
				lines_to_show += rewrap_message(msg, chat_zoom, chat_font, get_console_text_width(), NULL);
#else //FR_VERSION
				lines_to_show += rewrap_message(msg, chat_zoom, get_console_text_width(), NULL);
#endif //FR_VERSION
				if (lines_to_show >= 10) lines_to_show = 10;
			}
			return;
		}
	}
#ifndef ENGLISH
	for (itab = 0; itab < tabs_in_use-nb_tab_button_1; itab++)
	{
		if (tabs_2[itab].channel == channel)
		{
			if (current_tab != itab && !tabs_2[itab].highlighted && tabs_2[current_tab].channel != CHAT_ALL && !get_show_window(console_root_win) && current_bar == 2)
				widget_set_color (tab_bar_win_2, tabs_2[itab].button, 1.0f, 1.0f, 0.0f);
			if (current_tab == itab && current_bar == 2) {
#ifdef FR_VERSION
				lines_to_show += rewrap_message(msg, chat_zoom, chat_font, console_text_width, NULL);
#else //FR_VERSION
				lines_to_show += rewrap_message(msg, chat_zoom, console_text_width, NULL);
#endif //FR_VERSION
				if (lines_to_show >= 10) lines_to_show = 10;
			}
			return;
		}
    }
#endif //ENGLISH

	// we need a new button
	new_button = add_tab_button (channel);
#ifndef ENGLISH
	if(tabs_1[current_tab].channel != CHAT_ALL && current_bar == 1) {
		widget_set_color (tab_bar_win_1, tabs_1[new_button].button, 1.0f, 1.0f, 0.0f);
	}
#else //ENGLISH
	if(tabs[current_tab].channel != CHAT_ALL) {
		widget_set_color (tab_bar_win, tabs[new_button].button, 1.0f, 1.0f, 0.0f);
	}
#endif //ENGLISH
}

void create_tab_bar(void)
{
#ifndef ENGLISH
	window_info *console_win = &windows_list.window[console_root_win];
    widget_list *console_out_w = widget_find(console_root_win, console_out_id);

    int tab_bar_x = 10;
    if (nb_ligne_tabs == 0)
    {
    	int tab_bar_y = 3;
    	tab_bar_win_1 = create_window ("Tab bar", -1, 0, tab_bar_x, tab_bar_y, tab_bar_width_1 < ELW_BOX_SIZE ? ELW_BOX_SIZE : tab_bar_width_1, tab_bar_height, ELW_USE_BACKGROUND|ELW_SHOW);
        nb_ligne_tabs = 1;

        add_tab_button (CHAT_LIST);
    	add_tab_button (CHAT_HIST);
    	add_tab_button (CHAT_ALL);
    	//add_tab_button (CHAT_NONE);
	    current_tab = 2;
	    current_bar = 1;
    	widget_set_color (tab_bar_win_1, tabs_1[current_tab].button, 0.57f, 1.0f, 0.59f);
    	current_filter = tabs_1[current_tab].channel;
    }
    else if (nb_ligne_tabs == 1)
    {
    	int tab_bar_y = 22;
    	tab_bar_win_2 = create_window ("Tab bar 2", -1, 0, tab_bar_x+widget_get_width (tab_bar_win_1, tabs_1[0].button)+widget_get_width (tab_bar_win_1, tabs_1[1].button)+widget_get_width (tab_bar_win_1, tabs_1[2].button)+2, tab_bar_y, tab_bar_width_2 < ELW_BOX_SIZE ? ELW_BOX_SIZE : tab_bar_width_2, tab_bar_height, ELW_USE_BACKGROUND|ELW_SHOW);
        nb_ligne_tabs = 2;

    	widget_set_color (tab_bar_win_2, tabs_2[current_tab].button, 0.57f, 1.0f, 0.59f);
    }
	// On décalle et on change la taille de la console_out
	widget_move(console_root_win, console_out_id, 10, 10 + nb_ligne_tabs*tab_bar_height);
	widget_resize(console_root_win, console_out_id, console_out_w->len_x, console_win->len_y - input_widget->len_y - CONSOLE_SEP_HEIGHT - HUD_MARGIN_Y - 10 - nb_ligne_tabs*tab_bar_height);
	nr_console_lines = console_out_w->len_y / (int)(DEFAULT_FONT_Y_LEN * chat_zoom);
	display_console_handler((widget_find(console_root_win, console_out_id))->widget_info);
#else //ENGLISH
	int tab_bar_x = 10;
	int tab_bar_y = 3;

	tab_bar_win = create_window ("Tab bar", -1, 0, tab_bar_x, tab_bar_y, tab_bar_width < ELW_BOX_SIZE ? ELW_BOX_SIZE : tab_bar_width, tab_bar_height, ELW_USE_BACKGROUND|ELW_SHOW);

	add_tab_button (CHAT_LIST);
	add_tab_button (CHAT_HIST);
	add_tab_button (CHAT_ALL);
	//add_tab_button (CHAT_NONE);
	current_tab = 2;
	widget_set_color (tab_bar_win, tabs[current_tab].button, 0.57f, 1.0f, 0.59f);
	current_filter = tabs[current_tab].channel;
#endif //ENGLISH
}

void display_tab_bar(void)
{
#ifndef ENGLISH
	if (tab_bar_win_1 < 0)
	{
		create_tab_bar ();
	}
	else
	{
		show_window (tab_bar_win_1);
		select_window (tab_bar_win_1);
	}
    if (tab_bar_win_2 >= 0)
    {
		show_window (tab_bar_win_2);
		select_window (tab_bar_win_2);
    }
#else //ENGLISH
	if (tab_bar_win < 0)
	{
		create_tab_bar ();
	}
	else
	{
		show_window (tab_bar_win);
		select_window (tab_bar_win);
	}
#endif //ENGLISH
}

void change_to_current_tab(const char *input)
{
	Uint8 channel;
	int itab;
	int input_len = strlen(input);

	if(input[0] == '@' || input[0] == char_at_str[0])
	{
		channel = CHAT_CHANNEL1 + current_channel;
	}
	else if(my_strncompare(input, "#gm ", 4) || (my_strncompare(input, gm_cmd_str,strlen(gm_cmd_str)) && input_len > strlen(gm_cmd_str)+1 && input[strlen(gm_cmd_str)] == ' '))
	{
		channel = CHAT_GM;
	}
	else if(my_strncompare(input, "#mod ", 5) || (my_strncompare(input, mod_cmd_str, strlen(mod_cmd_str)) && input_len > strlen(mod_cmd_str)+1 && input[strlen(mod_cmd_str)] == ' '))
	{
		channel = CHAT_MOD;
	}
#ifndef ENGLISH
	else if(my_strncompare(input, "#dev ", 7) || (my_strncompare(input, dev_cmd_str, strlen(dev_cmd_str)) && input_len > strlen(dev_cmd_str)+1 && input[strlen(dev_cmd_str)] == ' '))
	{
		channel = CHAT_DEV;
	}
	else if(my_strncompare(input, "#coord ", 7) || (my_strncompare(input, coord_cmd_str, strlen(coord_cmd_str)) && input_len > strlen(coord_cmd_str)+1 && input[strlen(coord_cmd_str)] == ' '))
	{
		channel = CHAT_COORD;
	}
#endif //ENGLISH
	else if(my_strncompare(input, "#bc ", 4) || (my_strncompare(input, bc_cmd_str, strlen(bc_cmd_str)) && input_len > strlen(bc_cmd_str)+1 && input[strlen(bc_cmd_str)] == ' '))
	{
		channel = CHAT_SERVER;
	}
	else if(input[0] == '/' || input[0]==char_slash_str[0])
	{
		channel = CHAT_PERSONAL;
	}
	else if(input[0] == '#' || input[0] == char_cmd_str[0])
	{
		//We don't want to switch tab on commands.
		channel = CHAT_ALL;
	}
	else
	{
		channel = CHAT_LOCAL;
	}
	switch (channel)
	{
		case CHAT_LOCAL:
			if (!local_chat_separate)
			{
				channel = CHAT_ALL;
			}
		break;
		case CHAT_PERSONAL:
			if (!personal_chat_separate)
			{
				channel = CHAT_ALL;
			}
		break;
		case CHAT_GM:
			if (!guild_chat_separate)
			{
				channel = CHAT_ALL;
			}
		break;
		case CHAT_SERVER:
			if (!server_chat_separate)
			{
				channel = CHAT_ALL;
			}
		break;
		case CHAT_MOD:
			if (!mod_chat_separate)
			{
				channel = CHAT_ALL;
			}
		break;
#ifndef ENGLISH
		case CHAT_DEV:
			if (!dev_chat_separate)
			{
				channel = CHAT_ALL;
			}
		break;
		case CHAT_COORD:
			if (!coord_chat_separate)
			{
				channel = CHAT_ALL;
			}
		break;
#endif //ENGLISH
#ifdef FR_VERSION
		case CHAT_COMBAT:
		break;
#endif //FR_VERSION
	}
	if(channel != CHAT_ALL)
	{
#ifndef ENGLISH
		for(itab = 0; itab < nb_tab_button_1; itab++)
#else //ENGLISH
		for(itab = 0; itab < tabs_in_use; itab++)
#endif //ENGLISH
		{
#ifndef ENGLISH
			if(tabs_1[itab].channel == channel)
#else //ENGLISH
			if(tabs[itab].channel == channel)
#endif //ENGLISH
			{
				if(itab != current_tab) //We don't want to switch to the tab we're already in
				{
#ifndef ENGLISH
					switch_to_tab(itab,1);
#else //ENGLISH
					switch_to_tab(itab);
#endif //ENGLISH
				}
				return;
			}
		}
#ifndef ENGLISH
		for(itab = 0; itab < tabs_in_use-nb_tab_button_1; itab++)
		{
			if(tabs_2[itab].channel == channel)
			{
				if(itab != current_tab) //We don't want to switch to the tab we're already in
				{
					switch_to_tab(itab,2);
                }
                return;
            }
		}
#endif //ENGLISH
		//We didn't find any tab to switch to, create new
		itab = add_tab_button (channel);
		if (itab >= 0)
#ifndef ENGLISH
			switch_to_tab(itab,1);
#else //ENGLISH
			switch_to_tab (itab);
#endif //ENGLISH
	}
}

void convert_tabs (int new_wc)
{
	int iwc, ibc;
	Uint8 chan;

	if (new_wc == 1)
	{
		// first close possible remaining tab buttons that are no
		// longer active
		for (ibc = 2; ibc < tabs_in_use; ibc++)
		{
#ifndef ENGLISH
			chan = tabs_1[ibc].channel;
#else //ENGLISH
			chan = tabs[ibc].channel;
#endif //ENGLISH
			for (iwc = 0; iwc < MAX_CHAT_TABS; iwc++)
			{
				if (channels[iwc].chan_nr == chan && channels[iwc].open)
					break;
			}

			if (iwc >= MAX_CHAT_TABS)
#ifndef ENGLISH
                remove_tab_button (chan,1);
#else //ENGLISH
				remove_tab_button (chan);
#endif //ENGLISH
		}

		// now add buttons for every tab that doesn't have a button yet
		for (iwc = 2; iwc < MAX_CHAT_TABS; iwc++)
		{
			if (channels[iwc].open)
			{
				chan = channels[iwc].chan_nr;
				for (ibc = 0; ibc < tabs_in_use; ibc++)
				{
#ifndef ENGLISH
					if (tabs_1[ibc].channel == chan)
#else //ENGLISH
					if (tabs[ibc].channel == chan)
#endif //ENGLISH
						break;
				}

				if (ibc >= tabs_in_use)
				{
					add_tab_button (chan);
				}
			}
		}
	}
	else if (new_wc == 2)
	{
		// first close possible remaining tabs that are no
		// longer active
		for (iwc = 2; iwc < MAX_CHAT_TABS; iwc++)
		{
			if (channels[iwc].open)
			{
				chan = channels[iwc].chan_nr;
				for (ibc = 0; ibc < tabs_in_use; ibc++)
				{
#ifndef ENGLISH
					if (tabs_1[ibc].channel == chan)
#else //ENGLISH
					if (tabs[ibc].channel == chan)
#endif //ENGLISH
						break;
				}

				if (ibc >= tabs_in_use)
				{
					remove_chat_tab (chan);
				}
			}
		}

		// now add tabs for every button that doesn't have a tab yet
		for (ibc = 2; ibc < tabs_in_use; ibc++)
		{
#ifndef ENGLISH
			chan = tabs_1[ibc].channel;
#else //ENGLISH
			chan = tabs[ibc].channel;
#endif //ENGLISH
			for (iwc = 0; iwc < MAX_CHAT_TABS; iwc++)
			{
				if (channels[iwc].chan_nr == chan && channels[iwc].open)
					break;
			}

			if (iwc >= MAX_CHAT_TABS)
				// unfortunately we have no clue about the
				// number of lines written in this channel, so
				// we won't see anything until new messages
				// arrive. Oh well.
				add_chat_tab (0, chan);
		}
	}
}

int command_jlc(char * text, int len)
{
	unsigned int num;
	char number[12];

	num = chan_int_from_name(text, NULL);
	if(num <= 0) {
		return 0;//Don't know this name
	}
	safe_snprintf(number, sizeof(number), " %d", num);
	if(strlen(number) <= strlen(text)) { //it is entirely possible that the number
		safe_strncpy(text, number, strlen(text));	//could be longer than the name, and hence we may
	}							//not have enough storage space to replace the name
	return 0; //note: this change could also put us over the 160-char limit if not checked
}

////////////////////////////////////////////////////////////////////////
//  channel color stuff

channelcolor channel_colors[MAX_CHANNEL_COLORS];
char channel_number_buffer[10] = {0};
Uint32 channel_to_change = 0;
int selected_color = -1;
static int channel_colors_set = 0;

int channel_color_win = -1;
int color_button1_id = 0;
int color_button2_id = 7;
int color_button3_id = 14;
int color_button4_id = 21;
int color_button5_id = 1;
int color_button6_id = 8;
int color_button7_id = 15;
int color_button8_id = 22;
int color_button9_id = 2;
int color_button10_id = 9;
int color_button11_id = 16;
int color_button12_id = 23;
int color_button13_id = 3;
int color_button14_id = 10;
int color_button15_id = 17;
int color_button16_id = 24;
int color_button17_id = 4;
int color_button18_id = 11;
int color_button19_id = 18;
int color_button20_id = 25;
int color_button21_id = 5;
int color_button22_id = 12;
int color_button23_id = 19;
int color_button24_id = 26;
int color_button25_id = 6;
int color_button26_id = 13;
int color_button27_id = 20;
int color_button28_id = 27;
int color_set_button_id = 31;
int color_delete_button_id = 32;
int color_label_id = 41;

int display_channel_color_handler(window_info *win)
{
	char string[50] = {0};

	safe_snprintf(string, sizeof(string), "%s %i", channel_color_str, channel_to_change);
	label_set_text(channel_color_win, color_label_id, string);
	return 1;
}

int click_channel_color_handler(widget_list *w, int mx, int my, Uint32 flags)
{
	if(w->id >= color_button1_id && w->id <= color_button28_id)
	{
		selected_color = w->id;
		do_click_sound();
		return 1;
	}
	if(w->id == color_set_button_id)
	{
		if(channel_to_change > 0 && selected_color >= 0)
		{
			int i;
			for(i=0; i<MAX_CHANNEL_COLORS; i++)
			{
				if(channel_colors[i].nr == channel_to_change)
					break;
			}
			if(i<MAX_CHANNEL_COLORS)
			{
				channel_colors[i].color = selected_color;
				do_click_sound();
				hide_window(channel_color_win);
				channel_to_change = 0;
				selected_color = -1;
				channel_colors_set = 1;
				return 1;
			}
			for(i=0; i<MAX_CHANNEL_COLORS; i++)
			{
				if(channel_colors[i].nr == 0)
					break;
			}

			if(i<MAX_CHANNEL_COLORS)
			{
				channel_colors[i].nr = channel_to_change;
				channel_colors[i].color = selected_color;
				do_click_sound();
				hide_window(channel_color_win);
				channel_to_change = 0;
				selected_color = -1;
				channel_colors_set = 1;
				return 1;
			} else {
				LOG_TO_CONSOLE(c_red3, "You reached the maximum numbers of channel colors.");
				return 1;
			}
		}
		return 0;
	}
	if(w->id == color_delete_button_id)
	{
		int i;
		for(i=0; i<MAX_CHANNEL_COLORS; i++)
		{
			if(channel_colors[i].nr == channel_to_change)
				break;
		}
		if(i<MAX_CHANNEL_COLORS)
		{
			channel_colors[i].color = -1;
			channel_colors[i].nr = 0;
			channel_colors_set = 1;
		}
		do_click_sound();
		hide_window(channel_color_win);
		channel_to_change = 0;
		selected_color = -1;
		return 1;
	}
	return 0;
}

int display_channel_color_win(Uint32 channel_number)
{
	int our_root_win = -1;
	int x, y = 6;
	int window_width = 470;

	if(channel_number == 0){
		return -1;
	}

	channel_to_change = channel_number;

	if(channel_color_win < 0)
	{
		if (!windows_on_top) {
			our_root_win = game_root_win;
		}
		/* Create the window */
		channel_color_win = create_window(channel_color_title_str, our_root_win, 0, 300, 40, window_width, 350, ELW_WIN_DEFAULT);
		set_window_handler(channel_color_win, ELW_HANDLER_DISPLAY, &display_channel_color_handler);
		/* Add labels */
		x = 10;
		color_label_id = label_add_extended(channel_color_win, color_label_id, NULL, x, y, 0, 0.9f, 0.77f, 0.57f, 0.39f, channel_color_str);
		/* Add color buttons */
		y += 24;
		color_button1_id = button_add_extended(channel_color_win, color_button1_id, NULL, x, y, 107, 30, 0, 1.0, 1.0, 0.7, 0.76, "red1");
		widget_set_OnClick(channel_color_win, color_button1_id, click_channel_color_handler);
		x += 115;
		color_button2_id = button_add_extended(channel_color_win, color_button2_id, NULL, x, y, 107, 30, 0, 1.0, 0.98, 0.35, 0.35, "red2");
		widget_set_OnClick(channel_color_win, color_button2_id, click_channel_color_handler);
		x += 115;
		color_button3_id = button_add_extended(channel_color_win, color_button3_id, NULL, x, y, 107, 30, 0, 1.0, 0.87, 0.0, 0.0, "red3");
		widget_set_OnClick(channel_color_win, color_button3_id, click_channel_color_handler);
		x += 115;
		color_button4_id = button_add_extended(channel_color_win, color_button4_id, NULL, x, y, 107, 30, 0, 1.0, 0.49, 0.01, 0.01, "red4");
		widget_set_OnClick(channel_color_win, color_button4_id, click_channel_color_handler);
		x = 10;
		y += 39;
		color_button5_id = button_add_extended(channel_color_win, color_button5_id, NULL, x, y, 107, 30, 0, 1.0, 0.97, 0.77, 0.62, "orange1");
		widget_set_OnClick(channel_color_win, color_button5_id, click_channel_color_handler);
		x += 115;
		color_button6_id = button_add_extended(channel_color_win, color_button6_id, NULL, x, y, 107, 30, 0, 1.0, 0.99, 0.48, 0.23, "orange2");
		widget_set_OnClick(channel_color_win, color_button6_id, click_channel_color_handler);
		x += 115;
		color_button7_id = button_add_extended(channel_color_win, color_button7_id, NULL, x, y, 107, 30, 0, 1.0, 0.75, 0.4, 0.06, "orange3");
		widget_set_OnClick(channel_color_win, color_button7_id, click_channel_color_handler);
		x += 115;
		color_button8_id = button_add_extended(channel_color_win, color_button8_id, NULL, x, y, 107, 30, 0, 1.0, 0.51, 0.19, 0.01, "orange4");
		widget_set_OnClick(channel_color_win, color_button8_id, click_channel_color_handler);
		x = 10;
		y += 39;
		color_button9_id = button_add_extended(channel_color_win, color_button9_id, NULL, x, y, 107, 30, 0, 1.0, 0.98, 0.98, 0.75, "yellow1");
		widget_set_OnClick(channel_color_win, color_button9_id, click_channel_color_handler);
		x += 115;
		color_button10_id = button_add_extended(channel_color_win, color_button10_id, NULL, x, y, 107, 30, 0, 1.0, 0.99, 0.93, 0.22, "yellow2");
		widget_set_OnClick(channel_color_win, color_button10_id, click_channel_color_handler);
		x += 115;
		color_button11_id = button_add_extended(channel_color_win, color_button11_id, NULL, x, y, 107, 30, 0, 1.0, 0.91, 0.68, 0.08, "yellow3");
		widget_set_OnClick(channel_color_win, color_button11_id, click_channel_color_handler);
		x += 115;
		color_button12_id = button_add_extended(channel_color_win, color_button12_id, NULL, x, y, 107, 30, 0, 1.0, 0.51, 0.44, 0.02, "yellow4");
		widget_set_OnClick(channel_color_win, color_button12_id, click_channel_color_handler);
		x = 10;
		y += 39;
		color_button13_id = button_add_extended(channel_color_win, color_button13_id, NULL, x, y, 107, 30, 0, 1.0, 0.79, 1.0, 0.8, "green1");
		widget_set_OnClick(channel_color_win, color_button13_id, click_channel_color_handler);
		x += 115;
		color_button14_id = button_add_extended(channel_color_win, color_button14_id, NULL, x, y, 107, 30, 0, 1.0, 0.02, 0.98, 0.61, "green2");
		widget_set_OnClick(channel_color_win, color_button14_id, click_channel_color_handler);
		x += 115;
		color_button15_id = button_add_extended(channel_color_win, color_button15_id, NULL, x, y, 107, 30, 0, 1.0, 0.15, 0.77, 0.0, "green3");
		widget_set_OnClick(channel_color_win, color_button15_id, click_channel_color_handler);
		x += 115;
		color_button16_id = button_add_extended(channel_color_win, color_button16_id, NULL, x, y, 107, 30, 0, 1.0, 0.08, 0.58, 0.02, "green4");
		widget_set_OnClick(channel_color_win, color_button16_id, click_channel_color_handler);
		x = 10;
		y += 39;
		color_button17_id = button_add_extended(channel_color_win, color_button17_id, NULL, x, y, 107, 30, 0, 1.0, 0.66, 0.94, 0.98, "blue1");
		widget_set_OnClick(channel_color_win, color_button17_id, click_channel_color_handler);
		x += 115;
		color_button18_id = button_add_extended(channel_color_win, color_button18_id, NULL, x, y, 107, 30, 0, 1.0, 0.46, 0.59, 0.97, "blue2");
		widget_set_OnClick(channel_color_win, color_button18_id, click_channel_color_handler);
		x += 115;
		color_button19_id = button_add_extended(channel_color_win, color_button19_id, NULL, x, y, 107, 30, 0, 1.0, 0.27, 0.28, 0.82, "blue3");
		widget_set_OnClick(channel_color_win, color_button19_id, click_channel_color_handler);
		x += 115;
		color_button20_id = button_add_extended(channel_color_win, color_button20_id, NULL, x, y, 107, 30, 0, 1.0, 0.06, 0.06, 0.73, "blue4");
		widget_set_OnClick(channel_color_win, color_button20_id, click_channel_color_handler);
		x = 10;
		y += 39;
		color_button21_id = button_add_extended(channel_color_win, color_button21_id, NULL, x, y, 107, 30, 0, 1.0, 0.82, 0.71, 0.98, "purple1");
		widget_set_OnClick(channel_color_win, color_button21_id, click_channel_color_handler);
		x += 115;
		color_button22_id = button_add_extended(channel_color_win, color_button22_id, NULL, x, y, 107, 30, 0, 1.0, 0.85, 0.36, 0.96, "purple2");
		widget_set_OnClick(channel_color_win, color_button22_id, click_channel_color_handler);
		x += 115;
		color_button23_id = button_add_extended(channel_color_win, color_button23_id, NULL, x, y, 107, 30, 0, 1.0, 0.51, 0.33, 0.96, "purple3");
		widget_set_OnClick(channel_color_win, color_button23_id, click_channel_color_handler);
		x += 115;
		color_button24_id = button_add_extended(channel_color_win, color_button24_id, NULL, x, y, 107, 30, 0, 1.0, 0.42, 0.0, 0.67, "purple4");
		widget_set_OnClick(channel_color_win, color_button24_id, click_channel_color_handler);
		x = 10;
		y += 39;
		color_button25_id = button_add_extended(channel_color_win, color_button25_id, NULL, x, y, 107, 30, 0, 1.0, 1.0, 1.0, 1.0, "grey1");
		widget_set_OnClick(channel_color_win, color_button25_id, click_channel_color_handler);
		x += 115;
		color_button26_id = button_add_extended(channel_color_win, color_button26_id, NULL, x, y, 107, 30, 0, 1.0, 0.6, 0.6, 0.6, "grey2");
		widget_set_OnClick(channel_color_win, color_button26_id, click_channel_color_handler);
		x += 115;
		color_button27_id = button_add_extended(channel_color_win, color_button27_id, NULL, x, y, 107, 30, 0, 1.0, 0.62, 0.62, 0.62, "grey3");
		widget_set_OnClick(channel_color_win, color_button27_id, click_channel_color_handler);
		x += 115;
		color_button28_id = button_add_extended(channel_color_win, color_button28_id, NULL, x, y, 107, 30, 0, 1.0, 0.16, 0.16, 0.16, "grey4");
		widget_set_OnClick(channel_color_win, color_button28_id, click_channel_color_handler);
		/* Add set/delete buttons */
		x = (window_width - 100 - 110) /3;
		y += 44;
		color_set_button_id = button_add_extended(channel_color_win, color_set_button_id, NULL, x, y, 110, 30, 0, 1.0, 0.77f, 0.57f, 0.39f, channel_color_add_str);
		widget_set_OnClick(channel_color_win, color_set_button_id, click_channel_color_handler);
		x += x + 110;
		color_delete_button_id = button_add_extended(channel_color_win, color_delete_button_id, NULL, x, y, 110, 30, 0, 1.0, 0.77f, 0.57f, 0.39f, channel_color_delete_str);
		widget_set_OnClick(channel_color_win, color_delete_button_id, click_channel_color_handler);
	}
	else
	{
		toggle_window(channel_color_win);
	}

	return channel_color_win;
}

void load_channel_colors (){
	char fname[128];
	FILE *fp;
	int i;
	off_t file_size;

	if (channel_colors_set) {
		/*
		 * save existing channel colors instead of loading them if we are already logged in
		 * this will take place when relogging after disconnection
		 */
		save_channel_colors();
		return;
	}

	for(i=0; i<MAX_CHANNEL_COLORS; i++)
	{
		channel_colors[i].nr = 0;
		channel_colors[i].color = -1;
	}

	safe_snprintf(fname, sizeof(fname), "channel_colors_%s.dat",username_str);
	my_tolower(fname);

	/* sliently ignore non existing file */
	if (file_exists_config(fname)!=1)
		return;

	file_size = get_file_size_config(fname);

	/* if the file exists but is not a valid size, don't use it */
	if ((file_size == 0) || (file_size != sizeof(channel_colors)))
	{
		LOG_ERROR("%s: Invalid format (size mismatch) \"%s\"\n", reg_error_str, fname);
		return;
	}

	fp = open_file_config(fname,"rb");
	if(fp == NULL){
		LOG_ERROR("%s: %s \"%s\": %s\n", reg_error_str, cant_open_file, fname, strerror(errno));
		return;
	}

	if (fread (channel_colors,sizeof(channel_colors),1, fp) != 1)
	{
		LOG_ERROR("%s() fail during read of file [%s] : %s\n", __FUNCTION__, fname, strerror(errno));
		fclose (fp);
		return;
	}

	fclose (fp);
	channel_colors_set = 1;
}

void save_channel_colors(){
	char fname[128];
	FILE *fp;

	if (!channel_colors_set)
		return;

	safe_snprintf(fname, sizeof(fname), "channel_colors_%s.dat",username_str);
	my_tolower(fname);
	fp=open_file_config(fname,"wb");
	if(fp == NULL){
		LOG_ERROR("%s: %s \"%s\": %s\n", reg_error_str, cant_open_file, fname, strerror(errno));
		return;
	}

	if (fwrite (channel_colors,sizeof(channel_colors),1, fp) != 1)
	{
		LOG_ERROR("%s() fail during write of file [%s] : %s\n", __FUNCTION__, fname, strerror(errno));
	}

	fclose(fp);
}

int command_channel_colors(char * text, int len)
{
	int i;
	char string[20];

#ifdef FR_VERSION
	LOG_TO_CONSOLE(c_grey1, "Couleurs actuels des canaux :");
#else //FR_VERSION
	LOG_TO_CONSOLE(c_grey1, "Your currently set channel colors:");
#endif //FR_VERSION
	for(i=0; i<MAX_CHANNEL_COLORS; i++)
	{
		if(channel_colors[i].nr >0 && channel_colors[i].color > -1)
		{
#ifdef FR_VERSION
			safe_snprintf(string, sizeof(string), "Canal %u", channel_colors[i].nr);
#else //FR_VERSION
			safe_snprintf(string, sizeof(string), "Channel %u", channel_colors[i].nr);
#endif //FR_VERSION
			LOG_TO_CONSOLE(channel_colors[i].color, string);
		}
	}
	return 1;
}
