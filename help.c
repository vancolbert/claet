#include <stdlib.h>
#include <string.h>
#include "help.h"
#include "asc.h"
#include "elwindows.h"
#include "encyclopedia.h"
#include "interface.h"
#include "tabs.h"
#include "textures.h"
#ifdef OPENGL_TRACE
#include "gl_init.h"
#endif
#ifndef ENGLISH
#include "url.h"
#endif //ENGLISH

int help_win=-1;
int help_menu_x=150;
int help_menu_y=70;
int help_menu_x_len=HELP_TAB_WIDTH;
int help_menu_y_len=HELP_TAB_HEIGHT;
int help_menu_scroll_id = 0;

#ifdef ENGLISH
// Pixels to Scroll
int help_max_lines=1000;
#endif //ENGLISH

int helppage;

#ifndef ENGLISH
#define MAX_HISTO_PAGE 20
typedef struct
{
	int page_precedente;
	int pos_barre_defilement;
} struct_historique;
int helpindex = -1;
static struct_historique HistoriqueHelp[MAX_HISTO_PAGE];
static int maxhistoriquepagehelp = 0;
static int historiquepagehelp = 0;

void reset_help()
{
	helpindex = -1;
	historiquepagehelp = 0;
	maxhistoriquepagehelp = 0;
	memset (HistoriqueHelp, 0, sizeof(HistoriqueHelp));
}
#endif //ENGLISH

int display_help_handler(window_info *win)
{
	_Text *t=Page[helppage].T.Next;
	_Image *i=Page[helppage].I.Next;
	int j;
	j=vscrollbar_get_pos(help_win,0);

#ifndef ENGLISH
	glDisable(GL_TEXTURE_2D);
	glColor3f(0.77f,0.57f,0.39f);
	glBegin(GL_LINES);

	glVertex3i(win->len_x-(20*4), win->len_y,0);
	glVertex3i(win->len_x-(20*4), win->len_y - 20,0);
	glVertex3i(win->len_x-(20*4), win->len_y - 20,0);
	glVertex3i(win->len_x       , win->len_y - 20,0);
	glVertex3i(win->len_x-(20*3), win->len_y - 20,0);
	glVertex3i(win->len_x-(20*3), win->len_y,0);
	glVertex3i(win->len_x-(20*2), win->len_y - 20,0);
	glVertex3i(win->len_x-(20*2), win->len_y,0);
	glEnd();

	// <
	if (historiquepagehelp > 0)
	{
		glColor3f(0.77f,0.57f,0.39f);
		glBegin(GL_LINES);
		glVertex3i(win->len_x-(20*4)+16, win->len_y - 16,0);
		glVertex3i(win->len_x-(20*4)+ 4, win->len_y - 10,0);
		glVertex3i(win->len_x-(20*4)+ 4, win->len_y - 10,0);
		glVertex3i(win->len_x-(20*4)+16, win->len_y -  4,0);
		glEnd();
	}

	//
	glColor3f(0.77f,0.57f,0.39f);
	glBegin(GL_LINES);
	glVertex3i(win->len_x-(20*3)+ 4, win->len_y -  4,0);
	glVertex3i(win->len_x-(20*3)+10, win->len_y - 16,0);
	glVertex3i(win->len_x-(20*3)+10, win->len_y - 16,0);
	glVertex3i(win->len_x-(20*3)+16, win->len_y -  4,0);
	glEnd();

	// >>
	if (maxhistoriquepagehelp > historiquepagehelp)
	{
		glColor3f(0.77f,0.57f,0.39f);
		glBegin(GL_LINES);
		glVertex3i(win->len_x-(20*2)+ 4, win->len_y - 16,0);
		glVertex3i(win->len_x-(20*2)+16, win->len_y - 10,0);
		glVertex3i(win->len_x-(20*2)+16, win->len_y - 10,0);
		glVertex3i(win->len_x-(20*2)+ 4, win->len_y -  4,0);
		glEnd();
	}

	glEnable(GL_TEXTURE_2D);
#endif //ENGLISH

	while(t){
		int ylen=(t->size)?18:15;
		int xlen=strlen(t->text)*((t->size)?11:8);

#ifdef ENGLISH
		if((t->y-j > 0) && (t->y-j < help_menu_y_len-20 ))
#else
		if((t->y-j > 0) && (t->y-j < help_menu_y_len-30 ))
#endif //ENGLISH
		{
			if(t->ref)
			{
				//draw a line
				glColor3f(0.5,0.5,0.5);
				glDisable(GL_TEXTURE_2D);
				glBegin(GL_LINES);
				glVertex3i(t->x+4,t->y+ylen-j,0);
				glVertex3i(t->x+4+xlen-8,t->y+ylen-j,0);
				glEnd();
				glEnable(GL_TEXTURE_2D);
#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
			}
			if(t->size)
			{
				if(t->ref && mouse_x>(t->x+win->cur_x) && mouse_x<(t->x+xlen+win->cur_x) && mouse_y>(t->y+win->cur_y-j) && mouse_y<(t->y+ylen+win->cur_y-j))
				glColor3f(0.3,0.6,1.0);
				else
				glColor3f(t->r,t->g,t->b);
				draw_string(t->x,t->y-j,(unsigned char*)t->text,1);
			}
			else
			{
				if(t->ref && mouse_x>(t->x+win->cur_x) && mouse_x<(t->x+xlen+win->cur_x) && mouse_y>(t->y+win->cur_y-j) && mouse_y<(t->y+ylen+win->cur_y-j))
				glColor3f(0.3,0.6,1.0);
				else
				glColor3f(t->r,t->g,t->b);
				draw_string_small(t->x,t->y-j,(unsigned char*)t->text,1);
			}
		}
		t=t->Next;
	}

	glColor3f(1.0f,1.0f,1.0f);
	while(i){
		if((i->y-j > 0) && (i->yend-j < help_menu_y_len-40 ))
		{
			if(i->mouseover==1)
			{
				i=i->Next;
				continue;
			}
			if(mouse_x>(i->x+win->cur_x) && mouse_x<(win->cur_x+i->xend) && mouse_y>(i->y+win->cur_y-j) && mouse_y<(win->cur_y+i->yend-j))
			{
				if(i->Next!=NULL)
				{
					if(i->Next->mouseover==1)
						i=i->Next;
				}
			}
#ifdef	NEW_TEXTURES
			bind_texture(i->id);
#else	/* NEW_TEXTURES */
			get_and_set_texture_id(i->id);
#endif	/* NEW_TEXTURES */
			glBegin(GL_QUADS);
			draw_2d_thing(i->u, i->v, i->uend, i->vend,i->x, i->y-j,i->xend,i->yend-j);
			glEnd();
		}
		i=i->Next;
	}
	return 1;
}

