#include <stdlib.h>
#include <math.h>
#include <string.h>
#ifndef _MSC_VER
#include <strings.h>
#endif
#include <time.h>
#include "buddy.h"
#include "asc.h"
#include "chat.h"
#include "console.h"
#include "elwindows.h"
#include "gamewin.h"
#include "icon_window.h"
#include "init.h"
#include "interface.h"
#include "multiplayer.h"
#include "queue.h"
#include "translate.h"
#ifdef OPENGL_TRACE
#include "gl_init.h"
#endif

#define MAX_ACCEPT_BUDDY_WINDOWS MAX_BUDDY

int buddy_win=-1;
int buddy_scroll_id = 0;
int buddy_button_id = 1;
int buddy_menu_x=150;
int buddy_menu_y=70;
int buddy_menu_x_len=150;
int buddy_menu_y_len=220;

int buddy_add_win = -1;
int buddy_name_input_id = 0;
int buddy_add_button_id = -1;
int buddy_add_x_len = 290;
int buddy_add_y_len = 97;

int buddy_change_win = -1;
int buddy_change_x_len = 290;
int buddy_change_y_len = 255;
int buddy_type_input_id = -1;
int buddy_change_button_id = -1;
int buddy_delete = 0; //For the checkbox
char *buddy_to_change = NULL;
#ifndef ENGLISH
//BUDDY-FIXME: once server-side offline buddies are supported, this variable will be unneeded
time_t c_time;//used to prevent buddylist flood when changing colours, etc
#endif //ENGLISH


struct accept_window {
	int window_id; //Window ID
	char name[32]; //Name of the buddy to accept
	char *text; //Buffer for the text to display
	int checkbox; //Checkbox widget id
} accept_windows[MAX_ACCEPT_BUDDY_WINDOWS];
int buddy_accept_x_len = 400;
int buddy_accept_y_len = 130;
queue_t *buddy_request_queue;

unsigned char buddy_name_buffer[16] = {0};
char description_buffer[255] = {0};
_buddy buddy_list[MAX_BUDDY];
#ifdef FR_VERSION
int buddy_nb = 0;     // nombre d'�l�ments pr�sents dans la liste
int buddy_lines = 19; // nombre d'�l�ments affichables dans la fen�tre
#endif //FR_VERSION

int create_buddy_interface_win(const char *title, void *argument);

int buddy_list_name_cmp( const void *arg1, const void *arg2)
{
	const _buddy *b1=arg1, *b2=arg2;
	if(b1->type==b2->type)
		return strcasecmp(b1->name,b2->name);
	else
		return b1->type-b2->type;
}

int display_buddy_handler(window_info *win)
{
	int i=0,x=2,y=2;
	int offset;

	glEnable(GL_TEXTURE_2D);
	// Draw buddies
#ifdef FR_VERSION
	/* mieux vaut effectuer le tri � la source dans le add_buddy() */
#else //FR_VERSION
	qsort(buddy_list,MAX_BUDDY,sizeof(_buddy),buddy_list_name_cmp);
#endif //FR_VERSION

	offset = vscrollbar_get_pos (buddy_win,buddy_scroll_id);
	if (offset >= 0)
	{
#ifdef FR_VERSION
		for (i = offset; i < offset + buddy_lines && i < buddy_nb; i++)
#else //FR_VERSION
		for (i = offset; i < offset + 19; i++)
#endif //FR_VERSION
		{
			switch(buddy_list[i].type){
				case 0:glColor3f(1.0,1.0,1.0);break;
				case 1:glColor3f(1.0,0,0);break;
				case 2:glColor3f(0,1.0,0);break;
				case 3:glColor3f(0.25,0.5,1.0);break;
				case 4:glColor3f(1.0,1.0,0);break;
				case 0xFE:glColor3f(0.5,0.55,0.60);break;
				default:glColor3f(1.0,1.0,1.0);//invalid number? make it white
			}
			draw_string_zoomed(x,y,(unsigned char*)buddy_list[i].name,1,0.7);
			y+=10;
		}
	}
	//Draw a button for the requests
	if(!queue_isempty(buddy_request_queue)) {
		glDisable(GL_TEXTURE_2D);
		//glColor3f(0.77f, 0.59f, 0.39f);
		glColor3f(0.3, 1, 0.3);
		glBegin(GL_LINE_LOOP);
			glVertex2i(win->len_x/3, 0);
			glVertex2i(win->len_x/3+10, 16);
			glVertex2i(win->len_x-20, 16);
			glVertex2i(win->len_x-20, 0);
		glEnd();
		glEnable(GL_TEXTURE_2D);
		draw_string_zoomed(win->len_x/3+10,1,(unsigned char*)buddy_request_str,1,0.7);
	}
	glColor3f(0.77f, 0.57f, 0.39f);
#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
	return 1;
}

