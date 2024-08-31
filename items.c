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
#ifdef FR_VERSION
#include "fr_quickitems.h"
#include "themes.h"
#endif //FR_VERSION

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
int edit_quantity=-1;

int item_action_mode=ACTION_WALK;

int items_win= -1;
int items_menu_x=10;
int items_menu_y=20;
int items_grid_size=51;//Changes depending on the size of the root window (is 51 > 640x480 and 33 in 640x480).
int items_menu_x_len=6*51+110;
int items_menu_y_len=6*51+90;

int items_text[MAX_ITEMS_TEXTURES];

static char items_string[350]={0};
static size_t last_items_string_id = 0;
int item_dragged=-1;
int item_quantity=1;
int quantity_width=0;
int allow_equip_swap=0;
int use_item=-1;

int wear_items_x_offset=6*51+20;
int wear_items_y_offset=30;

int quantity_x_offset=6*51+20;
int quantity_y_offset=185;

int use_small_items_window = 0;
int manual_size_items_window = 0;
#ifdef ENGLISH
int items_mod_click_any_cursor = 1;
#endif //ENGLISH

int item_uid_enabled = 0;
const Uint16 unset_item_uid = (Uint16)-1;

#define NUMBUT 4
#define XLENBUT 29
#define YLENBUT 33
#undef NUMBUT
#define NUMBUT 5
static int but_y_off[NUMBUT] = { 0, YLENBUT, YLENBUT*2, YLENBUT*3, YLENBUT*4 };
#ifdef FR_VERSION
enum { BUT_GET, BUT_STORE, BUT_DROP, BUT_ITEM_LIST, BUT_MIX };
#else //FR_VERSION
enum { BUT_STORE, BUT_GET, BUT_DROP, BUT_MIX, BUT_ITEM_LIST };
#endif //FR_VERSION
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

#ifdef FR_VERSION
int items_stoall_nolastcol = 0;
int items_stoall_nofirstcol = 0;
int items_dropall_nolastcol = 0;
int items_dropall_nofirstcol = 0;
int allow_wheel_quantity_edit = 0;
int allow_wheel_quantity_drag = 0;

int last_quantity = 1;     // m�morise la valeur initiale d'une quantit� �dit�e pour annuler l'�dition
int item_dragged_max_quantity = 1; // quantit� max disponible pour un objet pris du sac
int wear_grid_size = 51;

static void equip_item(int item_pos_to_equip, int destination_pos);
#endif //FR_VERSION

static void drop_all_handler();

void set_shown_string(char colour_code, const char *the_text)
{
	if (strlen(the_text) == 0)
	{
		inventory_item_string[0] = '\0';
		inventory_item_string_id++;
		return;
	}
	inventory_item_string[0] = to_color_char(colour_code);
	safe_strncpy2(inventory_item_string+1, the_text, sizeof(inventory_item_string)-2, strlen(the_text));
	inventory_item_string[sizeof(inventory_item_string)-1] = 0;
	inventory_item_string_id++;
}


/* return index of button or -1 if mouse not over a button */
static int over_button(window_info *win, int mx, int my)
{
	if (mx>(win->len_x-(XLENBUT+3)) && mx<win->len_x-3 && my>wear_items_y_offset
		&& my<wear_items_y_offset+but_y_off[NUMBUT-1]+YLENBUT) {
		return (my -  wear_items_y_offset) / YLENBUT;
	}
	return -1;
}


void gray_out(int x_start, int y_start, int gridsize){

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	//glBlendFunc(GL_DST_COLOR, GL_ONE); //this brightens up
	glBlendFunc(GL_ZERO, GL_SRC_COLOR); //this brightens down
	glColor3f(0.4f, 0.2f, 0.2f);
	glBegin(GL_QUADS);
		glVertex3i(x_start,y_start,0);
		glVertex3i(x_start+gridsize,y_start,0);
		glVertex3i(x_start+gridsize,y_start+gridsize,0);
		glVertex3i(x_start,y_start+gridsize,0);
	glEnd();
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glColor3f(1.0f, 1.0f, 1.0f);
}


void rendergrid(int columns, int rows, int left, int top, int width, int height)
{
	int x, y;
	int temp;

	glBegin(GL_LINES);

	for(y=0; y<=rows; y++){
		temp = top + y * height;
		glVertex2i(left,         temp);
		glVertex2i(left + width*columns, temp);
	}

	for(x=0; x<columns+1; x++){
		temp = left + x * width;
		glVertex2i(temp, top);
		glVertex2i(temp, top + height*rows);
	}

	glEnd();
}

int get_mouse_pos_in_grid(int mx, int my, int columns, int rows, int left, int top, int width, int height)
{
	int x, y, i;

	mx -= left;
	my -= top;

	i = 0;
	for (y = 0; y < rows; y++)
	{
		for (x = 0; x < columns; x++, i++)
		{
			if (mx >= x*width && mx <= (x+1)*width && my >= y*height && my <= (y+1)*height)
				return i;
		}
	}

	return -1;
}

void reset_quantity (int pos)
{
	int val;

	switch(pos)
	{
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
			LOG_ERROR ("Trying to reset invalid element of quantities, pos = %d", pos);
			return;
	}

	safe_snprintf (quantities.quantity[pos].str, sizeof(quantities.quantity[pos].str), "%d", val);
	quantities.quantity[pos].len = strlen (quantities.quantity[pos].str);
	quantities.quantity[pos].val = val;
}

#ifdef	NEW_TEXTURES
void get_item_uv(const Uint32 item, float* u_start, float* v_start,
	float* u_end, float* v_end)
{
	*u_start = (50.0f/256.0f) * (item % 5) + 0.5f / 256.0f;
	*u_end = *u_start + (50.0f/256.0f);
	*v_start = (50.0f/256.0f) * (item / 5) + 0.5f / 256.0f;
	*v_end = *v_start + (50.0f/256.0f);
}
#endif	/* NEW_TEXTURES */

void drag_item(int item, int storage, int mini)
{
#ifdef ENGLISH
	float u_start,v_start,u_end,v_end;
	int cur_item,this_texture;
	int cur_item_img;

	int quantity=item_quantity;
#else //ENGLISH
	int cur_item;
    int quantity = quantities.quantity[quantities.selected].val;
#endif //ENGLISH
	char str[20];

#ifdef FR_VERSION
	// on d�sactive tout drag propre � la barre rapide si un autre drag existe
	fr_quickitem_dragged = -1;
#endif //FR_VERSION

	if(storage) {
		if (item < 0 || item >= STORAGE_ITEMS_SIZE)
			// oops
			return;

		cur_item=storage_items[item].image_id;
		if(!storage_items[item].quantity) {
#ifdef ENGLISH
			use_item = storage_item_dragged=-1;
#else //ENGLISH
			storage_item_dragged=-1;
#endif //ENGLISH
			return;
		}
		if (quantity > storage_items[item].quantity)
			quantity = storage_items[item].quantity;
	} else {
		if (item < 0 || item >= ITEM_NUM_ITEMS)
			// oops
			return;

		cur_item=item_list[item].image_id;
		if(!item_list[item].quantity) {
#ifdef ENGLISH
			use_item = item_dragged=-1;
#else //ENGLISH
			item_dragged=-1;
#endif //ENGLISH
			return;
		}
#ifdef FR_VERSION
		/* grace � item_dragged_quantity_max on peut afficher la quantit� (stackable ou non) */
		if (quantity > item_dragged_max_quantity) quantity = item_dragged_max_quantity;
#else //FR_VERSION
		if(item_list[item].is_stackable){
			if(quantity>item_list[item].quantity)quantity=item_list[item].quantity;
		} else quantity=-1;//The quantity for non-stackable items is misleading so don't show it...
#endif //FR_VERSION
	}

#ifdef ENGLISH
	cur_item_img=cur_item%25;
#ifdef	NEW_TEXTURES
	get_item_uv(cur_item_img, &u_start, &v_start, &u_end, &v_end);
#else	/* NEW_TEXTURES */
	u_start=0.2f*(cur_item_img%5);
	u_end=u_start+(float)50/256;
	v_start=(1.0f+((float)50/256)/256.0f)-((float)50/256*(cur_item_img/5));
	v_end=v_start-(float)50/256;
#endif	/* NEW_TEXTURES */

	//get the texture this item belongs to
	this_texture=get_items_texture(cur_item/25);

#ifdef	NEW_TEXTURES
	bind_texture(this_texture);
#else	/* NEW_TEXTURES */
	get_and_set_texture_id(this_texture);
#endif	/* NEW_TEXTURES */
	glBegin(GL_QUADS);
	if(mini)
		draw_2d_thing(u_start,v_start,u_end,v_end,mouse_x-16,mouse_y-16,mouse_x+16,mouse_y+16);
	else
		draw_2d_thing(u_start,v_start,u_end,v_end,mouse_x-25,mouse_y-25,mouse_x+24,mouse_y+24);
	glEnd();
#else //ENGLISH
	/* autant r�utiliser la fonction draw_item */
	if(mini)
		draw_item(cur_item, mouse_x-16, mouse_y-16, 33);
	else
		draw_item(cur_item, mouse_x-25, mouse_y-25, 51);
#endif //ENGLISH

	if(!mini && quantity!=-1){
		safe_snprintf(str,sizeof(str),"%i",quantity);
		draw_string_small(mouse_x-25, mouse_y+10, (unsigned char*)str, 1);
	}
}

void get_your_items (const Uint8 *data)
{
	int i,total_items,pos,len;
	Uint8 flags;

	if (item_uid_enabled)
		len=10;
	else
		len=8;

	//data[0] -> num_items
	//data[1] -> image_id
	//data[3] -> quantity
	//data[7] -> pos
	//data[8] -> flags
	//data[9] -> id


	total_items=data[0];

	//clear the items first
	for(i=0;i<ITEM_NUM_ITEMS;i++){
		item_list[i].quantity=0;
	}

	for(i=0;i<total_items;i++){
		pos=data[i*len+1+6];
		// try not to wipe out cooldown information if no real change
		if(item_list[pos].image_id != SDL_SwapLE16(*((Uint16 *)(data+i*len+1))) ){
			item_list[pos].cooldown_time = 0;
			item_list[pos].cooldown_rate = 1;
		}
		item_list[pos].image_id=SDL_SwapLE16(*((Uint16 *)(data+i*len+1)));
		item_list[pos].quantity=SDL_SwapLE32(*((Uint32 *)(data+i*len+1+2)));
		item_list[pos].pos=pos;
#ifdef NEW_SOUND
		item_list[pos].action = ITEM_NO_ACTION;
		item_list[pos].action_time = 0;
#endif // NEW_SOUND
		flags=data[i*len+1+7];
		if (item_uid_enabled)
			item_list[pos].id=SDL_SwapLE16(*((Uint16 *)(data+i*len+1+8)));
		else
			item_list[pos].id=unset_item_uid;
		item_list[pos].is_resource=((flags&ITEM_RESOURCE)>0);
		item_list[pos].is_reagent=((flags&ITEM_REAGENT)>0);
		item_list[pos].use_with_inventory=((flags&ITEM_INVENTORY_USABLE)>0);
		item_list[pos].is_stackable=((flags&ITEM_STACKABLE)>0);
	}

#ifdef FR_VERSION
	build_fr_quickitems(0);
#endif //FR_VERSION
	build_manufacture_list();
	check_castability();
}