int click_help_handler(window_info *win, int mx, int my, Uint32 flags)
{
	_Text *t=Page[helppage].T.Next;

#ifndef ENGLISH
	if(mx > win->len_x-(20*4) && my > win->len_y - 20 && my < win->len_y )
	{
		// Previous
		if ( mx < win->len_x-(20*3))
		{
			if (historiquepagehelp > 0)
			{
				historiquepagehelp --;

				helppage = HistoriqueHelp[historiquepagehelp].page_precedente;
				HistoriqueHelp[historiquepagehelp+1].pos_barre_defilement = vscrollbar_get_pos(help_win, help_menu_scroll_id);

				vscrollbar_set_bar_len(help_win, help_menu_scroll_id, Page[helppage].max_y);
                vscrollbar_set_pos(help_win, help_menu_scroll_id, HistoriqueHelp[historiquepagehelp].pos_barre_defilement);
			}
		}
		// Home
		else if (mx < win->len_x-(20*2))
		{
			if (helppage != helpindex)
			{
				helppage = helpindex;
				if (historiquepagehelp + 1 < MAX_HISTO_PAGE)
				{
					historiquepagehelp ++;

					HistoriqueHelp[historiquepagehelp].page_precedente = helppage;
					HistoriqueHelp[historiquepagehelp-1].pos_barre_defilement = vscrollbar_get_pos(help_win, help_menu_scroll_id);

				    vscrollbar_set_bar_len(help_win, help_menu_scroll_id, Page[helppage].max_y);
                    vscrollbar_set_pos(help_win, help_menu_scroll_id, 0);
				}
				else
				{
					int i;
					for(i = 1; i < MAX_HISTO_PAGE; i++)
					{
						HistoriqueHelp[i-1].page_precedente = HistoriqueHelp[i].page_precedente;
					}
					HistoriqueHelp[historiquepagehelp].page_precedente = helppage;
					HistoriqueHelp[historiquepagehelp-1].pos_barre_defilement = vscrollbar_get_pos(help_win, help_menu_scroll_id);

				    vscrollbar_set_bar_len(help_win, help_menu_scroll_id, Page[helppage].max_y);
                    vscrollbar_set_pos(help_win, help_menu_scroll_id, 0);
				}
				maxhistoriquepagehelp = historiquepagehelp;
			}
		}
		// Next
		else
		{
			if (historiquepagehelp < maxhistoriquepagehelp)
			{
				historiquepagehelp++;

				helppage = HistoriqueHelp[historiquepagehelp].page_precedente;
				HistoriqueHelp[historiquepagehelp-1].pos_barre_defilement = vscrollbar_get_pos(help_win, help_menu_scroll_id);

				vscrollbar_set_bar_len(help_win, help_menu_scroll_id, Page[helppage].max_y);
                vscrollbar_set_pos(help_win, help_menu_scroll_id, HistoriqueHelp[historiquepagehelp].pos_barre_defilement);
			}
		}
	}
	else
	{
#endif //ENGLISH
	if(flags&ELW_WHEEL_UP) {
		vscrollbar_scroll_up(help_win, help_menu_scroll_id);
	} else if(flags&ELW_WHEEL_DOWN) {
		vscrollbar_scroll_down(help_win, help_menu_scroll_id);
	} else {
		int j = vscrollbar_get_pos(help_win, help_menu_scroll_id);

		while(t){
			int xlen=strlen(t->text)*((t->size)?11:8),ylen=(t->size)?18:15;
			if(t->ref && mx>(t->x) && mx<(t->x+xlen) && my>(t->y-j) && my<(t->y+ylen-j)){
				// check if its a webpage
				if (!strncasecmp(t->ref, "http://", 7)) {
					open_web_link(t->ref);
				} else {
					//changing page
					int i;
					for(i=0;i<numpage+1;i++){
						if(!xmlStrcasecmp((xmlChar*)Page[i].Name,(xmlChar*)t->ref)){
							helppage=i;
#ifndef ENGLISH
                            // store histopage
							if (historiquepagehelp+1 < MAX_HISTO_PAGE)
							{
 					 		   historiquepagehelp++;

 					 		   HistoriqueHelp[historiquepagehelp].page_precedente= helppage;
							   HistoriqueHelp[historiquepagehelp-1].pos_barre_defilement = vscrollbar_get_pos(help_win, help_menu_scroll_id);
	   			            }
	   			            else
	   			            {
							 	int i;
							 	for (i=1; i < MAX_HISTO_PAGE; i++)
								{
							 		HistoriqueHelp[i-1].page_precedente=HistoriqueHelp[i].page_precedente;
								}
						 		HistoriqueHelp[historiquepagehelp].page_precedente=helppage;
								HistoriqueHelp[historiquepagehelp-1].pos_barre_defilement = vscrollbar_get_pos(help_win, help_menu_scroll_id);
							}
							maxhistoriquepagehelp=historiquepagehelp;
#endif //ENGLISH
							vscrollbar_set_pos(help_win, help_menu_scroll_id, 0);
							vscrollbar_set_bar_len(help_win, help_menu_scroll_id, Page[helppage].max_y);
							break;
						}
					}
#ifndef ENGLISH
	      }
#endif //ENGlISH
				break;
			}
			t=t->Next;
		}
	}
#ifndef ENGLISH
    }
#endif //ENGLISH
	return 1;
}

