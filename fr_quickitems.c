#include <stdlib.h>
#include <string.h>
#include "fr_quickitems.h"
#include "asc.h"
#include "cursors.h"
#include "elwindows.h"
#include "gamewin.h"
#include "gl_init.h"
#include "hud.h"
#include "init.h"
#include "interface.h"
#include "items.h"
#include "storage.h"
#include "multiplayer.h"
#include "textures.h"
#include "translate.h"
#include "events.h"
#include "sound.h"
#include "errors.h"
#include "io/elpathwrapper.h"
#include "themes.h"
#include "context_menu.h"
#include "item_info.h"


item fr_quickitem_list[FR_QUICKITEMS_MAXSIZE];
int fr_quickitem_dragged = -1;

void init_fr_quickitems();
int get_fr_quickitems_y_base();

int display_fr_quickitems_handler(window_info *win);
int mouseover_fr_quickitems_handler(window_info *win, int mx, int my);
int click_fr_quickitems_handler(window_info *win, int mx, int my, Uint32 flags);

static int context_fr_quickitems_handler(window_info *win, int widget_id, int mx, int my, int option);
static size_t cm_quickbar_id = CM_INIT_VALUE;
int cm_quickbar_enabled = 1;
int cm_quickbar_protected = 0;
enum { CMQB_ENABLE=0, CMQB_ALTKEY, CMQB_SEP1, CMQB_DRAG, CMQB_FLIP, CMQB_RESET, CMQB_ONTOP, CMQB_SEP2, CMQB_CLEAN, CMQB_COMPLETE, CMQB_AUTOADD, CMQB_AUTOREM, CMQB_COMPRESS, CMQB_AUTOZIP, CMQB_INSERT };

int fr_quickitems_insertmode = 0;
int fr_quickitems_autocompress = 0;
int fr_quickitems_autocomplete = 0;
int fr_quickitems_autoremove = 0;

/* Paramètres courants de la barre (écrasés par les valeurs personnelles mémorisées) */
int quickitems_size = 6;
int quickbar_dir = VERTICAL;
int quickbar_x = 30+3;
int quickbar_y = HUD_MARGIN_X;
int quickbar_x_len = (1+30)*1 +1;
int quickbar_y_len = (1+30)*6 +1;
int quickbar_draggable = 0;
int quickbar_on_top = 1;
int quickbar_loaded = 0;


/* Retourne les coordonnées d'une case selon sa position (utilisé par display & click handler) */
void set_quickitem_coords(int pos, int *x_start, int *x_end, int *y_start, int *y_end)
{
	*x_start= 1;
	*x_end  = *x_start + fr_quickitems_dim;
	*y_start= 1 + (int)(pos/fr_quickitems_div) * fr_quickitems_sep + (fr_quickitems_dim+1)*pos;
	*y_end  = *y_start + fr_quickitems_dim;

	if (quickbar_dir != VERTICAL)
	{
		pos = *x_start; *x_start = *y_start; *y_start = pos;
		pos = *x_end; *x_end = *y_end; *y_end = pos;
	}
}

/* Retourne l'indice de la case de la barre rapide correspondant aux coordonnées de la souris */
int get_quickitem_from_mouse(int mx, int my)
{
	int i, x_start, x_end, y_start, y_end;
	for (i = 0; i < quickitems_size; i++)
	{
		set_quickitem_coords(i, &x_start, &x_end, &y_start, &y_end);
		if (mx>=x_start && mx<=x_end && my>=y_start && my<=y_end) return i;
	}
	return -1;
}

/* Détermine la place occupée en hauteur par la barre dans l'interface */
int get_fr_quickitems_y_base()
{
	if (quickbar_draggable) return 0;
	if (quickbar_x > HUD_MARGIN_X) return 0;
	return quickbar_y + quickbar_y_len;
}


void drag_quickitem(int item)
{
	if ((item < 0) || (item >= FR_QUICKITEMS_MAXSIZE) || (fr_quickitem_list[item].quantity < 0))
	{
		fr_quickitem_dragged = -1;
		return;
	}

	// dessine l'item avec la texture correspondante dans la case
	draw_item(fr_quickitem_list[item].image_id, mouse_x-25, mouse_y-25, 50+1);

	// Affichage "grisé" de la case en cas d'indisponibilité de l'item
	if (fr_quickitem_list[item].quantity == 0)
	{
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBegin(GL_QUADS);
			glColor4f(fr_quickitems_zerocolor.rouge, fr_quickitems_zerocolor.vert, fr_quickitems_zerocolor.bleu, fr_quickitems_zerocolor.alpha);
			glVertex2f(mouse_x-25, mouse_y-25);
			glVertex2f(mouse_x-25, mouse_y+25);
			glVertex2f(mouse_x+25, mouse_y+25);
			glVertex2f(mouse_x+25, mouse_y-25);
		glEnd();
		glDisable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);
	}
}


void resize_fr_quickitems()
{
	if (quickbar_dir == VERTICAL)
		quickbar_y_len = (1+fr_quickitems_dim)*quickitems_size + (int)((quickitems_size-1)/fr_quickitems_div)*fr_quickitems_sep + 1;
	else
		quickbar_x_len = (1+fr_quickitems_dim)*quickitems_size + (int)((quickitems_size-1)/fr_quickitems_div)*fr_quickitems_sep + 1;
	resize_window(quickbar_win, quickbar_x_len, quickbar_y_len);
}