int click_buddy_handler (window_info *win, int mx, int my, Uint32 flags)
{
	int x=mx,y=my;
	char str[50];

	// scroll the winow with the mouse wheel
	if(flags & ELW_WHEEL_UP) {
		vscrollbar_scroll_up(buddy_win,buddy_scroll_id);
		return 1;
	} else if(flags & ELW_WHEEL_DOWN) {
		vscrollbar_scroll_down(buddy_win,buddy_scroll_id);
		return 1;
	}

	// only handle mouse button clicks, not scroll wheels moves
	if ( (flags & ELW_MOUSE_BUTTON) == 0)
		return 0;

	if(x>win->len_x-20) {
		//Clicked on the scrollbar. Let it fall through.
		return 0;
	} else if(!queue_isempty(buddy_request_queue) && mx > win->len_x/3 && y < 16) {
		//Clicked on the requests button
		while(!queue_isempty(buddy_request_queue)) {
			char *name = queue_pop(buddy_request_queue);
			select_window(create_buddy_interface_win(buddy_accept_str, name));
			free(name);
		}
		return 1;
	}

	// clicked on a buddy's name
	y /= 10;
	y += vscrollbar_get_pos(buddy_win,buddy_scroll_id);
#ifdef ENGLISH
	if((strlen(buddy_list[y].name) == 0)||(buddy_list[y].type > 0xFE)) {
		//There's no name. Fall through.
#else //ENGLISH
	if((strlen(buddy_list[y].name) == 0)||(buddy_list[y].type >= 0xFE)) {
		//There's no name, or buddy is offline. Fall through.
#endif //ENGLISH
		return 0;
	}
	if(flags&ELW_RIGHT_MOUSE) {
		if(flags&ELW_CTRL) {
			//CTRL + right click, delete buddy.
			safe_snprintf(str, sizeof(str), "%c#del_buddy %s", RAW_TEXT, buddy_list[y].name);
			my_tcp_send(my_socket, (Uint8*)str, strlen(str+1)+1);
		} else {
			//Right click, open edit window
			create_buddy_interface_win(buddy_change_str, &buddy_list[y]);
		}
#ifdef ENGLISH
	} else {
#else //ENGLISH
	} else if (buddy_list[y].type < 0xFE) {
#endif //ENGLISH
		//start a pm to them
		// clear the buffer
		clear_input_line();

		// insert the person's name
		safe_snprintf (str, sizeof(str),"/%s ", buddy_list[y].name);
		//put_string_in_buffer (&input_text_line, str, 0);
		//We'll just reuse the paste function here
		paste_in_input_field((unsigned char*)str);
	}
	return 1;
}

void init_buddy()
{
	int i;

	for (i = 0; i < MAX_BUDDY; i++)
	{
		buddy_list[i].type = 0xff;
		memset (buddy_list[i].name, 0, sizeof (buddy_list[i].name));
	}
#ifdef FR_VERSION
	buddy_nb = 0;
	vscrollbar_set_bar_len(buddy_win, buddy_scroll_id, buddy_nb);
#endif //FR_VERSION
	for(i = 0; i < MAX_ACCEPT_BUDDY_WINDOWS; i++)
	{
		accept_windows[i].window_id = -1;
		memset(accept_windows[i].name, 0, sizeof(accept_windows[i].name));
	}
	queue_initialise(&buddy_request_queue);
#ifndef ENGLISH
	//BUDDY-FIXME: once server-side offline buddies are supported, the next 2 lines can go
	time(&c_time);//note the current time
	c_time += 10;
#endif //ENGLISH
}

/*
int clika(widget_list *w){
	w->pos_x+=10;
	return 0;
}
int clikaa(widget_list *w){
	progressbar *b = (progressbar *)w->widget_info;
	b->progress++;
	return 0;
}
*/

int click_add_buddy_handler(widget_list *w, int mx, int my, Uint32 flags)
{
	if(strlen((char*)buddy_name_buffer) == 0) {
		return 1;
	} else {
		char string[255];

		string[0] = RAW_TEXT;
		safe_snprintf(string+1, sizeof(string)-1, "#add_buddy %s", buddy_name_buffer);
		my_tcp_send(my_socket, (Uint8*)string, strlen(string+1)+1);
		hide_window(buddy_add_win);
		buddy_name_buffer[0] = '\0';
		description_buffer[0] = '\0';
		return 1;
	}
}

int click_change_buddy_handler(widget_list *w, int mx, int my, Uint32 flags)
{
	char string[255];
#ifdef ENGLISH
	int send_message = 1;
#endif //ENGLISH

	if(buddy_delete) {
		safe_snprintf(string, sizeof(string), "%c#del_buddy %s", RAW_TEXT, buddy_to_change);
		buddy_delete = 0;
#ifdef ENGLISH
	} else if (buddy_type_input_id != -1) {
		safe_snprintf(string, sizeof(string), "%c#change_buddy %s %i", RAW_TEXT, buddy_to_change, multiselect_get_selected(buddy_change_win, buddy_type_input_id));
#endif //ENGLISH
	} else {
#ifdef ENGLISH
		send_message = 0;
#else //ENGLISH
		safe_snprintf(string, sizeof(string), "%c#change_buddy %s %i", RAW_TEXT, buddy_to_change, multiselect_get_selected(buddy_change_win, buddy_type_input_id));
#endif //ENGLISH
	}
#ifdef ENGLISH
	if (send_message) {
		my_tcp_send(my_socket, (Uint8*)string, strlen(string+1)+1);
	}
#else //ENGLIDH
	my_tcp_send(my_socket, (Uint8*)string, strlen(string+1)+1);
	//BUDDY-FIXME: once server-side offline buddies are supported, the next line can go
	time(&c_time);
#endif //ENGLISH
	destroy_window(buddy_change_win);
	buddy_change_win = -1;
	buddy_to_change = NULL;
	return 1;
}

int display_add_buddy_handler(window_info *win)
{
	/* Draw description_buffer and the separator */
	glColor3f(0.77f, 0.57f, 0.39f);
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINES);
		glVertex3i(0, win->len_y-40,0);
		glVertex3i(win->len_x, win->len_y-40,0);
	glEnd();
	glEnable(GL_TEXTURE_2D);
	draw_string_small(5, win->len_y-5-30, (unsigned char*)description_buffer, 2);
#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
	return 1;
}
int name_onmouseover_handler(widget_list *widget, int mx, int my)
{
	put_small_colored_text_in_box(c_orange1, (unsigned char*)buddy_long_name_str, strlen(buddy_long_name_str),
                               buddy_add_x_len-10, (char*)description_buffer);
	return 1;
}
int type_onmouseover_handler(widget_list *widget, int mx, int my)
{
	put_small_colored_text_in_box(c_orange1, (unsigned char*)buddy_long_type_str, strlen(buddy_long_type_str),
                               buddy_add_x_len-10, (char*)description_buffer);
	return 1;
}
int delete_onmouseover_handler(widget_list *widget, int mx, int my)
{
	put_small_colored_text_in_box(c_orange1, (unsigned char*)buddy_long_delete_str, strlen(buddy_long_delete_str),
							buddy_change_x_len-10, (char*)description_buffer);
	return 1;
}

