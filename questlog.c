#ifndef NEW_QUESTLOG

#include <stdlib.h>
#include <string.h>
#include "dialogues.h"
#include "questlog.h"
#include "asc.h"
#include "elwindows.h"
#include "gamewin.h"
#include "init.h"
#include "interface.h"
#include "tabs.h"
#include "errors.h"
#include "translate.h"
#include "io/elpathwrapper.h"
#ifdef POPUP_AIDE_FR
#include "popup.h"
#include "session.h"
#endif

int questlog_win=0;
int questlog_menu_x=150;
int questlog_menu_y=70;
int questlog_menu_x_len=STATS_TAB_WIDTH;
int questlog_menu_y_len=STATS_TAB_HEIGHT;
#define questlog_split 85

_logquest *logquest, *current_quest, *last_quest, *selected_quest, *mouseover_quest;
int count_quest = 0;
_logdata *current = NULL;

int	quest_quest_page_start=0;
int	quest_quest_page_pos=0;

int	quest_text_page_start=0;
int	quest_text_page_pos=0;
FILE *qlf = NULL;

int questlog_y;

void load_questlog()
{
	FILE *f = NULL;

	char temp[1000];
	char questlog_ini[2048];

	// Utilisation d'un fichier nominatif de quetes
	strcpy (temp, username_str);
	my_tolower(temp);

#ifndef WINDOWS
    snprintf (questlog_ini, sizeof (questlog_ini), "%s/%s_quest.log", configdir, temp);
	// don't use my_fopen here, not everyone uses local settings
	f= fopen(questlog_ini,"rb"); //try to load local settings
	if(!f)	//use global settings
		f= my_fopen(questlog_ini,"rb");

#else
	strcpy(questlog_ini, configdir);
	strcat(questlog_ini, temp);
	strcat(questlog_ini, "_quest.log");
	f= my_fopen(questlog_ini,"rb");
#endif

	mouseover_quest = selected_quest = last_quest = current_quest = logquest = NULL;
	count_quest = 0;
	current = NULL;

	if(!f)	return;

	while(!feof(f))
	{
		temp[0] = 0;
		fgets(temp, 999,f);
		if(temp[0] == 0) break;
		if (temp[1] != '[')
			add_questlog_line2("[Quête Inconnue]", temp, strlen(temp)-1, 0); // -1 a cause du \n
		else
			add_questlog_line(temp, strlen(temp)-1); // -1 a cause du \n
	}

	fclose(f);
	if (logquest != NULL)
	{
		selected_quest = logquest;
		current = selected_quest->Data;
	}
}

// Libération des ressources des quetes
void unload_questlog()
{
	_logquest *q= logquest;
	while (q != NULL)
	{
		_logquest *tmpq = q;

		//frees all the quest list
		_logdata *t= q->Data;
		while(t!=NULL)
		{
			_logdata *tmp= t;

			if(t->msg)
				free(t->msg);
			t = t->Next;
			free(tmp);
		}

		q = q->Next;
		if (tmpq)
		{
			if (tmpq->Title)
				free(tmpq->Title);
			free (tmpq);
		}
	}

	if (qlf != NULL)
		fclose(qlf);
}

// Justification du texte à la largueur de la fenetre
void string_fix(char *t, int len)
{
	char *s = t;
	int maxchar = (questlog_menu_x_len - 25) / 8;	// calculate maximum amount of characters per line
	int i = 0, j = 0, lastspace = 0, c = 0;


	while (s[i] != 0 && len >= 0)
	{
		if (s[i] == ' ')
		{
			c = 0;
			lastspace = i;
		}
		if (j > maxchar)
		{
			j = c;
			t[lastspace] = '\n';
		}
		i++;
		j++;
		c++;
		len--;
	}
}