/* Modification de la barre pour la rendre déplaçable (avec barre de titre) ou non */
void toggle_fr_quickitems_draggable()
{
	Uint32 flags = get_flags(quickbar_win);
	if (!quickbar_draggable)
	{
		flags &= ~ELW_SHOW_LAST;
		flags |= ELW_DRAGGABLE | ELW_TITLE_BAR;
		change_flags(quickbar_win, flags);
		quickbar_draggable = 1;
	}
	else
	{
		flags |= ELW_SHOW_LAST;
		flags &= ~(ELW_DRAGGABLE | ELW_TITLE_BAR);
		change_flags(quickbar_win, flags);
		quickbar_draggable = 0;
		quickbar_x = window_width - windows_list.window[quickbar_win].cur_x;
		quickbar_y = windows_list.window[quickbar_win].cur_y;
	}
}

/* Changement de l'orientation de la barre rapide (vertical/horizontal) */
void flip_fr_quickitems()
{
	int temp = quickbar_x_len;
	quickbar_x_len = quickbar_y_len;
	quickbar_y_len = temp;
	quickbar_dir = (quickbar_dir == VERTICAL) ? HORIZONTAL : VERTICAL;
	resize_window(quickbar_win, quickbar_x_len, quickbar_y_len);
}

/* Redéfinition de la barre rapide dans sa position par défaut */
void reset_fr_quickitems()
{
	quickbar_on_top = 1;
	quickbar_draggable = 0;
	change_flags(quickbar_win, ELW_TITLE_NONE|ELW_SHOW|ELW_USE_BACKGROUND|ELW_SHOW_LAST);

	quickbar_x = fr_quickitems_dim + 3;
	quickbar_y = (hud_x) ? HUD_MARGIN_X : 0;
	move_window(quickbar_win, -1, 0, window_width - quickbar_x, quickbar_y);

	quickbar_dir = VERTICAL;
	quickbar_x_len = (1+fr_quickitems_dim)*1 + 1;
	quickbar_y_len = (1+fr_quickitems_dim)*quickitems_size + (int)((quickitems_size-1)/fr_quickitems_div)*fr_quickitems_sep + 1;
	resize_window(quickbar_win, quickbar_x_len, quickbar_y_len);
}


void clean_fr_quickitems()
{
	int i;
	for (i=0; i<FR_QUICKITEMS_MAXSIZE; i++) fr_quickitem_list[i].quantity = -1;
}

void compress_fr_quickitems()
{
	int i, empty_slot = -1;
	for (i=0; i<FR_QUICKITEMS_MAXSIZE; i++)
	{
		if ((empty_slot >= 0) && (fr_quickitem_list[i].quantity >= 0))
		{
			fr_quickitem_list[empty_slot] = fr_quickitem_list[i];
			fr_quickitem_list[i].quantity = -1;
			empty_slot++;
		}
		else if ((empty_slot < 0) && (fr_quickitem_list[i].quantity < 0)) empty_slot = i;
	}
}

void sync_fr_quickitems()
{
	int i, j;

	// Comptage du nombre d'item similaire disponibles dans l'inventaire
	for (i=0; i<FR_QUICKITEMS_MAXSIZE; i++)
	{
		if (fr_quickitem_list[i].quantity < 0) continue;

		fr_quickitem_list[i].pos = -1;
		fr_quickitem_list[i].quantity = 0;
		for (j=0; j<ITEM_WEAR_START; j++)
		{
			if (item_list[j].quantity < 1) continue;
			if (item_list[j].id != unset_item_uid)
			{
				if (item_list[j].id != fr_quickitem_list[i].id) continue;
			}
			else
			// On compare les images. Le soucis est que parfois la même image sert pour 2 objets différents.
			if (item_list[j].image_id != fr_quickitem_list[i].image_id) continue;

			fr_quickitem_list[i].quantity += item_list[j].quantity;
			if (fr_quickitem_list[i].pos < 0)
			{
				fr_quickitem_list[i].pos = j;
				fr_quickitem_list[i].cooldown_time = item_list[j].cooldown_time;
				fr_quickitem_list[i].cooldown_rate = item_list[j].cooldown_rate;
			}
			// si l'objet est empilable, inutile de continuer le parcours du sac
			if (fr_quickitem_list[i].is_stackable == 1) break;
		}
	}
}


void build_fr_quickitems(int force_complete)
{
	// Synchronisation des raccourcis avec l'inventaire (position, quantité, cooldown)
	sync_fr_quickitems();

	// En option : suppression du raccourci quand objet indisponible
	if (fr_quickitems_autoremove)
	{
		int i;
		for (i=0; i<FR_QUICKITEMS_MAXSIZE; i++)
			if (fr_quickitem_list[i].quantity == 0)
				fr_quickitem_list[i].quantity = -1;
		// lancer la compression puisque des trous ont pu être créés
		if (fr_quickitems_autocompress) compress_fr_quickitems();
	}

	// En option : complete les raccourcis avec tout objet disponible
	if (fr_quickitems_autocomplete || force_complete)
	{
		int i, j, empty_slot, already_here;
		for (i=0; i<ITEM_WEAR_START; i++)
		{
			if (item_list[i].quantity<1) continue;
			empty_slot = -1;
			already_here = 0;
			for (j=0; j<FR_QUICKITEMS_MAXSIZE; j++)
			{
				if (fr_quickitem_list[j].quantity < 0)
				{
					if (empty_slot < 0) empty_slot = j;
					continue;
				}
				if (item_list[i].id != unset_item_uid)
				{
					if (fr_quickitem_list[j].id != item_list[i].id) continue;
				}
				else
				if (fr_quickitem_list[j].image_id != item_list[i].image_id) continue;
				already_here = 1;
				break;
			}
			if (already_here) continue;
			if (empty_slot < 0) break;
			fr_quickitem_list[empty_slot] = item_list[i];
		}
		// relancer une synchro puisque des raccourcis ont pu être ajoutés
		sync_fr_quickitems();
	}
}