#ifdef NEW_SOUND
void check_for_item_sound(int pos)
{
	int i, snd = -1;

#ifdef _EXTRA_SOUND_DEBUG
//	printf("Used item: %d, Image ID: %d, Action: %d\n", pos, item_list[pos].image_id, item_list[pos].action);
#endif // _EXTRA_SOUND_DEBUG
	if (item_list[pos].action != ITEM_NO_ACTION)
	{
		// Play the sound that goes with this action
		switch (item_list[pos].action)
		{
			case USE_INVENTORY_ITEM:
				snd = get_index_for_inv_use_item_sound(item_list[pos].image_id);
				break;
			case ITEM_ON_ITEM:
				// Find the second item (being used with)
				for (i = 0; i < ITEM_NUM_ITEMS; i++)
				{
					if (i != pos && item_list[i].action == ITEM_ON_ITEM)
					{
						snd = get_index_for_inv_usewith_item_sound(item_list[pos].image_id, item_list[i].action);
						break;
					}
				}
				break;
		}
		if (snd > -1)
			add_sound_object(snd, 0, 0, 1);
		// Reset the action
		item_list[pos].action = ITEM_NO_ACTION;
		item_list[pos].action_time = 0;
	}
}

void update_item_sound(int interval)
{
	int i;
	// Iterate through the list of items, checking for out of date item actions (> 2 sec old)
	for (i = 0; i < ITEM_NUM_ITEMS; i++)
	{
		if (item_list[i].action != ITEM_NO_ACTION)
		{
			item_list[i].action_time += interval;
			if (item_list[i].action_time >= 2000)
			{
				// Item action state is out of date so reset it
				item_list[i].action = ITEM_NO_ACTION;
				item_list[i].action_time = 0;
			}
		}
	}
}
#endif // NEW_SOUND

void remove_item_from_inventory(int pos)
{
	item_list[pos].quantity=0;

#ifdef NEW_SOUND
	check_for_item_sound(pos);
#endif // NEW_SOUND

#ifdef FR_VERSION
	build_fr_quickitems(0);
#endif //FR_VERSION
	build_manufacture_list();
	check_castability();
}