int display_accept_buddy_handler(window_info *win)
{
	if(win != NULL) {
		int i;

		glColor3f(0.77f, 0.57f, 0.39f);
		for(i = 0; i < MAX_ACCEPT_BUDDY_WINDOWS; i++) {
			if(accept_windows[i].window_id == win->window_id) {
				draw_string_small(5, 5, (unsigned char*)accept_windows[i].text, 2);
				break;
			}
		}
		return 1;
	} else {
		return 0;
	}
}

int name_input_keypress_handler(widget_list *widget, int mx, int my, Uint32 key, Uint32 unikey)
{
	if(unikey == '\r' && strlen((char*)buddy_name_buffer) > 0) {
		return click_add_buddy_handler(widget, mx, my, ELW_LEFT_MOUSE);
	} else {
		return 0;
	}
}

int click_accept_yes(widget_list *w, int mx, int my, Uint32 flags)
{
	char string[255];
	int window_id = w->window_id;
	int i;

	//TODO: get rid of this when the widget handlers can take custom arguments
	/* Find the position of the window clicked in the accept_windows array */
	for(i = 0; i < MAX_ACCEPT_BUDDY_WINDOWS; i++) {
		if(accept_windows[i].window_id == window_id) {
			break;
		}
	}
	if(i == MAX_ACCEPT_BUDDY_WINDOWS) {
		/* We didn't find it */
		return 0;
	}
	safe_snprintf(string, sizeof(string), "%c#accept_buddy %s", RAW_TEXT, accept_windows[i].name);

	my_tcp_send(my_socket, (Uint8*)string, strlen(string+1)+1);
	if(accept_windows[i].checkbox >= 0 && checkbox_get_checked(accept_windows[i].window_id, accept_windows[i].checkbox) > 0) {
		safe_snprintf(string, sizeof(string), "%c#add_buddy %s", RAW_TEXT, accept_windows[i].name);

		my_tcp_send(my_socket, (Uint8*)string, strlen(string+1)+1);
	}
	destroy_window(accept_windows[i].window_id);
	accept_windows[i].window_id = -1;
	accept_windows[i].checkbox = -1;
	free(accept_windows[i].text);
	accept_windows[i].text = NULL;
	accept_windows[i].name[0] = '\0';

	return 1;
}