/* Ajoute un raccourci pour l'objet indiqué dans l'inventaire à la 1ère case possible de la barre rapide */
int affect_fr_quickitems(int pos)
{
	int i;
	int id_quick = -1;

	// recherche si un raccourci existe déjà
	for (i=0; i < FR_QUICKITEMS_MAXSIZE; i++)
	{
		if (fr_quickitem_list[i].quantity < 0)
		{
			// récupère la 1ère place libre au passage
			if (id_quick < 0) id_quick = i;
			continue;
		}
		if (item_list[pos].id != unset_item_uid)
		{
			if (fr_quickitem_list[i].id==item_list[pos].id) break;
		}
		else if (fr_quickitem_list[i].image_id==item_list[pos].image_id) break;
	}
	// raccourci existant trouvé en case i
	if (i < FR_QUICKITEMS_MAXSIZE)
	{
		// raccourci déjà présent et visible
		if (i < quickitems_size)
		{
			id_quick = i;
		}
		// raccourci déjà présent et caché, place libre visible
		else if ((id_quick >= 0) && (id_quick < quickitems_size))
		{
			fr_quickitem_list[i].quantity = -1;
		}
		// raccourci déjà présent et caché, aucune place libre : insertion
		else if (fr_quickitems_insertmode)
		{
			for (id_quick = i; id_quick >= quickitems_size; id_quick--)
			{
				fr_quickitem_list[id_quick] = fr_quickitem_list[id_quick - 1];
				fr_quickitem_list[id_quick].pos = id_quick;
			}
		}
		// raccourci déjà présent et caché, aucune place libre : échange
		else
		{
			id_quick = quickitems_size - 1;
			fr_quickitem_list[i] = fr_quickitem_list[id_quick];
			fr_quickitem_list[i].pos = i;
		}
	}
	// aucun raccourci correspondant déjà en place
	else
	{
		// aucune place libre ou aucune visible
		if ((id_quick < 0) || (id_quick >= quickitems_size))
		{
			i = (id_quick < 0) ? FR_QUICKITEMS_MAXSIZE - 1 : id_quick;
			if (fr_quickitems_insertmode)
			{
				for (id_quick = i; id_quick >= quickitems_size; id_quick--)
				{
					fr_quickitem_list[id_quick] = fr_quickitem_list[id_quick - 1];
					fr_quickitem_list[id_quick].pos = id_quick;
				}
			}
			else
			{
				id_quick = quickitems_size - 1;
				fr_quickitem_list[i] = fr_quickitem_list[id_quick];
				fr_quickitem_list[i].pos = i;
			}
		}
	}

	if (id_quick >= 0)
	{
		fr_quickitem_list[id_quick] = item_list[pos];
		fr_quickitem_list[id_quick].pos = id_quick;
		add_sound_object(get_index_for_sound_type_name("Drop Item"), 0, 0, 1);
		build_fr_quickitems(0);
		return 1;
	}

	add_sound_object(get_index_for_sound_type_name("alert1"), 0, 0, 1);
	return 0;
}


/* Initialisation (fenêtre, handlers et menu contextuel */
void init_fr_quickitems ()
{
	// on remonte/descend les barres rapides placées sur le hud sous le logo
	if ((! hud_x) && (! quickbar_draggable) && (quickbar_dir==VERTICAL) && (quickbar_x<=HUD_MARGIN_X))
	{
		if (quickbar_y == HUD_MARGIN_X) quickbar_y = 0;
	}

	if (quickbar_dir == VERTICAL)
	{
		quickbar_x_len = (1+fr_quickitems_dim)*1 + 1;
		quickbar_y_len = (1+fr_quickitems_dim)*quickitems_size + (int)((quickitems_size-1)/fr_quickitems_div)*fr_quickitems_sep + 1;
	}
	else
	{
		quickbar_x_len = (1+fr_quickitems_dim)*quickitems_size + (int)((quickitems_size-1)/fr_quickitems_div)*fr_quickitems_sep + 1;
		quickbar_y_len = (1+fr_quickitems_dim)*1 + 1;
	}

	if (quickbar_win < 0)
	{
		quickbar_win = create_window("Quickbar", -1, 0, window_width - quickbar_x, quickbar_y, quickbar_x_len, quickbar_y_len, ELW_USE_BACKGROUND|ELW_SHOW_LAST);

		set_window_handler(quickbar_win, ELW_HANDLER_DISPLAY, &display_fr_quickitems_handler);
		set_window_handler(quickbar_win, ELW_HANDLER_CLICK, &click_fr_quickitems_handler);
		set_window_handler(quickbar_win, ELW_HANDLER_MOUSEOVER, &mouseover_fr_quickitems_handler);

		cm_quickbar_id = cm_create(cm_quickbar_menu_str, context_fr_quickitems_handler);
		cm_bool_line(cm_quickbar_id, CMQB_ENABLE, &cm_quickbar_enabled, NULL);
		cm_bool_line(cm_quickbar_id, CMQB_ALTKEY, &cm_quickbar_protected, NULL);
		cm_bool_line(cm_quickbar_id, CMQB_DRAG,   &quickbar_draggable, NULL);
		cm_bool_line(cm_quickbar_id, CMQB_ONTOP,  &quickbar_on_top, NULL);
		cm_bool_line(cm_quickbar_id, CMQB_AUTOREM, &fr_quickitems_autoremove, NULL);
		cm_bool_line(cm_quickbar_id, CMQB_AUTOADD, &fr_quickitems_autocomplete, NULL);
		cm_bool_line(cm_quickbar_id, CMQB_AUTOZIP, &fr_quickitems_autocompress, NULL);
		cm_bool_line(cm_quickbar_id, CMQB_INSERT,  &fr_quickitems_insertmode, NULL);
		cm_bool_line(cm_quickbar_id, CMQB_INSERT+2, &allow_wheel_quantity_drag, NULL);
	}
	else
	{
		change_flags(quickbar_win, ELW_USE_BACKGROUND|ELW_SHOW_LAST);
		if (quickbar_draggable)
		{
			show_window(quickbar_win);
		}
		else if (quickbar_y > window_height-10 || quickbar_x < 10)
		{
			move_window(quickbar_win, -1, 0, window_width - fr_quickitems_dim - 3, HUD_MARGIN_X);
		}
		else
		{
			move_window(quickbar_win, -1, 0, window_width - quickbar_x, quickbar_y);
		}
	}
	// on change les flags de la fenêtre draggable seulement maintenant pour éviter
	// qu'elle soit créée avec le menu contextuel par défaut de la barre de titre
	if (quickbar_draggable)
	{
		change_flags(quickbar_win, ELW_USE_BACKGROUND|ELW_SHOW_LAST|ELW_TITLE_BAR|ELW_DRAGGABLE);
	}
}


