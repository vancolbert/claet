#include <stdlib.h>
#include <string.h>
#include "knowledge.h"
#include "asc.h"
#include "books.h"
#include "context_menu.h"
#include "elconfig.h"
#include "elwindows.h"
#include "gamewin.h"
#include "hud.h"
#include "multiplayer.h"
#include "notepad.h"
#include "paste.h"
#include "sound.h"
#include "stats.h"
#include "tabs.h"
#include "textures.h"
#include "translate.h"
#include "widgets.h"
#include "init.h"
int knowledge_win = -1;
int knowledge_menu_x = 100;
int knowledge_menu_y = 20;
int knowledge_menu_x_len = STATS_TAB_WIDTH;
int knowledge_menu_y_len = STATS_TAB_HEIGHT;
int knowledge_scroll_id = 13;
knowledge knowledge_list[KNOWLEDGE_LIST_SIZE];
int knowledge_count = 0;
char knowledge_string[400] = "";
static size_t cm_know_id = CM_INIT_VALUE;
static INPUT_POPUP ipu_know;
static char highlight_string[KNOWLEDGE_NAME_SIZE] = "";
static int know_show_win_help = 0;
static int mouse_over_progress_bar = 0;
#define MAX_NOMS_CATEGORIES 13
const char *noms_categories[] = {
	"Armes",
	"Armures",
	"Bijoux",
	"Combat",
	"Cristaux",
	"Essences",
	"Extraction",
	"Fabrication",
	"Généralités",
	"Invocation",
	"Métallurgie",
	"Potions",
	"Ingénierie",
	"Inconnue"
};
typedef struct {
	Uint8 id; // type de catégorie dans les connaissances
	char nom[100];
	Uint8 mouse_over;
	Uint8 click;
	Uint8 nbtotal; // nombre de connaissances dans cette catégorie
	Uint8 nbpresent; // nombre de connaissances lues
} categorie;
#define MAX_CATEGORIES 30
categorie categories[MAX_CATEGORIES]; // liste des catégories
int nb_categories = 0; // nombre de catégories à afficher (= max pos - 1)
int categorie_courante = -1; // catégorie sélectionnée pour affichage des connaissances
int nb_liste_courante = 0; // nombre de connaissances dans la liste courante
knowledge liste_courante[KNOWLEDGE_LIST_SIZE]; // connaissances de la catégorie courante
int separation = 133; // aligné avec l'onglet pour faire joli...
int saut_ligne = 13;
void check_book_known() {
	static Uint16 last_checked_book = -1;
	if (your_info.researching != last_checked_book) {
		last_checked_book = your_info.researching;
		if ((your_info.researching >= knowledge_count) && (your_info.researching < sizeof(knowledge_list))) {
			LOG_TO_CONSOLE(c_red2, unknown_book_long_str);
		}
	}
}
int is_researching(void) {
	if (your_info.researching < sizeof(knowledge_list)) {
		return 1;
	} else {
		return 0;
	}
}
float get_research_fraction(void) {
	float progress = 0;
	if (your_info.research_total > 0) {
		progress = (float)your_info.research_completed / (float)your_info.research_total;
	}
	if (progress > 1) {
		progress = 1;
	}
	return progress;
}
static float research_rate = -1;
void update_research_rate(void) {
	static int last_research_completed = -1;
	if (last_research_completed > your_info.research_completed) {
		last_research_completed = -1;
	}
	if (last_research_completed > 0) {
		if ((your_info.research_completed - last_research_completed) > 0) {
			research_rate = 1.0 / (float)(your_info.research_completed - last_research_completed);
			last_research_completed = your_info.research_completed;
		}
	} else {
		last_research_completed = your_info.research_completed;
		research_rate = 2.0 / (float)(your_info.wil.cur + your_info.rea.cur);
	}
}
static float get_research_eta(void) {
	if (research_rate < 0) {
		return 0;
	}
	return research_rate * (your_info.research_total - your_info.research_completed);
}
char *get_research_eta_str(char *str, size_t size) {
	float eta = get_research_eta();
	if (eta < 0.01) {
		safe_snprintf(str, size, completed_research);
	} else if (eta < 1) {
		safe_snprintf(str, size, lessthanaminute_str);
	} else {
		int ieta = (int)(eta + 0.5);
		safe_snprintf(str, size, "Temps : %i %s", ieta, (ieta == 1)?minute_str:minutes_str);
	}
	return str;
}
// Change la catégorie courante et recompose la liste des connaissances à afficher
void change_categorie(int cat) {
	int i;
	// affectation de la nouvelle catégorie courante si valide
	if (cat < nb_categories) {
		categorie_courante = cat;
	}
	// mise à jour de l'info dans la liste des catégories
	for (i = 0; i < nb_categories; i++) {
		categories[i].click = 0;
	}
	if (categorie_courante >= 0) {
		categories[categorie_courante].click = 1;
	}
	// reconstitution de la liste des connaissances à afficher
	nb_liste_courante = 0;
	for (i = 0; i <= knowledge_count; i++) {
		if (knowledge_list[i].type != categorie_courante) {
			continue;
		}
		if (!knowledge_list[i].affiche) {
			continue;
		}
		liste_courante[nb_liste_courante++] = knowledge_list[i];
	}
/*
        // nettoyage du reste de la liste courante
        // note: inutile puisque non affiché (nb_liste_courante servant de limite)
        for (i=nb_liste_courante; i<KNOWLEDGE_LIST_SIZE; i++)
        {
                liste_courante[i].present = -1;
                liste_courante[i].mouse_over = -1;
                liste_courante[i].name[0] = '\0';
                liste_courante[i].has_book = -1;
                liste_courante[i].is_stored = -1;
                liste_courante[i].affiche = -1;
                liste_courante[i].type = -1;
                liste_courante[i].id = -1;
                liste_courante[i].click = 0;
        }
 */
	// réajustement de la barre de défilement
	vscrollbar_set_bar_len(knowledge_win, knowledge_scroll_id, nb_liste_courante);
	vscrollbar_set_pos(knowledge_win, knowledge_scroll_id, 0);
}
int display_knowledge_handler(window_info *win) {
	int i, x = 2, y = 2;
	int scroll = vscrollbar_get_pos(knowledge_win, knowledge_scroll_id);
	char points_string[16];
	char *research_string = not_researching_anything;
	int is_researching = 1;
	int progress = (125 * your_info.research_completed + 1) / (your_info.research_total + 1);
	x += separation + 2;
	// On liste les categories
	for (i = 0; i < nb_categories; i++) {
		if (categories[i].mouse_over) {
			glColor3f(0.1f, 0.1f, 0.9f);
		} else if (categories[i].click) {
			glColor3f(0.8f, 0.0f, 0.0f);
		} else {
			float percent = (categories[i].nbtotal > 0) ? ((float)categories[i].nbpresent / categories[i].nbtotal) : 1;
			// on amoindrit tout pourcentage inferieur a 100% pour marquer la difference
			if (percent < 1) {
				percent = percent * 0.6f;
			}
			// couleur evolutive du blancs (0%) vers le vert (100%)
			glColor3f(1.0f - percent, 1.0f, 1.0f - percent);
		}
		draw_string_zoomed(4, 2 + saut_ligne * i, (unsigned char *)categories[i].nom, 1, 0.8);
	}
	float max_name_x = win->len_x - x - 32;
	if (your_info.research_total && (your_info.research_completed == your_info.research_total)) {
		safe_snprintf(points_string, sizeof(points_string), "%s", completed_research);
	} else {
		safe_snprintf(points_string, sizeof(points_string), "%i/%i", your_info.research_completed, your_info.research_total);
	}
	if (your_info.researching < knowledge_count) {
		int j;
		for (j = 0; j <= knowledge_count; j++) {
			if (your_info.researching == knowledge_list[j].id) {
				research_string = knowledge_list[j].name;
				break;
			}
		}
	} else if (your_info.researching < KNOWLEDGE_LIST_SIZE) {
		research_string = unknown_book_short_str;
	} else {
		research_string = not_researching_anything;
		points_string[0] = '\0';
		progress = 1;
		is_researching = 0;
	}
	glDisable(GL_TEXTURE_2D);
	// trait de separation
	glBegin(GL_LINES);
	glColor4f(0.77f, 0.57f, 0.39f, 1.0f);
	glVertex3i(separation, 5, 0);
	glVertex3i(separation, 195, 0);
	glEnd();
	glColor3f(0.77f, 0.57f, 0.39f);
	glBegin(GL_LINES);
	// window separators
	glVertex3i(0, 200, 0);
	glVertex3i(win->len_x, 200, 0);
	glVertex3i(0, 300, 0);
	glVertex3i(win->len_x, 300, 0);
	// progress bar
	glVertex3i(445, 315, 0);
	glVertex3i(570, 315, 0);
	glVertex3i(445, 335, 0);
	glVertex3i(570, 335, 0);
	glVertex3i(445, 315, 0);
	glVertex3i(445, 335, 0);
	glVertex3i(570, 315, 0);
	glVertex3i(570, 335, 0);
	glEnd();
	glBegin(GL_QUADS);
	// progress bar
	glColor3f(0.40f, 0.40f, 1.00f);
	glVertex3i(446, 316, 0);
	glVertex3i(445 + progress, 316, 0);
	glColor3f(0.10f, 0.10f, 0.80f);
	glVertex3i(445 + progress, 334, 0);
	glVertex3i(446, 334, 0);
	glColor3f(0.77f, 0.57f, 0.39f);
	glEnd();
	glEnable(GL_TEXTURE_2D);
	// draw text
	draw_string_small(4, 210, (unsigned char *)knowledge_string, 4);
	glColor3f(1.0f, 1.0f, 1.0f);
	draw_string_small(10, 320, (unsigned char *)researching_str, 1);
	draw_string_small(100, 320, (unsigned char *)research_string, 1);
	draw_string_small(480, 320, (unsigned char *)points_string, 1);
	if (is_researching && mouse_over_progress_bar) {
		char eta_string[21];
		int eta_pos;
		get_research_eta_str(eta_string, sizeof(eta_string));
		eta_pos = (int)(rx - strlen(eta_string) * SMALL_FONT_X_LEN) / 2;
		draw_string_small(500 + eta_pos, 285, (unsigned char *)eta_string, 1);
		mouse_over_progress_bar = 0;
	}
	// Draw knowledges
	for (i = scroll; i < scroll + 15 && i < nb_liste_courante; i++) {
		int highlight = 0;
		if (*highlight_string && (strlen(liste_courante[i].name) > 0) && (get_string_occurance(highlight_string, liste_courante[i].name, strlen(liste_courante[i].name), 1) != -1)) {
			highlight = 1;
		}
		if (liste_courante[i].mouse_over) {
			glColor3f(0.1f, 0.1f, 0.9f);
		} else if (liste_courante[i].click) {
			glColor3f(0.9f, 0.0f, 0.0f);
		} else if (liste_courante[i].present) {
			if (highlight) {
				glColor3f(1.0f, 0.6f, 0.6f);
			} else {
				glColor3f(0.0f, 0.8f, 0.0f);
			}
		} else if (liste_courante[i].is_stored) {
			if (highlight) {
				glColor3f(0.7f, 0.4f, 0.9f);
			} else {
				glColor3f(0.7f, 0.7f, 0.7f);
			}
		} else {
			if (highlight) {
				glColor3f(0.7f, 0.4f, 0.0f);
			} else {
				glColor3f(0.5f, 0.5f, 0.5f);
			}
		}
		draw_string_zoomed_width(x, y, (unsigned char *)liste_courante[i].name, max_name_x, 1, 0.7);
		y += saut_ligne;
	}
	return 1;
}
int mouseover_knowledge_handler(window_info *win, int mx, int my) {
	int i;
	if (mx >= win->len_x - 140 && mx < win->len_x - 15 && my > 315 && my < 335) {
		mouse_over_progress_bar = 1;
	}
	if (cm_window_shown() != CM_INIT_VALUE) {
		return 0;
	}
	for (i = 0; i <= nb_liste_courante; i++) {
		liste_courante[i].mouse_over = 0;
	}
	for (i = 0; i <= nb_categories; i++) {
		categories[i].mouse_over = 0;
	}
	if (my > 0) {
		know_show_win_help = 1;
	}
	if (mx > win->len_x - 20) {
		return 0;
	}
	if (my > 192) {
		return 0;
	}
	i = my / saut_ligne;
	if (mx >= separation) {
		i += vscrollbar_get_pos(knowledge_win, knowledge_scroll_id);
		if (i < nb_liste_courante) {
			liste_courante[i].mouse_over = 1;
		}
	} else {
		if (i < nb_categories) {
			categories[i].mouse_over = 1;
		}
	}
	return 0;
}
int click_knowledge_handler(window_info *win, int mx, int my, Uint32 flags) {
	int x, y, idx;
	Uint8 str[3];
	x = mx;
	y = my;
	if (x > win->len_x - 20) {
		return 0;
	}
	if (y > 192) {
		return 0;
	}
	if (flags & ELW_WHEEL_UP) {
		vscrollbar_scroll_up(knowledge_win, knowledge_scroll_id);
		return 1;
	} else if (flags & ELW_WHEEL_DOWN) {
		vscrollbar_scroll_down(knowledge_win, knowledge_scroll_id);
		return 1;
	} else {
		// On clique dans la liste des connaissances
		if (x >= separation) {
			idx = y / saut_ligne + vscrollbar_get_pos(knowledge_win, knowledge_scroll_id);
			if (idx < nb_liste_courante) {
				int i;
				for (i = 0; i < nb_liste_courante; i++) {
					liste_courante[i].click = 0;
				}
				liste_courante[idx].click = 1;
				str[0] = GET_KNOWLEDGE_INFO;
				*(Uint16 *)(str + 1) = SDL_SwapLE16((short)liste_courante[idx].id);
				my_tcp_send(my_socket, str, 3);
			}
		}
		// On clique dans la liste des catégories
		else {
			if (y <= nb_categories * saut_ligne) {
				change_categorie(y / saut_ligne);
			}
		}
		do_click_sound();
	}
	return 1;
}
void get_knowledge_list(Uint16 size, const char *list) {
	int i;
	// make sure the entire knowledge list is 0 incase of short data
	for (i = 0; i < KNOWLEDGE_LIST_SIZE; i++) {
		knowledge_list[i].present = 0;
	}
	// watch for size being too large
	if (size * 8 > KNOWLEDGE_LIST_SIZE) {
		size = KNOWLEDGE_LIST_SIZE / 8;
	}
	// now copy the data
	for (i = 0; i < size; i++) {
		int j;
		int k;
		for (j = 0; j < 8; j++) {
			for (k = 0; k < KNOWLEDGE_LIST_SIZE; k++) {
				if (knowledge_list[k].id == i * 8 + j) {
					if (j == 0) {
						knowledge_list[k].present = list[i] & 0x01;
					}
					if (j == 1) {
						knowledge_list[k].present = list[i] & 0x02;
					}
					if (j == 2) {
						knowledge_list[k].present = list[i] & 0x04;
					}
					if (j == 3) {
						knowledge_list[k].present = list[i] & 0x08;
					}
					if (j == 4) {
						knowledge_list[k].present = list[i] & 0x10;
					}
					if (j == 5) {
						knowledge_list[k].present = list[i] & 0x20;
					}
					if (j == 6) {
						knowledge_list[k].present = list[i] & 0x40;
					}
					if (j == 7) {
						knowledge_list[k].present = list[i] & 0x80;
					}
					break;
				}
			}
		}
	}
	// Pour mettre à jour les compteurs des catégories le plus simple est de réinitialiser
	init_categories();
	change_categorie(categorie_courante);
}
void get_new_knowledge(Uint16 idx) {
	int idx_connaissance, idx_categorie;
	// changement de l'état dans la liste des connaissances
	for (idx_connaissance = 0; idx_connaissance < knowledge_count; idx_connaissance++) {
		if (idx == knowledge_list[idx_connaissance].id) {
			break;
		}
	}
	if (idx_connaissance == knowledge_count) {
		return; // connaissance non trouvée !
	}
	knowledge_list[idx_connaissance].present = 1;
	// mise à jour du compteur de connaissances lues dans la catégorie
	for (idx_categorie = 0; idx_categorie < nb_categories; idx_categorie++) {
		if (knowledge_list[idx_connaissance].type == categories[idx_categorie].id) {
			break;
		}
	}
	if (idx_categorie == nb_categories) {
		return; // catégorie non trouvée !
	}
	categories[idx_categorie].nbpresent++; // à moins qu'un recomptage complet soit plus sûr ?
	// changement de l'état si nécessaire dans la liste courante
	if (idx_categorie == categorie_courante) {
		for (idx_connaissance = 0; idx_connaissance < nb_liste_courante; idx_connaissance++) {
			if (idx == liste_courante[idx_connaissance].id) {
				break;
			}
		}
		if (idx_connaissance == nb_liste_courante) {
			return; // connaissance non trouvée !
		}
		liste_courante[idx_connaissance].present = 1;
	}
}
static void set_hightlight_callback(const char *new_highlight_string, void *data) {
	safe_strncpy(highlight_string, new_highlight_string, KNOWLEDGE_NAME_SIZE);
}
static int cm_knowledge_handler(window_info *win, int widget_id, int mx, int my, int option) {
	switch (option) {
	case 0:
		close_ipu(&ipu_know);
		init_ipu(&ipu_know, knowledge_win, DEFAULT_FONT_X_LEN * 20, -1, 40, 1, NULL, set_hightlight_callback);
		ipu_know.x = mx;
		ipu_know.y = my;
		display_popup_win(&ipu_know, know_highlight_prompt_str);
		if (ipu_know.popup_win >= 0 && ipu_know.popup_win < windows_list.num_windows) {
			windows_list.window[ipu_know.popup_win].opaque = 1;
		}
		break;
	case 1:
		set_hightlight_callback("", NULL);
		break;
	case 2: {
		int i;
		for (i = 0; i <= nb_liste_courante; i++) {
			if (liste_courante[i].mouse_over) {
				copy_to_clipboard(liste_courante[i].name);
				break;
			}
		}
		break;
	}
	}
	return 1;
}
void fill_knowledge_win() {
	set_window_handler(knowledge_win, ELW_HANDLER_DISPLAY, &display_knowledge_handler);
	set_window_handler(knowledge_win, ELW_HANDLER_CLICK, &click_knowledge_handler);
	set_window_handler(knowledge_win, ELW_HANDLER_MOUSEOVER, &mouseover_knowledge_handler);
	knowledge_scroll_id = vscrollbar_add_extended(knowledge_win, knowledge_scroll_id, NULL, knowledge_menu_x_len - 20, 0, 20, 200, 0, 15, 0.77f, 0.57f, 0.39f, 0, 1, nb_liste_courante);
	if (cm_valid(!cm_know_id)) {
		cm_know_id = cm_create(know_highlight_cm_str, cm_knowledge_handler);
		cm_add_window(cm_know_id, knowledge_win);
		init_ipu(&ipu_know, -1, -1, -1, 1, 1, NULL, NULL);
	}
}
void display_knowledge() {
	if (knowledge_win < 0) {
		knowledge_win = create_window("Knowledge", game_root_win, 0, knowledge_menu_x, knowledge_menu_y, knowledge_menu_x_len, knowledge_menu_y_len, ELW_WIN_DEFAULT);
		fill_knowledge_win();
	} else {
		show_window(knowledge_win);
		select_window(knowledge_win);
	}
}
void init_categories() {
	int i, empty_slot = -1;
	// initialisation de la liste des categories
	for (i = 0; i < MAX_CATEGORIES; i++) {
		categories[i].nbpresent = 0;
		categories[i].nbtotal = 0;
	}
	nb_categories = 0;
	// parcours des connaissances pour remplir les catégories
	for (i = 0; i < knowledge_count; i++) {
		int cat = knowledge_list[i].type;
		if (!knowledge_list[i].affiche) {
			continue;
		}
		if (cat < MAX_CATEGORIES) {
			if (categories[cat].nbtotal <= 0) {
				categories[cat].id = cat;
				safe_snprintf(categories[cat].nom, sizeof(categories[cat].nom), "%s", noms_categories[cat < MAX_NOMS_CATEGORIES ? cat : MAX_NOMS_CATEGORIES]);
				nb_categories++;
			}
			if (knowledge_list[i].present) {
				categories[cat].nbpresent++;
			}
			categories[cat].nbtotal++;
		}
	}
	// compactage de la liste des catégories (supprime les trous éventuels)
	// permettant d'avoir un indice correspondant à l'ordre d'affichage
	for (i = 0; i < MAX_CATEGORIES; i++) {
		if ((empty_slot >= 0) && (categories[i].nbtotal > 0)) {
			categories[empty_slot] = categories[i];
			categories[i].nbtotal = 0;
			empty_slot++;
		} else if ((empty_slot < 0) && (categories[i].nbtotal <= 0)) {
			empty_slot = i;
		}
	}
	// si jamais la catégorie courante n'est plus valide
	if (categorie_courante >= nb_categories) {
		change_categorie(-1);
	}
}