int click_accept_no(widget_list *w, int mx, int my, Uint32 flags)
{
	int i;
	int window_id = w->window_id;

	//TODO: get rid of this too
	/* Find the position of the window clicked in the accept_windows array */
	for(i = 0; i < MAX_ACCEPT_BUDDY_WINDOWS; i++) {
		if(accept_windows[i].window_id == window_id) {
			break;
		}
	}
	if(i == MAX_ACCEPT_BUDDY_WINDOWS) {
		/* We didn't find it */
		return 0;
	}
	destroy_window(accept_windows[i].window_id);
	accept_windows[i].window_id = -1;
	accept_windows[i].checkbox = -1;
	free(accept_windows[i].text);
	accept_windows[i].text = NULL;
	accept_windows[i].name[0] = '\0';

	return 1;
}

int click_delete_checkbox_label(widget_list *w, int mx, int my, Uint32 flags)
{
	buddy_delete = !buddy_delete;
	return 1;
}

int create_buddy_interface_win(const char *title, void *argument)
{
	int label_id = 10; //temporary variable
	int string_width;
	int extra_space;
	int x = 5;
	int y = 5;

	if(strcmp(title, buddy_add_str) == 0) {
		if(buddy_add_win < 0) {
			/* Create the window */
			buddy_add_win = create_window(title, buddy_win, 0, buddy_menu_x_len/2, buddy_menu_y_len/4, buddy_add_x_len, buddy_add_y_len+10, ELW_WIN_DEFAULT);
			set_window_handler(buddy_add_win, ELW_HANDLER_DISPLAY, &display_add_buddy_handler);
			/* Add name input and label */
			label_id = label_add_extended(buddy_add_win, label_id, NULL, x, y, 0, 0.9f, 0.77f, 0.57f, 0.39f, buddy_name_str);
			string_width = get_string_width((unsigned char*)buddy_name_str)*0.9f;
			x = string_width+5;
			y = 5;
			buddy_name_input_id = pword_field_add_extended(buddy_add_win, buddy_name_input_id, NULL, x, y, buddy_add_x_len-x*2+10, 20, P_TEXT, 0.9f, 0.77f, 0.57f, 0.39f, buddy_name_buffer, MAX_USERNAME_LENGTH+1);
			widget_set_OnMouseover(buddy_add_win, buddy_name_input_id, name_onmouseover_handler);
			widget_set_OnMouseover(buddy_add_win, label_id, name_onmouseover_handler);
			widget_set_OnKey(buddy_add_win, buddy_name_input_id, name_input_keypress_handler);
			/* Add "Add buddy" button */
			y += 5+20;
			extra_space = 2+((buddy_add_x_len-string_width*2) - get_string_width((unsigned char*)buddy_add_str))/2-15;
			buddy_add_button_id = button_add(buddy_add_win, NULL, buddy_add_str, x+extra_space, y);
			widget_set_OnClick(buddy_add_win, buddy_add_button_id, click_add_buddy_handler);
			return buddy_add_win;
		} else {
			toggle_window(buddy_add_win);
			return buddy_add_win;
		}
	} else if (strcmp(title, buddy_change_str) == 0) {
		_buddy *buddy = argument;
		buddy_to_change = buddy->name;
		if(buddy_change_win >= 0) {
			/* Destroy the window to make sure everything's set up. */
			destroy_window(buddy_change_win);
			buddy_change_win = -1;
			buddy_delete = 0;
		}
		/* Create the window */
#ifdef ENGLISH
		buddy_change_win = create_window(title, buddy_win, 0, buddy_menu_x_len/2, buddy_menu_y_len/4, buddy_change_x_len, buddy_change_y_len - ((buddy->type==0xFE)?127:0), ELW_WIN_DEFAULT);
#else //ENGLISH
		buddy_change_win = create_window(title, buddy_win, 0, buddy_menu_x_len/2, buddy_menu_y_len/4, buddy_change_x_len, buddy_change_y_len, ELW_WIN_DEFAULT);
#endif //ENGLISH
		set_window_handler(buddy_change_win, ELW_HANDLER_DISPLAY, &display_add_buddy_handler);
		/* Add name label and name */
		label_id = label_add_extended(buddy_change_win, label_id, NULL, x, y, 0, 0.9f, 0.77f, 0.57f, 0.39f, buddy_name_str);
		widget_set_OnMouseover(buddy_change_win, label_id, name_onmouseover_handler);
		x += get_string_width((unsigned char*)buddy_name_str)*0.9f+5;
		label_id = label_add_extended(buddy_change_win, ++label_id, NULL, x, y, 0, 0.9f, 0.77f, 0.57f, 0.39f, buddy_to_change);
		widget_set_OnMouseover(buddy_change_win, label_id, name_onmouseover_handler);
		label_id++;
		x = 5;
#ifdef ENGLISH
		y += 25;

		x += string_width = get_string_width((unsigned char*)buddy_type_str)*0.9f;
		buddy_type_input_id = -1;
		if (buddy->type < 0xFE) {
#endif //ENGLISH
		/* Add type label and input widget */
#ifndef ENGLISH
		y += 25;
		x += string_width = get_string_width((unsigned char*)buddy_type_str)*0.9f;
#endif //ENGLISH
		label_id = label_add_extended(buddy_change_win, label_id, NULL, 5, y, 0, 0.9f, 0.77f, 0.57f, 0.39f, buddy_type_str);

		buddy_type_input_id = multiselect_add(buddy_change_win, NULL, x, y, buddy_add_x_len-string_width*2);
		multiselect_button_add(buddy_change_win, buddy_type_input_id, 0, 0, buddy_white_str, 1);
		multiselect_button_add(buddy_change_win, buddy_type_input_id, 0, 25, buddy_red_str, 0);
		multiselect_button_add(buddy_change_win, buddy_type_input_id, 0, 50,  buddy_green_str, 0);
		multiselect_button_add(buddy_change_win, buddy_type_input_id, 0, 75, buddy_blue_str, 0);
		multiselect_button_add(buddy_change_win, buddy_type_input_id, 0, 100, buddy_yellow_str, 0);
		multiselect_set_selected(buddy_change_win, buddy_type_input_id, buddy->type);

		widget_set_OnMouseover(buddy_change_win, label_id, type_onmouseover_handler);
		widget_set_OnMouseover(buddy_change_win, buddy_type_input_id, type_onmouseover_handler);
		label_id++;
		y += 5+multiselect_get_height(buddy_change_win, buddy_type_input_id);
#ifdef ENGLISH
        }
#endif //ENGLISH
		/* Delete buddy checkbox and label */
		//Center the checkbox+label
		extra_space = 2+ceilf((buddy_change_x_len-string_width*2 - (20 + get_string_width((unsigned char*)buddy_delete_str)*0.9f))/2.0);
		checkbox_add(buddy_change_win, NULL, x+extra_space, y, 15, 15, &buddy_delete);
		x += 20;
		label_add_extended(buddy_change_win, label_id, NULL, extra_space+x, y, 0, 0.9f, 0.77f, 0.59f, 0.39f, buddy_delete_str);
		widget_set_OnClick(buddy_change_win, label_id, click_delete_checkbox_label);
		widget_set_OnMouseover(buddy_change_win, label_id, delete_onmouseover_handler);
		label_id++;
		x -= 20;
		y += 20;
		/* Add button */
		extra_space = 2+((buddy_change_x_len-string_width*2) - get_string_width((unsigned char*)buddy_change_str))/2-15;
		buddy_change_button_id = button_add(buddy_change_win, NULL, buddy_change_str, x+extra_space, y);
		widget_set_OnClick(buddy_change_win, buddy_change_button_id, click_change_buddy_handler);
		return buddy_change_win;
	} else if (strcmp(title, buddy_accept_str) == 0) {
		char string[250] = {0};
		int current_window;
		int yes_button;
		int no_button;
		int win_height = buddy_accept_y_len;

		/* Argument is the name of the buddy that want to add us */
		for(current_window = 0; current_window < MAX_ACCEPT_BUDDY_WINDOWS; current_window++) {
			/* Find a free slot in the array */
			if(accept_windows[current_window].window_id == -1) {
				break;
			}
		}

		if (current_window >= MAX_ACCEPT_BUDDY_WINDOWS)
		{
			// uh oh, no free window
			return -1;
		}

		safe_snprintf(accept_windows[current_window].name, sizeof (accept_windows[current_window].name), "%s", (char *)argument);
		if(is_in_buddylist(accept_windows[current_window].name)) {
			/* We don't need to make room for the checkbox because the buddy is already in our list. */
			win_height -= 20;
		}
		accept_windows[current_window].window_id = create_window(title, game_root_win, 0, 200, 200, buddy_accept_x_len, win_height, ELW_TITLE_BAR|ELW_DRAGGABLE|ELW_USE_BACKGROUND|ELW_USE_BORDER|ELW_SHOW|ELW_TITLE_NAME|ELW_ALPHA_BORDER);
		set_window_handler(accept_windows[current_window].window_id, ELW_HANDLER_DISPLAY, &display_accept_buddy_handler);
		/* Add text */
		safe_snprintf(string, sizeof(string), buddy_wants_to_add_str, accept_windows[current_window].name);
		//label_add_extended(buddy_accept_win, label_id++, NULL, x, y, 0, 0, 0, 0.8, -1, -1, -1, string);
		accept_windows[current_window].text = malloc((strlen(string)+5)*sizeof(*accept_windows[current_window].text));
		put_small_colored_text_in_box(c_blue1, (unsigned char*)string, strlen(string), buddy_accept_x_len-10, accept_windows[current_window].text);
		y += 40;
		/* Add checkbox if we don't have him/her in our list. */
		if(!is_in_buddylist(accept_windows[current_window].name)) {
			accept_windows[current_window].checkbox = checkbox_add(accept_windows[current_window].window_id, NULL, x, y, 15, 15, NULL);
			x += 20;

			safe_snprintf (string, sizeof (string), buddy_add_to_list_str);
			label_add_extended(accept_windows[current_window].window_id, label_id++, NULL, x, y, 0, 0.8, -1, -1, -1, string);

			y += 20;
			x = 5;
		}
		/* Add buttons */
		x = (buddy_accept_x_len - (get_string_width((unsigned char*)yes_str)*0.9 + 2 + 10 + 2 + get_string_width((unsigned char*)no_str)))/2-40;
		yes_button = button_add_extended(accept_windows[current_window].window_id, 101, NULL, x, y, 0, 0, 0, 1.0f, 0.77f, 0.57f, 0.39f, yes_str);

		x += get_string_width((unsigned char*)yes_str) + 2 + 50 ;
		no_button = button_add_extended(accept_windows[current_window].window_id, 102, NULL, x, y, 0, 0, 0, 1.0f, 0.77f, 0.57f, 0.39f, no_str);
		widget_set_OnClick(accept_windows[current_window].window_id, yes_button, click_accept_yes);
		widget_set_OnClick(accept_windows[current_window].window_id, no_button, click_accept_no);

		return accept_windows[current_window].window_id;
	} else {
		return -1;
	}
}