/* Affichage de la barre rapide */
int display_fr_quickitems_handler(window_info *win)
{
    char str[80];
	int delta, i; //, j;
	Uint32 _cur_time = SDL_GetTicks(); /* grab a snapshot of current time */

	glEnable(GL_TEXTURE_2D);
	glColor3f(1.0f,1.0f,1.0f);

	// dessin du contenu de chaque case (item, qty, cooldown)
	for (i = 0; i < quickitems_size; i++)
 	{
		if (fr_quickitem_list[i].quantity >= 0)
		{
			int x_start, x_end, y_start, y_end;

			//get the x and y
			set_quickitem_coords(i, &x_start, &x_end, &y_start, &y_end);

			// dessine l'item avec la texture correspondante dans la case
			draw_item(fr_quickitem_list[i].image_id, x_start, y_start, fr_quickitems_dim+1);

			// Affichage "grisé" de la case en cas d'indisponibilité de l'item
			if (fr_quickitem_list[i].quantity == 0)
			{
				glDisable(GL_TEXTURE_2D);
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glBegin(GL_QUADS);
					glColor4f(fr_quickitems_zerocolor.rouge, fr_quickitems_zerocolor.vert, fr_quickitems_zerocolor.bleu, fr_quickitems_zerocolor.alpha);
					glVertex2f(x_start, y_start);
					glVertex2f(x_start, y_end);
					glVertex2f(x_end, y_end);
					glVertex2f(x_end, y_start);
				glEnd();
				glDisable(GL_BLEND);
				glEnable(GL_TEXTURE_2D);
			}

			// Affichage de la durée du cooldown de l'item
			else if (fr_quickitem_list[i].cooldown_time > _cur_time)
			{
				float cooldown = ((float)(fr_quickitem_list[i].cooldown_time - _cur_time)) / ((float)fr_quickitem_list[i].cooldown_rate);
				float x_center = (x_start + x_end)*0.5f;
				float y_center = (y_start + y_end)*0.5f;

				if (cooldown < 0.0f)
					cooldown = 0.0f;
				else if (cooldown > 1.0f)
					cooldown = 1.0f;

				glDisable(GL_TEXTURE_2D);
				glEnable(GL_BLEND);

				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glBegin(GL_TRIANGLE_FAN);
					glColor4f(fr_quickitems_coolcolor.rouge, fr_quickitems_coolcolor.vert, fr_quickitems_coolcolor.bleu, fr_quickitems_coolcolor.alpha);

					glVertex2f(x_center, y_center);

					if (cooldown >= 0.875f)
                    {
						float t = tan(2.0f*M_PI*(1.0f - cooldown));
						glVertex2f(t*x_end + (1.0f - t)*x_center, y_start);
						glVertex2f(x_end, y_start);
						glVertex2f(x_end, y_end);
						glVertex2f(x_start, y_end);
						glVertex2f(x_start, y_start);
					}
                    else if (cooldown >= 0.625f)
                    {
						float t = 0.5f + 0.5f*tan(2.0f*M_PI*(0.75f - cooldown));
						glVertex2f(x_end, t*y_end + (1.0f - t)*y_start);
						glVertex2f(x_end, y_end);
						glVertex2f(x_start, y_end);
						glVertex2f(x_start, y_start);
					}
                    else if (cooldown >= 0.375f)
                    {
						float t = 0.5f + 0.5f*tan(2.0f*M_PI*(0.5f - cooldown));
						glVertex2f(t*x_start + (1.0f - t)*x_end, y_end);
						glVertex2f(x_start, y_end);
						glVertex2f(x_start, y_start);
					}
                    else if (cooldown >= 0.125f)
                    {
						float t = 0.5f + 0.5f*tan(2.0f*M_PI*(0.25f - cooldown));
						glVertex2f(x_start, t*y_start + (1.0f - t)*y_end);
						glVertex2f(x_start, y_start);
					}
                    else
                    {
						float t = tan(2.0f*M_PI*(cooldown));
						glVertex2f(t*x_start + (1.0f - t)*x_center, y_start);
					}

					glVertex2f(x_center, y_start);
				glEnd();

				glDisable(GL_BLEND);
				glEnable(GL_TEXTURE_2D);
			}

			// Affichage de la quantité disponible dans l'inventaire
			safe_snprintf(str, sizeof(str), "%i", fr_quickitem_list[i].quantity);
    		draw_string_small_shadowed(x_start, y_end-15, (unsigned char*)str, 1, 1.0f,1.0f,1.0f, 0.0f,0.0f,0.0f);
		}
	}

	glDisable(GL_TEXTURE_2D);
	use_window_color(quickbar_win, ELW_COLOR_LINE);

	// dessin des cadres de chaque case
	for (i = 0; i < quickitems_size; i++)
	{
		delta = (int)(i/fr_quickitems_div) * fr_quickitems_sep;
		glBegin(GL_LINE_LOOP);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			if (quickbar_dir==VERTICAL)
			{
				glVertex3i(0,                   1 + delta + (fr_quickitems_dim+1)* i, 0);
				glVertex3i(fr_quickitems_dim+1, 1 + delta + (fr_quickitems_dim+1)* i, 0);
				glVertex3i(fr_quickitems_dim+1, 1 + delta + (fr_quickitems_dim+1)*(i+1), 0);
				glVertex3i(0,                   1 + delta + (fr_quickitems_dim+1)*(i+1), 0);
			}
			else
			{
				glVertex3i(delta + (fr_quickitems_dim+1)* i,    1, 0);
				glVertex3i(delta + (fr_quickitems_dim+1)*(i+1), 1, 0);
				glVertex3i(delta + (fr_quickitems_dim+1)*(i+1), 1+fr_quickitems_dim+1, 0);
				glVertex3i(delta + (fr_quickitems_dim+1)* i,    1+fr_quickitems_dim+1, 0);
			}
			glEnd();
		glEnd();
	}
	glEnable(GL_TEXTURE_2D);


	return 1;
}