// Ajoute une chaine au QuestLog
void add_questlog(char *t, int len)
{
	//char *s= t;

	// write on file
	if (qlf == NULL){

		char questlog_ini[256];
		char text[256];

		strcpy (text, username_str);
		my_tolower(text);

#ifndef WINDOWS
		strcpy(questlog_ini, configdir);
		strcat(questlog_ini, text);
#else
		strcpy(questlog_ini, configdir);
		strcat(questlog_ini, text);
#endif
		strcat(questlog_ini,"_quest.log");

		qlf= fopen(questlog_ini/*"quest.log"*/,"ab");
		if (qlf== NULL)	return;
	}

	// Converting multiline msg in single line
	/*while (*s)
	{
		if(*s=='\n') *s= ' ';
		s++;
	}*/
	if (len <= 0) len = strlen(t);

	// On a pas de tag Alors on l'ajoute à la chaîne
	if (t[1] != '[')
	{
		if (add_questlog_line2("[Quête Inconnue]", t, len, 0))
		{
			fwrite (t, sizeof(char), 1 , qlf);						// Color
			fwrite ("[Quête Inconnue]", sizeof(char), 16 , qlf);	// Quest
			fwrite (t+1, sizeof(char), len - 1, qlf);				// Text
			fputc  (10, qlf);
		}
	}
	else if (memchr(t+1, ']', len-1) != NULL)
	{
		if (add_questlog_line(t, len))
		{
			fwrite(t, sizeof(char), len, qlf);
			fputc(10, qlf);
		}
	}
}

// On extrait le TAG de la quete du message
int add_questlog_line(char *t, int len)
{
	int ret;
	char str [80];
	int finish = 0;
	char *p = memchr(t+1, ']', len-1);

	if (p == NULL) return 0;

	memcpy (str, t+1, (p - t));
	str[p - t] = '\0';
	len -= (p - t);

	// Message de fin de quête = le titre de la quête fini par /]
	if (str[p - t - 2] == '/')
	{
		str[p - t - 2] = ']';
		str[p - t - 1] = '\0';
		finish = 1;
		#ifdef POPUP_AIDE_FR
        if (!strcmp(str,(char*)"[Combat]\0")&&fullsession_start_time>10000) {afficher_message_aide(6);}
        #endif
	}
	// Message silencieux = le titre de la quête commence par [#
	if (str[1] == '#')
	{
		int i;
		for (i = 1; str[i] != '\0'; i++)
			str[i] = str[i + 1];
	}

	*p = *t;
	ret = add_questlog_line2(str, p , len, finish);
	*p = ']';
	return ret;
}

// Ajout du texte dans la stucture mémoire
int add_questlog_line2(char *q, char *t, int len, int finish)
{
	_logquest *quest = logquest;
	_logdata *l, *l2;
	char buffer [1000];

	// Recheche la quete
	while (quest != NULL)
	{
		if (strcmp(quest->Title, q) == 0)
			break;

		quest = quest->Next;
	}

	// Si on ne la trouve pas on alloue une nouvelle quete
	if (quest == NULL)
	{
		quest = (_logquest*)malloc(sizeof(_logquest));
		memset (quest, 0, sizeof(_logquest));
		quest->Title = strdup(q);

		/* Ajout la quete en fin
		if (last_quest != NULL)
		{
			last_quest->Next = quest;
			last_quest = quest;
		}
		*/
		// Ajout en premier (les quetes les plus récentes sont placées en têtes
		if (logquest != NULL)
		{
			quest->Next = logquest;
			current_quest = logquest = quest;
		}
		else
		{
			current_quest = logquest = last_quest = quest;
		}
		count_quest ++;
	}

	// Création de la nouvelle ligne de données
	if (len <= 0)
		len = strlen(t);

	l = (_logdata*)malloc(sizeof(_logdata));
	l->Next= NULL;
	l->msg= (char*)malloc(len+1);
	strncpy(buffer, t, len);
	string_fix (buffer, len);
	strncpy(l->msg, buffer, len);
	l->msg[len]= 0;

	// On vérifie que la chaine n'existe pas déjà dans la quete
	l2 = quest->Data;
	while ( l2 != NULL )
	{
		if (strcmp(l->msg, l2->msg) == 0)
		{
			free(l->msg);
			free(l);
			return 0;
		}

		l2 = l2->Next;
	}

	// Si il a des éléments dans la liste, On ajoute à la fin
	if (quest->Last)
	{
		quest->Last->Next = l;
		quest->Last = l;
	}
	// Sinon on initialise le premier élément
	else
	{
		quest->Data = quest->Last = l;
	}
	quest->Count ++;
	if (quest->Finish == 0 && finish)
		quest->Finish = finish;

	return 1;
}

