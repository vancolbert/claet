#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include "items.h"
#include "asc.h"
#include "cursors.h"
#include "context_menu.h"
#include "text.h"
#include "elconfig.h"
#include "elwindows.h"
#include "errors.h"
#include "gamewin.h"
#include "gl_init.h"
#include "global.h"
#include "hud.h"
#include "init.h"
#include "interface.h"
#include "item_info.h"
#include "item_lists.h"
#include "manufacture.h"
#include "misc.h"
#include "multiplayer.h"
#include "platform.h"
#include "sound.h"
#include "storage.h"
#include "textures.h"
#include "translate.h"
#include "counters.h"
#include "widgets.h"
#include "spells.h"
#include "fr_quickitems.h"
#include "themes.h"
item item_list[ITEM_NUM_ITEMS];
struct quantities quantities = {
	0,
	{
		{1, 1, "1"},
		{5, 1, "5"},
		{10, 2, "10"},
		{20, 2, "20"},
		{50, 2, "50"},
		{100, 3, "100"},
		{1, 1, "1"}, // item list preview quantity - not editable or displayed with others
	}
};
int edit_quantity = -1;
int item_action_mode = ACTION_WALK;
int items_win = -1;
int items_menu_x = 10;
int items_menu_y = 20;
int items_grid_size = 51; // Changes depending on the size of the root window (is 51 > 640x480 and 33 in 640x480).
int items_menu_x_len = 6 * 51 + 110;
int items_menu_y_len = 6 * 51 + 90;
int items_text[MAX_ITEMS_TEXTURES];
static char items_string[350] = {0};
static size_t last_items_string_id = 0;
int item_dragged = -1;
int item_quantity = 1;
int quantity_width = 0;
int allow_equip_swap = 0;
int use_item = -1;
int wear_items_x_offset = 6 * 51 + 20;
int wear_items_y_offset = 30;
int quantity_x_offset = 6 * 51 + 20;
int quantity_y_offset = 185;
int use_small_items_window = 0;
int manual_size_items_window = 0;
int item_uid_enabled = 0;
const Uint16 unset_item_uid = (Uint16) - 1;
#define NUMBUT 4
#define XLENBUT 29
#define YLENBUT 33
#undef NUMBUT
#define NUMBUT 5
static int but_y_off[NUMBUT] = {0, YLENBUT, YLENBUT * 2, YLENBUT * 3, YLENBUT * 4};
enum {BUT_GET, BUT_STORE, BUT_DROP, BUT_ITEM_LIST, BUT_MIX, };
int items_mix_but_all = 0;
int items_stoall_nofirstrow = 0;
int items_stoall_nolastrow = 0;
int items_dropall_nofirstrow = 0;
int items_dropall_nolastrow = 0;
int items_auto_get_all = 0;
int items_list_on_left = 0;
static const char *item_help_str = NULL;
static const char *item_desc_str = NULL;
static int mouse_over_but = -1;
static size_t cm_stoall_but = CM_INIT_VALUE;
static size_t cm_dropall_but = CM_INIT_VALUE;
static size_t cm_mix_but = CM_INIT_VALUE;
static size_t cm_getall_but = CM_INIT_VALUE;
static size_t cm_itemlist_but = CM_INIT_VALUE;
static int mouseover_item_pos = -1;
int items_stoall_nolastcol = 0;
int items_stoall_nofirstcol = 0;
int items_dropall_nolastcol = 0;
int items_dropall_nofirstcol = 0;
int allow_wheel_quantity_edit = 0;
int allow_wheel_quantity_drag = 0;
int last_quantity = 1; // mémorise la valeur initiale d'une quantité éditée pour annuler l'édition
int item_dragged_max_quantity = 1; // quantité max disponible pour un objet pris du sac
int wear_grid_size = 51;
static void equip_item(int item_pos_to_equip, int destination_pos);
static void drop_all_handler();
void set_shown_string(char colour_code, const char *the_text) {
	if (strlen(the_text) == 0) {
		inventory_item_string[0] = '\0';
		inventory_item_string_id++;
		return;
	}
	inventory_item_string[0] = to_color_char(colour_code);
	safe_strncpy2(inventory_item_string + 1, the_text, sizeof(inventory_item_string) - 2, strlen(the_text));
	inventory_item_string[sizeof(inventory_item_string) - 1] = 0;
	inventory_item_string_id++;
}
/* return index of button or -1 if mouse not over a button */
static int over_button(window_info *win, int mx, int my) {
	if (mx > (win->len_x - (XLENBUT + 3)) && mx < win->len_x - 3 && my > wear_items_y_offset && my < wear_items_y_offset + but_y_off[NUMBUT - 1] + YLENBUT) {
		return (my - wear_items_y_offset) / YLENBUT;
	}
	return -1;
}
void gray_out(int x_start, int y_start, int gridsize) {
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ZERO, GL_SRC_COLOR); // this brightens down
	glColor3f(0.4f, 0.2f, 0.2f);
	glBegin(GL_QUADS);
	glVertex3i(x_start, y_start, 0);
	glVertex3i(x_start + gridsize, y_start, 0);
	glVertex3i(x_start + gridsize, y_start + gridsize, 0);
	glVertex3i(x_start, y_start + gridsize, 0);
	glEnd();
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glColor3f(1.0f, 1.0f, 1.0f);
}
void rendergrid(int columns, int rows, int left, int top, int width, int height) {
	int x, y;
	int temp;
	glBegin(GL_LINES);
	for (y = 0; y <= rows; y++) {
		temp = top + y * height;
		glVertex2i(left, temp);
		glVertex2i(left + width * columns, temp);
	}
	for (x = 0; x < columns + 1; x++) {
		temp = left + x * width;
		glVertex2i(temp, top);
		glVertex2i(temp, top + height * rows);
	}
	glEnd();
}
int get_mouse_pos_in_grid(int mx, int my, int columns, int rows, int left, int top, int width, int height) {
	int x, y, i;
	mx -= left;
	my -= top;
	i = 0;
	for (y = 0; y < rows; y++) {
		for (x = 0; x < columns; x++, i++) {
			if (mx >= x * width && mx <= (x + 1) * width && my >= y * height && my <= (y + 1) * height) {
				return i;
			}
		}
	}
	return -1;
}
void reset_quantity(int pos) {
	int val;
	switch (pos) {
	case 0:
		val = 1;
		break;
	case 1:
		val = 5;
		break;
	case 2:
		val = 10;
		break;
	case 3:
		val = 20;
		break;
	case 4:
		val = 50;
		break;
	case 5:
		val = 100;
		break;
	default:
		LOG_ERROR("Trying to reset invalid element of quantities, pos = %d", pos);
		return;
	}
	safe_snprintf(quantities.quantity[pos].str, sizeof(quantities.quantity[pos].str), "%d", val);
	quantities.quantity[pos].len = strlen(quantities.quantity[pos].str);
	quantities.quantity[pos].val = val;
}
void get_item_uv(const Uint32 item, float *u_start, float *v_start, float *u_end, float *v_end) {
	*u_start = (50.0f / 256.0f) * (item % 5) + 0.5f / 256.0f;
	*u_end = *u_start + (50.0f / 256.0f);
	*v_start = (50.0f / 256.0f) * (item / 5) + 0.5f / 256.0f;
	*v_end = *v_start + (50.0f / 256.0f);
}
void drag_item(int item, int storage, int mini) {
	int cur_item;
	int quantity = quantities.quantity[quantities.selected].val;
	char str[20];
	// on désactive tout drag propre à la barre rapide si un autre drag existe
	fr_quickitem_dragged = -1;
	if (storage) {
		if (item < 0 || item >= STORAGE_ITEMS_SIZE) {
			// oops
			return;
		}
		cur_item = storage_items[item].image_id;
		if (!storage_items[item].quantity) {
			storage_item_dragged = -1;
			return;
		}
		if (quantity > storage_items[item].quantity) {
			quantity = storage_items[item].quantity;
		}
	} else {
		if (item < 0 || item >= ITEM_NUM_ITEMS) {
			// oops
			return;
		}
		cur_item = item_list[item].image_id;
		if (!item_list[item].quantity) {
			item_dragged = -1;
			return;
		}
		/* grace à item_dragged_quantity_max on peut afficher la quantité (stackable ou non) */
		if (quantity > item_dragged_max_quantity) {
			quantity = item_dragged_max_quantity;
		}
	}
	/* autant réutiliser la fonction draw_item */
	if (mini) {
		draw_item(cur_item, mouse_x - 16, mouse_y - 16, 33);
	} else {
		draw_item(cur_item, mouse_x - 25, mouse_y - 25, 51);
	}
	if (!mini && quantity != -1) {
		safe_snprintf(str, sizeof(str), "%i", quantity);
		draw_string_small(mouse_x - 25, mouse_y + 10, (unsigned char *)str, 1);
	}
}
void get_your_items(const Uint8 *data) {
	int i, total_items, pos, len;
	Uint8 flags;
	if (item_uid_enabled) {
		len = 10;
	} else {
		len = 8;
	}
	// data[0] -> num_items
	// data[1] -> image_id
	// data[3] -> quantity
	// data[7] -> pos
	// data[8] -> flags
	// data[9] -> id
	total_items = data[0];
	// clear the items first
	for (i = 0; i < ITEM_NUM_ITEMS; i++) {
		item_list[i].quantity = 0;
	}
	for (i = 0; i < total_items; i++) {
		pos = data[i * len + 1 + 6];
		// try not to wipe out cooldown information if no real change
		if (item_list[pos].image_id != SDL_SwapLE16(*((Uint16 *)(data + i * len + 1)))) {
			item_list[pos].cooldown_time = 0;
			item_list[pos].cooldown_rate = 1;
		}
		item_list[pos].image_id = SDL_SwapLE16(*((Uint16 *)(data + i * len + 1)));
		item_list[pos].quantity = SDL_SwapLE32(*((Uint32 *)(data + i * len + 1 + 2)));
		item_list[pos].pos = pos;
		item_list[pos].action = ITEM_NO_ACTION;
		item_list[pos].action_time = 0;
		flags = data[i * len + 1 + 7];
		if (item_uid_enabled) {
			item_list[pos].id = SDL_SwapLE16(*((Uint16 *)(data + i * len + 1 + 8)));
		} else {
			item_list[pos].id = unset_item_uid;
		}
		item_list[pos].is_resource = ((flags & ITEM_RESOURCE) > 0);
		item_list[pos].is_reagent = ((flags & ITEM_REAGENT) > 0);
		item_list[pos].use_with_inventory = ((flags & ITEM_INVENTORY_USABLE) > 0);
		item_list[pos].is_stackable = ((flags & ITEM_STACKABLE) > 0);
	}
	build_fr_quickitems(0);
	build_manufacture_list();
	check_castability();
}
void check_for_item_sound(int pos) {
	int i, snd = -1;
	if (item_list[pos].action != ITEM_NO_ACTION) {
		// Play the sound that goes with this action
		switch (item_list[pos].action) {
		case USE_INVENTORY_ITEM:
			snd = get_index_for_inv_use_item_sound(item_list[pos].image_id);
			break;
		case ITEM_ON_ITEM:
			// Find the second item (being used with)
			for (i = 0; i < ITEM_NUM_ITEMS; i++) {
				if (i != pos && item_list[i].action == ITEM_ON_ITEM) {
					snd = get_index_for_inv_usewith_item_sound(item_list[pos].image_id, item_list[i].action);
					break;
				}
			}
			break;
		}
		if (snd > -1) {
			add_sound_object(snd, 0, 0, 1);
		}
		// Reset the action
		item_list[pos].action = ITEM_NO_ACTION;
		item_list[pos].action_time = 0;
	}
}
void update_item_sound(int interval) {
	int i;
	// Iterate through the list of items, checking for out of date item actions (> 2 sec old)
	for (i = 0; i < ITEM_NUM_ITEMS; i++) {
		if (item_list[i].action != ITEM_NO_ACTION) {
			item_list[i].action_time += interval;
			if (item_list[i].action_time >= 2000) {
				// Item action state is out of date so reset it
				item_list[i].action = ITEM_NO_ACTION;
				item_list[i].action_time = 0;
			}
		}
	}
}
void remove_item_from_inventory(int pos) {
	item_list[pos].quantity = 0;
	check_for_item_sound(pos);
	build_fr_quickitems(0);
	build_manufacture_list();
	check_castability();
}
void get_new_inventory_item(const Uint8 *data) {
	int pos;
	Uint8 flags;
	int quantity;
	int image_id;
	Uint16 id;
	if (item_uid_enabled) {
		id = SDL_SwapLE16(*((Uint16 *)(data + 8)));
	} else {
		id = unset_item_uid;
	}
	pos = data[6];
	flags = data[7];
	image_id = SDL_SwapLE16(*((Uint16 *)(data)));
	quantity = SDL_SwapLE32(*((Uint32 *)(data + 2)));
	/* Tosh : on ne compte pas les Lumens dans les compteurs de récolte (image_id=3) */
	if (now_harvesting() && (quantity >= item_list[pos].quantity) && image_id != 3) {
		increment_harvest_counter(item_list[pos].quantity > 0 ? quantity - item_list[pos].quantity : quantity);
	}
	// don't touch cool down when it's already active
	if (item_list[pos].quantity == 0 || item_list[pos].image_id != image_id) {
		item_list[pos].cooldown_time = 0;
		item_list[pos].cooldown_rate = 1;
	}
	item_list[pos].quantity = quantity;
	item_list[pos].image_id = image_id;
	item_list[pos].pos = pos;
	item_list[pos].id = id;
	item_list[pos].is_resource = ((flags & ITEM_RESOURCE) > 0);
	item_list[pos].is_reagent = ((flags & ITEM_REAGENT) > 0);
	item_list[pos].use_with_inventory = ((flags & ITEM_INVENTORY_USABLE) > 0);
	item_list[pos].is_stackable = ((flags & ITEM_STACKABLE) > 0);
	check_for_item_sound(pos);
	build_fr_quickitems(0);
	build_manufacture_list();
	check_castability();
}
void draw_item(int id, int x_start, int y_start, int gridsize) {
	float u_start, v_start, u_end, v_end;
	int cur_item;
	int this_texture;
	// get the UV coordinates.
	cur_item = id % 25;
	get_item_uv(cur_item, &u_start, &v_start, &u_end, &v_end);
	// get the texture this item belongs to
	this_texture = get_items_texture(id / 25);
	bind_texture(this_texture);
	glBegin(GL_QUADS);
	draw_2d_thing(u_start, v_start, u_end, v_end, x_start, y_start, x_start + gridsize - 1, y_start + gridsize - 1);
	glEnd();
}
int display_items_handler(window_info *win) {
	char str[80];
	char my_str[10];
	int x, y, i;
	int item_is_weared = 0;
	Uint32 _cur_time = SDL_GetTicks(); /* grab a snapshot of current time */
	char *but_labels[NUMBUT] = {get_all_str, sto_all_str, drp_all_str, itm_lst_str, NULL};
	glEnable(GL_TEXTURE_2D);
	// draw the button labels
	but_labels[BUT_MIX] = (items_mix_but_all) ?mix_all_str :mix_one_str;
	for (i = 0; i < NUMBUT; i++) {
		strap_word(but_labels[i], my_str);
		glColor3f(0.77f, 0.57f, 0.39f);
		draw_string_small(win->len_x + gx_adjust - XLENBUT, wear_items_y_offset + but_y_off[i] + 2 + gy_adjust, (unsigned char *)my_str, 2);
	}
	x = quantity_x_offset + quantity_width / 2;
	y = quantity_y_offset + 3;
	glColor3f(0.3f, 0.5f, 1.0f);
	for (i = 0; i < ITEM_EDIT_QUANT; x += quantity_width, ++i) {
		if (i == edit_quantity) {
			glColor3f(1.0f, 0.0f, 0.3f);
			draw_string_small(1 + gx_adjust + x - strlen(quantities.quantity[i].str) * 4, y + gy_adjust, (unsigned char *)quantities.quantity[i].str, 1);
			glColor3f(0.3f, 0.5f, 1.0f);
		} else if (i == quantities.selected) {
			glColor3f(0.0f, 1.0f, 0.3f);
			draw_string_small(1 + gx_adjust + x - strlen(quantities.quantity[i].str) * 4, y + gy_adjust, (unsigned char *)quantities.quantity[i].str, 1);
			glColor3f(0.3f, 0.5f, 1.0f);
		} else {
			draw_string_small(1 + gx_adjust + x - strlen(quantities.quantity[i].str) * 4, y + gy_adjust, (unsigned char *)quantities.quantity[i].str, 1);
		}
	}
	draw_string_small(6 * quantity_width - strlen(quantity_str) * 8 - 2, quantity_y_offset - 18, (unsigned char *)quantity_str, 1);
	glColor3f(0.57f, 0.67f, 0.49f);
	draw_string_small(wear_items_x_offset + wear_grid_size - (8 * strlen(equip_str)) / 2 + 1, wear_items_y_offset - 18, (unsigned char *)equip_str, 1);
	glColor3f(1.0f, 1.0f, 1.0f);
	// ok, now let's draw the objects...
	for (i = ITEM_NUM_ITEMS - 1; i >= 0; i--) {
		if (item_list[i].quantity) {
			int cur_pos;
			int x_start, x_end, y_start, y_end;
			// get the x and y
			cur_pos = i;
			if (cur_pos >= ITEM_WEAR_START) { // the items we 'wear' are smaller
				cur_pos -= ITEM_WEAR_START;
				item_is_weared = 1;
				x_start = wear_items_x_offset + wear_grid_size * (cur_pos % 2) + 1;
				x_end = x_start + wear_grid_size - 1;
				y_start = wear_items_y_offset + wear_grid_size * (cur_pos / 2);
				y_end = y_start + wear_grid_size - 1;
				draw_item(item_list[i].image_id, x_start, y_start, wear_grid_size);
			} else {
				item_is_weared = 0;
				x_start = items_grid_size * (cur_pos % 6) + 1;
				x_end = x_start + items_grid_size - 1;
				y_start = items_grid_size * (cur_pos / 6);
				y_end = y_start + items_grid_size - 1;
				draw_item(item_list[i].image_id, x_start, y_start, items_grid_size - 1);
			}
			if (item_list[i].cooldown_time > _cur_time) {
				float cooldown = ((float)(item_list[i].cooldown_time - _cur_time)) / ((float)item_list[i].cooldown_rate);
				float x_center = (x_start + x_end) * 0.5f;
				float y_center = (y_start + y_end) * 0.5f;
				float flash_effect_offset = 0.0f;
				if (cooldown < 0.0f) {
					cooldown = 0.0f;
				} else if (cooldown > 1.0f) {
					cooldown = 1.0f;
				}
				glDisable(GL_TEXTURE_2D);
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glBegin(GL_TRIANGLE_FAN);
				if (cooldown < 1.0f) {
					flash_effect_offset = sin(pow(1.0f - cooldown, 4.0f) * 2.0f * M_PI * 30.0);
				}
				glColor4f(fr_quickitems_coolcolor.rouge - flash_effect_offset / 20.0f, fr_quickitems_coolcolor.vert - flash_effect_offset / 20.0f, fr_quickitems_coolcolor.bleu + flash_effect_offset / 8.0f, fr_quickitems_coolcolor.alpha + flash_effect_offset / 15.0f);
				glVertex2f(x_center, y_center);
				if (cooldown >= 0.875f) {
					float t = tan(2.0f * M_PI * (1.0f - cooldown));
					glVertex2f(t * x_end + (1.0f - t) * x_center, y_start);
					glVertex2f(x_end, y_start);
					glVertex2f(x_end, y_end);
					glVertex2f(x_start, y_end);
					glVertex2f(x_start, y_start);
				} else if (cooldown >= 0.625f) {
					float t = 0.5f + 0.5f * tan(2.0f * M_PI * (0.75f - cooldown));
					glVertex2f(x_end, t * y_end + (1.0f - t) * y_start);
					glVertex2f(x_end, y_end);
					glVertex2f(x_start, y_end);
					glVertex2f(x_start, y_start);
				} else if (cooldown >= 0.375f) {
					float t = 0.5f + 0.5f * tan(2.0f * M_PI * (0.5f - cooldown));
					glVertex2f(t * x_start + (1.0f - t) * x_end, y_end);
					glVertex2f(x_start, y_end);
					glVertex2f(x_start, y_start);
				} else if (cooldown >= 0.125f) {
					float t = 0.5f + 0.5f * tan(2.0f * M_PI * (0.25f - cooldown));
					glVertex2f(x_start, t * y_start + (1.0f - t) * y_end);
					glVertex2f(x_start, y_start);
				} else {
					float t = tan(2.0f * M_PI * (cooldown));
					glVertex2f(t * x_start + (1.0f - t) * x_center, y_start);
				}
				glVertex2f(x_center, y_start);
				glEnd();
				glDisable(GL_BLEND);
				glEnable(GL_TEXTURE_2D);
			}
			if (!item_is_weared) {
				safe_snprintf(str, sizeof(str), "%i", item_list[i].quantity);
				if ((mouseover_item_pos == i) && enlarge_text()) {
					draw_string_shadowed(x_start, (i & 1)?(y_end - 15):(y_end - 25), (unsigned char *)str, 1, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
				} else {
					draw_string_small_shadowed(x_start, (i & 1)?(y_end - 15):(y_end - 25), (unsigned char *)str, 1, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f);
				}
			}
		}
	}
	mouseover_item_pos = -1;
	glColor3f(1.0f, 1.0f, 1.0f);
	// draw the load string
	if (!use_small_items_window) {
		safe_snprintf(str, sizeof(str), "%s:", attributes.carry_capacity.shortname);
		draw_string_small(items_grid_size * 6 + 6, items_grid_size * 6 - SMALL_FONT_Y_LEN * 2, (unsigned char *)str, 1);
		safe_snprintf(str, sizeof(str), "%i/%i", your_info.carry_capacity.cur, your_info.carry_capacity.base);
		draw_string_small(items_grid_size * 6 + 6, items_grid_size * 6 - SMALL_FONT_Y_LEN, (unsigned char *)str, 1);
	} else {
		safe_snprintf(str, sizeof(str), "%s: %i/%i", attributes.carry_capacity.shortname, your_info.carry_capacity.cur, your_info.carry_capacity.base);
		if (win->len_y > items_grid_size * 6 + 25 + 15) {
			draw_string_small(2, quantity_y_offset - 19, (unsigned char *)str, 1);
		}
	}
	// now, draw the inventory text, if any.
	if (last_items_string_id != inventory_item_string_id) {
		put_small_text_in_box((unsigned char *)inventory_item_string, strlen(inventory_item_string), win->len_x - 2, items_string);
		last_items_string_id = inventory_item_string_id;
	}
	i = (win->len_y - items_grid_size * 6 - 25) / 15 - use_small_items_window;
	if (i > 0) {
		draw_string_small(4, items_grid_size * 6 + 5, (unsigned char *)items_string, i);
	}
	// Render the grid *after* the images. It seems impossible to code
	// it such that images are rendered exactly within the boxes on all
	// cards
	glDisable(GL_TEXTURE_2D);
	glColor3f(0.77f, 0.57f, 0.39f);
	// draw the grids
	rendergrid(6, 6, 0, 0, items_grid_size, items_grid_size);
	glColor3f(0.57f, 0.67f, 0.49f);
	rendergrid(2, 4, wear_items_x_offset, wear_items_y_offset, wear_grid_size, wear_grid_size);
	// draw the button boxes
	glColor3f(0.77f, 0.57f, 0.39f);
	for (i = 0; i < NUMBUT; i++) {
		glBegin(GL_LINE_LOOP);
		glVertex3i(win->len_x - 3, wear_items_y_offset + but_y_off[i], 0);
		glVertex3i(win->len_x - (XLENBUT + 3), wear_items_y_offset + but_y_off[i], 0);
		glVertex3i(win->len_x - (XLENBUT + 3), wear_items_y_offset + but_y_off[i] + YLENBUT, 0);
		glVertex3i(win->len_x - 3, wear_items_y_offset + but_y_off[i] + YLENBUT, 0);
		glEnd();
	}
	// highlight a button with the mouse over
	if (mouse_over_but != -1) {
		glColor3f(0.99f, 0.77f, 0.55f);
		glBegin(GL_LINE_LOOP);
		glVertex3i(win->len_x - 2, wear_items_y_offset + but_y_off[mouse_over_but] - 1, 0);
		glVertex3i(win->len_x - (XLENBUT + 4), wear_items_y_offset + but_y_off[mouse_over_but] - 1, 0);
		glVertex3i(win->len_x - (XLENBUT + 4), wear_items_y_offset + but_y_off[mouse_over_but] + YLENBUT + 1, 0);
		glVertex3i(win->len_x - 2, wear_items_y_offset + but_y_off[mouse_over_but] + YLENBUT + 1, 0);
		glEnd();
	}
	// draw the unwear button boxes
	glColor3f(0.77f, 0.57f, 0.39f);
	glEnable(GL_TEXTURE_2D);
	draw_string_small(wear_items_x_offset + wear_grid_size * 0.5 - 3 * 4, wear_items_y_offset + wear_grid_size * 4 + 10, (unsigned char *)" < ", 2);
/*
        TODO: Bouton désactivé en attente d'une amélioration serveur sur le DEPOSITE_ITEM
        draw_string_small(wear_items_x_offset+wear_grid_size*1.5-3*4, wear_items_y_offset+wear_grid_size*4+10, (unsigned char*)"<<<", 2);
 */
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINE_LOOP);
	glVertex3i(wear_items_x_offset + 4, wear_items_y_offset + wear_grid_size * 4 + 10, 0);
	glVertex3i(wear_items_x_offset + wear_grid_size - 4, wear_items_y_offset + wear_grid_size * 4 + 10, 0);
	glVertex3i(wear_items_x_offset + wear_grid_size - 4, wear_items_y_offset + wear_grid_size * 4 + 25, 0);
	glVertex3i(wear_items_x_offset + 4, wear_items_y_offset + wear_grid_size * 4 + 25, 0);
	glEnd();
/*
        TODO: Bouton désactivé en attente d'une amélioration serveur sur le DEPOSITE_ITEM
        glBegin(GL_LINE_LOOP);
                glVertex3i(wear_items_x_offset+wear_grid_size+4,  wear_items_y_offset+wear_grid_size*4+10,0);
                glVertex3i(wear_items_x_offset+wear_grid_size*2-4, wear_items_y_offset+wear_grid_size*4+10,0);
                glVertex3i(wear_items_x_offset+wear_grid_size*2-4, wear_items_y_offset+wear_grid_size*4+25,0);
                glVertex3i(wear_items_x_offset+wear_grid_size+4,  wear_items_y_offset+wear_grid_size*4+25,0);
        glEnd();
 */
	// now, draw the quantity boxes
	glColor3f(0.3f, 0.5f, 1.0f);
	rendergrid(ITEM_EDIT_QUANT, 1, quantity_x_offset, quantity_y_offset, quantity_width, 20);
	glEnable(GL_TEXTURE_2D);
	// display help text for button if mouse over one
	if ((mouse_over_but != -1) && show_help_text) {
		char *helpstr[NUMBUT] = {getall_help_str, stoall_help_str, ((disable_double_click) ?drpall_help_str :dcdrpall_help_str), itmlst_help_str, mixoneall_help_str};
		show_help(helpstr[mouse_over_but], 0, win->len_y + 10);
		show_help(cm_help_options_str, 0, win->len_y + 10 + SMALL_FONT_Y_LEN);
	}
	// show help set in the mouse_over handler
	else {
		int offset = 10;
		if (show_help_text && (item_help_str != NULL)) {
			show_help(item_help_str, 0, win->len_y + offset);
			offset += SMALL_FONT_Y_LEN;
		}
		if (item_desc_str != NULL) {
			show_help(item_desc_str, 260, win->len_y + offset - 15);
		}
		item_help_str = NULL;
		item_desc_str = NULL;
	}
	mouse_over_but = -1;
	return 1;
}
/* return 1 if sent the move command */
int move_item(int item_pos_to_mov, int destination_pos) {
	int drop_on_stack = 0;
	int i;
	int temp_case = -1;
	int temp_case2 = -1;
	/* source provenant des équipements et destination également */
	if ((item_pos_to_mov >= ITEM_WEAR_START) && (destination_pos >= ITEM_WEAR_START)) {
		// équipement vers lui-même = doucle-clic : on tente de le déséquiper
		if (item_pos_to_mov == destination_pos) {
			return move_item(item_pos_to_mov, 0);
		}
		// si la source est empilable on ne pourra assurer sa place une fois déséquipée : abandon
		// TODO: à moins de tenter la recherche dans l'inventaire par item_uid
		if (item_list[item_pos_to_mov].is_stackable) {
			return 0;
		}
		// destination occupée
		if (item_list[destination_pos].quantity) {
			// si la destination est empilable on ne pourra assurer sa place une fois déséquipée : abandon
			// TODO: à moins de tenter la recherche dans l'inventaire par item_uid
			if (item_list[destination_pos].is_stackable) {
				return 0;
			}
			// il faut pouvoir déséquiper les 2 objets pour les remettre l'un à la place de l'autre
			for (i = 0; i < ITEM_WEAR_START; i++) {
				if (!item_list[i].quantity) {
					if (temp_case < 0) {
						temp_case = i;
					} else {
						temp_case2 = i;
						break;
					}
				}
			}
			if (temp_case < 0) {
				return 0;
			}
			if (temp_case2 < 0) {
				return 0;
			}
			// nous pouvons opérer l'échange des 2 équipements
			equip_item(destination_pos, temp_case);
			item_list[temp_case].pos = temp_case;
			equip_item(item_pos_to_mov, temp_case2);
			item_list[temp_case2].pos = temp_case2;
			equip_item(temp_case2, destination_pos);
			equip_item(temp_case, item_pos_to_mov);
			return 1;
		}
		// destination libre
		else {
			for (i = 0; i < ITEM_WEAR_START; i++) {
				if (!item_list[i].quantity) {
					temp_case = i;
					break;
				}
			}
			if (temp_case < 0) {
				return 0;
			}
			equip_item(item_pos_to_mov, temp_case);
			item_list[temp_case].pos = temp_case;
			equip_item(temp_case, destination_pos);
			return 1;
		}
	} else
	/* if the dragged item is equipped and the destintion is occupied, try to find another slot */
	if ((item_pos_to_mov >= ITEM_WEAR_START) && (item_list[destination_pos].quantity)) {
		int i;
		int have_free_pos = 0;
		/* find first free slot, use a free slot in preference to a stack as the server does the stacking */
		for (i = 0; i < ITEM_WEAR_START; i++) {
			if (!item_list[i].quantity) {
				destination_pos = i;
				have_free_pos = 1;
				break;
			}
		}
		/* Si aucune case libre mais objet empillable, on le cherche dans l'inventaire avec son uid */
		if (!have_free_pos && item_list[item_pos_to_mov].is_stackable && (item_list[item_pos_to_mov].id != unset_item_uid)) {
			for (i = 0; i < ITEM_WEAR_START; i++) {
				if (item_list[i].id == item_list[item_pos_to_mov].id) {
					destination_pos = i;
					have_free_pos = 1;
					break;
				}
			}
		}
		/* if no free slot, try to find an existing stack.  But be careful of dupe image ids */
		if (!have_free_pos && item_list[item_pos_to_mov].is_stackable) {
			int num_stacks_found = 0;
			int proposed_slot = -1;
			for (i = 0; i < ITEM_WEAR_START; i++) {
				if (item_list[i].is_stackable && (item_list[i].image_id == item_list[item_pos_to_mov].image_id)) {
					num_stacks_found++;
					proposed_slot = i;
				}
			}
			/* only use the stack if  we're sure there are no other possibilities */
			if (num_stacks_found == 1) {
				destination_pos = proposed_slot;
				drop_on_stack = 1;
			} else {
				set_shown_string(c_red2, items_stack_str);
			}
			/*  This still leaves one possibility for the dreaded server accusation.
			        If we have no free inventory slots, one or more stackable items
			        unequipped, and a single, different equipped item with the same id as
			        the aforementioned stack.  When we try to unequip the single item, the
			        client tries to place it on that stack. This may mean we have to
			        abandon this feature; i.e. allowing a stackable item to be unequipping
			        when there are no free slots. (pjbroad/bluap) */
		}
	}
	/* source provenant du sac et destination (sac ou équipement) déjà occupée par un autre objet */
	else if ((item_list[destination_pos].quantity > 0) && (item_pos_to_mov != destination_pos)) {
		// destination dans les équipements sur un objet empilable
		if ((destination_pos >= ITEM_WEAR_START) && (item_list[destination_pos].is_stackable)) {
			// échange de place impossible car le serveur risque de déséquiper sur un stack
			// on se contente alors de la méthode d'origine (échange d'équipement mais pas de place)
			if (!move_item(destination_pos, 0)) {
				return 0;
			}
			equip_item(item_pos_to_mov, destination_pos);
			return 1;
		}
		// pour le reste il faut trouver une place libre dans le sac pour l'échange
		for (i = 0; i < ITEM_WEAR_START; i++) {
			if (!item_list[i].quantity) {
				temp_case = i;
				break;
			}
		}
		if (temp_case < 0) {
			return 0;
		}
		// destination dans le sac : libération de la case et drop du drag
		equip_item(destination_pos, temp_case);
		item_list[temp_case].pos = temp_case;
		if ((destination_pos >= ITEM_WEAR_START) && (item_list[item_pos_to_mov].quantity > 1)) {
			// s'il s'agit d'équiper un objet empilable présent en plusieurs exemplaire
			// on ne tentera pas en vain de replacer sur une case qui ne sera pas libre
			equip_item(item_pos_to_mov, destination_pos);
		} else {
			// néanmoins le replacement pourra encore échoué dans certains cas
			// s'il s'agit d'équiper un objet ne pouvant être porté... tant pis !
			equip_item(item_pos_to_mov, destination_pos);
			equip_item(temp_case, item_pos_to_mov);
		}
		return 1;
	}
	/* move item */
	if (drop_on_stack || !item_list[destination_pos].quantity) {
		Uint8 str[20];
		// send the drop info to the server
		str[0] = MOVE_INVENTORY_ITEM;
		str[1] = item_list[item_pos_to_mov].pos;
		str[2] = destination_pos;
		my_tcp_send(my_socket, str, 3);
		return 1;
	} else {
		return 0;
	}
}
static void equip_item(int item_pos_to_equip, int destination_pos) {
	Uint8 str[20];
	// send the drop info to the server
	str[0] = MOVE_INVENTORY_ITEM;
	str[1] = item_list[item_pos_to_equip].pos;
	str[2] = destination_pos;
	my_tcp_send(my_socket, str, 3);
}
void wheel_change_quantity(Uint32 flags) {
	int ctrl_on = flags & ELW_CTRL;
	int shift_on = flags & ELW_SHIFT;
	int step = 1;
	if (flags & ELW_WHEEL_UP) {
		if (!ctrl_on || !shift_on) {
			step = ceil(pow(10, quantities.quantity[quantities.selected].len) / 100);
			if (ctrl_on) {
				step = ceil(step / 10);
			}
			if (shift_on) {
				step = step * 10;
			}
		}
		quantities.quantity[quantities.selected].val += step;
		if ((item_dragged != -1) && (quantities.quantity[quantities.selected].val > item_dragged_max_quantity)) {
			quantities.quantity[quantities.selected].val = item_dragged_max_quantity;
		}
		if (quantities.quantity[quantities.selected].val > 100000) {
			quantities.quantity[quantities.selected].val = 100000;
		}
	} else {
		// si on décrémente la quantité d'un objet venant du sac, on part au max de sa quantité totale dispo
		if ((item_dragged != -1) && (quantities.quantity[quantities.selected].val > item_dragged_max_quantity)) {
			quantities.quantity[quantities.selected].val = item_dragged_max_quantity;
			sprintf(quantities.quantity[quantities.selected].str, "%i", quantities.quantity[quantities.selected].val);
			quantities.quantity[quantities.selected].len = strlen(quantities.quantity[quantities.selected].str);
		}
		// si on décrémente la quantité d'un objet venant du dépot, on part au max de sa quantité totale dispo
		else if ((storage_item_dragged != -1) && (quantities.quantity[quantities.selected].val > storage_items[storage_item_dragged].quantity)) {
			quantities.quantity[quantities.selected].val = storage_items[storage_item_dragged].quantity;
			sprintf(quantities.quantity[quantities.selected].str, "%i", quantities.quantity[quantities.selected].val);
			quantities.quantity[quantities.selected].len = strlen(quantities.quantity[quantities.selected].str);
		}
		if (!ctrl_on || !shift_on) {
			step = ceil(pow(10, quantities.quantity[quantities.selected].len) / 100);
			if (ctrl_on) {
				step = ceil(step / 10);
			}
			if (shift_on) {
				step = step * 10;
			}
		}
		if (quantities.quantity[quantities.selected].val > step) {
			quantities.quantity[quantities.selected].val -= step;
		} else {
			quantities.quantity[quantities.selected].val = 1;
		}
	}
	sprintf(quantities.quantity[quantities.selected].str, "%i", quantities.quantity[quantities.selected].val);
	quantities.quantity[quantities.selected].len = strlen(quantities.quantity[quantities.selected].str);
	item_quantity = quantities.quantity[quantities.selected].val;
}
int click_items_handler(window_info *win, int mx, int my, Uint32 flags) {
	Uint8 str[100];
	int right_click = flags & ELW_RIGHT_MOUSE;
	int ctrl_on = flags & ELW_CTRL;
	int shift_on = flags & ELW_SHIFT;
	if (flags & ELW_WHEEL) {
		// modification de la quantité de la case en cours d'édition avec la molette
		if ((allow_wheel_quantity_edit) && (edit_quantity != -1)) {
			quantities.selected = edit_quantity;
			wheel_change_quantity(flags);
			return 1;
		}
		// modification de la quantité de l'objet en cours de drag avec la molette
		else if ((allow_wheel_quantity_drag) && ((item_dragged != -1) || (storage_item_dragged != -1))) {
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
	// only handle mouse button clicks, not scroll wheels moves (unless its the mix button)
	if (((flags & ELW_MOUSE_BUTTON) == 0) && (over_button(win, mx, my) != BUT_MIX)) {
		return 0;
	}
	// ignore middle mouse button presses
	if ((flags & ELW_MID_MOUSE) != 0) {
		return 0;
	}
	if (!right_click && over_button(win, mx, my) != -1) {
		do_click_sound();
	}
	if (right_click) {
		if (item_dragged != -1 || use_item != -1 || storage_item_dragged != -1) {
			use_item = -1;
			item_dragged = -1;
			storage_item_dragged = -1;
			item_action_mode = ACTION_WALK;
			return 1;
		}
		if (mx >= wear_items_x_offset && mx < wear_items_x_offset + 2 * wear_grid_size && my >= wear_items_y_offset && my < wear_items_y_offset + 4 * wear_grid_size) {
			switch (item_action_mode) {
			case ACTION_WALK:
				item_action_mode = ACTION_LOOK;
				break;
			case ACTION_LOOK:
			default:
				item_action_mode = ACTION_WALK;
			}
			return 1;
		} else if (mx >= quantity_x_offset && mx < quantity_x_offset + ITEM_EDIT_QUANT * quantity_width && my >= quantity_y_offset && my < quantity_y_offset + 20) {
			// fall through...
		} else {
			switch (item_action_mode) {
			case ACTION_WALK:
				item_action_mode = ACTION_LOOK;
				break;
			case ACTION_LOOK:
				item_action_mode = ACTION_USE;
				break;
			case ACTION_USE:
				item_action_mode = ACTION_USE_WITEM;
				break;
			case ACTION_USE_WITEM:
				item_action_mode = ACTION_WALK;
				break;
			default:
				item_action_mode = ACTION_WALK;
			}
			return 1;
		}
	}
	if (item_action_mode == ACTION_USE_WITEM) {
		action_mode = ACTION_USE_WITEM;
	}
	if (item_action_mode == ACTION_USE) {
		action_mode = ACTION_USE;
	}
	// see if we changed the quantity
	if (mx >= quantity_x_offset && mx < quantity_x_offset + ITEM_EDIT_QUANT * quantity_width && my >= quantity_y_offset && my < quantity_y_offset + 20) {
		int pos = get_mouse_pos_in_grid(mx, my, ITEM_EDIT_QUANT, 1, quantity_x_offset, quantity_y_offset, quantity_width, 20);
		if (pos == -1) {} else if (flags & ELW_LEFT_MOUSE) {
			if (edit_quantity != -1) {
				if (!quantities.quantity[edit_quantity].len) {
					// Reset the quantity
					reset_quantity(edit_quantity);
				}
				edit_quantity = -1;
			}
			item_quantity = quantities.quantity[pos].val;
			quantities.selected = pos;
		} else if (right_click) {
			// si cette quantité est déjà en cours d'édition
			// le clic-droit annule l'édition en remettant la valeur par défaut
			if (edit_quantity == pos) {
				// Reset the quantity
				safe_snprintf(quantities.quantity[pos].str, sizeof(quantities.quantity[pos].str), "%d", last_quantity);
				quantities.quantity[pos].len = strlen(quantities.quantity[pos].str);
				quantities.quantity[pos].val = last_quantity;
				edit_quantity = -1;
				return 1;
			}
			last_quantity = quantities.quantity[pos].val;
			quantities.selected = pos;
			// Edit the given quantity
			edit_quantity = pos;
		}
		return 1;
	}
	if (edit_quantity != -1) {
		if (!quantities.quantity[edit_quantity].len) {
			reset_quantity(edit_quantity);
		}
		item_quantity = quantities.quantity[edit_quantity].val;
		quantities.selected = edit_quantity;
		edit_quantity = -1;
	}
	// see if we clicked on any item in the main category
	else if (mx > 0 && mx < 6 * items_grid_size && my > 0 && my < 6 * items_grid_size) {
		int pos = get_mouse_pos_in_grid(mx, my, 6, 6, 0, 0, items_grid_size, items_grid_size);
		item_list[pos].action = ITEM_NO_ACTION;
		item_list[pos].action_time = 0;
		if (pos == -1) {} else if (item_dragged != -1) {
			if (item_dragged == pos) { // let's try auto equip
				int i;
				for (i = ITEM_WEAR_START; i < ITEM_WEAR_START + 8; i++) {
					if (item_list[i].quantity < 1) {
						move_item(pos, i);
						item_dragged = -1;
						break;
					}
				}
			} else {
				if (move_item(item_dragged, pos)) {
					do_drop_item_sound();
				} else {
					do_alert1_sound();
				}
				item_dragged = -1;
			}
		} else if (storage_item_dragged != -1) {
			str[0] = WITHDRAW_ITEM;
			*((Uint16 *)(str + 1)) = SDL_SwapLE16(storage_items[storage_item_dragged].pos);
			*((Uint32 *)(str + 3)) = SDL_SwapLE32(item_quantity);
			my_tcp_send(my_socket, str, 6);
			do_drop_item_sound();
			if (storage_items[storage_item_dragged].quantity <= item_quantity) {
				storage_item_dragged = -1;
			}
		} else if (item_list[pos].quantity) {
			if (ctrl_on && shift_on) {
				// utilisation de Ctrl+Shift au lieu de Ctrl pour déposer l'objet au sol
				str[0] = DROP_ITEM;
				str[1] = item_list[pos].pos;
				if (item_list[pos].is_stackable) {
					*((Uint32 *)(str + 2)) = SDL_SwapLE32(item_list[pos].quantity);
				} else {
					*((Uint32 *)(str + 2)) = SDL_SwapLE32(36); // Drop all
				}
				my_tcp_send(my_socket, str, 6);
				do_drop_item_sound();
			} else if (shift_on) {
				// ajoute l'objet en raccourci sur le 1er emplacement libre de la barre rapide
				affect_fr_quickitems(item_list[pos].pos);
			} else if (ctrl_on) {
				// range cet objet (toute quantité dispo dans le sac) dans le dépot
				if ((storage_win < 0) || view_only_storage || !get_show_window(storage_win)) {
					return 0;
				}
				str[0] = DEPOSITE_ITEM;
				str[1] = pos;
				if (item_list[pos].is_stackable) {
					*((Uint32 *)(str + 2)) = SDL_SwapLE32(item_list[pos].quantity);
				} else {
					*((Uint32 *)(str + 2)) = SDL_SwapLE32(36);
				}
				my_tcp_send(my_socket, str, 6);
				do_drop_item_sound();
			} else if (item_action_mode == ACTION_LOOK) {
				click_time = cur_time;
				str[0] = LOOK_AT_INVENTORY_ITEM;
				str[1] = item_list[pos].pos;
				my_tcp_send(my_socket, str, 2);
			} else if (item_action_mode == ACTION_USE) {
				if (item_list[pos].use_with_inventory) {
					str[0] = USE_INVENTORY_ITEM;
					str[1] = item_list[pos].pos;
					my_tcp_send(my_socket, str, 2);
					item_list[pos].action = USE_INVENTORY_ITEM;
				}
			} else if (item_action_mode == ACTION_USE_WITEM) {
				if (use_item != -1) {
					str[0] = ITEM_ON_ITEM;
					str[1] = item_list[use_item].pos;
					str[2] = item_list[pos].pos;
					my_tcp_send(my_socket, str, 3);
					item_list[use_item].action = ITEM_ON_ITEM;
					item_list[pos].action = ITEM_ON_ITEM;
					if (!shift_on) {
						use_item = -1;
					}
				} else {
					use_item = pos;
				}
			} else {
				// calcul lors de la saisie de l'objet de la quantité max transportable
				int i;
				item_dragged_max_quantity = 0;
				if (item_list[pos].is_stackable) {
					item_dragged_max_quantity = item_list[pos].quantity;
				}
				if (!item_list[pos].is_stackable) {
					// parcours du contenu du sac pour les items non empilables
					for (i = 0; i < ITEM_WEAR_START; i++) {
						if (item_list[i].quantity < 1) {
							continue;
						}
						if (item_list[i].id != unset_item_uid) {
							if (item_list[i].id != item_list[pos].id) {
								continue;
							}
						} else if ((item_list[i].image_id != item_list[pos].image_id) || (item_list[i].is_stackable)) {
							continue;
						}
						item_dragged_max_quantity += item_list[i].quantity;
					}
				} else {
					item_dragged_max_quantity = item_list[pos].quantity;
				}
				item_dragged = pos;
				do_drag_item_sound();
			}
		}
	}
	// Get All button
	else if (over_button(win, mx, my) == BUT_GET) {
		// déplacement du code dans une fonction qui pourra aussi être appelée par un raccourci clavier
		get_all_handler();
	}
	// Sto All button
	else if (over_button(win, mx, my) == BUT_STORE && storage_win >= 0 && view_only_storage == 0 && get_show_window(storage_win) /*thanks alberich*/) {
		str[0] = TOUT_DEPOT;
		my_tcp_send(my_socket, str, 1);
	}
	// Drop All button
	else if (over_button(win, mx, my) == BUT_DROP) {
		drop_all_handler();
	}
	// Mix One/All button
	else if (over_button(win, mx, my) == BUT_MIX) {
		if (items_mix_but_all) {
			mix_handler(255, mixbut_empty_str);
		} else {
			mix_handler(1, mixbut_empty_str);
		}
	}
	// Item List button
	else if (over_button(win, mx, my) == BUT_ITEM_LIST) {
		toggle_items_list_window(win);
	} else if (my >= wear_items_y_offset + wear_grid_size * 4 + 10 && my <= wear_items_y_offset + wear_grid_size * 4 + 25) {
		int i, j, last_pos = 0;
		// bouton pour mettre tout l'équipement dans le sac
		if (mx >= wear_items_x_offset + 4 && mx <= wear_items_x_offset + wear_grid_size - 4) {
			for (i = ITEM_WEAR_START; i < ITEM_NUM_ITEMS; i++) {
				int destination_pos = -1;
				if (item_list[i].quantity < 1) {
					continue;
				}
				if (item_list[i].is_stackable) {
					for (j = 0; j < ITEM_WEAR_START; j++) {
						if (item_list[j].quantity < 1) {
							continue;
						}
						if (item_list[j].id != unset_item_uid) {
							if (item_list[i].id != item_list[j].id) {
								continue;
							}
						} else if ((item_list[i].image_id != item_list[j].image_id) || item_list[j].is_stackable) {
							continue;
						}
						destination_pos = j;
						break;
					}
				}
				if (destination_pos < 0) {
					for (j = last_pos; j < ITEM_WEAR_START; j++) {
						if (item_list[j].quantity > 0) {
							continue;
						}
						destination_pos = j;
						last_pos = j + 1;
						break;
					}
				}
				if (destination_pos < 0) {
					continue;
				}
				equip_item(i, destination_pos);
			}
			return 1;
		}
		// bouton pour mettre tout l'équipement dans le dépot
/*
        ATTENTION : lorsqu'on demande au serveur de ranger un objet au dépot,
        celui-ci ne tient pas compte de la position dans le sac de l'objet indiqué.
        Il parcourra tout le sac à la recherche de cette objet pour ranger la quantité demandée
        en partant de la première case. Il est alors impossible au client de savoir
        quelle case va être finalement libérée dans le sac (la recherche du premier objet
        semblable dans le sac n'étant pas fiable avec l'image_id).
        Avec l'ajout des ITEM_UID, le client pourrait deviner ce que va faire le serveur :
        rechercher la 1ère occurence de l'objet qui sera rangée et éventuellement enchainer
        avec un déplacement de l'objet déséquipé vers la case de l'objet rangé.
        Celà resterait un enchainement de manipulation un peu fastidieux...
        D'autant que ce comportement du serveur est regrettable aussi le reste du temps :
        avoir 20 objets identiques dans son sac, ranger le dernier pour voir que le serveur
        range le premier quoiqu'on lui dise...
        TODO: L'idéal serait donc de modifier le comportement du serveur sur un DEPOSITE_ITEM,
        qu'il se comporte de la même manière que lors d'un DROP_ITEM (dépot au sol) :
        il parcours l'inventaire à partir de la position indiquée pour trouver les objets
        correspondant à retirer, une fois arrivé à la 36ème case, si la quantité demandée
        n'est pas atteinte, il refait une boucle de la position indiquée jusqu'à 0.
                else if (mx>=wear_items_x_offset+wear_grid_size+4 && mx<=wear_items_x_offset+wear_grid_size-4)
                {
                        int destination_pos = -1;
                        if ((storage_win<0) || view_only_storage || !get_show_window(storage_win)) return 0;
                        for (j = 0; j < ITEM_WEAR_START; j++)
                        {
                                if (! item_list[j].quantity)
                                {
                                        destination_pos = j; break;
                                }
                        }
                        printf("dest po [%i]\n", destination_pos);
                        for (i=ITEM_WEAR_START; i<ITEM_NUM_ITEMS; i++) if (item_list[i].quantity > 0)
                        {
                                printf("pos [%i] (stack %i)\n", i, item_list[i].is_stackable);
                                if (item_list[i].is_stackable) for (j = 0; j < ITEM_WEAR_START; j++)
                                {
                                        if (item_list[j].is_stackable && (item_list[i].image_id == item_list[j].image_id))
                                        {
                                                printf("equip on stack [%i]->[%i]\n", i, j);
                                                equip_item(i, j); continue;
                                        }
                                }
                                if (destination_pos >= 0)
                                {
                                        printf("desequip [%i]->[%i]\n", i, destination_pos);
                                        equip_item(i, destination_pos);
                                        str[0] = DEPOSITE_ITEM;
                                        str[1] = destination_pos;
 *((Uint32*)(str+2))=SDL_SwapLE32(1);
                                        my_tcp_send(my_socket, str, 6);
                                }
                        }
                        return 1;
                }
 */
		return 0;
	}
	// see if we clicked on any item in the wear category
	else if (mx > wear_items_x_offset && mx < wear_items_x_offset + 2 * wear_grid_size && my > wear_items_y_offset && my < wear_items_y_offset + 4 * wear_grid_size) {
		int pos = 36 + get_mouse_pos_in_grid(mx, my, 2, 4, wear_items_x_offset, wear_items_y_offset, wear_grid_size, wear_grid_size);
		if (pos < 36) {}
		// on lache un objet du dépot vers une case d'équipement
		else if (storage_item_dragged != -1) {
			int i, temp_case = -1;
			// recherche d'une case libre dans le sac qui servira d'intermédiaire
			for (i = 0; i < ITEM_WEAR_START; i++) {
				if (item_list[i].quantity > 0) {
					if (!item_list[i].is_stackable) {
						continue;
					}
					if (item_list[i].id != unset_item_uid) {
						if (item_list[i].id != storage_items[storage_item_dragged].id) {
							continue;
						}
					} else if (item_list[i].image_id != storage_items[storage_item_dragged].image_id) {
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
			// recherche d'une case libre dans les équipements (en priorité celle cliquée)
			if (item_list[pos].quantity > 0) {
				for (i = ITEM_WEAR_START; i < ITEM_NUM_ITEMS; i++) {
					if (!item_list[i].quantity) {
						pos = i;
						break;
					}
				}
				if (item_list[pos].quantity > 0) {
					return 0;
				}
			}
			// on met dans le sac un seul exemplaire de l'objet du dépot avant de l'équiper
			str[0] = WITHDRAW_ITEM;
			*((Uint16 *)(str + 1)) = SDL_SwapLE16(storage_items[storage_item_dragged].pos);
			*((Uint32 *)(str + 3)) = SDL_SwapLE32(1);
			my_tcp_send(my_socket, str, 6);
			item_list[temp_case].pos = temp_case;
			equip_item(temp_case, pos);
			do_drop_item_sound();
			storage_item_dragged = -1;
			return 1;
		} else if (item_list[pos].quantity) {
			if (item_action_mode == ACTION_LOOK) {
				str[0] = LOOK_AT_INVENTORY_ITEM;
				str[1] = item_list[pos].pos;
				my_tcp_send(my_socket, str, 2);
			} else if (item_dragged == -1 && left_click) {
				// Ctrl+click : on tente le rangement au dépot d'un équipement porté
				if ((ctrl_on) && (storage_win != -1) && !view_only_storage && get_show_window(storage_win)) {
					int i, temp_case = -1;
					// si la source est empilable on doit d'abord trouver s'il sera empilé
					if (item_list[pos].is_stackable) {
						for (i = 0; i < ITEM_WEAR_START; i++) {
							if (item_list[i].id != unset_item_uid) {
								if (item_list[i].id != item_list[pos].id) {
									continue;
								}
							} else if ((item_list[i].image_id != item_list[pos].image_id) || !item_list[i].is_stackable) {
								continue;
							}
							temp_case = i;
							break;
						}
					}
					// sinon recherche d'une case libre dans le sac pour déséquiper
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
					// demande de déplacement dans le sac puis vers le dépot
					equip_item(pos, temp_case);
					str[0] = DEPOSITE_ITEM;
					str[1] = temp_case;
					*((Uint32 *)(str + 2)) = SDL_SwapLE32(1);
					my_tcp_send(my_socket, str, 6);
					return 1;
				}
				// Shift+click : ajoute l'objet en raccourci sur le 1er emplacement libre de la barre rapide
				else if (shift_on) {
					affect_fr_quickitems(item_list[pos].pos);
					return 1;
				}
				// calcul lors de la saisie de l'objet de la quantité max transportable = 1 pour un objet porté
				item_dragged_max_quantity = 1;
				item_dragged = pos;
				do_drag_item_sound();
			} else if (item_dragged != -1 && left_click) {
				// on lache un objet de l'inventaire vers une case d'équipement occupée
				if (((item_dragged == pos) && move_item(item_dragged, pos)) || (allow_equip_swap && move_item(item_dragged, pos))) {
					do_drag_item_sound();
				} else {
					do_alert1_sound();
				}
				item_dragged = -1;
			}
		} else if (item_dragged != -1) {
			// on lache un objet de l'inventaire vers une case d'équipement vide
			move_item(item_dragged, pos);
			item_dragged = -1;
			do_drop_item_sound();
		}
	}
	// clear the message area if double-clicked
	else if (my > (win->len_y - (use_small_items_window?105:85))) {
		static Uint32 last_click = 0;
		if (safe_button_click(&last_click)) {
			set_shown_string(0, "");
			return 1;
		}
	}
	return 1;
}
void set_description_help(int pos) {
	Uint16 item_id = item_list[pos].id;
	int image_id = item_list[pos].image_id;
	if (show_item_desc_text && item_info_available() && (get_item_count(item_id, image_id) == 1)) {
		item_desc_str = get_item_description(item_id, image_id);
	}
}
int mouseover_items_handler(window_info *win, int mx, int my) {
	int pos;
	// check and record if mouse if over a button
	if ((mouse_over_but = over_button(win, mx, my)) != -1) {
		return 0; // keep standard cursor
	}
	if (mx > 0 && mx < 6 * items_grid_size && my > 0 && my < 6 * items_grid_size) {
		pos = get_mouse_pos_in_grid(mx, my, 6, 6, 0, 0, items_grid_size, items_grid_size);
		if (pos == -1) {} else if (item_list[pos].quantity) {
			set_description_help(pos);
			if (item_action_mode == ACTION_LOOK) {
				elwin_mouse = CURSOR_EYE;
			} else if (item_action_mode == ACTION_USE) {
				elwin_mouse = CURSOR_USE;
			} else if (item_action_mode == ACTION_USE_WITEM) {
				elwin_mouse = CURSOR_USE_WITEM;
			} else {
				if (item_dragged == -1) {
					item_help_str = pick_item_help_str;
				}
				elwin_mouse = CURSOR_PICK;
			}
			mouseover_item_pos = pos;
			return 1;
		}
	} else if (mx > wear_items_x_offset && mx < wear_items_x_offset + 2 * wear_grid_size && my > wear_items_y_offset && my < wear_items_y_offset + 4 * wear_grid_size) {
		pos = 36 + get_mouse_pos_in_grid(mx, my, 2, 4, wear_items_x_offset, wear_items_y_offset, wear_grid_size, wear_grid_size);
		item_help_str = equip_here_str;
		if (pos == -1) {} else if (item_list[pos].quantity) {
			set_description_help(pos);
			if (item_action_mode == ACTION_LOOK) {
				elwin_mouse = CURSOR_EYE;
			} else if (item_action_mode == ACTION_USE) {
				elwin_mouse = CURSOR_USE;
			} else if (item_action_mode == ACTION_USE_WITEM) {
				elwin_mouse = CURSOR_USE_WITEM;
			} else {
				elwin_mouse = CURSOR_PICK;
			}
			return 1;
		}
	} else if (show_help_text && mx > quantity_x_offset && mx < quantity_x_offset + ITEM_EDIT_QUANT * quantity_width && my > quantity_y_offset && my < quantity_y_offset + 6 * 20) {
		item_help_str = quantity_edit_str;
	} else if (show_help_text && *inventory_item_string && (my > (win->len_y - (use_small_items_window?105:85)))) {
		item_help_str = (disable_double_click)?click_clear_str :double_click_clear_str;
	} else if (my >= wear_items_y_offset + wear_grid_size * 4 + 10 && my <= wear_items_y_offset + wear_grid_size * 4 + 25) {
		if (mx >= wear_items_x_offset + 4 && mx <= wear_items_x_offset + wear_grid_size - 4) {
			show_help(unwear_all_to_inv_str, 0, quantity_y_offset + 30);
		} else if (mx >= wear_items_x_offset + wear_grid_size + 4 && mx <= wear_items_x_offset + wear_grid_size * 2 - 4) {
		}
	}
	return 0;
}
int keypress_items_handler(window_info *win, int x, int y, Uint32 key, Uint32 keysym) {
	if (edit_quantity != -1) {
		char *str = quantities.quantity[edit_quantity].str;
		int *len = &quantities.quantity[edit_quantity].len;
		int *val = &quantities.quantity[edit_quantity].val;
		if (key == SDLK_DELETE) {
			reset_quantity(edit_quantity);
			edit_quantity = -1;
			return 1;
		} else if (key == SDLK_BACKSPACE) {
			if (*len > 0) {
				(*len)--;
				str[*len] = 0;
				*val = atoi(str);
			}
			return 1;
		} else if (keysym == '\r') {
			if (!*val) {
				reset_quantity(edit_quantity);
			}
			item_quantity = *val;
			quantities.selected = edit_quantity;
			edit_quantity = -1;
			return 1;
		} else if (keysym >= '0' && keysym <= '9' && *len < 5) {
			str[*len] = keysym;
			(*len)++;
			str[*len] = 0;
			*val = atoi(str);
			return 1;
		}
	}
	return 0;
}
// Fonction indépendante aussi bien pour le bouton que le raccourci clavier
void get_all_handler() {
	int pos;
	actor *me = get_our_actor();
	if (!me) {
		return; // Wtf!?
	}
	for (pos = 0; pos < NUM_BAGS; pos++) {
		if ((bag_list[pos].x == me->x_tile_pos) && (bag_list[pos].y == me->y_tile_pos)) {
			if (!get_show_window(ground_items_win)) {
				// if auto empty bags enable, set the open timer
				if (items_auto_get_all) {
					ground_items_empty_next_bag = SDL_GetTicks();
				} else {
					ground_items_empty_next_bag = 0;
				}
				open_bag(bag_list[pos].obj_3d_id);
			} else {
				pick_up_all_items();
			}
			break;
		}
	}
}
static void drop_all_handler() {
	Uint8 str[6] = {0};
	int i;
	int dropped_something = 0;
	static Uint32 last_click = 0;
	/* provide some protection for inadvertent pressing (double click that can be disabled) */
	if (safe_button_click(&last_click)) {
		set_shown_string(0, "");
		for (i = ((items_dropall_nofirstrow)?6:0); i < ((items_dropall_nolastrow)?30:36); i++) {
			if ((!items_dropall_nofirstcol || (i % 6)) && (!items_dropall_nolastcol || ((i + 1) % 6))) {
				str[0] = DROP_ITEM;
				str[1] = item_list[i].pos;
				*((Uint32 *)(str + 2)) = SDL_SwapLE32(item_list[i].quantity);
				my_tcp_send(my_socket, str, 6);
				dropped_something = 1;
			}
		}
		if (dropped_something) {
			add_sound_object(get_index_for_sound_type_name("Drop Item"), 0, 0, 1);
		}
	} else {
		set_shown_string(c_orange2, dc_warning_str);
	}
}
static int resize_items_handler(window_info *win) {
	// contrôle sur la largeur max de la fenêtre (arbitrairement = écran - 100)
	if (win->len_x > window_width - 100) {
		win->len_x = window_width - 100;
	}
	// taille de la grille en fonction de la largeur de la fenêtre
	items_grid_size = (manual_size_items_window) ? (win->len_x - 10 - 30) / 8 : (win->len_x - 110) / 6;
	// contrôle sur la hauteur min de la fenêtre (d'après grille + cases quantité)
	if (win->len_y < 6 * items_grid_size + 25) {
		win->len_y = 6 * items_grid_size + 25;
	}
	// contrôle sur la hauteur max de la fenêtre (d'après grille + espace suffisant)
	if (win->len_y > 6 * items_grid_size + 140) {
		win->len_y = 6 * items_grid_size + 140;
	}
	// contrôle si la hauteur n'est pas trop grande par rapport à la taille de l'écran
	if (win->len_y > window_height - 10) {
		// hauteur de la fenêtre bloqué au max possible selon la hauteur d'écran
		win->len_y = window_height - 10;
		// recalcul de la taille de la grille en fonction de la hauteur de la fenêtre
		items_grid_size = (win->len_y - 25) / 6;
		// recalcul de la largeur de la fenêtre en fonction de la taille de la grille
		win->len_x = (manual_size_items_window) ? items_grid_size * 8 + 10 + 30 : items_grid_size * 6 + 110;
	}
	// taille des cases de quantiés d'après la largeur de la fenêtre
	quantity_width = (win->len_x < 330) ? win->len_x / 6 : (win->len_x - 20) / 6;
	// position verticale des cases de quantités d'après la hauteur de la fenêtre
	quantity_y_offset = win->len_y - 21;
	quantity_x_offset = 1;
	// taille de la grille des équipements
	wear_grid_size = (manual_size_items_window) ? items_grid_size : 33;
	// position horizontale des équipements d'après la taille de la grille
	wear_items_x_offset = 6 * items_grid_size + (win->len_x - 6 * items_grid_size - 2 * wear_grid_size - 30) / 2 - 1;
	wear_items_y_offset = items_grid_size;
	// détermine l'état de la variable 'small item' en fonction de la taille de la grille
	// laisser à 1 si on souhaite que l'indication de charge reste en bas dans tous les cas
	use_small_items_window = ((manual_size_items_window) || (items_grid_size < 38)) ? 1 : 0;
	// redécoupe le texte de l'inventaire en fonction de la largeur de la fenêtre
	put_small_text_in_box((unsigned char *)inventory_item_string, strlen(inventory_item_string), win->len_x, items_string);
	return 0;
}
int show_items_handler(window_info *win) {
	/* appel la fonction du resize pour valider les tailles & positions */
	resize_items_handler(&windows_list.window[items_win]);
	item_quantity = quantities.quantity[quantities.selected].val;
	cm_remove_regions(items_win);
	cm_add_region(cm_getall_but, items_win, win->len_x - (XLENBUT + 3), wear_items_y_offset + but_y_off[0], XLENBUT, YLENBUT);
	cm_add_region(cm_dropall_but, items_win, win->len_x - (XLENBUT + 3), wear_items_y_offset + but_y_off[2], XLENBUT, YLENBUT);
	cm_add_region(cm_mix_but, items_win, win->len_x - (XLENBUT + 3), wear_items_y_offset + but_y_off[4], XLENBUT, YLENBUT);
	cm_add_region(cm_itemlist_but, items_win, win->len_x - (XLENBUT + 3), wear_items_y_offset + but_y_off[3], XLENBUT, YLENBUT);
	/* make sure we redraw any string */
	last_items_string_id = 0;
	return 1;
}
static int context_items_handler(window_info *win, int widget_id, int mx, int my, int option) {
	if (option < ELW_CM_MENU_LEN) {
		return cm_title_handler(win, widget_id, mx, my, option);
	}
	switch (option) {
	case ELW_CM_MENU_LEN + 1:
		show_items_handler(win);
		break;
	case ELW_CM_MENU_LEN + 10:
		send_input_text_line("#depot", 6);
		break;
	}
	return 1;
}
void display_items_menu() {
	if (items_win < 0) {
		int our_root_win = -1;
		if (!windows_on_top) {
			our_root_win = game_root_win;
		}
		if (items_menu_x_len < 8 * 33 + 45) {
			items_menu_x_len = 8 * 33 + 45;
		}
		if (items_menu_y_len < 6 * 33 + 25) {
			items_menu_y_len = 6 * 33 + 25;
		}
		items_win = create_window(win_inventory, our_root_win, 0, items_menu_x, items_menu_y, items_menu_x_len, items_menu_y_len, ELW_RESIZEABLE | ELW_WIN_DEFAULT);
		set_window_min_size(items_win, 8 * 33 + 45, 6 * 33 + 25);
		set_window_handler(items_win, ELW_HANDLER_RESIZE, &resize_items_handler);
		/* appel la fonction du resize pour valider les tailles & positions */
		resize_items_handler(&windows_list.window[items_win]);
		set_window_handler(items_win, ELW_HANDLER_DISPLAY, &display_items_handler);
		set_window_handler(items_win, ELW_HANDLER_CLICK, &click_items_handler);
		set_window_handler(items_win, ELW_HANDLER_MOUSEOVER, &mouseover_items_handler);
		set_window_handler(items_win, ELW_HANDLER_KEYPRESS, &keypress_items_handler);
		set_window_handler(items_win, ELW_HANDLER_SHOW, &show_items_handler);
		cm_add(windows_list.window[items_win].cm_id, cm_items_menu_str, context_items_handler);
		cm_bool_line(windows_list.window[items_win].cm_id, ELW_CM_MENU_LEN + 1, &manual_size_items_window, NULL);
		cm_bool_line(windows_list.window[items_win].cm_id, ELW_CM_MENU_LEN + 2, &item_window_on_drop, "item_window_on_drop");
		cm_bool_line(windows_list.window[items_win].cm_id, ELW_CM_MENU_LEN + 3, &allow_equip_swap, NULL);
		cm_bool_line(windows_list.window[items_win].cm_id, ELW_CM_MENU_LEN + 4, &allow_wheel_quantity_edit, NULL);
		cm_bool_line(windows_list.window[items_win].cm_id, ELW_CM_MENU_LEN + 5, &allow_wheel_quantity_drag, NULL);
		cm_bool_line(windows_list.window[items_win].cm_id, ELW_CM_MENU_LEN + 7, &cm_quickbar_enabled, NULL);
		cm_bool_line(windows_list.window[items_win].cm_id, ELW_CM_MENU_LEN + 8, &cm_quickbar_protected, NULL);
		cm_stoall_but = cm_create(cm_stoall_menu_str, NULL);
		cm_bool_line(cm_stoall_but, 0, &items_stoall_nofirstrow, NULL);
		cm_bool_line(cm_stoall_but, 1, &items_stoall_nolastrow, NULL);
		cm_bool_line(cm_stoall_but, 2, &items_stoall_nofirstcol, NULL);
		cm_bool_line(cm_stoall_but, 3, &items_stoall_nolastcol, NULL);
		cm_dropall_but = cm_create(cm_dropall_menu_str, NULL);
		cm_bool_line(cm_dropall_but, 0, &items_dropall_nofirstrow, NULL);
		cm_bool_line(cm_dropall_but, 1, &items_dropall_nolastrow, NULL);
		cm_bool_line(cm_dropall_but, 2, &items_dropall_nofirstcol, NULL);
		cm_bool_line(cm_dropall_but, 3, &items_dropall_nolastcol, NULL);
		cm_mix_but = cm_create(mixall_str, NULL);
		cm_bool_line(cm_mix_but, 0, &items_mix_but_all, NULL);
		cm_getall_but = cm_create(auto_get_all_str, NULL);
		cm_bool_line(cm_getall_but, 0, &items_auto_get_all, NULL);
		cm_itemlist_but = cm_create(item_list_but_str, NULL);
		cm_bool_line(cm_itemlist_but, 0, &items_list_on_left, NULL);
		show_items_handler(&windows_list.window[items_win]);
	} else {
		show_window(items_win);
		select_window(items_win);
	}
}
void get_items_cooldown(const Uint8 *data, int len) {
	int iitem, nitems, ibyte, pos;
	Uint8 cooldown, max_cooldown;
	// reset old cooldown values
	for (iitem = 0; iitem < ITEM_NUM_ITEMS; iitem++) {
		item_list[iitem].cooldown_time = 0;
		item_list[iitem].cooldown_rate = 1;
	}
	nitems = len / 5;
	if (nitems <= 0) {
		return;
	}
	ibyte = 0;
	for (iitem = 0; iitem < nitems; iitem++) {
		pos = data[ibyte];
		max_cooldown = SDL_SwapLE16(*((Uint16 *)(&data[ibyte + 1])));
		cooldown = SDL_SwapLE16(*((Uint16 *)(&data[ibyte + 3])));
		ibyte += 5;
		item_list[pos].cooldown_rate = 1000 * (Uint32)max_cooldown;
		item_list[pos].cooldown_time = cur_time + 1000 * (Uint32)cooldown;
	}
	sync_fr_quickitems();
}