static void quickbar_item_description_help(window_info *win, int pos, int slot)
{
	Uint16 item_id = item_list[pos].id;
	int image_id = item_list[pos].image_id;
	if (show_item_desc_text && item_info_available() && (get_item_count(item_id, image_id) == 1))
	{
		const char *str = get_item_description(item_id, image_id);
		if (str != NULL)
		{
			int xpos = 0, ypos = 0;
			int len_str = (strlen(str) + 1) * SMALL_FONT_X_LEN;
			/* vertical place right (or left) and aligned with slot */
			if (quickbar_dir==VERTICAL)
			{
				xpos = win->len_x + 5;
				if ((xpos + len_str + win->cur_x) > window_width)
					xpos = -len_str;
				ypos = slot * 30 + (30 - SMALL_FONT_Y_LEN) / 2;
			}
			/* horizontal place right at bottom (or top) of window */
			else
			{
				xpos = 0;
				ypos = win->len_y + 5;
				if ((xpos + len_str + win->cur_x) > window_width)
					xpos = window_width - win->cur_x - len_str;
				if ((xpos + win->cur_x) < 0)
					xpos = -win->cur_x + 5;
				if ((ypos + SMALL_FONT_Y_LEN + win->cur_y) > window_height)
					ypos = -(5 + SMALL_FONT_Y_LEN + (quickbar_draggable * ELW_TITLE_HEIGHT));
			}
			show_help(str, xpos, ypos);
		}
	}
}


/* Comportement en cas de survol de la barre */
int mouseover_fr_quickitems_handler(window_info *win, int mx, int my)
{
	int id_quick = get_quickitem_from_mouse(mx, my);
	int id_item = -1;
	int i = 0;
	if (id_quick < 0) return 0;
	if (fr_quickitem_list[id_quick].quantity < 0) return 0;

	if (fr_quickitem_list[id_quick].quantity > 0)
	{
		switch (qb_action_mode)
		{
			case ACTION_LOOK      : elwin_mouse=CURSOR_EYE; break;
			case ACTION_USE       : elwin_mouse=CURSOR_USE; break;
			case ACTION_USE_WITEM : elwin_mouse=CURSOR_USE_WITEM; break;
			default               : elwin_mouse=CURSOR_PICK;
		}
	}
	else elwin_mouse=CURSOR_PICK;

	// Recherche l'item dans l'inventaire correspondant au clic sur la quickbar
	if (fr_quickitem_list[id_quick].quantity > 0)
	{
		// TODO: if (fr_quickitem_list[id_quick].is_stackable) id_item = fr_quickitem_list[id_quick].pos; else
		// Ou même directement : id_item = fr_quickitem_list[id_quick].pos;
		for (i = 0; i < ITEM_WEAR_START; i++)
		{
			if (item_list[i].quantity < 1) continue;
			if (item_list[i].id != unset_item_uid)
			{
				if (item_list[i].id != fr_quickitem_list[id_quick].id) continue;
			}
			else if (item_list[i].image_id != fr_quickitem_list[id_quick].image_id) continue;
			id_item = i;
			break;
		}
	}
	quickbar_item_description_help(win, id_item, id_quick);
	return 1;
}