int click_buddy_button_handler(widget_list *w, int mx, int my, Uint32 flags)
{
	if(buddy_add_win < 0) {
		buddy_add_win = create_buddy_interface_win(buddy_add_str, NULL);
	} else {
		toggle_window(buddy_add_win);
	}
	return 1;
}

#ifdef FR_VERSION
static int resize_buddy_handler(window_info *win)
{
	// controles tailles min et max
	win->len_x = buddy_menu_x_len;
	if (win->len_y > MAX_BUDDY*10 + 22) win->len_y = MAX_BUDDY*10 + 22;
	if (win->len_y < 64) win->len_y = 64;

	// enregistrement des dimensions de la fen�tre
	buddy_menu_x_len = win->len_x;
	buddy_menu_y_len = win->len_y;
	buddy_lines = (buddy_menu_y_len - 22) / 10;

	// redimensionnement et positionnement des barres de d�filement
	widget_resize(buddy_win, buddy_scroll_id, 20, buddy_menu_y_len - 2*20);
	widget_set_size(buddy_win, buddy_scroll_id, buddy_lines);
	vscrollbar_set_bar_len(buddy_win, buddy_scroll_id, buddy_nb);

	widget_move(buddy_win, buddy_button_id, 0, buddy_menu_y_len-20);
	return 0;
}
#endif //FR_VERSION