// On se positionne sur la bonne quete
void goto_quest_entry (int ln)
{
	int	cnt = ln;
	if (logquest == NULL)
		current_quest = NULL;

	if(ln <= 0)
	{
		// at or before the start
		current_quest = logquest;
		return;
	}
	else if (ln >= count_quest)
	{
		// at or after then end
		current_quest = last_quest;
		return;
	}

	//reset to the start
	current_quest = logquest;
	// loop thru all of the entries
	while(current_quest->Next && cnt-- > 0)
	{
		current_quest = current_quest->Next;
	}
}

// On se positionne sur la bonne entrée de texte
void goto_questlog_entry(int ln)
{
	int	cnt = ln;

	if (selected_quest == NULL)
		current = NULL;

	if(ln <= 0)
	{
		// at or before the start
		current = selected_quest->Data;
		return;
	}
	else if (ln >= selected_quest->Count)
	{
		// at or after then end
		current = selected_quest->Last;
		return;
	}

	//reset to the start
	current = selected_quest->Data;
	// loop thru all of the entries
	while(current->Next && cnt-- > 0)
	{
		current = current->Next;
	}
}

// Dessine une chaine de texte
int draw_questlog_string(char *t)
{
	char temp[256];
	int i= 0;

	//we split the string in lines and draw it.
	while(*t!=0){
		while(*t!=10 && *t!=0){
			temp[i]= *t;
			t++;
			i++;
		}
		if(*t!=0)	t++;
		temp[i]= 0;
		i= 0;
		draw_string_small(2,questlog_y,(unsigned char*)temp,1);
		questlog_y+= 15;
		if(questlog_y > (questlog_menu_y_len-15))
			return 1;
	}
	return 0;
}

// Gestion de l'affichage de la fenêtre de quêtes
int	display_questlog_handler(window_info *win)
{
	_logquest *q = current_quest;
	_logdata *t  = current;
	char titre [100];

	//calc where the scroll bar goes
	int scroll_text  = quest_text_page_pos;
	int scroll_quest = quest_quest_page_pos;

	set_font(0);

	//glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glColor3f(0.77f,0.57f,0.39f);
	glBegin(GL_LINES);

	// window separators
	glVertex3i(0,questlog_split,0);
	glVertex3i(win->len_x,questlog_split,0);

	//scroll bar Quete
	glVertex3i(win->len_x-20,0,0);
	glVertex3i(win->len_x-20,win->len_y,0);

	glVertex3i(win->len_x-15, 10,0);
	glVertex3i(win->len_x-10, 5,0);
	glVertex3i(win->len_x-10,5,0);
	glVertex3i(win->len_x-5, 10,0);

	glVertex3i(win->len_x-15,questlog_split-10,0);
	glVertex3i(win->len_x-10,questlog_split-5,0);
	glVertex3i(win->len_x-10,questlog_split-5,0);
	glVertex3i(win->len_x-5, questlog_split-10,0);

	//scroll bar Texte
	glVertex3i(win->len_x-15,questlog_split+10,0);
	glVertex3i(win->len_x-10,questlog_split+5,0);
	glVertex3i(win->len_x-10,questlog_split+5,0);
	glVertex3i(win->len_x-5, questlog_split+10,0);

	glVertex3i(win->len_x-15,win->len_y-10,0);
	glVertex3i(win->len_x-10,win->len_y-5,0);
	glVertex3i(win->len_x-10,win->len_y-5,0);
	glVertex3i(win->len_x-5, win->len_y-10,0);
	glEnd();

	glBegin(GL_QUADS);

	//scroll bar Quete
	glVertex3i(win->len_x-13,(15)+scroll_quest,0);
	glVertex3i(win->len_x-7, (15)+scroll_quest,0);
	glVertex3i(win->len_x-7, (35)+scroll_quest,0);
	glVertex3i(win->len_x-13,(35)+scroll_quest,0);

	// scroll bar Texte
	glVertex3i(win->len_x-13,questlog_split+15+scroll_text,0);
	glVertex3i(win->len_x-7, questlog_split+15+scroll_text,0);
	glVertex3i(win->len_x-7, questlog_split+35+scroll_text,0);
	glVertex3i(win->len_x-13,questlog_split+35+scroll_text,0);
	glEnd();
	glEnable(GL_TEXTURE_2D);

	// Draw all quest
	questlog_y = 2;

	while (q != NULL && questlog_y < questlog_split - 15)
	{
		int questlog_x;

		strcpy(titre, q->Title);
		if (q->Finish)
			strcat(titre, " Terminée");
		questlog_x = (win->len_x / 2) - ((strlen(titre) * 8) / 2);

		if (q == selected_quest)
			glColor3f(1.0f,0.5f,0.2f);
		else if (q == mouseover_quest)
			glColor3f(0.1f,0.1f,0.9f);
		else if (q->Finish)
			glColor3f(0.2f,1.0f,0.2f);
		else
			glColor3f(0.9f,0.9f,0.9f);

		draw_string_small(questlog_x, questlog_y, (unsigned char*)titre, 1);
		questlog_y += 16;
		q = q->Next;
	}

	// Draw all texts from list
	questlog_y= questlog_split;
	while(t != NULL)
	{
		if (t->msg && draw_questlog_string(t->msg))	return 1;
		t = t->Next;
	}

	set_font(0);
	return 1;
}