void get_new_inventory_item (const Uint8 *data)
{
	int pos;
	Uint8 flags;
	int quantity;
	int image_id;
	Uint16 id;

	if (item_uid_enabled)
		id=SDL_SwapLE16(*((Uint16 *)(data+8)));
	else
		id=unset_item_uid;

	pos= data[6];
	flags= data[7];
	image_id=SDL_SwapLE16(*((Uint16 *)(data)));
	quantity=SDL_SwapLE32(*((Uint32 *)(data+2)));

#ifdef ENGLISH
	if (now_harvesting() && (quantity >= item_list[pos].quantity) ) {	//some harvests, eg hydrogenium and wolfram, also decrease an item number. only count what goes up
#else
	 /* Tosh : on ne compte pas les Lumens dans les compteurs de r�colte (image_id=3) */
	if (now_harvesting() && (quantity >= item_list[pos].quantity) && image_id != 3) {
#endif //ENGLISH
		increment_harvest_counter(item_list[pos].quantity > 0 ? quantity - item_list[pos].quantity : quantity);
	}

	// don't touch cool down when it's already active
	if(item_list[pos].quantity == 0 || item_list[pos].image_id != image_id){
		item_list[pos].cooldown_time = 0;
		item_list[pos].cooldown_rate = 1;
	}
	item_list[pos].quantity=quantity;
	item_list[pos].image_id=image_id;
	item_list[pos].pos=pos;
	item_list[pos].id=id;
	item_list[pos].is_resource=((flags&ITEM_RESOURCE)>0);
	item_list[pos].is_reagent=((flags&ITEM_REAGENT)>0);
	item_list[pos].use_with_inventory=((flags&ITEM_INVENTORY_USABLE)>0);
	item_list[pos].is_stackable=((flags&ITEM_STACKABLE)>0);

#ifdef NEW_SOUND
	check_for_item_sound(pos);
#endif // NEW_SOUND

#ifdef FR_VERSION
	build_fr_quickitems(0);
#endif //FR_VERSION
	build_manufacture_list();
	check_castability();
}



void draw_item(int id, int x_start, int y_start, int gridsize){
	float u_start,v_start,u_end,v_end;
	int cur_item;
	int this_texture;

	//get the UV coordinates.
	cur_item=id%25;
#ifdef	NEW_TEXTURES
	get_item_uv(cur_item, &u_start, &v_start, &u_end, &v_end);
#else	/* NEW_TEXTURES */
#ifdef ENGLISH
	u_start=0.2f*(cur_item%5);
	u_end=u_start+(float)50/256;
	v_start=(1.0f+((float)50/256)/256.0f)-((float)50/256*(cur_item/5));
	v_end=v_start-(float)50/256;
#else //ENGLISH
	/* en commentaire un calcul alternatif du mapping � envisager
	 * en accord avec un repositionnement des items sur les planches
	 * de textures en respectant une grille pr�cise
	 */
	u_start=0.2f*(cur_item%5);
//	u_start = (float)1/256 + (cur_item%5)*((float)51/256);
	u_end=u_start+(float)50/256;
	v_start=(1.0f+((float)50/256)/256.0f)-((float)50/256*(cur_item/5));
//	v_start = 1.0f - (float)1/256 - ((float)51/256*(cur_item/5));
	v_end=v_start-(float)50/256;
#endif //ENGLISH
#endif	/* NEW_TEXTURES */

	//get the texture this item belongs to
	this_texture=get_items_texture(id/25);

#ifdef	NEW_TEXTURES
	bind_texture(this_texture);
#else	/* NEW_TEXTURES */
	get_and_set_texture_id(this_texture);
#endif	/* NEW_TEXTURES */
	glBegin(GL_QUADS);
		draw_2d_thing(u_start,v_start,u_end,v_end,x_start,y_start,x_start+gridsize-1,y_start+gridsize-1);
	glEnd();
}


int display_items_handler(window_info *win)
{
	char str[80];
    char my_str[10];
	int x,y,i;
	int item_is_weared=0;
	Uint32 _cur_time = SDL_GetTicks(); /* grab a snapshot of current time */
#ifdef FR_VERSION
	char *but_labels[NUMBUT] = { get_all_str, sto_all_str, drp_all_str, itm_lst_str, NULL };
#else //FR_VERSION
	char *but_labels[NUMBUT] = { sto_all_str, get_all_str, drp_all_str, NULL, itm_lst_str };
#endif //FR_VERSION

	glEnable(GL_TEXTURE_2D);

	/*
	* Labrat: I never realised that a store all patch had been posted to Berlios by Awn in February '07
	* Thanks to Awn for his earlier efforts (but this is not a derivative of his earlier work)
	*
	*My next step will be to code an #ifdef STORE_ALL section to save the 0-35 loop in the click handler for future proofing
	*  ready for server side implementation
	*/
	// draw the button labels
	but_labels[BUT_MIX] = (items_mix_but_all) ?mix_all_str :mix_one_str;
	for (i=0; i<NUMBUT; i++) {
		strap_word(but_labels[i],my_str);
	glColor3f(0.77f,0.57f,0.39f);
		draw_string_small(win->len_x+gx_adjust-XLENBUT, wear_items_y_offset+but_y_off[i]+2+gy_adjust, (unsigned char*)my_str, 2);
	}

   	x=quantity_x_offset+quantity_width/2;
	y=quantity_y_offset+3;
	glColor3f(0.3f,0.5f,1.0f);
	for(i=0;i<ITEM_EDIT_QUANT;x+=quantity_width,++i){
		if(i==edit_quantity){
			glColor3f(1.0f, 0.0f, 0.3f);
			draw_string_small(1+gx_adjust+x-strlen(quantities.quantity[i].str)*4, y+gy_adjust, (unsigned char*)quantities.quantity[i].str, 1);
			glColor3f(0.3f, 0.5f, 1.0f);
		} else if(i==quantities.selected){
			glColor3f(0.0f, 1.0f, 0.3f);
			draw_string_small(1+gx_adjust+x-strlen(quantities.quantity[i].str)*4, y+gy_adjust, (unsigned char*)quantities.quantity[i].str, 1);
			glColor3f(0.3f, 0.5f, 1.0f);
		} else  draw_string_small(1+gx_adjust+x-strlen(quantities.quantity[i].str)*4, y+gy_adjust, (unsigned char*)quantities.quantity[i].str, 1);
	}
#ifdef FR_VERSION
	draw_string_small(6*quantity_width - strlen(quantity_str)*8 - 2, quantity_y_offset-18, (unsigned char*)quantity_str, 1);
#else //FR_VERSION
	draw_string_small(win->len_x-strlen(quantity_str)*8-5, quantity_y_offset-19, (unsigned char*)quantity_str, 1);
#endif //FR_VERSION

	glColor3f(0.57f,0.67f,0.49f);
#ifdef ENGLISH
	draw_string_small (wear_items_x_offset + 33 - (8 * strlen(equip_str))/2, wear_items_y_offset-18, (unsigned char*)equip_str, 1);
#else //ENGLISH
	draw_string_small (wear_items_x_offset + wear_grid_size - (8 * strlen(equip_str))/2 + 1, wear_items_y_offset-18, (unsigned char*)equip_str, 1);
#endif //ENGLISH

	glColor3f(1.0f,1.0f,1.0f);
	//ok, now let's draw the objects...
	for(i=ITEM_NUM_ITEMS-1;i>=0;i--){
		if(item_list[i].quantity){
			int cur_pos;
			int x_start,x_end,y_start,y_end;

			//get the x and y
			cur_pos=i;
			if(cur_pos>=ITEM_WEAR_START){//the items we 'wear' are smaller
				cur_pos-=ITEM_WEAR_START;
				item_is_weared=1;
#ifdef ENGLISH
				x_start=wear_items_x_offset+33*(cur_pos%2)+1;
				x_end=x_start+32-1;
				y_start=wear_items_y_offset+33*(cur_pos/2);
				y_end=y_start+32-1;
				draw_item(item_list[i].image_id,x_start,y_start,32);
#else //ENGLISH
				x_start=wear_items_x_offset+wear_grid_size*(cur_pos%2)+1;
				x_end=x_start+wear_grid_size-1;
				y_start=wear_items_y_offset+wear_grid_size*(cur_pos/2);
				y_end=y_start+wear_grid_size-1;
				draw_item(item_list[i].image_id,x_start,y_start,wear_grid_size);
#endif //ENGLISH
			} else {
				item_is_weared=0;
				x_start=items_grid_size*(cur_pos%6)+1;
				x_end=x_start+items_grid_size-1;
				y_start=items_grid_size*(cur_pos/6);
				y_end=y_start+items_grid_size-1;
				draw_item(item_list[i].image_id,x_start,y_start,items_grid_size - 1);
			}


			if (item_list[i].cooldown_time > _cur_time)
			{
				float cooldown = ((float)(item_list[i].cooldown_time - _cur_time)) / ((float)item_list[i].cooldown_rate);
				float x_center = (x_start + x_end)*0.5f;
				float y_center = (y_start + y_end)*0.5f;
				float flash_effect_offset = 0.0f;

				if (cooldown < 0.0f)
					cooldown = 0.0f;
				else if (cooldown > 1.0f)
					cooldown = 1.0f;

				glDisable(GL_TEXTURE_2D);
				glEnable(GL_BLEND);

				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glBegin(GL_TRIANGLE_FAN);
					if (cooldown < 1.0f)
						flash_effect_offset = sin(pow(1.0f - cooldown, 4.0f) * 2.0f * M_PI * 30.0);
#ifdef FR_VERSION
					glColor4f(fr_quickitems_coolcolor.rouge - flash_effect_offset / 20.0f,
					          fr_quickitems_coolcolor.vert  - flash_effect_offset / 20.0f,
						      fr_quickitems_coolcolor.bleu  + flash_effect_offset / 8.0f,
					          fr_quickitems_coolcolor.alpha + flash_effect_offset / 15.0f);
#else //FR_VERSION
					glColor4f(0.14f - flash_effect_offset / 20.0f, 0.35f - flash_effect_offset / 20.0f, 0.82f + flash_effect_offset / 8.0f, 0.48f + flash_effect_offset / 15.0f);
#endif //FR_VERSION

					glVertex2f(x_center, y_center);

					if (cooldown >= 0.875f) {
						float t = tan(2.0f*M_PI*(1.0f - cooldown));
						glVertex2f(t*x_end + (1.0f - t)*x_center, y_start);
						glVertex2f(x_end, y_start);
						glVertex2f(x_end, y_end);
						glVertex2f(x_start, y_end);
						glVertex2f(x_start, y_start);
					} else if (cooldown >= 0.625f) {
						float t = 0.5f + 0.5f*tan(2.0f*M_PI*(0.75f - cooldown));
						glVertex2f(x_end, t*y_end + (1.0f - t)*y_start);
						glVertex2f(x_end, y_end);
						glVertex2f(x_start, y_end);
						glVertex2f(x_start, y_start);
					} else if (cooldown >= 0.375f) {
						float t = 0.5f + 0.5f*tan(2.0f*M_PI*(0.5f - cooldown));
						glVertex2f(t*x_start + (1.0f - t)*x_end, y_end);
						glVertex2f(x_start, y_end);
						glVertex2f(x_start, y_start);
					} else if (cooldown >= 0.125f) {
						float t = 0.5f + 0.5f*tan(2.0f*M_PI*(0.25f - cooldown));
						glVertex2f(x_start, t*y_start + (1.0f - t)*y_end);
						glVertex2f(x_start, y_start);
					} else {
						float t = tan(2.0f*M_PI*(cooldown));
						glVertex2f(t*x_start + (1.0f - t)*x_center, y_start);
					}

					glVertex2f(x_center, y_start);
				glEnd();

				glDisable(GL_BLEND);
				glEnable(GL_TEXTURE_2D);
			}

			if(!item_is_weared){
				safe_snprintf(str, sizeof(str), "%i", item_list[i].quantity);
				if ((mouseover_item_pos == i) && enlarge_text())
					draw_string_shadowed(x_start, (i&1)?(y_end-15):(y_end-25), (unsigned char*)str, 1,1.0f,1.0f,1.0f, 0.0f, 0.0f, 0.0f);
				else
					draw_string_small_shadowed(x_start, (i&1)?(y_end-15):(y_end-25), (unsigned char*)str, 1,1.0f,1.0f,1.0f, 0.0f, 0.0f, 0.0f);
			}
		}
	}
	mouseover_item_pos = -1;

	glColor3f(1.0f,1.0f,1.0f);

	//draw the load string
	if (!use_small_items_window)
	{
		safe_snprintf(str, sizeof(str),"%s:", attributes.carry_capacity.shortname);
		draw_string_small(items_grid_size*6+6, items_grid_size*6-SMALL_FONT_Y_LEN*2, (unsigned char*)str, 1);
		safe_snprintf(str, sizeof(str), "%i/%i", your_info.carry_capacity.cur, your_info.carry_capacity.base);
		draw_string_small(items_grid_size*6+6, items_grid_size*6-SMALL_FONT_Y_LEN, (unsigned char*)str, 1);
	}
	else
	{
		safe_snprintf(str, sizeof(str), "%s: %i/%i", attributes.carry_capacity.shortname, your_info.carry_capacity.cur, your_info.carry_capacity.base);
#ifdef FR_VERSION
		if (win->len_y > items_grid_size*6 + 25 + 15) draw_string_small(2, quantity_y_offset-19, (unsigned char*)str, 1);
#else //FR_VERSION
		draw_string_small(2, quantity_y_offset-19, (unsigned char*)str, 1);
#endif //FR_VERSION
	}

	//now, draw the inventory text, if any.
#ifdef FR_VERSION
	if (last_items_string_id != inventory_item_string_id)
	{
		put_small_text_in_box((unsigned char*)inventory_item_string, strlen(inventory_item_string), win->len_x-2, items_string);
		last_items_string_id = inventory_item_string_id;
	}
	i = (win->len_y - items_grid_size*6 - 25) / 15 - use_small_items_window;
	if (i>0) draw_string_small(4, items_grid_size*6 + 5, (unsigned char*)items_string, i);
#else //FR_VERSION
	if (last_items_string_id != inventory_item_string_id)
	{
		put_small_text_in_box((unsigned char*)inventory_item_string, strlen(inventory_item_string), win->len_x-10, items_string);
		last_items_string_id = inventory_item_string_id;
	}
	draw_string_small(4, win->len_y - (use_small_items_window?105:85), (unsigned char*)items_string, 4);
#endif //FR_VERSION

	// Render the grid *after* the images. It seems impossible to code
	// it such that images are rendered exactly within the boxes on all
	// cards
	glDisable(GL_TEXTURE_2D);
	glColor3f(0.77f,0.57f,0.39f);

	//draw the grids
	rendergrid(6, 6, 0, 0, items_grid_size, items_grid_size);

	glColor3f(0.57f,0.67f,0.49f);
#ifdef FR_VERSION
	rendergrid(2, 4, wear_items_x_offset, wear_items_y_offset, wear_grid_size, wear_grid_size);
#else //FR_VERSION
	rendergrid(2, 4, wear_items_x_offset, wear_items_y_offset, 33, 33);
#endif //FR_VERSION

	// draw the button boxes
	glColor3f(0.77f,0.57f,0.39f);
	for (i=0; i<NUMBUT; i++) {
		glBegin(GL_LINE_LOOP);
			glVertex3i(win->len_x-3, wear_items_y_offset+but_y_off[i],0);
			glVertex3i(win->len_x-(XLENBUT+3), wear_items_y_offset+but_y_off[i],0);
			glVertex3i(win->len_x-(XLENBUT+3), wear_items_y_offset+but_y_off[i]+YLENBUT,0);
			glVertex3i(win->len_x-3, wear_items_y_offset+but_y_off[i]+YLENBUT,0);
		glEnd();
	}

	// highlight a button with the mouse over
	if (mouse_over_but != -1)
	{
		glColor3f(0.99f,0.77f,0.55f);
		glBegin(GL_LINE_LOOP);
			glVertex3i(win->len_x-2, wear_items_y_offset+but_y_off[mouse_over_but]-1,0);
			glVertex3i(win->len_x-(XLENBUT+4), wear_items_y_offset+but_y_off[mouse_over_but]-1,0);
			glVertex3i(win->len_x-(XLENBUT+4), wear_items_y_offset+but_y_off[mouse_over_but]+YLENBUT+1,0);
			glVertex3i(win->len_x-2, wear_items_y_offset+but_y_off[mouse_over_but]+YLENBUT+1,0);
		glEnd();
	}

#ifdef FR_VERSION
	// draw the unwear button boxes
	glColor3f(0.77f,0.57f,0.39f);

	glEnable(GL_TEXTURE_2D);
	draw_string_small(wear_items_x_offset+wear_grid_size*0.5-3*4, wear_items_y_offset+wear_grid_size*4+10, (unsigned char*)" < ", 2);
/*
	TODO: Bouton d�sactiv� en attente d'une am�lioration serveur sur le DEPOSITE_ITEM
	draw_string_small(wear_items_x_offset+wear_grid_size*1.5-3*4, wear_items_y_offset+wear_grid_size*4+10, (unsigned char*)"<<<", 2);
*/
	glDisable(GL_TEXTURE_2D);

	glBegin(GL_LINE_LOOP);
		glVertex3i(wear_items_x_offset+4,  wear_items_y_offset+wear_grid_size*4+10,0);
		glVertex3i(wear_items_x_offset+wear_grid_size-4, wear_items_y_offset+wear_grid_size*4+10,0);
		glVertex3i(wear_items_x_offset+wear_grid_size-4, wear_items_y_offset+wear_grid_size*4+25,0);
		glVertex3i(wear_items_x_offset+4,  wear_items_y_offset+wear_grid_size*4+25,0);
	glEnd();
/*
	TODO: Bouton d�sactiv� en attente d'une am�lioration serveur sur le DEPOSITE_ITEM
	glBegin(GL_LINE_LOOP);
		glVertex3i(wear_items_x_offset+wear_grid_size+4,  wear_items_y_offset+wear_grid_size*4+10,0);
		glVertex3i(wear_items_x_offset+wear_grid_size*2-4, wear_items_y_offset+wear_grid_size*4+10,0);
		glVertex3i(wear_items_x_offset+wear_grid_size*2-4, wear_items_y_offset+wear_grid_size*4+25,0);
		glVertex3i(wear_items_x_offset+wear_grid_size+4,  wear_items_y_offset+wear_grid_size*4+25,0);
	glEnd();
*/
#endif //FR_VERSION

    //now, draw the quantity boxes
	glColor3f(0.3f,0.5f,1.0f);
	rendergrid(ITEM_EDIT_QUANT, 1, quantity_x_offset, quantity_y_offset, quantity_width, 20);

	glEnable(GL_TEXTURE_2D);

	// display help text for button if mouse over one
	if ((mouse_over_but != -1) && show_help_text) {
#ifdef FR_VERSION
		char *helpstr[NUMBUT] = { getall_help_str, stoall_help_str, ((disable_double_click) ?drpall_help_str :dcdrpall_help_str), itmlst_help_str, mixoneall_help_str };
#else //FR_VERSION
		char *helpstr[NUMBUT] = { stoall_help_str, getall_help_str, ((disable_double_click) ?drpall_help_str :dcdrpall_help_str), mixoneall_help_str, itmlst_help_str };
#endif //FR_VERSION
		show_help(helpstr[mouse_over_but], 0, win->len_y+10);
		show_help(cm_help_options_str, 0, win->len_y+10+SMALL_FONT_Y_LEN);
	}
	// show help set in the mouse_over handler
	else {
		int offset = 10;
		if (show_help_text && (item_help_str != NULL)) {
			show_help(item_help_str, 0, win->len_y+offset);
			offset += SMALL_FONT_Y_LEN;
		}
		if (item_desc_str != NULL)
#ifdef FR_VERSION
			show_help(item_desc_str, 260, win->len_y+offset - 15);
#else //FR_VERSION
			show_help(item_desc_str, 0, win->len_y+offset);
#endif //FR_VERSION
		item_help_str = NULL;
			item_desc_str = NULL;
		}

	mouse_over_but = -1;

#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE

	return 1;
}


/* return 1 if sent the move command */
int move_item(int item_pos_to_mov, int destination_pos)
{
	int drop_on_stack = 0;
#ifdef FR_VERSION
	int i;
	int temp_case = -1;
	int temp_case2 = -1;

	/* source provenant des �quipements et destination �galement */
	if ((item_pos_to_mov >= ITEM_WEAR_START) && (destination_pos >= ITEM_WEAR_START))
	{
		// �quipement vers lui-m�me = doucle-clic : on tente de le d�s�quiper
		if (item_pos_to_mov == destination_pos) return move_item(item_pos_to_mov, 0);

		// si la source est empilable on ne pourra assurer sa place une fois d�s�quip�e : abandon
		// TODO: � moins de tenter la recherche dans l'inventaire par item_uid
		if (item_list[item_pos_to_mov].is_stackable) return 0;

		// destination occup�e
		if (item_list[destination_pos].quantity)
		{
			// si la destination est empilable on ne pourra assurer sa place une fois d�s�quip�e : abandon
			// TODO: � moins de tenter la recherche dans l'inventaire par item_uid
			if (item_list[destination_pos].is_stackable) return 0;

			// il faut pouvoir d�s�quiper les 2 objets pour les remettre l'un � la place de l'autre
			for (i=0; i < ITEM_WEAR_START; i++)
			{
				if (! item_list[i].quantity)
				{
					if (temp_case < 0) temp_case = i;
					else { temp_case2 = i; break; }
				}
			}
			if (temp_case < 0) return 0;
			if (temp_case2 < 0) return 0;

			// nous pouvons op�rer l'�change des 2 �quipements
			equip_item(destination_pos, temp_case);
			item_list[temp_case].pos = temp_case;
			equip_item(item_pos_to_mov, temp_case2);
			item_list[temp_case2].pos = temp_case2;
			equip_item(temp_case2, destination_pos);
			equip_item(temp_case, item_pos_to_mov);
			return 1;
		}
		// destination libre
		else
		{
			for (i=0; i < ITEM_WEAR_START; i++) if (! item_list[i].quantity) { temp_case = i; break; }
			if (temp_case < 0) return 0;
			equip_item(item_pos_to_mov, temp_case);
			item_list[temp_case].pos = temp_case;
			equip_item(temp_case, destination_pos);
			return 1;
		}
	}
	else
#endif //FR_VERSION
	/* if the dragged item is equipped and the destintion is occupied, try to find another slot */
	if ((item_pos_to_mov >= ITEM_WEAR_START) && (item_list[destination_pos].quantity)){
		int i;
		int have_free_pos = 0;
		/* find first free slot, use a free slot in preference to a stack as the server does the stacking */
		for (i = 0; i < ITEM_WEAR_START; i++){
			if (!item_list[i].quantity){
				destination_pos = i;
				have_free_pos = 1;
				break;
			}
		}
#ifdef FR_VERSION
		/* Si aucune case libre mais objet empillable, on le cherche dans l'inventaire avec son uid */
		if (!have_free_pos && item_list[item_pos_to_mov].is_stackable && (item_list[item_pos_to_mov].id != unset_item_uid))
		{
			for (i = 0; i < ITEM_WEAR_START; i++){
				if (item_list[i].id == item_list[item_pos_to_mov].id)
				{
					destination_pos = i;
					have_free_pos = 1;
					break;
				}
			}
		}
#endif //FR_VERSION
		/* if no free slot, try to find an existing stack.  But be careful of dupe image ids */
		if (!have_free_pos && item_list[item_pos_to_mov].is_stackable){
			int num_stacks_found = 0;
			int proposed_slot = -1;
			for (i = 0; i < ITEM_WEAR_START; i++){
				if (item_list[i].is_stackable && (item_list[i].image_id == item_list[item_pos_to_mov].image_id)){
					num_stacks_found++;
					proposed_slot = i;
				}
			}

			/* only use the stack if  we're sure there are no other possibilities */
			if (num_stacks_found == 1){
				destination_pos = proposed_slot;
				drop_on_stack = 1;
			}
			else
				set_shown_string(c_red2, items_stack_str);
			/*  This still leaves one possibility for the dreaded server accusation.
				If we have no free inventory slots, one or more stackable items
				unequipped, and a single, different equipped item with the same id as
				the aforementioned stack.  When we try to unequip the single item, the
				client tries to place it on that stack. This may mean we have to
				abandon this feature; i.e. allowing a stackable item to be unequipping
				when there are no free slots. (pjbroad/bluap) */
		}
	}
#ifdef FR_VERSION
	/* source provenant du sac et destination (sac ou �quipement) d�j� occup�e par un autre objet */
	else if ((item_list[destination_pos].quantity > 0) && (item_pos_to_mov != destination_pos))
	{
		// destination dans les �quipements sur un objet empilable
		if ((destination_pos >= ITEM_WEAR_START) && (item_list[destination_pos].is_stackable))
		{
			// �change de place impossible car le serveur risque de d�s�quiper sur un stack
			// on se contente alors de la m�thode d'origine (�change d'�quipement mais pas de place)
			if (! move_item(destination_pos, 0)) return 0;
			equip_item(item_pos_to_mov, destination_pos);
			return 1;
		}

		// pour le reste il faut trouver une place libre dans le sac pour l'�change
		for (i=0; i < ITEM_WEAR_START; i++) if (! item_list[i].quantity) { temp_case = i; break; }
		if (temp_case < 0) return 0;

		// destination dans le sac : lib�ration de la case et drop du drag
		equip_item(destination_pos, temp_case);
		item_list[temp_case].pos = temp_case;

		if ((destination_pos >= ITEM_WEAR_START) && (item_list[item_pos_to_mov].quantity > 1))
		{
			// s'il s'agit d'�quiper un objet empilable pr�sent en plusieurs exemplaire
			// on ne tentera pas en vain de replacer sur une case qui ne sera pas libre
			equip_item(item_pos_to_mov, destination_pos);
		}
		else
		{
			// n�anmoins le replacement pourra encore �chou� dans certains cas
			// s'il s'agit d'�quiper un objet ne pouvant �tre port�... tant pis !
			equip_item(item_pos_to_mov, destination_pos);
			equip_item(temp_case, item_pos_to_mov);
		}
		return 1;
	}
#endif //FR_VERSION

	/* move item */
	if(drop_on_stack || !item_list[destination_pos].quantity){
		Uint8 str[20];
		//send the drop info to the server
		str[0]=MOVE_INVENTORY_ITEM;
		str[1]=item_list[item_pos_to_mov].pos;
		str[2]=destination_pos;
		my_tcp_send(my_socket,str,3);
		return 1;
	}
	else
		return 0;
}

static void equip_item(int item_pos_to_equip, int destination_pos)
{
	Uint8 str[20];
	//send the drop info to the server
	str[0]=MOVE_INVENTORY_ITEM;
	str[1]=item_list[item_pos_to_equip].pos;
	str[2]=destination_pos;
	my_tcp_send(my_socket,str,3);
}


#ifdef FR_VERSION
void wheel_change_quantity(Uint32 flags)
{
	int ctrl_on = flags & ELW_CTRL;
	int shift_on = flags & ELW_SHIFT;
	int step = 1;

	if (flags & ELW_WHEEL_UP)
	{
		if (!ctrl_on || !shift_on)
		{
			step = ceil(pow(10, quantities.quantity[quantities.selected].len) / 100);
			if (ctrl_on) step = ceil(step/10);
			if (shift_on) step = step*10;
		}
		quantities.quantity[quantities.selected].val += step;
		if ((item_dragged!=-1) && (quantities.quantity[quantities.selected].val>item_dragged_max_quantity))
			quantities.quantity[quantities.selected].val = item_dragged_max_quantity;
		if (quantities.quantity[quantities.selected].val > 100000)
			quantities.quantity[quantities.selected].val = 100000;
	}
	else
	{
		// si on d�cr�mente la quantit� d'un objet venant du sac, on part au max de sa quantit� totale dispo
		if ((item_dragged!=-1) && (quantities.quantity[quantities.selected].val > item_dragged_max_quantity))
		{
			quantities.quantity[quantities.selected].val = item_dragged_max_quantity;
			sprintf(quantities.quantity[quantities.selected].str, "%i", quantities.quantity[quantities.selected].val);
			quantities.quantity[quantities.selected].len = strlen(quantities.quantity[quantities.selected].str);
		}
		// si on d�cr�mente la quantit� d'un objet venant du d�pot, on part au max de sa quantit� totale dispo
		else if ((storage_item_dragged!=-1) && (quantities.quantity[quantities.selected].val > storage_items[storage_item_dragged].quantity))
		{
			quantities.quantity[quantities.selected].val = storage_items[storage_item_dragged].quantity;
			sprintf(quantities.quantity[quantities.selected].str, "%i", quantities.quantity[quantities.selected].val);
			quantities.quantity[quantities.selected].len = strlen(quantities.quantity[quantities.selected].str);
		}
		if (!ctrl_on || !shift_on)
		{
			step = ceil(pow(10, quantities.quantity[quantities.selected].len) / 100);
			if (ctrl_on) step = ceil(step/10);
			if (shift_on) step = step*10;
		}
		if (quantities.quantity[quantities.selected].val > step) quantities.quantity[quantities.selected].val -= step;
		else quantities.quantity[quantities.selected].val = 1;
	}

	sprintf(quantities.quantity[quantities.selected].str, "%i", quantities.quantity[quantities.selected].val);
	quantities.quantity[quantities.selected].len = strlen(quantities.quantity[quantities.selected].str);
	item_quantity = quantities.quantity[quantities.selected].val;
}
#endif //FR_VERSION


int click_items_handler(window_info *win, int mx, int my, Uint32 flags)
{
    Uint8 str[100];
	int right_click = flags & ELW_RIGHT_MOUSE;
	int ctrl_on = flags & ELW_CTRL;
	int shift_on = flags & ELW_SHIFT;
#ifdef ENGLISH
	int alt_on = flags & ELW_ALT;
	int pos;
	actor *me;
#else //ENGLISH
	//int pos; // autres variables utilis�es maintenant dans get_all_handler()
#endif //ENGLISH

#ifdef FR_VERSION
	if (flags & ELW_WHEEL)
	{
		// modification de la quantit� de la case en cours d'�dition avec la molette
		if ((allow_wheel_quantity_edit) && (edit_quantity != -1))
		{
			quantities.selected = edit_quantity;
			wheel_change_quantity(flags);
			return 1;
		}
		// modification de la quantit� de l'objet en cours de drag avec la molette
		else if ((allow_wheel_quantity_drag) && ((item_dragged!=-1) || (storage_item_dragged!=-1)))
		{
			if (quantities.selected != ITEM_EDIT_QUANT)
			{
				*quantities.quantity[ITEM_EDIT_QUANT].str = *quantities.quantity[quantities.selected].str;
				quantities.quantity[ITEM_EDIT_QUANT].len = quantities.quantity[quantities.selected].len;
				quantities.quantity[ITEM_EDIT_QUANT].val = quantities.quantity[quantities.selected].val;
				quantities.selected = ITEM_EDIT_QUANT;
			}
			wheel_change_quantity(flags);
			return 1;
		}
	}
#endif //FR_VERSION

	// only handle mouse button clicks, not scroll wheels moves (unless its the mix button)
	if (((flags & ELW_MOUSE_BUTTON) == 0) && (over_button(win, mx, my) != BUT_MIX)) return 0;

	// ignore middle mouse button presses
	if ((flags & ELW_MID_MOUSE) != 0) return 0;

	if (!right_click && over_button(win, mx, my) != -1)
		do_click_sound();

	if(right_click) {
		if(item_dragged!=-1 || use_item!=-1 || storage_item_dragged!=-1){
			use_item=-1;
			item_dragged=-1;
			storage_item_dragged=-1;
			item_action_mode=ACTION_WALK;
			return 1;
		}

#ifdef FR_VERSION
		if(mx>=wear_items_x_offset && mx<wear_items_x_offset+2*wear_grid_size && my>=wear_items_y_offset && my<wear_items_y_offset+4*wear_grid_size) {
#else //FR_VERSION
		if(mx>=wear_items_x_offset && mx<wear_items_x_offset+66 && my>=wear_items_y_offset && my<wear_items_y_offset+133) {
#endif //FR_VERSION
			switch(item_action_mode){
				case ACTION_WALK:
					item_action_mode=ACTION_LOOK;
					break;
				case ACTION_LOOK:
				default:
					item_action_mode=ACTION_WALK;
			}
			return 1;
		} else if(mx>=quantity_x_offset && mx<quantity_x_offset+ITEM_EDIT_QUANT*quantity_width && my>=quantity_y_offset && my<quantity_y_offset+20){
			//fall through...
		} else {
			switch(item_action_mode) {
			case ACTION_WALK:
				item_action_mode=ACTION_LOOK;
				break;
			case ACTION_LOOK:
				item_action_mode=ACTION_USE;
				break;
			case ACTION_USE:
				item_action_mode=ACTION_USE_WITEM;
				break;
			case ACTION_USE_WITEM:
				item_action_mode=ACTION_WALK;
				break;
			default:
				item_action_mode=ACTION_WALK;
			}
			return 1;
		}
	}

	if(item_action_mode==ACTION_USE_WITEM)	action_mode=ACTION_USE_WITEM;
#ifndef ENGLISH
	if(item_action_mode==ACTION_USE)	action_mode=ACTION_USE;
#endif //ENGLISH

	//see if we changed the quantity
	if(mx>=quantity_x_offset && mx<quantity_x_offset+ITEM_EDIT_QUANT*quantity_width
			&& my>=quantity_y_offset && my<quantity_y_offset+20) {
		int pos=get_mouse_pos_in_grid(mx, my, ITEM_EDIT_QUANT, 1, quantity_x_offset, quantity_y_offset, quantity_width, 20);

		if(pos==-1){
		} else if(flags & ELW_LEFT_MOUSE){
			if(edit_quantity!=-1){
				if(!quantities.quantity[edit_quantity].len){
					//Reset the quantity
					reset_quantity(edit_quantity);
				}
				edit_quantity=-1;
			}

			item_quantity=quantities.quantity[pos].val;
			quantities.selected=pos;
		} else if(right_click){
#ifdef FR_VERSION
			// si cette quantit� est d�j� en cours d'�dition
			// le clic-droit annule l'�dition en remettant la valeur par d�faut
			if (edit_quantity == pos)
			{
				//Reset the quantity
				//reset_quantity(edit_quantity);
				safe_snprintf (quantities.quantity[pos].str, sizeof(quantities.quantity[pos].str), "%d", last_quantity);
				quantities.quantity[pos].len = strlen (quantities.quantity[pos].str);
				quantities.quantity[pos].val = last_quantity;
				edit_quantity=-1;
				return 1;
			}
			last_quantity = quantities.quantity[pos].val;
			quantities.selected=pos;
#endif //FR_VERSION
			//Edit the given quantity
			edit_quantity=pos;
		}

		return 1;
	}

	if(edit_quantity!=-1){
		if(!quantities.quantity[edit_quantity].len)reset_quantity(edit_quantity);
		item_quantity=quantities.quantity[edit_quantity].val;
		quantities.selected=edit_quantity;
		edit_quantity=-1;
	}

	//see if we clicked on any item in the main category
	else if(mx>0 && mx < 6*items_grid_size &&
	   my>0 && my < 6*items_grid_size) {
		int pos=get_mouse_pos_in_grid(mx, my, 6, 6, 0, 0, items_grid_size, items_grid_size);

#ifdef NEW_SOUND
		item_list[pos].action = ITEM_NO_ACTION;
		item_list[pos].action_time = 0;
#endif // NEW_SOUND
		if(pos==-1) {
		} else if(item_dragged!=-1){
			if(item_dragged == pos){ //let's try auto equip
				int i;
				for(i = ITEM_WEAR_START; i<ITEM_WEAR_START+8;i++) {
					if(item_list[i].quantity<1) {
						move_item(pos,i);
						item_dragged=-1;
						break;
					}
				}
			} else {
				if (move_item(item_dragged, pos)){
					do_drop_item_sound();
				}
				else {
					do_alert1_sound();
				}
				item_dragged=-1;
			}

		}
		else if(storage_item_dragged!=-1){
			str[0]=WITHDRAW_ITEM;
			*((Uint16*)(str+1))=SDL_SwapLE16(storage_items[storage_item_dragged].pos);
			*((Uint32*)(str+3))=SDL_SwapLE32(item_quantity);
			my_tcp_send(my_socket, str, 6);
			do_drop_item_sound();
			if(storage_items[storage_item_dragged].quantity<=item_quantity) storage_item_dragged=-1;
		}
		else if(item_list[pos].quantity){
#ifdef FR_VERSION
			if(ctrl_on && shift_on){
				// utilisation de Ctrl+Shift au lieu de Ctrl pour d�poser l'objet au sol
#else //FR_VERSION
			if (ctrl_on && (items_mod_click_any_cursor || (item_action_mode==ACTION_WALK))) {
#endif //FR_VERSION
				str[0]=DROP_ITEM;
				str[1]=item_list[pos].pos;
				if(item_list[pos].is_stackable)
					*((Uint32 *)(str+2))=SDL_SwapLE32(item_list[pos].quantity);
				else
					*((Uint32 *)(str+2))=SDL_SwapLE32(36);//Drop all
				my_tcp_send(my_socket, str, 6);
				do_drop_item_sound();
#ifdef FR_VERSION
			} else if (shift_on) {
				// ajoute l'objet en raccourci sur le 1er emplacement libre de la barre rapide
				affect_fr_quickitems(item_list[pos].pos);
			} else if (ctrl_on) {
				// range cet objet (toute quantit� dispo dans le sac) dans le d�pot
				if ((storage_win<0) || view_only_storage || !get_show_window(storage_win)) return 0;
				str[0] = DEPOSITE_ITEM;
				str[1] = pos;
				if(item_list[pos].is_stackable)
					*((Uint32*)(str+2))=SDL_SwapLE32(item_list[pos].quantity);
				else
					*((Uint32*)(str+2))=SDL_SwapLE32(36);
				my_tcp_send(my_socket, str, 6);
				do_drop_item_sound();
#else //FR_VERSION
			} else if (alt_on && (items_mod_click_any_cursor || (item_action_mode==ACTION_WALK))) {
				if ((storage_win >= 0) && (get_show_window(storage_win)) && (view_only_storage == 0)) {
						str[0]=DEPOSITE_ITEM;
						str[1]=item_list[pos].pos;
						*((Uint32*)(str+2))=SDL_SwapLE32(INT_MAX);
						my_tcp_send(my_socket, str, 6);
					}
					do_drop_item_sound();
				} else {
					if (view_only_storage)
						drop_fail_time = SDL_GetTicks();
					do_alert1_sound();
				}
#endif //FR_VERSION
			} else if(item_action_mode==ACTION_LOOK) {
				click_time=cur_time;
				str[0]=LOOK_AT_INVENTORY_ITEM;
				str[1]=item_list[pos].pos;
				my_tcp_send(my_socket,str,2);
			} else if(item_action_mode==ACTION_USE) {
				if(item_list[pos].use_with_inventory){
					str[0]=USE_INVENTORY_ITEM;
					str[1]=item_list[pos].pos;
					my_tcp_send(my_socket,str,2);
#ifdef NEW_SOUND
					item_list[pos].action = USE_INVENTORY_ITEM;
#ifdef _EXTRA_SOUND_DEBUG
//					printf("Using item: %d, inv pos: %d, Image ID: %d\n", item_list[pos].pos, pos, item_list[pos].image_id);
#endif // _EXTRA_SOUND_DEBUG
#endif // NEW_SOUND
				}
			} else if(item_action_mode==ACTION_USE_WITEM) {
				if(use_item!=-1) {
					str[0]=ITEM_ON_ITEM;
					str[1]=item_list[use_item].pos;
					str[2]=item_list[pos].pos;
					my_tcp_send(my_socket,str,3);
#ifdef NEW_SOUND
					item_list[use_item].action = ITEM_ON_ITEM;
					item_list[pos].action = ITEM_ON_ITEM;
#ifdef _EXTRA_SOUND_DEBUG
//					printf("Using item: %d on item: %d, Image ID: %d\n", pos, use_item, item_list[pos].image_id);
#endif // _EXTRA_SOUND_DEBUG
#endif // NEW_SOUND
					if (!shift_on)
						use_item=-1;
				} else {
					use_item=pos;
				}
			} else {
#ifdef FR_VERSION
				// calcul lors de la saisie de l'objet de la quantit� max transportable
				int i;
				item_dragged_max_quantity = 0;
				if (item_list[pos].is_stackable) item_dragged_max_quantity = item_list[pos].quantity;

				if (! item_list[pos].is_stackable)
				{
					// parcours du contenu du sac pour les items non empilables
					for (i=0; i<ITEM_WEAR_START; i++)
					{
						if (item_list[i].quantity < 1) continue;
						if (item_list[i].id != unset_item_uid)
						{
							if (item_list[i].id != item_list[pos].id) continue;
						}
						else
						if ((item_list[i].image_id != item_list[pos].image_id) || (item_list[i].is_stackable)) continue;
						item_dragged_max_quantity += item_list[i].quantity;
					}
				}
				else item_dragged_max_quantity = item_list[pos].quantity;
#endif //FR_VERSION
				item_dragged=pos;
				do_drag_item_sound();
			}
		}
	}

   	// Get All button
	else if(over_button(win, mx, my)==BUT_GET){
#ifndef ENGLISH
		// d�placement du code dans une fonction qui pourra aussi �tre appel�e par un raccourci clavier
		get_all_handler();
#else //ENGLISH
		int x,y;
     	me = get_our_actor ();
		if(!me)return(1);
       	x=me->x_tile_pos;
        y=me->y_tile_pos;

	    for(pos=0;pos<NUM_BAGS;pos++){
			if(bag_list[pos].x != 0 && bag_list[pos].y != 0 &&
				bag_list[pos].x == x && bag_list[pos].y == y)
            {
				if(get_show_window(ground_items_win))
					pick_up_all_items();
				else {
					// if auto empty bags enable, set the open timer
					if (items_auto_get_all)
						ground_items_empty_next_bag = SDL_GetTicks();
					else
						ground_items_empty_next_bag = 0;
                    open_bag(bag_list[pos].obj_3d_id);
                    }
				break; //we should only stand on one bag
                }
            }
        }
#endif //ENGLISH
    }

   	// Sto All button
	else if(over_button(win, mx, my)==BUT_STORE && storage_win >= 0 && view_only_storage == 0 && get_show_window(storage_win) /*thanks alberich*/){
#ifdef FR_VERSION
		str[0]=TOUT_DEPOT;
		my_tcp_send(my_socket, str, 1);
#else //FR_VERSION
#ifdef STORE_ALL
		/*
		* Future code to save server load by having one byte to represent the 36 slot inventory loop. Will need server support.
		*/
		str[0]=DEPOSITE_ITEM;
		str[1]=STORE_ALL;
		my_tcp_send(my_socket, str, 2);
#else
		for(pos=((items_stoall_nofirstrow)?6:0);pos<((items_stoall_nolastrow)?30:36);pos++){
			if(item_list[pos].quantity>0){
				str[0]=DEPOSITE_ITEM;
				str[1]=item_list[pos].pos;
				*((Uint32*)(str+2))=SDL_SwapLE32(item_list[pos].quantity);
				my_tcp_send(my_socket, str, 6);
			}
		}
#endif
#endif //FR_VERSION
	}

	// Drop All button
	else if(over_button(win, mx, my)==BUT_DROP){
		drop_all_handler();
	}

	// Mix One/All button
	else if(over_button(win, mx, my)==BUT_MIX){
		if (items_mix_but_all)
			mix_handler(255, mixbut_empty_str);
		else
			mix_handler(1, mixbut_empty_str);
	}

	// Item List button
	else if (over_button(win, mx, my)==BUT_ITEM_LIST)
		toggle_items_list_window(win);

#ifdef FR_VERSION
	else if(my>=wear_items_y_offset+wear_grid_size*4+10 && my<=wear_items_y_offset+wear_grid_size*4+25) {
		int i, j, last_pos=0;
		// bouton pour mettre tout l'�quipement dans le sac
		if (mx>=wear_items_x_offset+4 && mx<=wear_items_x_offset+wear_grid_size-4)
		{
			for (i=ITEM_WEAR_START; i<ITEM_NUM_ITEMS; i++)
			{
				int destination_pos = -1;
				if (item_list[i].quantity < 1) continue;
				if (item_list[i].is_stackable) for (j = 0; j < ITEM_WEAR_START; j++)
				{
					if (item_list[j].quantity < 1) continue;
					if (item_list[j].id != unset_item_uid)
					{
						if (item_list[i].id != item_list[j].id) continue;
					}
					else
					if ((item_list[i].image_id != item_list[j].image_id) || item_list[j].is_stackable) continue;
					destination_pos = j;
					break;
				}
				if (destination_pos < 0) for (j = last_pos; j < ITEM_WEAR_START; j++)
				{
					if (item_list[j].quantity > 0) continue;
					destination_pos = j;
					last_pos = j+1;
					break;
				}
				if (destination_pos < 0) continue;
				equip_item(i, destination_pos);
			}
			return 1;
		}
		// bouton pour mettre tout l'�quipement dans le d�pot
/*
	ATTENTION : lorsqu'on demande au serveur de ranger un objet au d�pot,
	celui-ci ne tient pas compte de la position dans le sac de l'objet indiqu�.
	Il parcourra tout le sac � la recherche de cette objet pour ranger la quantit� demand�e
	en partant de la premi�re case. Il est alors impossible au client de savoir
	quelle case va �tre finalement lib�r�e dans le sac (la recherche du premier objet
	semblable dans le sac n'�tant pas fiable avec l'image_id).

	Avec l'ajout des ITEM_UID, le client pourrait deviner ce que va faire le serveur :
	rechercher la 1�re occurence de l'objet qui sera rang�e et �ventuellement enchainer
	avec un d�placement de l'objet d�s�quip� vers la case de l'objet rang�.

	Cel� resterait un enchainement de manipulation un peu fastidieux...
	D'autant que ce comportement du serveur est regrettable aussi le reste du temps :
	avoir 20 objets identiques dans son sac, ranger le dernier pour voir que le serveur
	range le premier quoiqu'on lui dise...

	TODO: L'id�al serait donc de modifier le comportement du serveur sur un DEPOSITE_ITEM,
	qu'il se comporte de la m�me mani�re que lors d'un DROP_ITEM (d�pot au sol) :
	il parcours l'inventaire � partir de la position indiqu�e pour trouver les objets
	correspondant � retirer, une fois arriv� � la 36�me case, si la quantit� demand�e
	n'est pas atteinte, il refait une boucle de la position indiqu�e jusqu'� 0.

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
#endif //FR_VERSION

	//see if we clicked on any item in the wear category
#ifdef FR_VERSION
	else if(mx>wear_items_x_offset && mx<wear_items_x_offset+2*wear_grid_size &&
	   my>wear_items_y_offset && my<wear_items_y_offset+4*wear_grid_size){
		int pos=36+get_mouse_pos_in_grid(mx, my, 2, 4, wear_items_x_offset, wear_items_y_offset, wear_grid_size, wear_grid_size);
#else //FR_VERSION
	else if(mx>wear_items_x_offset && mx<wear_items_x_offset+2*33 &&
	   my>wear_items_y_offset && my<wear_items_y_offset+4*33){
		int pos=36+get_mouse_pos_in_grid(mx, my, 2, 4, wear_items_x_offset, wear_items_y_offset, 32, 32);
#endif //FR_VERSION

		if(pos<36) {
#ifdef FR_VERSION
		}
		// on lache un objet du d�pot vers une case d'�quipement
		else if (storage_item_dragged!=-1) {
			int i, temp_case = -1;
			// recherche d'une case libre dans le sac qui servira d'interm�diaire
			for (i=0; i < ITEM_WEAR_START; i++)
			{
				if (item_list[i].quantity > 0)
				{
					if (! item_list[i].is_stackable) continue;
					if (item_list[i].id != unset_item_uid)
					{
						if (item_list[i].id != storage_items[storage_item_dragged].id) continue;
					}
					else
					if (item_list[i].image_id != storage_items[storage_item_dragged].image_id) continue;
					temp_case = i; break;
				}
				else if (temp_case < 0) temp_case = i;
			}
			if (temp_case < 0) return 0;

			// recherche d'une case libre dans les �quipements (en priorit� celle cliqu�e)
			if (item_list[pos].quantity > 0)
			{
				for (i=ITEM_WEAR_START; i < ITEM_NUM_ITEMS; i++) if (! item_list[i].quantity) { pos = i; break; }
				if (item_list[pos].quantity > 0) return 0;
			}

			// on met dans le sac un seul exemplaire de l'objet du d�pot avant de l'�quiper
			str[0]=WITHDRAW_ITEM;
			*((Uint16*)(str+1))=SDL_SwapLE16(storage_items[storage_item_dragged].pos);
			*((Uint32*)(str+3))=SDL_SwapLE32(1);
			my_tcp_send(my_socket, str, 6);
			item_list[temp_case].pos = temp_case;
			equip_item(temp_case, pos);

			do_drop_item_sound();
			storage_item_dragged=-1;
			return 1;
#endif //FR_VERSION
		} else if(item_list[pos].quantity){
			if(item_action_mode == ACTION_LOOK) {
				str[0]=LOOK_AT_INVENTORY_ITEM;
				str[1]=item_list[pos].pos;
				my_tcp_send(my_socket, str, 2);
			} else if(item_dragged==-1 && left_click) {
#ifdef FR_VERSION
				// Ctrl+click : on tente le rangement au d�pot d'un �quipement port�
				if ((ctrl_on) && (storage_win!=-1) && !view_only_storage && get_show_window(storage_win))
				{
					int i, temp_case = -1;
					// si la source est empilable on doit d'abord trouver s'il sera empil�
					if (item_list[pos].is_stackable) for (i=0; i < ITEM_WEAR_START; i++)
					{
						if (item_list[i].id != unset_item_uid)
						{
							if (item_list[i].id != item_list[pos].id) continue;
						}
						else
						if ((item_list[i].image_id != item_list[pos].image_id) || ! item_list[i].is_stackable) continue;
						temp_case = i;
						break;
					}
					// sinon recherche d'une case libre dans le sac pour d�s�quiper
					if (temp_case < 0)
					{
						for (i=0; i < ITEM_WEAR_START; i++) if (! item_list[i].quantity) { temp_case = i; break; }
					}
					if (temp_case < 0) return 0;

					// demande de d�placement dans le sac puis vers le d�pot
					equip_item(pos, temp_case);
					str[0]=DEPOSITE_ITEM;
					str[1]=temp_case;
					*((Uint32*)(str+2))=SDL_SwapLE32(1);
					my_tcp_send(my_socket, str, 6);
					return 1;
				}
				// Shift+click : ajoute l'objet en raccourci sur le 1er emplacement libre de la barre rapide
				else if (shift_on) {
					affect_fr_quickitems(item_list[pos].pos);
					return 1;
				}

				// calcul lors de la saisie de l'objet de la quantit� max transportable = 1 pour un objet port�
				item_dragged_max_quantity = 1;
#endif //FR_VERSION
				item_dragged=pos;
				do_drag_item_sound();
			}
			else if(item_dragged!=-1 && left_click) {
#ifdef FR_VERSION
				// on lache un objet de l'inventaire vers une case d'�quipement occup�e
				if (((item_dragged == pos) && move_item(item_dragged, pos))
					|| (allow_equip_swap && move_item(item_dragged, pos))) {
					do_drag_item_sound();
				}
#else //FR_VERSION
				int can_move = (item_dragged == pos) || allow_equip_swap;
				if (allow_equip_swap && move_item(pos, 0)) {
					equip_item(item_dragged, pos);
					do_get_item_sound();
				}
#endif //FR_VERSION
				else {
					do_alert1_sound();
				}
				item_dragged=-1;
			}
		} else if(item_dragged!=-1){
#ifdef FR_VERSION
			// on lache un objet de l'inventaire vers une case d'�quipement vide
			move_item(item_dragged, pos);
#else //FR_VERSION
			equip_item(item_dragged, pos);
#endif //FR_VERSION
			item_dragged=-1;
			do_drop_item_sound();
		}
	}

	// clear the message area if double-clicked
	else if (my > (win->len_y - (use_small_items_window?105:85))) {
		static Uint32 last_click = 0;
		if (safe_button_click(&last_click)) {
			set_shown_string(0,"");
			return 1;
		}
	}

	return 1;
}

void set_description_help(int pos)
{
	Uint16 item_id = item_list[pos].id;
	int image_id = item_list[pos].image_id;
	if (show_item_desc_text && item_info_available() && (get_item_count(item_id, image_id) == 1))
		item_desc_str = get_item_description(item_id, image_id);
}

int mouseover_items_handler(window_info *win, int mx, int my) {
	int pos;

	// check and record if mouse if over a button
	if ((mouse_over_but = over_button(win, mx, my)) != -1)
		return 0; // keep standard cursor

	if(mx>0&&mx<6*items_grid_size&&my>0&&my<6*items_grid_size){
		pos=get_mouse_pos_in_grid(mx, my, 6, 6, 0, 0, items_grid_size, items_grid_size);

		if(pos==-1) {
		} else if(item_list[pos].quantity){
			set_description_help(pos);
#ifdef ENGLISH
			if ((item_dragged == -1) && (items_mod_click_any_cursor || (item_action_mode==ACTION_WALK)))
					item_help_str = mod_click_item_help_str;
#endif //ENGLISH
			if(item_action_mode==ACTION_LOOK) {
				elwin_mouse=CURSOR_EYE;
			} else if(item_action_mode==ACTION_USE) {
				elwin_mouse=CURSOR_USE;
			} else if(item_action_mode==ACTION_USE_WITEM) {
				elwin_mouse=CURSOR_USE_WITEM;
#ifdef ENGLISH
				if (use_item!=-1)
					item_help_str = multiuse_item_help_str;
#endif //ENGLISH
			} else {
#ifndef ENGLISH
				if (item_dragged == -1)
					item_help_str = pick_item_help_str;
#endif //ENGLISH
				elwin_mouse=CURSOR_PICK;
			}
			mouseover_item_pos = pos;

			return 1;
		}
#ifdef FR_VERSION
	} else if(mx>wear_items_x_offset && mx<wear_items_x_offset+2*wear_grid_size &&
	          my>wear_items_y_offset && my<wear_items_y_offset+4*wear_grid_size){
		pos=36+get_mouse_pos_in_grid(mx, my, 2, 4, wear_items_x_offset, wear_items_y_offset, wear_grid_size, wear_grid_size);
#else //FR_VERSION
	} else if(mx>wear_items_x_offset && mx<wear_items_x_offset+2*33 &&
				my>wear_items_y_offset && my<wear_items_y_offset+4*33) {
		pos=36+get_mouse_pos_in_grid(mx, my, 2, 4, wear_items_x_offset, wear_items_y_offset, 33, 33);
#endif //FR_VERSION
		item_help_str = equip_here_str;
		if(pos==-1) {
		} else if(item_list[pos].quantity){
			set_description_help(pos);
			if(item_action_mode==ACTION_LOOK) {
				elwin_mouse=CURSOR_EYE;
			} else if(item_action_mode==ACTION_USE) {
				elwin_mouse=CURSOR_USE;
			} else if(item_action_mode==ACTION_USE_WITEM) {
				elwin_mouse=CURSOR_USE_WITEM;
			} else {
				elwin_mouse=CURSOR_PICK;
			}

			return 1;
		}
	} else if(show_help_text && mx>quantity_x_offset && mx<quantity_x_offset+ITEM_EDIT_QUANT*quantity_width &&
			my>quantity_y_offset && my<quantity_y_offset+6*20){
		item_help_str = quantity_edit_str;
	} else if (show_help_text && *inventory_item_string && (my > (win->len_y - (use_small_items_window?105:85)))) {
		item_help_str = (disable_double_click)?click_clear_str :double_click_clear_str;
#ifdef FR_VERSION
	} else if(my>=wear_items_y_offset+wear_grid_size*4+10 && my<=wear_items_y_offset+wear_grid_size*4+25) {
		if (mx>=wear_items_x_offset+4 && mx<=wear_items_x_offset+wear_grid_size-4)
		{
			show_help(unwear_all_to_inv_str, 0, quantity_y_offset+30);
		}
		else if (mx>=wear_items_x_offset+wear_grid_size+4 && mx<=wear_items_x_offset+wear_grid_size*2-4)
		{
//			show_help(unwear_all_to_sto_str, 0, quantity_y_offset+30);
		}
#endif //FR_VERSION
	}

	return 0;
}

int keypress_items_handler(window_info * win, int x, int y, Uint32 key, Uint32 keysym)
{
	if(edit_quantity!=-1){
		char * str=quantities.quantity[edit_quantity].str;
		int * len=&quantities.quantity[edit_quantity].len;
		int * val=&quantities.quantity[edit_quantity].val;

		if(key==SDLK_DELETE){
			reset_quantity(edit_quantity);
			edit_quantity=-1;
			return 1;
		} else if(key==SDLK_BACKSPACE){
			if(*len>0){
				(*len)--;
				str[*len]=0;
				*val=atoi(str);
			}
			return 1;
		} else if(keysym=='\r'){
			if(!*val){
				reset_quantity(edit_quantity);
			}
			item_quantity=*val;
			quantities.selected=edit_quantity;
			edit_quantity=-1;
			return 1;
		} else if(keysym>='0' && keysym<='9' && *len<5){
			str[*len]=keysym;
			(*len)++;
			str[*len]=0;

			*val=atoi(str);
			return 1;
		}
	}

	return 0;
}

#ifndef ENGLISH
// Fonction ind�pendante aussi bien pour le bouton que le raccourci clavier
void get_all_handler()
{
	int pos;
	actor *me = get_our_actor();
	if (!me) return;//Wtf!?

	for (pos=0; pos<NUM_BAGS; pos++)
	{
		if ((bag_list[pos].x==me->x_tile_pos) && (bag_list[pos].y==me->y_tile_pos))
		{
			if (! get_show_window(ground_items_win))
			{
				// if auto empty bags enable, set the open timer
				if (items_auto_get_all)
					ground_items_empty_next_bag = SDL_GetTicks();
				else
					ground_items_empty_next_bag = 0;
				open_bag(bag_list[pos].obj_3d_id);
			}
			else pick_up_all_items();
			break;
		}
	}
}
#endif //ENGLISH

static void drop_all_handler ()
{
	Uint8 str[6] = {0};
	int i;
#ifdef NEW_SOUND
	int dropped_something = 0;
#endif // NEW_SOUND
	static Uint32 last_click = 0;

	/* provide some protection for inadvertent pressing (double click that can be disabled) */
	if (safe_button_click(&last_click))
	{
#ifndef FR_VERSION
		for(i = 0; i < ITEM_NUM_ITEMS; i++)
		{
			if (item_list[i].quantity != 0 &&  // only drop stuff that we're not wearing or not excluded
				item_list[i].pos >= ((items_dropall_nofirstrow)?6:0) &&
				item_list[i].pos < ((items_dropall_nolastrow)?30:ITEM_WEAR_START))
			{
#else //FR_VERSION
		set_shown_string(0, "");
		for (i=((items_dropall_nofirstrow)?6:0); i<((items_dropall_nolastrow)?30:36); i++)
		{
			if ((!items_dropall_nofirstcol || (i%6)) && (!items_dropall_nolastcol || ((i+1)%6)))
			{
#endif //FR_VERSION
				str[0] = DROP_ITEM;
				str[1] = item_list[i].pos;
				*((Uint32 *)(str+2)) = SDL_SwapLE32(item_list[i].quantity);
				my_tcp_send (my_socket, str, 6);
#ifdef NEW_SOUND
				dropped_something = 1;
#endif // NEW_SOUND
			}
		}
#ifdef NEW_SOUND
		if (dropped_something)
			add_sound_object(get_index_for_sound_type_name("Drop Item"), 0, 0, 1);
#endif // NEW_SOUND
	}
#ifdef FR_VERSION
	else set_shown_string(c_orange2, dc_warning_str);
#endif //FR_VERSION
}

#ifdef FR_VERSION
static int resize_items_handler(window_info *win)
{
	// contr�le sur la largeur max de la fen�tre (arbitrairement = �cran - 100)
	if (win->len_x > window_width - 100) win->len_x = window_width - 100;
	// taille de la grille en fonction de la largeur de la fen�tre
	items_grid_size = (manual_size_items_window) ? (win->len_x-10-30)/8 : (win->len_x-110)/6;

	// contr�le sur la hauteur min de la fen�tre (d'apr�s grille + cases quantit�)
	if (win->len_y < 6*items_grid_size+25) win->len_y = 6*items_grid_size+25;
	// contr�le sur la hauteur max de la fen�tre (d'apr�s grille + espace suffisant)
	if (win->len_y > 6*items_grid_size+140) win->len_y = 6*items_grid_size+140;
	// contr�le si la hauteur n'est pas trop grande par rapport � la taille de l'�cran
	if (win->len_y > window_height - 10)
	{
		// hauteur de la fen�tre bloqu� au max possible selon la hauteur d'�cran
		win->len_y = window_height - 10;
		// recalcul de la taille de la grille en fonction de la hauteur de la fen�tre
		items_grid_size = (win->len_y - 25) / 6;
		// recalcul de la largeur de la fen�tre en fonction de la taille de la grille
		win->len_x = (manual_size_items_window) ? items_grid_size*8+10+30 : items_grid_size*6+110;
	}

	// taille des cases de quanti�s d'apr�s la largeur de la fen�tre
	quantity_width = (win->len_x < 330) ? win->len_x / 6 : (win->len_x - 20) / 6;
	// position verticale des cases de quantit�s d'apr�s la hauteur de la fen�tre
	quantity_y_offset = win->len_y-21;
	quantity_x_offset = 1;

	// taille de la grille des �quipements
	wear_grid_size = (manual_size_items_window) ? items_grid_size : 33;
	// position horizontale des �quipements d'apr�s la taille de la grille
	wear_items_x_offset = 6*items_grid_size + (win->len_x-6*items_grid_size-2*wear_grid_size-30)/2 - 1;
	wear_items_y_offset = items_grid_size;

	// d�termine l'�tat de la variable 'small item' en fonction de la taille de la grille
	// laisser � 1 si on souhaite que l'indication de charge reste en bas dans tous les cas
	use_small_items_window = ((manual_size_items_window) || (items_grid_size < 38)) ? 1 : 0;

	// red�coupe le texte de l'inventaire en fonction de la largeur de la fen�tre
	put_small_text_in_box((unsigned char*)inventory_item_string, strlen(inventory_item_string), win->len_x, items_string);
	return 0;
}
#endif //FR_VERSION

int show_items_handler(window_info * win)
{
#ifdef FR_VERSION
	/* appel la fonction du resize pour valider les tailles & positions */
	resize_items_handler(&windows_list.window[items_win]);
#else //FR_VERSION
	if (!manual_size_items_window)
		use_small_items_window = ((window_height<=600) || (window_width<=800));

	if(!use_small_items_window) {
		items_grid_size=51;
		wear_items_y_offset=50;
		win->len_y=6*items_grid_size+90;
		quantity_width=69;
	} else {
		items_grid_size=33;
		wear_items_y_offset=33;
		win->len_y=6*items_grid_size+110;
		quantity_width=51;
	}

	win->len_x=6*items_grid_size+110;
	quantity_y_offset=win->len_y-21;
	quantity_x_offset=1;
	wear_items_x_offset=6*items_grid_size+6;
#endif //FR_VERSION
	item_quantity=quantities.quantity[quantities.selected].val;

	cm_remove_regions(items_win);
#ifdef FR_VERSION
//	cm_add_region(cm_stoall_but, items_win, win->len_x-(XLENBUT+3), wear_items_y_offset+but_y_off[1], XLENBUT, YLENBUT);
	cm_add_region(cm_getall_but, items_win, win->len_x-(XLENBUT+3), wear_items_y_offset+but_y_off[0], XLENBUT, YLENBUT);
	cm_add_region(cm_dropall_but, items_win, win->len_x-(XLENBUT+3), wear_items_y_offset+but_y_off[2], XLENBUT, YLENBUT);
	cm_add_region(cm_mix_but, items_win, win->len_x-(XLENBUT+3), wear_items_y_offset+but_y_off[4], XLENBUT, YLENBUT);
	cm_add_region(cm_itemlist_but, items_win, win->len_x-(XLENBUT+3), wear_items_y_offset+but_y_off[3], XLENBUT, YLENBUT);
#else //FR_VERSION
	cm_add_region(cm_stoall_but, items_win, win->len_x-(XLENBUT+3), wear_items_y_offset+but_y_off[0], XLENBUT, YLENBUT);
	cm_add_region(cm_getall_but, items_win, win->len_x-(XLENBUT+3), wear_items_y_offset+but_y_off[1], XLENBUT, YLENBUT);
	cm_add_region(cm_dropall_but, items_win, win->len_x-(XLENBUT+3), wear_items_y_offset+but_y_off[2], XLENBUT, YLENBUT);
	cm_add_region(cm_mix_but, items_win, win->len_x-(XLENBUT+3), wear_items_y_offset+but_y_off[3], XLENBUT, YLENBUT);
	cm_add_region(cm_itemlist_but, items_win, win->len_x-(XLENBUT+3), wear_items_y_offset+but_y_off[4], XLENBUT, YLENBUT);
#endif //FR_VERSION

	/* make sure we redraw any string */
	last_items_string_id = 0;

	return 1;
}

static int context_items_handler(window_info *win, int widget_id, int mx, int my, int option)
{
	if (option<ELW_CM_MENU_LEN)
		return cm_title_handler(win, widget_id, mx, my, option);
	switch (option)
	{
#ifdef FR_VERSION
		case ELW_CM_MENU_LEN+1: show_items_handler(win); break;
		case ELW_CM_MENU_LEN+10: send_input_text_line("#depot", 6); break;
#else //FR_VERSION
		case ELW_CM_MENU_LEN+1: manual_size_items_window = 1; show_items_handler(win); break;
		case ELW_CM_MENU_LEN+2: show_items_handler(win); break;
		case ELW_CM_MENU_LEN+7: send_input_text_line("#sto", 4); break;
#endif //FR_VERSION
	}
	return 1;
}

void display_items_menu()
{
	if(items_win < 0){
		int our_root_win = -1;
		if (!windows_on_top) {
			our_root_win = game_root_win;
		}
#ifdef FR_VERSION
		if (items_menu_x_len < 8*33+45) items_menu_x_len = 8*33+45;
		if (items_menu_y_len < 6*33+25) items_menu_y_len = 6*33+25;
		items_win= create_window(win_inventory, our_root_win, 0, items_menu_x, items_menu_y, items_menu_x_len, items_menu_y_len, ELW_RESIZEABLE|ELW_WIN_DEFAULT);
		set_window_min_size(items_win, 8*33+45, 6*33+25);
		set_window_handler(items_win, ELW_HANDLER_RESIZE, &resize_items_handler);
		/* appel la fonction du resize pour valider les tailles & positions */
		resize_items_handler(&windows_list.window[items_win]);
#else //FR_VERSION
		if (!manual_size_items_window)
			use_small_items_window = ((window_height<=600) || (window_width<=800));

		items_win= create_window(win_inventory, our_root_win, 0, items_menu_x, items_menu_y, items_menu_x_len, items_menu_y_len, ELW_WIN_DEFAULT);
#endif //FR_VERSION

		set_window_handler(items_win, ELW_HANDLER_DISPLAY, &display_items_handler );
		set_window_handler(items_win, ELW_HANDLER_CLICK, &click_items_handler );
		set_window_handler(items_win, ELW_HANDLER_MOUSEOVER, &mouseover_items_handler );
		set_window_handler(items_win, ELW_HANDLER_KEYPRESS, &keypress_items_handler );
		set_window_handler(items_win, ELW_HANDLER_SHOW, &show_items_handler );

		cm_add(windows_list.window[items_win].cm_id, cm_items_menu_str, context_items_handler);
#ifdef FR_VERSION
		cm_bool_line(windows_list.window[items_win].cm_id, ELW_CM_MENU_LEN+1, &manual_size_items_window, NULL);
		cm_bool_line(windows_list.window[items_win].cm_id, ELW_CM_MENU_LEN+2, &item_window_on_drop, "item_window_on_drop");
		cm_bool_line(windows_list.window[items_win].cm_id, ELW_CM_MENU_LEN+3, &allow_equip_swap, NULL);
		cm_bool_line(windows_list.window[items_win].cm_id, ELW_CM_MENU_LEN+4, &allow_wheel_quantity_edit, NULL);
		cm_bool_line(windows_list.window[items_win].cm_id, ELW_CM_MENU_LEN+5, &allow_wheel_quantity_drag, NULL);
		cm_bool_line(windows_list.window[items_win].cm_id, ELW_CM_MENU_LEN+7, &cm_quickbar_enabled, NULL);
		cm_bool_line(windows_list.window[items_win].cm_id, ELW_CM_MENU_LEN+8, &cm_quickbar_protected, NULL);
#else //FR_VERSION
		cm_bool_line(windows_list.window[items_win].cm_id, ELW_CM_MENU_LEN+1, &use_small_items_window, NULL);
		cm_bool_line(windows_list.window[items_win].cm_id, ELW_CM_MENU_LEN+2, &manual_size_items_window, NULL);
		cm_bool_line(windows_list.window[items_win].cm_id, ELW_CM_MENU_LEN+3, &item_window_on_drop, "item_window_on_drop");
		cm_bool_line(windows_list.window[items_win].cm_id, ELW_CM_MENU_LEN+4, &allow_equip_swap, NULL);
		cm_bool_line(windows_list.window[items_win].cm_id, ELW_CM_MENU_LEN+5, &items_mod_click_any_cursor, NULL);
#endif //FR_VERSION

#ifdef FR_VERSION
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
#else //FR_VERSION
		cm_stoall_but = cm_create(inv_keeprow_str, NULL);
		cm_bool_line(cm_stoall_but, 0, &items_stoall_nolastrow, NULL);
		cm_bool_line(cm_stoall_but, 1, &items_stoall_nolastrow, NULL);

		cm_dropall_but = cm_create(inv_keeprow_str, NULL);
		cm_bool_line(cm_dropall_but, 0, &items_dropall_nolastrow, NULL);

		cm_mix_but = cm_create(mix_all_str, NULL);
#endif //FR_VERSION
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

void get_items_cooldown (const Uint8 *data, int len)
{
	int iitem, nitems, ibyte, pos;
	Uint8 cooldown, max_cooldown;

	// reset old cooldown values
	for (iitem = 0; iitem < ITEM_NUM_ITEMS; iitem++)
	{
		item_list[iitem].cooldown_time = 0;
		item_list[iitem].cooldown_rate = 1;
	}

	nitems = len / 5;
	if (nitems <= 0) return;

	ibyte = 0;
	for (iitem = 0; iitem < nitems; iitem++)
	{
		pos = data[ibyte];
		max_cooldown = SDL_SwapLE16 (*((Uint16*)(&data[ibyte+1])));
		cooldown = SDL_SwapLE16 (*((Uint16*)(&data[ibyte+3])));
		ibyte += 5;

		item_list[pos].cooldown_rate = 1000 * (Uint32)max_cooldown;
		item_list[pos].cooldown_time = cur_time + 1000 * (Uint32)cooldown;
	}
#ifdef FR_VERSION
	sync_fr_quickitems();
#endif //FR_VERSION
}