void display_buddy()
{
	if(buddy_win < 0)
		{
			int our_root_win = -1;
			if (!windows_on_top) {
				our_root_win = game_root_win;
			}
#ifdef FR_VERSION
			buddy_win = create_window(win_buddy, our_root_win, 0, buddy_menu_x, buddy_menu_y, buddy_menu_x_len, buddy_menu_y_len, ELW_RESIZEABLE|ELW_WIN_DEFAULT);
			set_window_handler(buddy_win, ELW_HANDLER_RESIZE, &resize_buddy_handler);
			/* appel la fonction du resize pour valider les tailles & positions */
			resize_buddy_handler(&windows_list.window[buddy_win]);
#else //FR_VERSION
			buddy_win = create_window(win_buddy, our_root_win, 0, buddy_menu_x, buddy_menu_y, buddy_menu_x_len, buddy_menu_y_len, ELW_WIN_DEFAULT);
#endif //FR_VERSION

			set_window_handler(buddy_win, ELW_HANDLER_DISPLAY, &display_buddy_handler );
			set_window_handler(buddy_win, ELW_HANDLER_CLICK, &click_buddy_handler );

#ifdef FR_VERSION
			buddy_scroll_id = vscrollbar_add_extended (buddy_win, buddy_scroll_id, NULL, buddy_menu_x_len-20, 20, 20, buddy_menu_y_len-40, 0, buddy_lines, 0.77f, 0.57f, 0.39f, 0, 1, buddy_nb);
			buddy_button_id = button_add_extended(buddy_win, buddy_button_id, NULL, 0, buddy_menu_y_len-20, buddy_menu_x_len-20, 20, 0, 1.0, 0.77f, 0.57f, 0.39f, buddy_add_str);
#else //FR_VERSION
			buddy_scroll_id = vscrollbar_add_extended (buddy_win, buddy_scroll_id, NULL, 130, 20, 20, 180, 0, 1.0, 0.77f, 0.57f, 0.39f, 0, 1, MAX_BUDDY-19);
			buddy_button_id = button_add_extended(buddy_win, buddy_button_id, NULL, 0, buddy_menu_y_len-20, buddy_menu_x_len, 20, 0, 1.0, -1.0, -1.0, -1.0, buddy_add_str);
#endif //FR_VERSION
			widget_set_OnClick(buddy_win, buddy_button_id, click_buddy_button_handler);
			widget_set_type(buddy_win, buddy_button_id, &square_button_type);
		}
	else
		{
			toggle_window(buddy_win);
		}
}