// Gestion du déplacement de la souris sur la fenêtre
int mouseover_questlog_handler(window_info *win, int mx, int my)
{
	// Ques Texte
	if(mx < win->len_x-16 && my > 2 && my < questlog_split)
	{
		int cnt = (my-2) / 16;

		mouseover_quest = current_quest;
		// loop thru all of the entries
		while(mouseover_quest && cnt-- > 0)
		{
			mouseover_quest = mouseover_quest->Next;
		}
	}
	return 0;
}

// Gestion du click de la souris sur la fenêtre
int click_questlog_handler(window_info *win, int mx, int my, Uint32 flags)
{
	int x,y;

	x= mx;
	y= my;

	// Sélection d'une quete
	if(x < win->len_x-16 && y > 2 && y < questlog_split && !(flags&ELW_WHEEL_UP) && !(flags&ELW_WHEEL_DOWN))
	{
		_logquest *q = current_quest;
		int cnt = (y-2) / 16;

		// loop thru all of the entries
		while(q && cnt-- > 0)
		{
			q = q->Next;
		}

		if (q != NULL)
		{
			selected_quest = q;

			quest_text_page_start = 0;
			goto_questlog_entry(quest_text_page_start);
			quest_text_page_pos = 0;
		}

		return 1;
	}

	// Scroll Quete
	if((x > win->len_x-16 && y > (0) && y < (0)+16) || (y > (0) && y < questlog_split-2 && flags&ELW_WHEEL_UP))
	{
		if(quest_quest_page_start > 0)
		{
			quest_quest_page_start--;
			goto_quest_entry(quest_quest_page_start);
			quest_quest_page_pos=(questlog_split-( 50))*quest_quest_page_start / count_quest;
		}
		return 1;
	}
	if((x > win->len_x-16 && y > questlog_split-15 && y < questlog_split-2) || (y > (0) && y < questlog_split-2 && flags&ELW_WHEEL_DOWN))
	{
		if(quest_quest_page_start < count_quest)
		{
			quest_quest_page_start++;
			goto_quest_entry(quest_quest_page_start);
			quest_quest_page_pos=(questlog_split-(50))*quest_quest_page_start / count_quest;
		}
		return 1;
	}

	// Scroll Texte
	if((x > win->len_x-16 && y > questlog_split+2 && y < questlog_split+16) || (y > questlog_split+2 && y < win->len_y-2 && flags&ELW_WHEEL_UP))
	{
		if( selected_quest && quest_text_page_start > 0)
		{
			quest_text_page_start--;
			goto_questlog_entry(quest_text_page_start);
			quest_text_page_pos=((win->len_y-50-questlog_split)*quest_text_page_start)/selected_quest->Count;
		}
		return 1;
	}
	if((x > win->len_x-16 && y > win->len_y-15 && y < win->len_y-4) || (y > questlog_split+2 && y < win->len_y-2 && flags&ELW_WHEEL_DOWN))
	{
		if( selected_quest && quest_text_page_start < selected_quest->Count)
		{
			quest_text_page_start++;
			goto_questlog_entry(quest_text_page_start);
			quest_text_page_pos=((win->len_y-50-questlog_split)*quest_text_page_start)/selected_quest->Count;
		}
		return 1;
	}
	return 0;
}

