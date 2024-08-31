#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "storage.h"
#include "asc.h"
#include "context_menu.h"
#include "dialogues.h"
#include "elwindows.h"
#include "filter.h"
#include "gamewin.h"
#include "hud.h"
#include "init.h"
#include "items.h"
#include "item_info.h"
#include "item_lists.h"
#include "misc.h"
#include "multiplayer.h"
#include "sound.h"
#include "textures.h"
#include "translate.h"
#include "widgets.h"
#include "gl_init.h"
#define STORAGE_CATEGORIES_SIZE 50
#define STORAGE_CATEGORIES_DISPLAY 13
#define STORAGE_SCROLLBAR_CATEGORIES 1200
#define STORAGE_SCROLLBAR_ITEMS 1201
struct storage_category {
	char name[30];
	int id;
	int color;
} storage_categories[STORAGE_CATEGORIES_SIZE];
int no_storage_categories = 0;
int selected_category = -1;
int view_only_storage = 0;
Uint32 drop_fail_time = 0;
int sort_storage_categories = 0;
int active_storage_item = -1;
ground_item storage_items[STORAGE_ITEMS_SIZE] = {{0, 0, 0}};
int no_storage;
#define MAX_DESCR_LEN 202
char storage_text[MAX_DESCR_LEN] = {0};
static char last_storage_text[MAX_DESCR_LEN] = {0};
static char wrapped_storage_text[MAX_DESCR_LEN + 10] = {0};
static int print_quanities[STORAGE_ITEMS_SIZE];
static int number_to_print = 0;
static int next_item_to_print = 0;
static int printing_category = -1;
static int mouse_over_titlebar = 0;
static int mouse_over_storage = 0;
int disable_storage_filter = 0;
static char filter_item_text[40] = "";
static size_t filter_item_text_size = 0;
static Uint8 storage_items_filter[STORAGE_ITEMS_SIZE];
int storage_items_cols = 6;
int storage_items_rows = 6;
int storage_items_size = 6 * 6;
int storage_textarea_lines = 1;
int storage_textarea_size = 5 + 1 * 25 + 5;                    // =25
int storage_categories_size = (6 * 32 - 2 * 3) / 13;             // =14
int storage_win_x_len = 10 + 120 + 20 + 10 + 6 * 32 + 20 + 20; // =392
int storage_win_y_len = 10 + 6 * 32 + 10 + 5 + 15 + 5 + 10;        // =247
//	Look though the category for the selected item, pick it up if found.
//
static void select_item(int image_id, Uint16 item_id) {
	int i;
	int found_at = -1;
	for (i = 0; i < no_storage; i++) {
		if ((item_id != unset_item_uid) && (storage_items[i].id != unset_item_uid) && (storage_items[i].quantity > 0)) {
			if (storage_items[i].id == item_id) {
				found_at = i;
				break;
			}
		} else if ((storage_items[i].image_id == image_id) && (storage_items[i].quantity > 0)) {
			found_at = i;
			break;
		}
	}
	if (found_at < 0) {
		do_alert1_sound();
		item_lists_reset_pickup_fail_time();
	} else {
		active_storage_item = storage_items[found_at].pos;
		if (!view_only_storage) {
			storage_item_dragged = found_at;
			do_drag_item_sound();
		} else {
			do_click_sound();
		}
	}
}
static int wanted_category = -1;
static Uint16 wanted_item_id = -1;
static int wanted_image_id = -1;
static void move_to_category(int cat);
static int find_category(int id);
//	Called when the category is changed.
//	- Update the store of items/category.
//	- If in the process of picking up an item go for it if this is the category.
//
static void category_updated(void) {
	int i;
	for (i = 0; i < no_storage; i++) {
		update_category_maps(storage_items[i].image_id, storage_items[i].id, storage_categories[selected_category].id);
	}
	if (selected_category == wanted_category) {
		select_item(wanted_image_id, wanted_item_id);
		wanted_category = -1;
	}
}
//	Start the process of picking up the specified item from a specified category.
//	If the category is already selected, try picking up the item now, otherwise
//	set the requird category, the pick will continue when the category is availble.
//
void pickup_storage_item(int image_id, Uint16 item_id, int cat_id) {
	if ((storage_win < 0) || (find_category(cat_id) == -1)) {
		do_alert1_sound();
		item_lists_reset_pickup_fail_time();
		return;
	}
	wanted_category = find_category(cat_id);
	wanted_image_id = image_id;
	wanted_item_id = item_id;
	if (selected_category == wanted_category) {
		select_item(wanted_image_id, wanted_item_id);
		wanted_category = -1;
	} else {
		move_to_category(wanted_category);
	}
}
void withdraw_active_storage_item(int qte_item) {
	if (active_storage_item != -1 && wanted_category == -1) {
		Uint8 str[7];
		str[0] = WITHDRAW_ITEM;
		*((Uint16 *)(str + 1)) = SDL_SwapLE16(active_storage_item);
		*((Uint32 *)(str + 3)) = SDL_SwapLE32(qte_item);
		my_tcp_send(my_socket, str, 7);
		do_drop_item_sound();
	}
}
void get_storage_text(const Uint8 *in_data, int len) {
	safe_snprintf(storage_text, sizeof(storage_text), "%.*s", len, in_data);
	if ((len > 0) && (printing_category > -1) && (next_item_to_print < number_to_print)) {
		char the_text[MAX_DESCR_LEN + 20];
		if (!next_item_to_print) {
			safe_snprintf(the_text, sizeof(the_text), "%s:", &storage_categories[printing_category].name[1]);
			LOG_TO_CONSOLE(c_green2, the_text);
		}
		safe_snprintf(the_text, sizeof(the_text), "%d %s", print_quanities[next_item_to_print++], &storage_text[1]);
		LOG_TO_CONSOLE(c_grey1, the_text);
		storage_text[0] = '\0';
	}
}
static int category_cmp(const void *a, const void *b) {
	return strcmp(((const struct storage_category *)a)->name, ((const struct storage_category *)b)->name);
}
void get_storage_categories(const char *in_data, int len) {
	int i;
	int idx, idxp;
	idx = 1;
	for (i = 0; i < in_data[0] && i < STORAGE_CATEGORIES_SIZE && idx < len; i++) {
		storage_categories[i].id = (Uint8)in_data[idx++];
		storage_categories[i].name[0] = to_color_char(c_orange1);
		idxp = 1;
		while (idx < len && idxp < sizeof(storage_categories[i].name) - 1 && in_data[idx] != '\0') {
			storage_categories[i].name[idxp++] = in_data[idx++];
		}
		// always make sure the string is terminated
		storage_categories[i].name[idxp] = '\0';
		// was the string too long?
		if (idxp >= sizeof(storage_categories[i].name) - 1) {
			// skip rest of string
			while (idx < len && in_data[idx] != '\0') {
				idx++;
			}
		}
		idx++;
	}
	if (sort_storage_categories) {
		qsort(storage_categories, i, sizeof(*storage_categories), category_cmp);
	}
	for (i = in_data[0]; i < STORAGE_CATEGORIES_SIZE; i++) {
		storage_categories[i].id = -1;
		storage_categories[i].name[0] = 0;
	}
	no_storage_categories = in_data[0];
	if (storage_win > 0) {
		vscrollbar_set_bar_len(storage_win, STORAGE_SCROLLBAR_CATEGORIES, no_storage_categories);
	}
	selected_category = -1;
	active_storage_item = -1;
	display_storage_menu();
	if (!view_only_storage) {
		display_items_menu();
	}
}
int find_category(int id) {
	int i;
	for (i = 0; i < no_storage_categories; i++) {
		if (storage_categories[i].id == id) {
			return i;
		}
	}
	return -1;
}
void set_window_name(char *extra_sep, char *extra_name) {
	safe_snprintf(windows_list.window[storage_win].window_name, sizeof(windows_list.window[storage_win].window_name), "%s%s%s%s", win_storage, extra_sep, extra_name, ((view_only_storage) ?win_storage_vo:""));
}
void move_to_category(int cat) {
	Uint8 str[4];
	if (cat < 0 || cat >= no_storage_categories) {
		return;
	}
	storage_categories[cat].name[0] = to_color_char(c_red3);
	if (selected_category != -1 && cat != selected_category) {
		storage_categories[selected_category].name[0] = to_color_char(c_orange1);
	}
	set_window_name(" - ", storage_categories[cat].name + 1);
	str[0] = GET_STORAGE_CATEGORY;
	*((Uint8 *)(str + 1)) = storage_categories[cat].id;
	my_tcp_send(my_socket, str, 2);
}
static void update_item_filter(void) {
	if (!disable_storage_filter && (no_storage > 0) && (filter_item_text_size > 0)) {
		filter_items_by_description(storage_items_filter, storage_items, filter_item_text, no_storage);
	} else {
		size_t i;
		for (i = 0; i < STORAGE_ITEMS_SIZE; i++) {
			storage_items_filter[i] = 0;
		}
	}
}
void get_storage_items(const Uint8 *in_data, int len) {
	int i;
	int cat, pos;
	int idx;
	int plen;
	if (item_uid_enabled) {
		plen = 10;
	} else {
		plen = 8;
	}
	if (in_data[0] == 255) {
		// It's just an update - make sure we're in the right category
		idx = 2;
		active_storage_item = SDL_SwapLE16(*((Uint16 *)(&in_data[idx + 6])));
		//@tosh : on change �ventuellement de cat�gorie, lorsque l'on met
		//un objet au d�p�t.
		cat = find_category(in_data[1]);
		move_to_category(cat);
		for (i = 0; i < STORAGE_ITEMS_SIZE; i++) {
			if ((storage_items[i].pos == SDL_SwapLE16(*((Uint16 *)(&in_data[idx + 6])))) && (storage_items[i].quantity > 0)) {
				storage_items[i].image_id = SDL_SwapLE16(*((Uint16 *)(&in_data[idx])));
				storage_items[i].quantity = SDL_SwapLE32(*((Uint32 *)(&in_data[idx + 2])));
				if (item_uid_enabled) {
					storage_items[i].id = SDL_SwapLE16(*((Uint16 *)(&in_data[idx + 8])));
				} else {
					storage_items[i].id = unset_item_uid;
				}
				update_item_filter();
				return;
			}
		}
		for (i = 0; i < STORAGE_ITEMS_SIZE; i++) {
			if (storage_items[i].quantity == 0) {
				if (item_uid_enabled) {
					storage_items[i].id = SDL_SwapLE16(*((Uint16 *)(&in_data[idx + 8])));
				} else {
					storage_items[i].id = unset_item_uid;
				}
				storage_items[i].pos = SDL_SwapLE16(*((Uint16 *)(&in_data[idx + 6])));
				storage_items[i].image_id = SDL_SwapLE16(*((Uint16 *)(&in_data[idx])));
				storage_items[i].quantity = SDL_SwapLE32(*((Uint32 *)(&in_data[idx + 2])));
				no_storage++;
				update_item_filter();
				return;
			}
		}
	}
	no_storage = (len - 2) / plen;
	cat = find_category(in_data[1]);
	if (cat >= 0) {
		storage_categories[cat].name[0] = to_color_char(c_red3);
		if (selected_category != -1 && cat != selected_category) {
			storage_categories[selected_category].name[0] = to_color_char(c_orange1);
		}
		set_window_name(" - ", storage_categories[cat].name + 1);
		selected_category = cat;
	}
	idx = 2;
	for (i = 0; i < no_storage && i < STORAGE_ITEMS_SIZE; i++, idx += plen) {
		storage_items[i].image_id = SDL_SwapLE16(*((Uint16 *)(&in_data[idx])));
		storage_items[i].quantity = SDL_SwapLE32(*((Uint32 *)(&in_data[idx + 2])));
		storage_items[i].pos = SDL_SwapLE16(*((Uint16 *)(&in_data[idx + 6])));
		if (item_uid_enabled) {
			storage_items[i].id = SDL_SwapLE16(*((Uint16 *)(&in_data[idx + 8])));
		} else {
			storage_items[i].id = unset_item_uid;
		}
	}
	for ( ; i < STORAGE_ITEMS_SIZE; i++) {
		storage_items[i].quantity = 0;
	}
	vscrollbar_set_pos(storage_win, STORAGE_SCROLLBAR_ITEMS, 0);
	// on adapte la scrollbar au nombre d'objets dans la cat�gorie
	vscrollbar_set_bar_len(storage_win, STORAGE_SCROLLBAR_ITEMS, (no_storage - 1) / storage_items_cols + 1);
	pos = vscrollbar_get_pos(storage_win, STORAGE_SCROLLBAR_CATEGORIES);
	if (cat < pos) {
		vscrollbar_set_pos(storage_win, STORAGE_SCROLLBAR_CATEGORIES, cat);
	} else if (cat >= pos + storage_categories_size) {
		vscrollbar_set_pos(storage_win, STORAGE_SCROLLBAR_CATEGORIES, cat - storage_categories_size + 1);
	}
	if (selected_category != -1) {
		category_updated();
	}
	update_item_filter();
}
int storage_win = -1;
int storage_win_x = 100;
int storage_win_y = 100;
int cur_item_over = -1;
int storage_item_dragged = -1;
int display_storage_handler(window_info *win) {
	int i;
	int n = 0;
	int pos;
	int help_text_line = 0;
	have_storage_list = 0;  //We visited storage, so we may have changed something
	glColor3f(0.77f, 0.57f, 0.39f);
	glEnable(GL_TEXTURE_2D);
	for (i = pos = vscrollbar_get_pos(storage_win, STORAGE_SCROLLBAR_CATEGORIES); i < no_storage_categories && storage_categories[i].id != -1 && i < pos + storage_categories_size; i++, n++) {
		draw_string_small(15, 10 + 3 + n * 13, (unsigned char *)storage_categories[i].name, 1);
	}
	if (storage_text[0]) {
		if (strcmp(storage_text, last_storage_text) != 0) {
			safe_strncpy(last_storage_text, storage_text, sizeof(last_storage_text));
			put_small_text_in_box((Uint8 *)storage_text, strlen(storage_text), win->len_x - 15 * 2, wrapped_storage_text);
		}
		draw_string_small(15, storage_win_y_len - 10 - storage_textarea_size + 5, (unsigned char *)wrapped_storage_text, storage_textarea_lines);
	}
	glColor3f(1.0f, 1.0f, 1.0f);
	for (i = pos = storage_items_cols * vscrollbar_get_pos(storage_win, STORAGE_SCROLLBAR_ITEMS); i < pos + storage_items_size && i < no_storage; i++) {
		/* autant r�utiliser la fonction draw_item */
		if (!storage_items[i].quantity) {
			continue;
		}
		draw_item(storage_items[i].image_id, (i % storage_items_cols) * 32 + 281, ((i - pos) / storage_items_cols) * 32 + 10, 32);
		if (!disable_storage_filter && filter_item_text_size && storage_items_filter[i]) {
			gray_out(i % storage_items_cols * 31 + 281, ((i - pos) / storage_items_cols) * 31 + 10, 32);
		}
	}
	// Render the grid *after* the images. It seems impossible to code
	// it such that images are rendered exactly within the boxes on all
	// cards
	glDisable(GL_TEXTURE_2D);
	glColor3f(0.77f, 0.57f, 0.39f);
	glBegin(GL_LINE_LOOP);
	glVertex2i(10, 10);
	glVertex2i(10, storage_win_y_len - 10 - storage_textarea_size - 10);
	glVertex2i(10 + 250, storage_win_y_len - 10 - storage_textarea_size - 10);
	glVertex2i(10 + 250, 10);
	glEnd();
	glBegin(GL_LINE_LOOP);
	glVertex2i(10, storage_win_y_len - 10 - storage_textarea_size);
	glVertex2i(10, storage_win_y_len - 10);
	glVertex2i(storage_win_x_len - 10, storage_win_y_len - 10);
	glVertex2i(storage_win_x_len - 10, storage_win_y_len - 10 - storage_textarea_size);
	glEnd();
	if (view_only_storage) {
		Uint32 currentticktime = SDL_GetTicks();
		if (currentticktime < drop_fail_time) {
			drop_fail_time = 0;                             /* trap wrap */
		}
		if ((currentticktime - drop_fail_time) < 250) {
			glColor3f(0.8f, 0.2f, 0.2f);                      /* flash red if tried to drop into */
		} else {
			glColor3f(0.37f, 0.37f, 0.39f);         /* otherwise draw greyed out */
		}
	}
	rendergrid(storage_items_cols, storage_items_rows, 280, 10, 32, 32);
	glEnable(GL_TEXTURE_2D);
	// on dessine les quantit�s par dessus la grille !
	if (cur_item_over != -1 && mouse_in_window(win->window_id, mouse_x, mouse_y) == 1 && active_storage_item != storage_items[cur_item_over].pos) {
		char str[20];
		safe_snprintf(str, sizeof(str), "%d", storage_items[cur_item_over].quantity);
		draw_string_small_shadowed(mouse_x - win->pos_x - (strlen(str) * 0.5f) * 8 + 8, mouse_y - win->pos_y + 16, (unsigned char *)str, 1, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	}
	glColor3f(1.0f, 1.0f, 1.0f);
	if (active_storage_item >= 0) {
		/* Draw the active item's quantity on top of everything else. */
		for (i = pos = storage_items_cols * vscrollbar_get_pos(storage_win, STORAGE_SCROLLBAR_ITEMS); i < pos + storage_items_size && i < no_storage; i++) {
			if (storage_items[i].pos == active_storage_item) {
				if (storage_items[i].quantity) {
					char str[20];
					safe_snprintf(str, sizeof(str), "%d", storage_items[i].quantity);
					draw_string_small_shadowed(280 + 32 * (i % storage_items_cols), ((i - pos) / storage_items_cols) * 32 + 29, (unsigned char *)str, 1, 1.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f);
				}
				break;
			}
		}
	}
	if (!disable_storage_filter && !mouse_over_titlebar) {
		if (filter_item_text_size > 0) {
			static char tmp[50];
			safe_snprintf(tmp, sizeof(tmp), "%s[%s]", storage_filter_prompt_str, filter_item_text);
			show_help(tmp, 0, win->len_y + 10 + (help_text_line++) * SMALL_FONT_Y_LEN);
		} else if (show_help_text && mouse_over_storage) {
			show_help(storage_filter_help_str, 0, win->len_y + 10 + (help_text_line++) * SMALL_FONT_Y_LEN);
		}
	}
	mouse_over_storage = mouse_over_titlebar = 0;
	return 1;
}
int click_storage_handler(window_info *win, int mx, int my, Uint32 flags) {
	// modification de la quantit� de l'objet en cours de drag avec la molette
	if (flags & ELW_WHEEL) {
		if ((allow_wheel_quantity_drag) && ((item_dragged != -1) || (storage_item_dragged != -1))) {
			if (quantities.selected != ITEM_EDIT_QUANT) {
				*quantities.quantity[ITEM_EDIT_QUANT].str = *quantities.quantity[quantities.selected].str;
				quantities.quantity[ITEM_EDIT_QUANT].len = quantities.quantity[quantities.selected].len;
				quantities.quantity[ITEM_EDIT_QUANT].val = quantities.quantity[quantities.selected].val;
				quantities.selected = ITEM_EDIT_QUANT;
			}
			wheel_change_quantity(flags);
			return 1;
		}
	}
	if (flags & ELW_WHEEL_UP) {
		if (mx > 10 && mx < 260) {
			vscrollbar_scroll_up(storage_win, STORAGE_SCROLLBAR_CATEGORIES);
		} else if (mx > 280 && mx < 280 + 32 * storage_items_cols) {
			vscrollbar_scroll_up(storage_win, STORAGE_SCROLLBAR_ITEMS);
		}
	} else if (flags & ELW_WHEEL_DOWN) {
		if (mx > 10 && mx < 260) {
			vscrollbar_scroll_down(storage_win, STORAGE_SCROLLBAR_CATEGORIES);
		} else if (mx > 280 && mx < 280 + 32 * storage_items_cols) {
			vscrollbar_scroll_down(storage_win, STORAGE_SCROLLBAR_ITEMS);
		}
	} else if ((flags & ELW_MOUSE_BUTTON) == 0) {
		return 0;
	} else {
		if (my > 10 && my < storage_win_y_len - 10 - storage_textarea_size - 10) {
			if (mx > 10 && mx < 230) {
				int cat = -1;
				cat = (my - 10 - 3) / 13 + vscrollbar_get_pos(storage_win, STORAGE_SCROLLBAR_CATEGORIES);
				move_to_category(cat);
				do_click_sound();
			} else if (mx > 280 && mx < 280 + 32 * storage_items_cols) {
				if (view_only_storage && item_dragged != -1 && left_click) {
					drop_fail_time = SDL_GetTicks();
					do_alert1_sound();
				} else if (!view_only_storage && item_dragged != -1 && left_click) {
					Uint8 str[6];
					// objet lach� venant de l'inventaire
					int i, temp_case = -1;
					int temp_quantity = item_quantity;
					// si l'objet vient des �quipements il faut d'abord le d�s�quiper dans le sac
					if (item_dragged >= ITEM_WEAR_START) {
						if (item_list[item_dragged].is_stackable) {
							for (i = 0; i < ITEM_WEAR_START; i++) {
								if (item_list[i].id != unset_item_uid) {
									if (item_list[i].id != item_list[item_dragged].id) {
										continue;
									}
								} else if ((item_list[i].image_id != item_list[item_dragged].image_id) || !item_list[i].is_stackable) {
									continue;
								}
								temp_case = i;
								break;
							}
						}
						// sinon recherche d'une case libre dans le sac pour d�s�quiper
						if (temp_case < 0) {
							for (i = 0; i < ITEM_WEAR_START; i++) {
								if (!item_list[i].quantity) {
									temp_case = i;
									break;
								}
							}
						}
						if (temp_case < 0) {
							return 0;
						}
						// demande de d�placement dans le sac
						str[0] = MOVE_INVENTORY_ITEM;
						str[1] = item_list[item_dragged].pos;
						str[2] = temp_case;
						my_tcp_send(my_socket, str, 3);
						item_list[temp_case].pos = temp_case;
						item_dragged = temp_case;
						temp_quantity = 1;
					}
					str[0] = DEPOSITE_ITEM;
					str[1] = item_list[item_dragged].pos;
					*((Uint32 *)(str + 2)) = SDL_SwapLE32(temp_quantity);
					my_tcp_send(my_socket, str, 6);
					do_drop_item_sound();
					if (item_list[item_dragged].quantity <= item_quantity) {
						item_dragged = -1;                                          //Stop dragging this item...
					}
				} else if (right_click || (view_only_storage && left_click)) {
					storage_item_dragged = -1;
					item_dragged = -1;
					if (cur_item_over != -1) {
						Uint8 str[3];
						str[0] = LOOK_AT_STORAGE_ITEM;
						*((Uint16 *)(str + 1)) = SDL_SwapLE16(storage_items[cur_item_over].pos);
						my_tcp_send(my_socket, str, 3);
						active_storage_item = storage_items[cur_item_over].pos;
						do_click_sound();
					}
					// ctrl+clic sur un objet : ajout d'un raccourci dans la barre rapide
					// Note : mieux vaut �viter d'enregistrer un raccourci avec aussi peu d'info sur l'objet
					// En particulier nous ne savons pas si un objet du d�p�t est empilable ou non
/*
                                } else if (!view_only_storage && cur_item_over!=-1 && (flags & ELW_CTRL))
                                {
                                        int i, id_quick = -1;
                                        for (i=0; i < FR_QUICKITEMS_MAXSIZE; i++)
                                        {
                                                if ((fr_quickitem_list[i].quantity>=0) && (fr_quickitem_list[i].image_id==storage_list[cur_item_over].image_id))
                                                {
                                                        fr_quickitem_list[i].quantity = -1;
                                                }
                                                if ((id_quick < 0) && (fr_quickitem_list[i].quantity < 0)) id_quick = i;
                                        }
                                        if (id_quick >= 0)
                                        {
                                                fr_quickitem_list[id_quick].image_id = storage_list[cur_item_over].image_id;
                                                fr_quickitem_list[id_quick].quantity = storage_list[cur_item_over].quantity;
                                                fr_quickitem_list[id_quick].pos = id_quick;
                                                return 1;
                                        }
                                }
 */
				}
				// shift+clic sur un objet : on tente de l'�quiper
				else if (!view_only_storage && cur_item_over != -1 && (flags & ELW_SHIFT)) {
					Uint8 str[7];
					int i, temp_case, dest_pos;
					// recherche d'une case libre dans le sac qui servira d'interm�diaire
					temp_case = -1;
					for (i = 0; i < ITEM_WEAR_START; i++) {
						if (item_list[i].quantity > 0) {
							if (!item_list[i].is_stackable) {
								continue;
							}
							if (item_list[i].id != unset_item_uid) {
								if (item_list[i].id != storage_items[cur_item_over].id) {
									continue;
								}
							} else if (item_list[i].image_id != storage_items[cur_item_over].image_id) {
								continue;
							}
							temp_case = i;
							break;
						} else if (temp_case < 0) {
							temp_case = i;
						}
					}
					if (temp_case < 0) {
						return 0;
					}
					// recherche d'une case libre dans les �quipements
					dest_pos = -1;
					for (i = ITEM_WEAR_START; i < ITEM_NUM_ITEMS; i++) {
						if (!item_list[i].quantity) {
							dest_pos = i;
							break;
						}
					}
					if (dest_pos < 0) {
						return 0;
					}
					// on met dans l'inventaire un seul exemplaire de l'objet
					str[0] = WITHDRAW_ITEM;
					*((Uint16 *)(str + 1)) = SDL_SwapLE16(storage_items[cur_item_over].pos);
					*((Uint32 *)(str + 3)) = SDL_SwapLE32(1);
					my_tcp_send(my_socket, str, 7);
					// puis on tente d'�quiper l'objet depuis l'inventaire
					str[0] = MOVE_INVENTORY_ITEM;
					str[1] = temp_case;
					str[2] = dest_pos;
					my_tcp_send(my_socket, str, 3);
					do_drop_item_sound();
					// Ctrl+clic sur un objet : chargement dans l'inventaire d'une unit�
					// Double-clic (drop sur lui-m�me) : chargement de la quantit� s�lectionn�e
				} else if (!view_only_storage && cur_item_over != -1 && ((flags & ELW_CTRL) || (cur_item_over == storage_item_dragged))) {
					Uint8 str[7];
					str[0] = WITHDRAW_ITEM;
					*((Uint16 *)(str + 1)) = SDL_SwapLE16(storage_items[cur_item_over].pos);
					*((Uint32 *)(str + 3)) = (flags & ELW_CTRL) ? 1 : SDL_SwapLE32(item_quantity);
					my_tcp_send(my_socket, str, 7);
					do_drop_item_sound();
					if (storage_items[cur_item_over].quantity <= item_quantity) {
						storage_item_dragged = -1;
					}
				} else if (!view_only_storage && cur_item_over != -1) {
					storage_item_dragged = cur_item_over;
					active_storage_item = storage_items[cur_item_over].pos;
					do_drag_item_sound();
				}
			}
		}
	}
	return 1;
}
int mouseover_storage_handler(window_info *win, int mx, int my) {
	static int last_pos;
	int last_category;
	cur_item_over = -1;
	if (my < 0) {
		mouse_over_titlebar = 1;
	} else {
		mouse_over_storage = 1;
	}
	if (my > 10 && my < storage_win_y_len - 10 - storage_textarea_size - 10) {
		if (mx > 10 && mx < 130) {
			int i;
			int pos = last_pos = (my - 10 - 3) / 13;
			int p;
			for (i = p = vscrollbar_get_pos(storage_win, STORAGE_SCROLLBAR_CATEGORIES); i < no_storage_categories; i++) {
				if (i == selected_category) {} else if (i != p + pos) {
					storage_categories[i].name[0] = to_color_char(c_orange1);
				} else {
					storage_categories[i].name[0] = to_color_char(c_green2);
				}
			}
			return 0;
		} else if (mx > 280 && mx < 280 + storage_items_cols * 32) {
			cur_item_over = get_mouse_pos_in_grid(mx, my, storage_items_cols, storage_items_rows, 280, 10, 32, 32) + vscrollbar_get_pos(storage_win, STORAGE_SCROLLBAR_ITEMS) * storage_items_cols;
			if (cur_item_over >= no_storage || cur_item_over < 0 || !storage_items[cur_item_over].quantity) {
				cur_item_over = -1;
			}
		}
	}
	last_category = last_pos + vscrollbar_get_pos(storage_win, STORAGE_SCROLLBAR_CATEGORIES);
	if (last_pos >= 0 && last_category < STORAGE_CATEGORIES_SIZE && last_category != selected_category) {
		storage_categories[last_category].name[0] = to_color_char(c_orange1);
		last_pos = -1;
	}
	return 0;
}
static int keypress_storage_handler(window_info *win, int mx, int my, Uint32 key, Uint32 unikey) {
	char keychar = tolower(key_to_char(unikey));
	if (disable_storage_filter || (keychar == '`') || (key & ELW_CTRL) || (key & ELW_ALT)) {
		return 0;
	}
	if (keychar == SDLK_ESCAPE) {
		filter_item_text[0] = '\0';
		filter_item_text_size = 0;
		return 1;
	}
	item_info_help_if_needed();
	if (string_input(filter_item_text, sizeof(filter_item_text), keychar)) {
		filter_item_text_size = strlen(filter_item_text);
		if (filter_item_text_size > 0) {
			filter_items_by_description(storage_items_filter, storage_items, filter_item_text, no_storage);
		}
		return 1;
	}
	return 0;
}
static int resize_storage_handler(window_info *win) {
	// contr�le sur la largeur max de la fen�tre (arbitrairement = �cran - 100)
	if (win->len_x > window_width - 100) {
		win->len_x = window_width - 100;
	}
	// contr�le sur la largeur min de la fen�tre (liste + 1 case + scrolls + marges + X)
	if (win->len_x < 240 + 32 + 2 * 20 + 3 * 10 + 10) {
		win->len_x = 240 + 32 + 2 * 20 + 3 * 10 + 10;
	}
	// nombre de lignes pour la zone texte selon la largeur de la fen�tre (arbitrairement 45 caract�res)
	storage_textarea_lines = 1 + (45 * 8 / (win->len_x - 2 * 10 - 2 * 5));
	storage_textarea_size = 5 + storage_textarea_lines * 15 + 5;
	// contr�le sur la hauteur max de la fen�tre par rapport � l'�cran
	if (win->len_y > window_height) {
		win->len_y = window_height;
	}
	// contr�le sur la hauteur min de la fen�tre (1 ligne + textarea)
	if (win->len_y < 10 + 32 + 10 + storage_textarea_size + 10) {
		win->len_y = 10 + 32 + 10 + storage_textarea_size + 10;
	}
	// enregistrement des dimensions de la fen�tre
	storage_win_x_len = win->len_x;
	storage_win_y_len = win->len_y;
	// calcul du nombre de cases pour la grille (nb de colonnes et lignes)
	storage_items_cols = (storage_win_x_len - 10 - 240 - 20 - 10 - 20 - 10 - 10) / 32;
	storage_items_rows = (storage_win_y_len - 10 - 10 - storage_textarea_size - 10) / 32;
	storage_items_size = storage_items_cols * storage_items_rows;
	// calcul du nombre de cat�gories visibles
	storage_categories_size = (storage_win_y_len - 10 - 2 * 3 - 10 - storage_textarea_size - 10) / 13;
	// redimensionnement et positionnement des barres de d�filement
	widget_resize(storage_win, STORAGE_SCROLLBAR_CATEGORIES, 20, storage_win_y_len - storage_textarea_size - 3 * 10);
	widget_resize(storage_win, STORAGE_SCROLLBAR_ITEMS, 20, storage_items_rows * 32);
	widget_move(storage_win, STORAGE_SCROLLBAR_ITEMS, 280 + storage_items_cols * 32, 10);
	widget_set_size(storage_win, STORAGE_SCROLLBAR_CATEGORIES, storage_categories_size);
	widget_set_size(storage_win, STORAGE_SCROLLBAR_ITEMS, storage_items_rows);
	vscrollbar_set_bar_len(storage_win, STORAGE_SCROLLBAR_ITEMS, (no_storage - 1) / storage_items_cols + 1);
	// red�coupe le texte de l'inventaire en fonction de la largeur de la fen�tre
	put_small_text_in_box((Uint8 *)storage_text, strlen(storage_text), storage_win_x_len - 15 * 2, wrapped_storage_text);
	return 0;
}
void print_items(void) {
	int i;
	actor *me;
	me = get_our_actor();
	if (me) {
		if (me->fighting) {
			LOG_TO_CONSOLE(c_red1, "Impossible pendant un combat !");
			return;
		}
	}
	/* request the description for each item */
	number_to_print = next_item_to_print = 0;
	printing_category = selected_category;
	for (i = 0; i < no_storage && i < STORAGE_ITEMS_SIZE; i++) {
		if (storage_items[i].quantity) {
			Uint8 str[3];
			print_quanities[number_to_print++] = storage_items[i].quantity;
			str[0] = LOOK_AT_STORAGE_ITEM;
			*((Uint16 *)(str + 1)) = SDL_SwapLE16(storage_items[i].pos);
			my_tcp_send(my_socket, str, 3);
		}
	}
}
static int context_storage_handler(window_info *win, int widget_id, int mx, int my, int option) {
	if (option < ELW_CM_MENU_LEN) {
		return cm_title_handler(win, widget_id, mx, my, option);
	}
	switch (option) {
	case ELW_CM_MENU_LEN + 1:
		win->len_x = 592;
		win->len_y = 247;
		resize_storage_handler(win);
		break;
	case ELW_CM_MENU_LEN + 2:
		print_items();
		break;
	}
	return 1;
}
void display_storage_menu() {
	int i;
	/* Entropy suggested hack to determine if this is the view only "#sto" opened storage */
	view_only_storage = 0;
	for (i = 0; i < no_storage_categories; i++) {
		if ((storage_categories[i].id != -1) && (strcmp(&storage_categories[i].name[1], "Quest") == 0)) {
			view_only_storage = 1;
			break;
		}
	}
	if (storage_win <= 0) {
		int our_root_win = -1;
		if (!windows_on_top) {
			our_root_win = game_root_win;
		}
		storage_win = create_window(win_storage, our_root_win, 0, storage_win_x, storage_win_y, storage_win_x_len, storage_win_y_len, ELW_WIN_DEFAULT | ELW_TITLE_NAME | ELW_RESIZEABLE);
		set_window_handler(storage_win, ELW_HANDLER_RESIZE, &resize_storage_handler);
		/* appel la fonction du resize pour valider les tailles & positions */
		resize_storage_handler(&windows_list.window[storage_win]);
		set_window_handler(storage_win, ELW_HANDLER_DISPLAY, &display_storage_handler);
		set_window_handler(storage_win, ELW_HANDLER_CLICK, &click_storage_handler);
		set_window_handler(storage_win, ELW_HANDLER_MOUSEOVER, &mouseover_storage_handler);
		set_window_handler(storage_win, ELW_HANDLER_KEYPRESS, &keypress_storage_handler);
		vscrollbar_add_extended(storage_win, STORAGE_SCROLLBAR_CATEGORIES, NULL, 240, 10, 20, storage_win_y_len - storage_textarea_size - 3 * 10, 0, storage_categories_size, 0.77f, 0.57f, 0.39f, 0, 1, no_storage_categories);
		vscrollbar_add_extended(storage_win, STORAGE_SCROLLBAR_ITEMS, NULL, 280 + storage_items_cols * 32, 10, 20, storage_items_rows * 32, 0, storage_items_rows, 0.77f, 0.57f, 0.39f, 0, 1, 0);
		cm_add(windows_list.window[storage_win].cm_id, cm_storage_menu_str, context_storage_handler);
		cm_add(windows_list.window[storage_win].cm_id, cm_dialog_options_str, context_storage_handler);
		// note : &sort_storage_categories et &autoclose_storage_dialogue inutiles (comportement par d�faut du serveur)
		cm_bool_line(windows_list.window[storage_win].cm_id, ELW_CM_MENU_LEN + 4, &allow_wheel_quantity_drag, NULL);
		cm_bool_line(windows_list.window[storage_win].cm_id, ELW_CM_MENU_LEN + 5, &disable_storage_filter, NULL);
		cm_bool_line(windows_list.window[storage_win].cm_id, ELW_CM_MENU_LEN + 6, &auto_select_storage_option, NULL);
	} else {
		no_storage = 0;
		for (i = 0; i < no_storage_categories; i++) {
			storage_categories[i].name[0] = to_color_char(c_orange1);
		}
		show_window(storage_win);
		select_window(storage_win);
		vscrollbar_set_pos(storage_win, STORAGE_SCROLLBAR_CATEGORIES, 0);
		vscrollbar_set_pos(storage_win, STORAGE_SCROLLBAR_ITEMS, 0);
	}
	storage_text[0] = '\0';
	set_window_name("", "");
}
void close_storagewin() {
	if (storage_win >= 0 && !view_only_storage) {
		hide_window(storage_win);
	}
}