void fill_help_win ()
{
	int i;
	for(i=0;i<=numpage;i++)
	{
		if(my_strcompare(Page[i].Name,"HelpPage"))
#ifndef ENGLISH
		{
			helpindex = helppage = i;
            // Ackak : fait planter le client pour l'instant
			HistoriqueHelp[historiquepagehelp].page_precedente = helpindex;
#endif //ENGLISH
			break;
#ifndef ENGLISH
		}
#endif //ENGLISH
	}
	helppage=i;
	set_window_handler (help_win, ELW_HANDLER_DISPLAY, &display_help_handler);
	set_window_handler (help_win, ELW_HANDLER_CLICK, &click_help_handler);

#ifdef ENGLISH
	help_menu_scroll_id = vscrollbar_add_extended(help_win, help_menu_scroll_id, NULL, help_menu_x_len-20, 0, 20, help_menu_y_len, 0, 1.0, 0.77f, 0.57f, 0.39f, 0, 30, Page[helppage].max_y);
#else //ENGLISH
	help_menu_scroll_id = vscrollbar_add_extended(help_win, help_menu_scroll_id, NULL, help_menu_x_len-20, 0, 20, help_menu_y_len-20, 0, help_menu_y_len-20, 0.77f, 0.57f, 0.39f, 0, SMALL_FONT_Y_LEN, Page[helppage].max_y);
#endif //ENGLISH
}