// Gestion du drag dans la fenêtre
int drag_questlog_handler(window_info *win, int mx, int my, Uint32 flags, int dx, int dy)
{
	if( (win->drag_in == 1) ||
		(win->drag_in == 0 && count_quest > 0 &&
		  mx>win->len_x-20 && my>(0)+15+quest_quest_page_pos &&
		                      my<(0)+35+quest_quest_page_pos))
	{

		int scroll_area=questlog_split-(50);	// account for the X ^ V and the scrollbar

		quest_quest_page_pos += dy;
		if(quest_quest_page_pos < 0) quest_quest_page_pos= 0;
		if(quest_quest_page_pos >= scroll_area) quest_quest_page_pos=scroll_area;

		quest_quest_page_start= (count_quest * quest_quest_page_pos) / scroll_area;
		goto_quest_entry(quest_quest_page_start);

		// Drag dans la première partie
		win->drag_in = 1;

		return 1;
	}

	if( (win->drag_in == 2) ||
		(win->drag_in == 0 && selected_quest &&
		  mx>win->len_x-20 && my>questlog_split+15+quest_text_page_pos && my<questlog_split+35+quest_text_page_pos))
	{

		int scroll_area= win->len_y-50-questlog_split;	// account for the X ^ V and the scrollbar

		// Drag dans la seconde partie
		win->drag_in = 2;

		//if(left_click>1)
		quest_text_page_pos += dy;
		// bounds checking
		if(quest_text_page_pos < 0) quest_text_page_pos= 0;
		if(quest_text_page_pos >= scroll_area) quest_text_page_pos=scroll_area;
		//and set which item to list first
		quest_text_page_start= (selected_quest->Count * quest_text_page_pos)/scroll_area;
		goto_questlog_entry(quest_text_page_start);

		return 1;
	}

	return 0;
}

void fill_questlog_win ()
{
	set_window_handler(questlog_win, ELW_HANDLER_DISPLAY,   &display_questlog_handler );
	set_window_handler(questlog_win, ELW_HANDLER_CLICK,     &click_questlog_handler );
	set_window_handler(questlog_win, ELW_HANDLER_DRAG,      &drag_questlog_handler );
	set_window_handler(questlog_win, ELW_HANDLER_MOUSEOVER, &mouseover_questlog_handler );
}


// Initialisation de la fenêtre de Log
void display_questlog()
{
	if(questlog_win <= 0)
    {
		questlog_win= create_window("Quest", 0, 0, questlog_menu_x, questlog_menu_y, questlog_menu_x_len, questlog_menu_y_len, ELW_WIN_DEFAULT);
		fill_questlog_win ();
	}
	else
	{
		show_window(questlog_win);
		select_window(questlog_win);
	}
}

//	Draw a context menu like hightlight using the supplied coords.
//
void draw_highlight(int topleftx, int toplefty, int widthx, int widthy, size_t col)
{
	float colours[2][2][3] = { { {0.11f, 0.11f, 0.11f }, {0.77f, 0.57f, 0.39f} },
							  { {0.11, 0.11f, 0.11f}, {0.33, 0.42f, 0.70f} } };
	if (col > 1)
		col = 0;
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	glColor3fv(colours[col][0]);
	glVertex2i(topleftx, toplefty);
	glColor3fv(colours[col][1]);
	glVertex2i(topleftx, toplefty + widthy);
	glVertex2i(topleftx + widthx, toplefty + widthy);
	glColor3fv(colours[col][0]);
	glVertex2i(topleftx + widthx, toplefty);
	glEnd();
	glEnable(GL_TEXTURE_2D);
#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
}
#endif // #ifndef NEW_QUESTLOG