/* Comportement en cas de clic sur la barre */
int click_fr_quickitems_handler(window_info *win, int mx, int my, Uint32 flags)
{
	int i, id_quick, id_item;
	Uint8 str[100];
	int trigger=ELW_RIGHT_MOUSE|ELW_CTRL|ELW_SHIFT;//flags we'll use for the quickbar relocation handling
	int right_click = flags & ELW_RIGHT_MOUSE;

	if (flags & ELW_WHEEL)
	{
		// modification de la quantité de l'objet en cours de drag avec la molette
		if ((allow_wheel_quantity_drag) && (item_dragged!=-1))
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

	// only handle mouse button clicks, not scroll wheels moves
	if ((flags & ELW_MOUSE_BUTTON) == 0) return 0;

	// Gestion des actions effectuées avec le clic droit
	if (right_click)
	{

		// déplacements de la quickbar (avec clic droit et non gauche !)
		switch (flags & trigger) {
			//toggle draggable (fenetre volante ou non)
			case (ELW_RIGHT_MOUSE|ELW_CTRL) : toggle_fr_quickitems_draggable(); return 1;
			//toggle vertical/horisontal (meme si non draggable)
			case (ELW_RIGHT_MOUSE|ELW_SHIFT) : flip_fr_quickitems(); return 1;
			//reset (retour position par défaut)
			case (ELW_RIGHT_MOUSE|ELW_CTRL|ELW_SHIFT) : reset_fr_quickitems(); return 1;
		}

		// affichage du menu contextuel si option activée (+ éventuellement protection avec ALT)
		if (cm_quickbar_enabled && (!cm_quickbar_protected || (flags & ELW_ALT)))
		{
			cm_show_direct(cm_quickbar_id, quickbar_win, -1);
			if (cm_quickbar_protected) return 1;
		}

		// si un objet est en cours de drag, un clic-droit le repose (annulation)
		if (item_dragged >= 0) { item_dragged = -1; return 1; }
		if (fr_quickitem_dragged >= 0) { fr_quickitem_dragged = -1; return 1; }

		// action par défaut sur clic-droit : rotation des actions du curseur
		switch(qb_action_mode)
		{
			case ACTION_WALK: qb_action_mode=ACTION_LOOK; break;
			case ACTION_LOOK: qb_action_mode=ACTION_USE; break;
			case ACTION_USE : qb_action_mode=ACTION_USE_WITEM; break;
			case ACTION_USE_WITEM: if (use_item < 0) qb_action_mode=ACTION_WALK; else use_item=-1; break;
			default : use_item=-1; qb_action_mode=ACTION_WALK;
		}

		return 1;
	}


	// Recherche l'id de la case cliquée dans la quickbar
	id_quick = get_quickitem_from_mouse(mx, my);

	if (id_quick < 0) return 0;


	// tentatives de drop d'un objet en cours de drag venant du sac
	if (item_dragged >= 0)
	{
		int empty_slot = -1;

		// recherche si l'objet est déjà sur la quickbar
		for (i=0; i < FR_QUICKITEMS_MAXSIZE; i++)
		{
			if (fr_quickitem_list[i].quantity < 0) continue;
			if (item_list[item_dragged].id != unset_item_uid)
			{
				if (fr_quickitem_list[i].id != item_list[item_dragged].id) continue;
			}
			else
			if (fr_quickitem_list[i].image_id != item_list[item_dragged].image_id) continue;

			// suppression du raccourci déjà existant dans une autre case
			if (i != id_quick)
			{
				if ((empty_slot < 0) && !fr_quickitems_insertmode) empty_slot = i;
				fr_quickitem_list[i].quantity = -1;
			}
			// tentative d'équipement auto (considéré comme un double-clic)
			else if (fr_quickitem_list[i].quantity > 0)
			{
				for (i = ITEM_WEAR_START; i < ITEM_WEAR_START+ITEM_NUM_WEAR; i++)
				{
					if (item_list[i].quantity<1) { move_item(item_dragged, i); break; }
				}
				item_dragged = -1;
				return 1;
			}
		}

		// case de destination dans la quickbar déjà occupée
		if (fr_quickitem_list[id_quick].quantity >= 0)
		{
			/* mode par échange */
			if (! fr_quickitems_insertmode)
			{
				// si aucun raccourci n'existait avant, empty_slot non initialisé pour échange
				// on se contente alors de déplacer vers la 1ère case libre de la barre
				for (i=0; i<FR_QUICKITEMS_MAXSIZE && empty_slot<0; i++)
				{
					if (fr_quickitem_list[i].quantity<0) empty_slot = i;
				}
				if (empty_slot >= 0)
				{
					fr_quickitem_list[empty_slot] = fr_quickitem_list[id_quick];
				}
				fr_quickitem_list[id_quick].quantity = -1;
			}
			/* mode par insertion */
			else
			{
				// on cherche la 1ère case vide à partir de l'insertion (ou dernière case sinon)
				for (empty_slot=id_quick; empty_slot<FR_QUICKITEMS_MAXSIZE; empty_slot++)
				{
					if (fr_quickitem_list[empty_slot].quantity<0) break;
				}
				// on effectue les décalages de la case vide en remontant à la base de l'insertion
				for (i=empty_slot; i>id_quick; i--)
				{
					fr_quickitem_list[i] = fr_quickitem_list[i-1];
					fr_quickitem_list[i-1].quantity = -1;
				}
			}
		}

		// case de destination en quickbar libre
		if (fr_quickitem_list[id_quick].quantity < 0)
		{
			// enregistrement du raccourci pour l'item du drag
			fr_quickitem_list[id_quick] = item_list[item_dragged];
			fr_quickitem_list[id_quick].pos = item_dragged;
			add_sound_object(get_index_for_sound_type_name("Drop Item"), 0, 0, 1);
			build_fr_quickitems(0);
			item_dragged = -1;
			return 1;
		}

		// sinon ? (case occupée par un autre raccourci n'ayant pu être déplacé)
		add_sound_object(get_index_for_sound_type_name("alert1"), 0, 0, 1);
		return 0;
	}


	// tentatives de drop d'un raccourci de la barre
	if (fr_quickitem_dragged >= 0)
	{
		item temp_item;
		int empty_slot = -1;

		// lacher sur la case d'origine : abandon du drag
		if (fr_quickitem_dragged == id_quick)
		{
			fr_quickitem_dragged = -1; return 0;
		}

		temp_item = fr_quickitem_list[fr_quickitem_dragged];
		fr_quickitem_list[fr_quickitem_dragged].quantity = -1;

		// case de destination dans la quickbar déjà occupée
		if (fr_quickitem_list[id_quick].quantity >= 0)
		{
			/* mode par échange */
			if (! fr_quickitems_insertmode)
			{
				fr_quickitem_list[fr_quickitem_dragged] = fr_quickitem_list[id_quick];
				fr_quickitem_list[id_quick].quantity = -1;
			}
			/* mode par insertion */
			else
			{
				// on cherche la 1ère case vide à partir de l'insertion (ou dernière case sinon)
				for (empty_slot=id_quick; empty_slot<FR_QUICKITEMS_MAXSIZE; empty_slot++)
				{
					if (fr_quickitem_list[empty_slot].quantity<0) break;
				}
				// on effectue les décalages de la case vide en remontant à la base de l'insertion
				for (i=empty_slot; i>id_quick; i--)
				{
					fr_quickitem_list[i] = fr_quickitem_list[i-1];
					fr_quickitem_list[i-1].quantity = -1;
				}
			}
		}

		// case de destination en quickbar libre
		if (fr_quickitem_list[id_quick].quantity < 0)
		{
			// enregistrement du raccourci pour l'item du drag
			fr_quickitem_list[id_quick] = temp_item;
			add_sound_object(get_index_for_sound_type_name("Drop Item"), 0, 0, 1);
			if (fr_quickitems_autocompress) compress_fr_quickitems();
			fr_quickitem_dragged = -1;
			return 1;
		}

		// sinon ? (case occupée par un autre raccourci n'ayant pu être déplacé)
		add_sound_object(get_index_for_sound_type_name("alert1"), 0, 0, 1);
		return 0;
	}

	// Suppression de l'item enregistré dans la quickbar (ctrl+clic)
    if (shift_on && !ctrl_on && (fr_quickitem_list[id_quick].quantity >= 0))
	{
		fr_quickitem_list[id_quick].quantity = -1;
		add_sound_object(get_index_for_sound_type_name("Button Click"), 0, 0, 1);
		if (fr_quickitems_autocompress) compress_fr_quickitems();
		return 1;
	}


	// Sur un raccourci existant mais quantité vide, alors drag spécial pour déplacement quickbar
	if (fr_quickitem_list[id_quick].quantity == 0)
	{
		fr_quickitem_dragged = id_quick;
		return 1;
	}


	// Recherche l'item dans l'inventaire correspondant au clic sur la quickbar
	id_item = -1;
	if (fr_quickitem_list[id_quick].quantity > 0)
	{
		// TODO: if (fr_quickitem_list[id_quick].is_stackable) id_item = fr_quickitem_list[id_quick].pos; else
		// Ou même directement : id_item = fr_quickitem_list[id_quick].pos;
		for (i = 0; i < ITEM_WEAR_START; i++)
		{
			if (item_list[i].quantity < 1) continue;
			if (item_list[i].id != unset_item_uid)
			{
				if (item_list[i].id != fr_quickitem_list[id_quick].id) continue;
			}
			else if (item_list[i].image_id != fr_quickitem_list[id_quick].image_id) continue;
			id_item = i;
			break;
		}
	}
	if (id_item < 0) return 0;


	// Ctrl + Shift + Left : dépose tous les items correspondant au sol
	if (ctrl_on && shift_on)
	{
		str[0] = DROP_ITEM;
		str[1] = item_list[id_item].pos;
		*((Uint32 *)(str+2)) = SDL_SwapLE32(fr_quickitem_list[id_quick].quantity);
		my_tcp_send(my_socket, str, 4);
		add_sound_object(get_index_for_sound_type_name("Drop Item"), 0, 0, 1);
		return 1;
	}

	// Ctrl + Left : range tous les items correspondant dans le dépot
	else if (ctrl_on)
	{
		if ((storage_win<0) || view_only_storage || !get_show_window(storage_win)) return 0;
		str[0] = DEPOSITE_ITEM;
		str[1] = item_list[id_item].pos;
		*((Uint32 *)(str+2)) = SDL_SwapLE32(fr_quickitem_list[id_quick].quantity);
		my_tcp_send(my_socket, str, 6);
		add_sound_object(get_index_for_sound_type_name("Drop Item"), 0, 0, 1);
		return 1;
	}


	switch (qb_action_mode)
	{
		case ACTION_LOOK :
			click_time = cur_time;
			str[0] = LOOK_AT_INVENTORY_ITEM;
			str[1] = item_list[id_item].pos;
			my_tcp_send(my_socket, str, 2);
			break;

		case ACTION_USE :
			if (item_list[id_item].use_with_inventory)
			{
				str[0] = USE_INVENTORY_ITEM;
				str[1] = item_list[id_item].pos;
				my_tcp_send(my_socket, str, 2);
				item_list[id_item].action = USE_INVENTORY_ITEM;
			}
			// objet non utilisable : tentative d'équipement
			else
			{
				for (i = ITEM_WEAR_START; i < ITEM_WEAR_START+ITEM_NUM_WEAR; i++)
				{
					if (item_list[i].quantity<1) { move_item(id_item, i); break; }
				}
			}
			return 1;

		case ACTION_USE_WITEM :
			if (use_item < 0)
			{
				action_mode = ACTION_USE_WITEM;
				use_item = id_item;
				return 1;
			}
			str[0] = ITEM_ON_ITEM;
			str[1] = item_list[use_item].pos;
			str[2] = item_list[id_item].pos;
			my_tcp_send(my_socket,str,3);
			item_list[use_item].action = ITEM_ON_ITEM;
			item_list[id_item].action = ITEM_ON_ITEM;
			use_item = -1;
			return 1;

		default :
			if (item_dragged < 0)
			{
				// calcul lors de la saisie de l'objet de la quantité max transportable
				int i;
				item_dragged_max_quantity = 0;
				if (! item_list[id_item].is_stackable)
				{
					// parcours du contenu du sac pour les items non empilables
					for (i=0; i<ITEM_WEAR_START; i++)
					{
						if (item_list[i].quantity < 1) continue;
						if (item_list[i].id != unset_item_uid)
						{
							if (item_list[i].id != item_list[id_item].id) continue;
						}
						else
						if ((item_list[i].image_id != item_list[id_item].image_id) || item_list[i].is_stackable) continue;
						item_dragged_max_quantity += item_list[i].quantity;
					}
				}
				else item_dragged_max_quantity = item_list[id_item].quantity;
				item_dragged = id_item;
				add_sound_object(get_index_for_sound_type_name("Drag Item"), 0, 0, 1);
			}
	}

	return 1;
}

/* Actions du menu contextuel de la barre */
static int context_fr_quickitems_handler(window_info *win, int widget_id, int mx, int my, int option)
{
	switch (option)
	{
		case CMQB_DRAG: quickbar_draggable ^= 1; toggle_fr_quickitems_draggable(); break;
		case CMQB_FLIP: flip_fr_quickitems(); break;
		case CMQB_RESET: reset_fr_quickitems(); break;
		case CMQB_CLEAN: clean_fr_quickitems(); break;
		case CMQB_COMPLETE: build_fr_quickitems(1); break;
		case CMQB_COMPRESS: compress_fr_quickitems(); break;
		case CMQB_AUTOREM: if (fr_quickitems_autoremove) build_fr_quickitems(0); break;
		case CMQB_AUTOADD: if (fr_quickitems_autocomplete) build_fr_quickitems(0); break;
		case CMQB_AUTOZIP: if (fr_quickitems_autocompress) build_fr_quickitems(0); break;
	}
	return 1;
}


/* Chargement des raccourcis de la barre depuis le fichier du perso */
void load_fr_quickitems()
{
	char nom_fichier[128];
	FILE *fichier;
	int i;

	quickbar_loaded = 1;  // même si le chargement échoue (permet de sauver le fichier initial)

	memset(fr_quickitem_list, 0, sizeof(fr_quickitem_list));

	safe_snprintf(nom_fichier, sizeof(nom_fichier), "barre_inventaire_%s.dat", username_str);
	my_tolower(nom_fichier);
	fichier = open_file_config(nom_fichier, "rb");
	if (fichier == NULL)
	{
		for (i=0; i<FR_QUICKITEMS_MAXSIZE; i++) fr_quickitem_list[i].quantity = -1;
		LOG_ERROR("%s: %s \"%s\"\n", reg_error_str, cant_open_file, nom_fichier);
		return;
	}

	// controle que le fichier contient le bon nombre d'items (à la bonne taille) sinon abandon
	if (fread(fr_quickitem_list, sizeof(item), FR_QUICKITEMS_MAXSIZE, fichier) != FR_QUICKITEMS_MAXSIZE)
	{
		for (i=0; i<FR_QUICKITEMS_MAXSIZE; i++) fr_quickitem_list[i].quantity = -1;
		LOG_ERROR("%s: %s \"%s\"\n", reg_error_str, cant_open_file, nom_fichier);
	}
	else
	{
		build_fr_quickitems(0);
	}
	fclose(fichier);
}

/* Sauvegarde des raccourcis de la barre dans le fichier du perso */
void save_fr_quickitems()
{
	char nom_fichier[128];
	FILE *fichier;

	if (!quickbar_loaded) return;

	safe_snprintf(nom_fichier, sizeof(nom_fichier), "barre_inventaire_%s.dat", username_str);
	my_tolower(nom_fichier);
	fichier = open_file_config(nom_fichier, "wb");
	if (fichier == NULL)
	{
		LOG_ERROR("%s: %s \"%s\"\n", reg_error_str, cant_open_file, nom_fichier);
		return;
	}

    fwrite(fr_quickitem_list, sizeof(fr_quickitem_list), 1, fichier);
    fclose(fichier);
}

