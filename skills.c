#include <stdlib.h>
#include <string.h>
#include "skills.h"
#include "asc.h"
#include "elwindows.h"
#include "encyclopedia.h"
#include "interface.h"
#include "tabs.h"
#include "textures.h"
#include "url.h"
int skills_win = -1;
int skills_menu_x = 150;
int skills_menu_y = 70;
int skills_menu_x_len = HELP_TAB_WIDTH;
int skills_menu_y_len = HELP_TAB_HEIGHT;
int skills_menu_scroll_id = 0;
int skillspage;
#define MAX_HISTO_PAGE 20
typedef struct {
	int page_precedente;
	int pos_barre_defilement;
} struct_historique;
int skillsindex = -1;
static struct_historique HistoriqueSkills[MAX_HISTO_PAGE];
static int maxhistoriquepageskills = 0;
static int historiquepageskills = 0;
void reset_skills() {
	skillsindex = -1;
	historiquepageskills = 0;
	maxhistoriquepageskills = 0;
	memset(HistoriqueSkills, 0, sizeof(HistoriqueSkills));
}
int display_skills_handler(window_info *win) {
	_Text *t = Page[skillspage].T.Next;
	_Image *i = Page[skillspage].I.Next;
	int j = vscrollbar_get_pos(skills_win, skills_menu_scroll_id);
	glDisable(GL_TEXTURE_2D);
	glColor3f(0.77f, 0.57f, 0.39f);
	glBegin(GL_LINES);
	glVertex3i(win->len_x - (20 * 4), win->len_y, 0);
	glVertex3i(win->len_x - (20 * 4), win->len_y - 20, 0);
	glVertex3i(win->len_x - (20 * 4), win->len_y - 20, 0);
	glVertex3i(win->len_x, win->len_y - 20, 0);
	glVertex3i(win->len_x - (20 * 3), win->len_y - 20, 0);
	glVertex3i(win->len_x - (20 * 3), win->len_y, 0);
	glVertex3i(win->len_x - (20 * 2), win->len_y - 20, 0);
	glVertex3i(win->len_x - (20 * 2), win->len_y, 0);
	glEnd();
	// <
	if (historiquepageskills > 0) {
		glColor3f(0.77f, 0.57f, 0.39f);
		glBegin(GL_LINES);
		glVertex3i(win->len_x - (20 * 4) + 16, win->len_y - 16, 0);
		glVertex3i(win->len_x - (20 * 4) + 4, win->len_y - 10, 0);
		glVertex3i(win->len_x - (20 * 4) + 4, win->len_y - 10, 0);
		glVertex3i(win->len_x - (20 * 4) + 16, win->len_y - 4, 0);
		glEnd();
	}
	//
	glColor3f(0.77f, 0.57f, 0.39f);
	glBegin(GL_LINES);
	glVertex3i(win->len_x - (20 * 3) + 4, win->len_y - 4, 0);
	glVertex3i(win->len_x - (20 * 3) + 10, win->len_y - 16, 0);
	glVertex3i(win->len_x - (20 * 3) + 10, win->len_y - 16, 0);
	glVertex3i(win->len_x - (20 * 3) + 16, win->len_y - 4, 0);
	glEnd();
	// >>
	if (maxhistoriquepageskills > historiquepageskills) {
		glColor3f(0.77f, 0.57f, 0.39f);
		glBegin(GL_LINES);
		glVertex3i(win->len_x - (20 * 2) + 4, win->len_y - 16, 0);
		glVertex3i(win->len_x - (20 * 2) + 16, win->len_y - 10, 0);
		glVertex3i(win->len_x - (20 * 2) + 16, win->len_y - 10, 0);
		glVertex3i(win->len_x - (20 * 2) + 4, win->len_y - 4, 0);
		glEnd();
	}
	glEnable(GL_TEXTURE_2D);
	while (t) {
		int ylen = (t->size)?18:15;
		int xlen = strlen(t->text) * ((t->size)?11:8);
		if ((t->y - j > 0) && (t->y - j < skills_menu_y_len - 30)) {
			if (t->ref) {
				//draw a line
				glColor3f(0.5, 0.5, 0.5);
				glDisable(GL_TEXTURE_2D);
				glBegin(GL_LINES);
				glVertex3i(t->x + 4, t->y + ylen - j, 0);
				glVertex3i(t->x + 4 + xlen - 8, t->y + ylen - j, 0);
				glEnd();
				glEnable(GL_TEXTURE_2D);
			}
			if (t->size) {
				if (t->ref && mouse_x > (t->x + win->cur_x) && mouse_x < (t->x + xlen + win->cur_x) && mouse_y > (t->y + win->cur_y - j) && mouse_y < (t->y + ylen + win->cur_y - j)) {
					glColor3f(0.3, 0.6, 1.0);
				} else {
					glColor3f(t->r, t->g, t->b);
				}
				draw_string(t->x, t->y - j, (unsigned char *)t->text, 1);
			} else {
				if (t->ref && mouse_x > (t->x + win->cur_x) && mouse_x < (t->x + xlen + win->cur_x) && mouse_y > (t->y + win->cur_y - j) && mouse_y < (t->y + ylen + win->cur_y - j)) {
					glColor3f(0.3, 0.6, 1.0);
				} else {
					glColor3f(t->r, t->g, t->b);
				}
				draw_string_small(t->x, t->y - j, (unsigned char *)t->text, 1);
			}
		}
		t = t->Next;
	}
	glColor3f(1.0f, 1.0f, 1.0f);
	while (i) {
		if ((i->y - j > 0) && (i->yend - j < skills_menu_y_len - 40)) {
			if (i->mouseover == 1) {
				i = i->Next;
				continue;
			}
			if (mouse_x > (i->x + win->cur_x) && mouse_x < (win->cur_x + i->xend) && mouse_y > (i->y + win->cur_y - j) && mouse_y < (win->cur_y + i->yend - j)) {
				if (i->Next != NULL) {
					if (i->Next->mouseover == 1) {
						i = i->Next;
					}
				}
			}
			bind_texture(i->id);
			glBegin(GL_QUADS);
			draw_2d_thing(i->u, i->v, i->uend, i->vend, i->x, i->y - j, i->xend, i->yend - j);
			glEnd();
		}
		i = i->Next;
	}
	return 1;
}
int click_skills_handler(window_info *win, int mx, int my, Uint32 flags) {
	_Text *t = Page[skillspage].T.Next;
	if (mx > win->len_x - (20 * 4) && my > win->len_y - 20 && my < win->len_y ) {
		// Previous
		if ( mx < win->len_x - (20 * 3)) {
			if (historiquepageskills > 0) {
				historiquepageskills--;
				skillspage = HistoriqueSkills[historiquepageskills].page_precedente;
				HistoriqueSkills[historiquepageskills + 1].pos_barre_defilement = vscrollbar_get_pos(skills_win, skills_menu_scroll_id);
				vscrollbar_set_bar_len(skills_win, skills_menu_scroll_id, Page[skillspage].max_y);
				vscrollbar_set_pos(skills_win, skills_menu_scroll_id, HistoriqueSkills[historiquepageskills].pos_barre_defilement);
			}
		}
		// Home
		else if (mx < win->len_x - (20 * 2)) {
			if (skillspage != skillsindex) {
				skillspage = skillsindex;
				if (historiquepageskills + 1 < MAX_HISTO_PAGE) {
					historiquepageskills++;
					HistoriqueSkills[historiquepageskills].page_precedente = skillspage;
					HistoriqueSkills[historiquepageskills - 1].pos_barre_defilement = vscrollbar_get_pos(skills_win, skills_menu_scroll_id);
					vscrollbar_set_bar_len(skills_win, skills_menu_scroll_id, Page[skillspage].max_y);
					vscrollbar_set_pos(skills_win, skills_menu_scroll_id, 0);
				} else {
					int i;
					for (i = 1; i < MAX_HISTO_PAGE; i++) {
						HistoriqueSkills[i - 1].page_precedente = HistoriqueSkills[i].page_precedente;
					}
					HistoriqueSkills[historiquepageskills].page_precedente = skillspage;
					HistoriqueSkills[historiquepageskills - 1].pos_barre_defilement = vscrollbar_get_pos(skills_win, skills_menu_scroll_id);
					vscrollbar_set_bar_len(skills_win, skills_menu_scroll_id, Page[skillspage].max_y);
					vscrollbar_set_pos(skills_win, skills_menu_scroll_id, 0);
				}
				maxhistoriquepageskills = historiquepageskills;
			}
		}
		// Next
		else {
			if (historiquepageskills < maxhistoriquepageskills) {
				historiquepageskills++;
				skillspage = HistoriqueSkills[historiquepageskills].page_precedente;
				HistoriqueSkills[historiquepageskills - 1].pos_barre_defilement = vscrollbar_get_pos(skills_win, skills_menu_scroll_id);
				vscrollbar_set_bar_len(skills_win, skills_menu_scroll_id, Page[skillspage].max_y);
				vscrollbar_set_pos(skills_win, skills_menu_scroll_id, HistoriqueSkills[historiquepageskills].pos_barre_defilement);
			}
		}
	} else {
		if (flags & ELW_WHEEL_UP) {
			vscrollbar_scroll_up(skills_win, skills_menu_scroll_id);
		} else if (flags & ELW_WHEEL_DOWN) {
			vscrollbar_scroll_down(skills_win, skills_menu_scroll_id);
		} else {
			int j = vscrollbar_get_pos(skills_win, skills_menu_scroll_id);
			while (t) {
				int xlen = strlen(t->text) * ((t->size)?11:8), ylen = (t->size)?18:15;
				if (t->ref && mx > (t->x) && mx < (t->x + xlen) && my > (t->y - j) && my < (t->y + ylen - j)) {
					// check if its a webpage
					if (!strncasecmp(t->ref, "http://", 7)) {
						open_web_link(t->ref);
					} else {
						//changing page
						int i;
						for (i = 0; i < numpage + 1; i++) {
							if (!xmlStrcasecmp((xmlChar *)Page[i].Name, (xmlChar *)t->ref)) {
								skillspage = i;
								// store histopage
								if (historiquepageskills + 1 < MAX_HISTO_PAGE) {
									historiquepageskills++;
									HistoriqueSkills[historiquepageskills].page_precedente = skillspage;
									HistoriqueSkills[historiquepageskills - 1].pos_barre_defilement = vscrollbar_get_pos(skills_win, skills_menu_scroll_id);
								} else {
									int i;
									for (i = 1; i < MAX_HISTO_PAGE; i++) {
										HistoriqueSkills[i - 1].page_precedente = HistoriqueSkills[i].page_precedente;
									}
									HistoriqueSkills[historiquepageskills].page_precedente = skillspage;
									HistoriqueSkills[historiquepageskills - 1].pos_barre_defilement = vscrollbar_get_pos(skills_win, skills_menu_scroll_id);
								}
								maxhistoriquepageskills = historiquepageskills;
								vscrollbar_set_pos(skills_win, skills_menu_scroll_id, 0);
								vscrollbar_set_bar_len(skills_win, skills_menu_scroll_id, Page[skillspage].max_y);
								break;
							}
						}
					}
					break;
				}
				t = t->Next;
			}
		}
	}
	return 1;
}
void fill_skills_win() {
	int i;
	for (i = 0; i <= numpage; i++) {
		if (my_strcompare(Page[i].Name, "newskills")) {
			skillsindex = skillspage = i;
			HistoriqueSkills[historiquepageskills].page_precedente = skillsindex;
			break;
		}
	}
	skillspage = i;
	set_window_handler(skills_win, ELW_HANDLER_DISPLAY, &display_skills_handler);
	set_window_handler(skills_win, ELW_HANDLER_CLICK, &click_skills_handler);
	skills_menu_scroll_id = vscrollbar_add_extended(skills_win, skills_menu_scroll_id, NULL, skills_menu_x_len - 20, 0, 20, skills_menu_y_len - 20, 0, skills_menu_y_len - 20, 0.77f, 0.57f, 0.39f, 0, SMALL_FONT_Y_LEN, Page[skillspage].max_y);
}