void add_buddy (const char *name, int type, int len)
{
	int i, found = 0;
#ifdef ENGLISH
	char message[35];
#else
    //@tosh : Le message est sinon coup�
    char message[41];
#endif //ENGLISH

	add_name_to_tablist(name);
	// Check if the buddy already exists
	for (i = 0; i < MAX_BUDDY; i++)
	{
		if(strncasecmp(buddy_list[i].name, name, len) == 0){
			//this name is already in our list
			if(buddy_list[i].type != type){
				//colour change, not a new entry
				if(buddy_log_notice == 1){
					if(buddy_list[i].type == 0xFE){//logging on
						safe_snprintf (message, sizeof(message), buddy_logon_str, len, name);
						LOG_TO_CONSOLE (c_green1, message);
						flash_icon(tt_buddy, 5);
					}else if(type == 0xFE){//logging off
						safe_snprintf (message, sizeof(message), buddy_logoff_str, len, name);
						LOG_TO_CONSOLE (c_green1, message);
						flash_icon(tt_buddy, 5);
					}//else it's just a normal colour change
				}
				buddy_list[i].type=type;
			}
			found = 1;
			break;
		}
	}
	if (found != 1) {
		// find empty space
		for (i = 0; i < MAX_BUDDY; i++)
		{
			if (buddy_list[i].type == 0xff)
			{
				// found then add buddy
				buddy_list[i].type = type;
				safe_snprintf (buddy_list[i].name, sizeof(buddy_list[i].name), "%.*s", len, name);
#ifdef FR_VERSION
				// on tient � jour un compteur des amis dans la liste
				if (++buddy_nb >= MAX_BUDDY) buddy_nb = MAX_BUDDY-1;
				vscrollbar_set_bar_len(buddy_win, buddy_scroll_id, buddy_nb);
				// on tri la liste une fois ici plutot que dans le display handler
				qsort(buddy_list, MAX_BUDDY, sizeof(_buddy), buddy_list_name_cmp);
#endif //FR_VERSION
#ifdef ENGLISH
				// write optional online message
				if ((buddy_log_notice == 1) && (type != 0xFE))
				{
					safe_snprintf (message, sizeof(message), buddy_online_str, len, name);
#else //ENGLISH
				//BUDDY-FIXME: once server-side offline buddies are supported, this if-block will be removed (as del_buddy will only happen when the buddy really is deleted)
				if (buddy_log_notice == 1)
				{
					// if less than 5sec since the timer was
					// updated, then we don't notify. in cases of
					// bad lag, this won't help. if someone logs
					// on/off during that time, we miss the
					// notification
					time_t n_time;

					time (&n_time);
					if (difftime (c_time, n_time) > -5.0f) break;

					safe_snprintf (message, sizeof(message), buddy_logon_str, len, name);
#endif //ENGLISH
					LOG_TO_CONSOLE (c_green1, message);
#ifdef FR_VERSION
					flash_icon(tt_buddy, 10);
#else //FR_VERSION
					flash_icon(tt_buddy, 5);
#endif//FR_VERSION
				}
				break;
			}
		}
	}
}

void del_buddy (const char *name, int len)
{
	int i;
#ifndef ENGLISH
    //@tosh : passage de [36] � [41] pour �viter que le message soit coup�
	char message[41];
#endif //ENGLISH

	// find buddy
	for (i = 0; i < MAX_BUDDY; i++)
	{
		if (strncasecmp (name, buddy_list[i].name, len) == 0)
		{
			buddy_list[i].type = 0xff;
			memset (buddy_list[i].name, 0, sizeof (buddy_list[i].name));
#ifdef FR_VERSION
				// on tient � jour un compteur des amis dans la liste
				if (--buddy_nb < 0) buddy_nb = 0;
				vscrollbar_set_bar_len(buddy_win, buddy_scroll_id, buddy_nb);
#endif //FR_VERSION
#ifndef ENGLISH
			//BUDDY-FIXME: once server-side offline buddies are supported, this if-block will be removed (as del_buddy will only happen when the buddy really is deleted)
			if (buddy_log_notice == 1)
			{
				// if less than 5sec since the timer was
				// updated, then we don't notify. in cases of
				// bad lag, this won't help. if someone logs
				// on/off during that time, we miss the
				// notification
				time_t n_time;

				time (&n_time);
				if (difftime (c_time, n_time) > -5.0f) break;

				safe_snprintf (message, sizeof(message), buddy_logoff_str, len, name);
				LOG_TO_CONSOLE (c_green1, message);
			}
#endif //ENGLISH
			break;
		}
	}
}

void clear_buddy()
{
	int i;
	for(i=0; i<MAX_BUDDY; i++){
		buddy_list[i].type= 0xff;
		buddy_list[i].name[0]=0;
	}
#ifdef FR_VERSION
	buddy_nb = 0;
	vscrollbar_set_bar_len(buddy_win, buddy_scroll_id, buddy_nb);
#endif //FR_VERSION
}

int is_in_buddylist(const char *name)
{
	int i;
	char onlyname[32];
	if(!name || !*name) {
		return 0;
	}
	while(name[0] != '\0' && is_color ((unsigned char)name[0])){
		++name;
	}
	// strip GuildTag if existing
	for(i = 0; name[i]>32; i++){
		onlyname[i] = name[i];
	}
	onlyname[i] = '\0';

	for(i = 0; i < MAX_BUDDY; i++) {
#ifdef ENGLISH
		if(buddy_list[i].type < 0xff && strcasecmp(buddy_list[i].name, onlyname) == 0) {
#else //ENGLISH
		if(buddy_list[i].type < 0xff && strncasecmp(buddy_list[i].name, onlyname, strlen(buddy_list[i].name)) == 0) {
#endif //ENGLISH
			return 1;
		}
	}
	return 0;
}

void add_buddy_confirmation(char *name) {
	char *name_copy = malloc(strlen(name)+1);
	safe_snprintf(name_copy, strlen(name)+1, "%s", name);
	queue_push(buddy_request_queue, name_copy);
}
