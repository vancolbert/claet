#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <string.h>
#include "books.h"
#include "asc.h"
#include "draw_scene.h"
#include "errors.h"
#include "elwindows.h"
#include "init.h"
#include "knowledge.h"
#include "multiplayer.h"
#include "new_character.h"
#include "textures.h"
#include "translate.h"
#ifdef OPENGL_TRACE
#include "gl_init.h"
#endif
#include "io/elfilewrapper.h"
#ifdef FR_VERSION
#include "gamewin.h"
#include "gl_init.h"
#include "hud.h"
#include "interface.h"
#include "themes.h"
#endif //FR_VERSION

#ifdef BSD
 #include <stdlib.h>
#else
 #ifdef OSX
  #include <sys/malloc.h>
 #else
  #ifndef alloca         // newer versions of SDL have their own alloca!
   #include <malloc.h>
  #endif   //alloca
 #endif   //OSX
#endif   //BSD

/* NOTE: This file contains implementations of the following, currently unused, and commented functions:
 *          Look at the end of the file.
 *
 * int have_book(int);
 */

#ifndef FR_VERSION
#define _TITLE 0
#define _AUTHOR 1
#define _TEXT 2
#define _IMAGE 3
#define _IMAGE_TEXT 4
#define _PAGE 6
#endif //FR_VERSION

#define LOCAL 0
#define SERVER 1

#define KNOWLEDGE_BOOK_OFFSET 10000

#ifdef FR_VERSION
#define PAGE_RATIO_XY 1.25 // ratio à maintenir pour la taille d'une page (largeur = hauteur * PAGE_RATIO_XY)
#ifdef FR_VERSION
#define PAGE_MARGE_X 0.125 // marge latérale (droite ou gauche) en pourcentage de la largeur d'une page
#define PAGE_MARGE_Y 0.125 // marge verticale (haut ou bas) pourcentage de la hauteur d'une page
float book_zoom = 1.0f;
int book_reload = 0;
#else  //FR_VERSION
#define PAGE_MARGE_X 0.0625 // marge latérale (droite ou gauche) en pourcentage de la largeur d'une page
#define PAGE_MARGE_Y 0.0938 // marge verticale (haut ou bas) pourcentage de la hauteur d'une page
#endif //FR_VERSION
int book_win_x = 0;
int book_win_y = 40; // ou pourquoi pas HUD_MARGIN_Y
int page_largeur;
int page_hauteur;
#endif //FR_VERSION

#ifdef FR_VERSION
int livre_ouvert = -1; //Le numéro du livre actuellement ouvert
#else //FR_VERSION
int book_opened=-1;//The ID of the book opened
#endif //FR_VERSION
#ifdef FR_VERSION
int fenetre_image = -1;
char nom_fichier[200];
#endif //FR_VERSION

#ifdef FR_VERSION
static int texture_livre_1_page  = -1;
static int texture_livre_2_pages = -1;
#else //FR_VERSION
static int paper1_text = -1; // Index in the texture cache of the paper texture
static int book1_text = -1;  // Index in the texture cache of the book texture
#endif //FR_VERSION

#ifdef FR_VERSION
typedef struct
{
    char **lignes;                  // Pointeur vers les differentes lignes
    couleur_rvb *couleur_texte;    // Pointeur vers la couleur du texte
    int page_numero;                // Numéro de la page
} struct_pages;

typedef struct _struct_livres
{
    char titre[35];                  // Titre du livre
    int num;                         // Numéro du livre
    int type;                        // Type de livre (une ou deux pages)
    int max_largeur;                 // Largeur maximum sur une page
    int max_lignes;                  // Maximum de lignes sur une page
    int nombre_pages;                // Nombre de pages totales
    int nombre_chapitres;            // Nombre de chapitres
    int page_active;                 // Numéro de la page active
    int serveur;                     // Livre serveur ou non

    struct_pages ** pages;           // Pointeur vers la liste des pages

    struct _struct_livres * suivant; // Pointeur vers le livre suivant
} struct_livres;
#else //FR_VERSION
typedef struct {
	char file[200];

	int x;
	int y;

	int w;
	int h;

	int texture;

	int u[2];
	int v[2];
} _image;

typedef struct {
	char ** lines;
	_image * image;
	int page_no;
} page;

typedef struct _book {
	char title[35];
	int id;

	int type;

	int no_pages;
	page ** pages;
	int max_width;
	int max_lines;

	int server_pages;
	int have_server_pages;
	int pages_to_scroll;

	int active_page;

	struct _book * next;
} book;

book * books=NULL;
#endif //FR_VERSION

#ifdef FR_VERSION
void ajout_livre (struct_livres *livre_actuel);
#else //FR_VERSION
void add_book(book *bs);
#endif //FR_VERSION

#ifdef FR_VERSION
struct_livres * liste_livres = NULL;
void affiche_livre (struct_livres *b);
#else //FR_VERSION
book * books=NULL;
void display_book_window(book *b);
#endif //FR_VERSION

/*Memory handling etc.*/

#ifdef FR_VERSION
// Ajout d'une page supplémentaire dans le livre
struct_pages * ajout_page(struct_livres * livre_actuel)
{
    struct_pages *pages;

    if (!livre_actuel)
    {
#ifdef FR_DEBUG_LIVRES
        LOG_ERROR("Flag FR_LIVRES : soucis lors de l'ajout de la page\n");
#endif //FR_DEBUG_LIVRES
        return NULL;
    }

    // Allocation mémoire pour la page
    pages = (struct_pages*) calloc (1, sizeof(struct_pages));
    if (pages == NULL)
    {
#ifdef FR_DEBUG_LIVRES
        LOG_ERROR("Flag FR_LIVRES : erreur lors de l'allocation de la page\n");
#endif //FR_DEBUG_LIVRES
        return NULL;
    }

    // Allocation mémoire pour les lignes
    pages->lignes = (char **) calloc(livre_actuel->max_lignes + 1, sizeof(char *));
    if (pages->lignes == NULL)
    {
#ifdef FR_DEBUG_LIVRES
        LOG_ERROR("Flag FR_LIVRES : erreur lors de l'allocation des lignes\n");
#endif //FR_DEBUG_LIVRES
        return NULL;
    }

    pages->couleur_texte = (couleur_rvb *) calloc(livre_actuel->max_lignes+1, sizeof(couleur_rvb));
    if (pages->couleur_texte == NULL)
    {
#ifdef FR_DEBUG_LIVRES
        LOG_ERROR("Flag FR_LIVRES : erreur lors de l'allocation des couleurs\n");
#endif //FR_DEBUG_LIVRES
        return NULL;
    }

    // On ajoute une page au livre
    pages->page_numero = livre_actuel->nombre_pages + 1;

    livre_actuel->pages = (struct_pages **) realloc(livre_actuel->pages, (livre_actuel->nombre_pages+2) * sizeof(struct_pages*));
    if (livre_actuel->pages == NULL)
    {
#ifdef FR_DEBUG_LIVRES
        LOG_ERROR("Flag FR_LIVRES : erreur lors de la réallocation des pages\n");
#endif //FR_DEBUG_LIVRES
        return NULL;
    }

    // On ajoute la page
    livre_actuel->pages[livre_actuel->nombre_pages++] = pages;
    livre_actuel->pages[livre_actuel->nombre_pages] = NULL;

    return pages;
}

struct_livres *cree_livre (int type, int num, int serveur)
{
	struct_livres *livre_actuel = (struct_livres*)calloc(1, sizeof(struct_livres));

	// initialise la taille d'une page selon la taille de l'écran
	if (! page_hauteur || ! page_largeur)
	{
		// on prend en compte la barre du HUD et la position y, on retire 20 pour les boutons de navigation
		page_hauteur = window_height - HUD_MARGIN_Y - book_win_y - 20;
		// la largeur est finalement calculée à partir de la hauteur pour conserver le ratio
		page_largeur = page_hauteur / PAGE_RATIO_XY;
		// selon la résolution et le ratio, la largeur d'une double page peut se trouver trop grande
		if (page_largeur*2 > window_width - HUD_MARGIN_X - book_win_x)
		{
			page_largeur = (window_width - HUD_MARGIN_X - book_win_x) / 2;
			page_hauteur = page_largeur * PAGE_RATIO_XY;
		}
	}

	// calcul l'espace pour le contenu en fonction de la taille de la fenêtre et des marges
	livre_actuel->max_largeur = page_largeur - 2*PAGE_MARGE_X*page_largeur;
	livre_actuel->max_lignes = (page_hauteur - 2*PAGE_MARGE_Y*page_hauteur) / (int)(DEFAULT_FONT_Y_LEN * book_zoom);

	// un switch selon le type n'est plus nécessaire ici, ce test est laissé par sécurité
	if (type != 2)
    {
        type = 1;
    }

    livre_actuel->type = type;
    livre_actuel->num = num;
    livre_actuel->nombre_chapitres = 0;
    livre_actuel->serveur = serveur;

	ajout_livre(livre_actuel);

    return livre_actuel;
}
#else //FR_VERSION
page * add_page(book * b)
{
	page *p;

	if(!b) return NULL;

	p=(page*)calloc(1,sizeof(page));
	p->lines=(char**)calloc(b->max_lines+1,sizeof(char *));
	p->page_no=b->no_pages+1;

	b->pages=(page**)realloc(b->pages,(b->no_pages+2)*sizeof(page*));
	b->pages[b->no_pages++]=p;
	b->pages[b->no_pages]=NULL;

	return p;
}

book *create_book (const char* title, int type, int id)
{
	book *b=(book*)calloc(1,sizeof(book));

	switch(type){
		case 2:
			b->max_width=20;
			b->max_lines=15;
			type=2;
			break;
		case 1:
		default:
			b->max_width=29;
			b->max_lines=20;
			type=1;
			break;
	}
	b->type=type;
	b->id=id;
	safe_snprintf(b->title, sizeof(b->title), "%s", title);

	add_book(b);

	return b;
}

_image *create_image (const char* file, int x, int y, int w, int h, float u_start, float v_start, float u_end, float v_end)
{
	_image *img=(_image *)calloc(1,sizeof(_image));

	img->x=x;
	img->y=y;
	img->w=w;
	img->h=h;
	img->u[0]=u_start;
	img->u[1]=u_end;
	img->v[0]=v_start;
	img->v[1]=v_end;

#ifdef	NEW_TEXTURES
	img->texture = load_texture_cached(file, tt_image);
#else	/* NEW_TEXTURES */
	img->texture=load_texture_cache(file,0);
#endif	/* NEW_TEXTURES */
	if(!img->texture) {
		free(img);
		img=NULL;
	}

	return img;
}

void free_page(page * p)
{
	char **l=p->lines;

	if(p->image) free(p->image);
	for(;*l;l++) free(*l);
	free(p->lines);
	free(p);
}

void free_book(book * b)
{
	int i;
	page **p;

	p=b->pages;
	for(i=0;i<b->no_pages;i++,p++) free_page(*p);
	free(b->pages);
	free(b);
}
#endif //FR_VERSION

/*Multiple book handling*/

#ifdef FR_VERSION
// Retourne le livre dans la liste des livres en fonction de son numéro
struct_livres * recupere_livre(int num)
{
    struct_livres *livre_actuel;

    for (livre_actuel = liste_livres ; livre_actuel; livre_actuel = livre_actuel->suivant)
    {
        if (livre_actuel->num == num)
        {
            break;
        }
    }

    return livre_actuel;
}
#else //FR_VERSION
book * get_book(int id)
{
	book *b;

	for(b=books;b;b=b->next){
		if(b->id==id) break;
	}

	return b;
}
#endif //FR_VERSION

#ifdef FR_VERSION
// Permet d'ajouter un livre dans la liste chaine de structure des livres
void ajout_livre (struct_livres *livre_actuel)
{
	struct_livres *livre_ajout = liste_livres;

    // La liste n'est pas vide, on se place donc a la fin
	if (livre_ajout)
    {
		for (;livre_ajout->suivant ; livre_ajout = livre_ajout->suivant);
        {
		    livre_ajout->suivant=livre_actuel;
        }
	}
    // La liste actuelle est vide
    else
    {
		liste_livres = livre_actuel;
	}
}
#else //FR_VERSION
void add_book(book *bs)
{
	book *b=books;
	if(b) {
		for(;b->next;b=b->next);
		b->next=bs;
	} else {
		books=bs;
	}
}
#endif //FR_VERSION

/*Book parser*/

#ifndef FR_VERSION
page * add_str_to_page(char * str, int type, book *b, page *p)
{
	char ** lines=NULL;
	char ** newlines=NULL;
	char ** newlines_ptr;
	int i;

	if(!str) return NULL;
	if(!p)p=add_page(b);

	newlines=get_lines(str, b->max_width);
	newlines_ptr = newlines;
	lines=p->lines;

	for(i=0;*lines;i++,lines++);

	if(type==_AUTHOR){
		*lines++=(char*)calloc(1,sizeof(char));
	} else if (type==_TITLE){
		*lines++=(char*)calloc(1,sizeof(char));
	}

	for(;newlines && *newlines;i++) {
		if(type==_AUTHOR){
			memmove(*newlines+1,*newlines,strlen(*newlines)+1);
			**newlines = to_color_char (c_orange3);
		} else if(type==_TITLE){
			memmove(*newlines+1,*newlines,strlen(*newlines)+1);
			**newlines = to_color_char (c_orange4);
		}
		if(i>=b->max_lines){
			*lines=NULL;
			p=add_page(b);
			lines=p->lines;
			i=0;
		}
		*lines++=*newlines++;
		// Grum: don't free *newlines, it's the pointer to actual line.
		// It's this pointer that is copied to p->lines, not the data
		// that it points to.
		//free(*newlines);
	}
	// This is a temporary array that holds the pointers to the lines. It
	// can safely be freed.
	free(newlines_ptr);

	if(i<b->max_lines){
		if(type==_AUTHOR){
			*lines++=(char*)calloc(1,sizeof(char));
		} else if (type==_TITLE){
			*lines++=(char*)calloc(1,sizeof(char));
		}
	}
	*lines=NULL;

	return p;
}

char * wrap_line_around_image(char * line, int w, int x, int max_width, char * last)
{
	int i,j;
	if(last){
		for(i=0;i<max_width;i++){
			if(!*last||(i>=x && i<x+w)) {
				*line++=' ';
			} else {
				for(j=0;j<max_width && last[j] && last[j]!=' ' && last[j]!='\n' && last[j]!='\r';j++);
				if(i+j<x||(i>=x+w && i+j<max_width)){
					for(;j-->=0;line++,last++){
						if(*last=='\r')
							last++;
						if(*last=='\n'){
							last++;
							i=max_width;
							break;
						}
						*line=*last;
						i++;
						if(!*last)
							break;
					}
				} else {
					*line++=' ';
				}
			}
		}

		*line=0;
	}

	return last;
}

page * add_image_to_page(char * in_text, _image *img, book * b, page * p)
{
	char **line;
	char *last_ptr;
	int i=0;
	int max_width;
	int max_lines;
	int h, w, x, y;

	if (img == NULL || b == NULL) return NULL;
	if(!p || p->image)p=add_page(b);

	max_width=b->max_width;
	max_lines=b->max_lines;

	h=img->h/16+1;
	y=img->y/16+1;
	w=img->w/9;
	x=img->x/10;

	if(y+h>max_lines || w+x>max_width) return NULL;

	line=p->lines;

	for(;line[i];i++);

	if(i+h>=max_lines||y<=i) {
		p=add_page(b);
		line=p->lines;
		i=0;
	}

	p->image=img;

	if(in_text){
		line+=i;

		last_ptr=in_text;

		for(i=0;i<h;i++,line++){//
			*line=(char*)malloc((max_width+2)*sizeof(char));
			if(i && (w<(max_width/3*2) && x+w<max_width) && last_ptr && *last_ptr){//If it's more than 2/3rd of the page width, don't work on it
				last_ptr=wrap_line_around_image(*line,w,x,b->max_width+1,last_ptr);
			} else {
				**line=0;
			}
		}

		if(*last_ptr)
			p=add_str_to_page(last_ptr,_IMAGE_TEXT,b,p);
	}

	return p;
}

/*XML-parser*/

void add_xml_image_to_page(xmlNode * cur, book * b, page *p)
{
	char *image_path;
	int x,y,w,h;
	float u_start,v_start,u_end,v_end;
	_image *img;
	char *text=NULL;

	x=xmlGetInt(cur,(xmlChar*)"x");
	y=xmlGetInt(cur,(xmlChar*)"y");
	w=xmlGetInt(cur,(xmlChar*)"w");
	h=xmlGetInt(cur,(xmlChar*)"h");

	u_start=xmlGetFloat(cur,(xmlChar*)"u_start");
	u_end=xmlGetFloat(cur,(xmlChar*)"u_end");
	if(!u_end)
		u_end=1;

	v_start=xmlGetFloat(cur,(xmlChar*)"v_start");
	if(!v_start)
		v_start=1;
	v_end=xmlGetFloat(cur,(xmlChar*)"v_end");

	image_path=(char*)xmlGetProp(cur,(xmlChar*)"src");
	if(!image_path) return;

	img=create_image(image_path, x, y, w, h, u_start, v_start, u_end, v_end);

	if(cur->children && cur->children->content){
		MY_XMLSTRCPY(&text, (char*)cur->children->content);
	}

	if(add_image_to_page(text, img, b, p)==NULL)
		free(img);

	xmlFree(image_path);
	if(text)
		free(text);
}

void add_xml_str_to_page(xmlNode * cur, int type, book * b, page *p)
{
	char * string=NULL;
	if(cur->children && cur->children->content && MY_XMLSTRCPY(&string, (char*)cur->children->content)!=-1){
		add_str_to_page(string, type, b, p);
	} else {
#ifndef OSX
		LOG_ERROR("An error occured when parsing the content of the <%s>-tag on line %d - Check it for letters that cannot be translated into iso8859-1\n", cur->name, cur->line);
#else
		LOG_ERROR("An error occured when parsing the content of the <%s>-tag - Check it for letters that cannot be translated into iso8859-1\n", cur->name);
#endif
	}
	free(string);
}

void add_xml_page(xmlNode *cur, book * b)
{
	page *p=add_page(b);
	for(;cur;cur=cur->next){
		if(cur->type == XML_ELEMENT_NODE){
			if (!xmlStrcasecmp(cur->name,(xmlChar*)"title")){
				add_xml_str_to_page(cur,_TITLE,b,p);
			} else if (!xmlStrcasecmp(cur->name,(xmlChar*)"author")){
				add_xml_str_to_page(cur,_AUTHOR,b,p);
			} else if (!xmlStrcasecmp(cur->name,(xmlChar*)"text")){
				add_xml_str_to_page(cur,_TEXT,b,p);
			} else if (!xmlStrcasecmp(cur->name,(xmlChar*)"image")){
				add_xml_image_to_page(cur, b, p);
			}
		}
	}
}

book * parse_book(xmlNode *in, char * title, int type, int id)
{
	xmlNode * cur;
	book * b=create_book(title, type, id);

	for(cur=in;cur;cur=cur->next){
		if(cur->type == XML_ELEMENT_NODE){
			if(!xmlStrcasecmp(cur->name,(xmlChar*)"page")){
				add_xml_page(cur->children,b);
			}
		}
	}

	return b;
}
#endif //FR_VERSION

#ifdef FR_VERSION
void ajout_saut_de_ligne (struct_livres * livre_actuel, int nombre_de_saut)
{
    int i = 0;
    int j = 0;
    char ** numero_ligne = NULL;

    // Si aucune page n'existe, on en crée la premiere
    if (livre_actuel->pages == NULL)
    {
#ifdef FR_DEBUG_LIVRES
        LOG_ERROR("Flag FR_LIVRES : ajout de la premiere page du livre dans \"ajout_saut_de_ligne\"\n");
#endif //FR_DEBUG_LIVRES
        ajout_page (livre_actuel);
    }

	numero_ligne = livre_actuel->pages[livre_actuel->nombre_pages-1]->lignes;

    // On se place a la derniere ligne disponible sur la page
	for (i = 0; *numero_ligne; i++, numero_ligne++);

    for (j = 0; j < nombre_de_saut; j++)
    {
        if (i+j >= livre_actuel->max_lignes)
        {
            ajout_page (livre_actuel);
            i = 0;
#ifdef FR_DEBUG_LIVRES
            LOG_ERROR("Flag FR_LIVRES : le saut de ligne arrive en fin de page, on arrete de sauter des lignes et on ajoute une page.\n");
#endif //FR_DEBUG_LIVRES
            break;
        }
		livre_actuel->pages[livre_actuel->nombre_pages-1]->lignes[i+j] = "\0";
    }
}

char ** formatage_lignes(char *str, int max_width, float zoom, int align)
{
	char **str_lines = NULL;
	int num_lines, line_width, char_width, len, idx, i, j;
	int last_space, last_width, nb_space, indent, reste;
	int space = round((float)get_font_width(0) * DEFAULT_FONT_X_LEN * zoom / 12.0f);

	if (! str) return NULL;
	if (space == 0) space = 1;

	for (num_lines = 0; *str; num_lines++)
	{
		str_lines = (char **) realloc(str_lines, (num_lines+2) * sizeof(char *));

		len = line_width = last_space = last_width = indent = reste = nb_space = 0;
		for (idx = 0; str[idx]; idx++)
		{
			// nouvelle ligne explicite : fin de la ligne courante
			if (str[idx] == '\n') { idx++; break; }
			// retour à la ligne ignorés : consersion en espaces
			if (str[idx] == '\r') { str[idx] = ' '; }
			// on retient le dernier espace trouvé
			if (str[idx] == ' ')
			{
				//TODO: affiner avec des exceptions (suivi d'une ponctuation par ex)
				//TODO: compléter en retenant également les tirets et soft hyphens
				last_space = idx;
				last_width = line_width;
				nb_space++;
			}

			char_width = round(((float)get_char_width(str[idx])) * DEFAULT_FONT_X_LEN * zoom / 12.0f);

			// ligne trop large devant être tronquée...
			if (line_width + char_width > max_width)
			{
				// on retrouve la position du dernier espace
				if (last_space > 0)
				{
					// la largeur jusqu'au dernier espace doit faire minimum 50%
					if (100 * last_width / max_width > 50)
					{
						nb_space--;
						line_width = last_width;
						len = last_space;
						idx = last_space + 1;
						break;
					}
				}
				// sinon on tronque bêtement sur place
/*
				line_width += char_width;
				len++;
				idx++;
				// en observant les caractères à sauter pour la ligne suivante
				while (str[idx] == ' ') idx++;
				if (str[idx] == '\r') idx++;
				if (str[idx] == '\n') idx++;
*/
				// fin de la ligne courante
				break;
			}

			// on continue avec la ligne courante
			line_width += char_width;
			len++;
		}

		// alignement centré
		if      (align == 2) indent = (max_width - line_width) / (space * 2);
		// alignement à droite
		else if (align == 3) indent = (max_width - line_width) / space;
		// alignement justifié
		else if (align == 4)
		{
			// seules les lignes tronquées contenant au moins un espace sont à justifier
			if ((nb_space > 0) && (idx > 0) && (str[idx]) && (str[idx - 1] != '\n'))
			{
				reste = round((max_width - line_width) / space); // nombre d'espaces à insérer
				indent = reste / nb_space; // nb d'espaces ajoutés à chaque espace existant
				reste  = reste % nb_space; // nb d'espaces restant à ajouter

				str_lines[num_lines] = (char*) calloc(indent * nb_space + reste + len + 1, sizeof(char));
				i = 0;
				while (len-- > 0)
				{
					str_lines[num_lines][i++] = str[0];
					if (str[0] == ' ')
					{
						for (j=0; j<indent; j++) str_lines[num_lines][i++] = ' ';
						if (reste-- > 0) str_lines[num_lines][i++] = ' ';
					}
					str++;
					idx--;
				}
				str_lines[num_lines][i++] = '\0';
				str+= idx;
				continue;
			}
		}

		str_lines[num_lines] = (char*) calloc(indent + len + 1, sizeof(char));
		for (i = 0; i < indent; i++) str_lines[num_lines][i] = ' ';
		strncpy(indent + str_lines[num_lines], str, len);
		str_lines[num_lines][indent + len] = '\0';
		str+= idx;
	}
	if (str_lines) str_lines[num_lines] = NULL; //Used to get the bounds for displaying each line

	return str_lines;
}

// Ajoute du texte dans le livre
void ajout_texte_xml (struct_livres * livre_actuel, char * noeud, int align, couleur_rvb couleur_texte, int type)
{
    char ** lignes_de_texte;
    char ** texte_tempo;
	char ** numero_ligne = NULL;
    int i, k;

	// Si aucune page n'existe, on en crée la première
	if (livre_actuel->pages == NULL)
	{
#ifdef FR_DEBUG_LIVRES
		LOG_ERROR("Flag FR_LIVRES : ajout de la premiere page du livre dans \"ajout_texte_xml\"\n");
#endif //FR_DEBUG_LIVRES
		ajout_page(livre_actuel);
	}

	numero_ligne = livre_actuel->pages[livre_actuel->nombre_pages-1]->lignes;

	// On récupère les différents lignes du texte en fonction de la largeur maximum
	if (livre_actuel->pages != NULL)
	{
		// texte normal justifié quand aligné à gauche
		if ((type == 0) && (align == 1)) align = 4;
		// dialogues toujours alignés à gauche
		else if (type == 1) align = 1;
		set_font(police_livre);
		lignes_de_texte = formatage_lignes((char *)noeud, livre_actuel->max_largeur, book_zoom, align);
		set_font(0);
#ifdef FR_DEBUG_LIVRES
		LOG_ERROR("Flag FR_LIVRES : recuperation du texte\n");
#endif //FR_DEBUG_LIVRES
	}

	// On se place a la dernière ligne disponible sur la page
	for (i = 0; *numero_ligne; i++, numero_ligne++);

	// On se trouve a l'avant dernière ligne, on crée donc une nouvelle page
	// pour une meilleure présentation
	if (i == livre_actuel->max_lignes)
	{
#ifdef FR_DEBUG_LIVRES
		LOG_ERROR("Flag FR_LIVRES : ajout d'une page supplémentaire car on arrive en fin de page dans \"ajout_texte_xml\"\n");
#endif //FR_DEBUG_LIVRES
		ajout_page(livre_actuel);
		i = 0; // On remet la valeur de la ligne à 0
    }

	// C'est une strophe, on essai donc de ne pas faire de changement de page en cours
	// On vérifie donc qu'on ne va pas être obligé de changer de page en cours
	// et que la strophe a une taille inférieure à une page
	if (type == 2)
	{
		texte_tempo = lignes_de_texte;
		// On regarde le nombre de lignes qu'on va devoir ajouter
		for (k = 0; *texte_tempo; k++, texte_tempo++);
		if ((i + k >= livre_actuel->max_lignes) && (k <= livre_actuel->max_lignes))
		{
			ajout_page(livre_actuel);
			i = 0; // On remet la valeur de la ligne à 0
		}
	}

	// On fait le tour des différentes lignes qui ont été découpées
	// puis on les ajoute dans la page. Si le nombre de ligne est plus grand
	// que le maximum, un nouvelle page est crée.
	for (; *lignes_de_texte; lignes_de_texte++)
	{
#ifdef FR_DEBUG_LIVRES
		LOG_ERROR("Flag FR_LIVRES : valeur de i : %d\n", i);
#endif //FR_DEBUG_LIVRES
		livre_actuel->pages[livre_actuel->nombre_pages-1]->lignes[i] = lignes_de_texte[0];
		// Gestion de la couleur des lignes
		livre_actuel->pages[livre_actuel->nombre_pages-1]->couleur_texte[i] = couleur_texte;
#ifdef FR_DEBUG_LIVRES
		LOG_ERROR("Flag FR_LIVRES : ajout de la ligne : %s\n", livre_actuel->pages[livre_actuel->nombre_pages-1]->lignes[i]);
#endif //FR_DEBUG_LIVRES
        i++;

		if (i >= livre_actuel->max_lignes)
		{
#ifdef FR_DEBUG_LIVRES
			LOG_ERROR("Flag FR_LIVRES : ajout d'une page\n");
#endif //FR_DEBUG_LIVRES
			ajout_page(livre_actuel);
			i = 0;
		}
	}
}

// On vérifie les couleurs dans propriétés
couleur_rvb verif_couleur (xmlNode *courant)
{
    xmlChar *couleur_rouge_str = NULL;
    xmlChar *couleur_vert_str = NULL;
    xmlChar *couleur_bleu_str = NULL;
    couleur_rvb couleur_texte ={0.2f, 0.1f, 0.1f};
#ifdef FR_DEBUG_LIVRES
    int balise_couleur_existe = 0;
#endif //FR_DEBUG_LIVRES


    if ((couleur_rouge_str = xmlGetProp(courant, (xmlChar*)"rouge")) != NULL)
    {
#ifdef FR_DEBUG_LIVRES
        LOG_ERROR("Flag FR_LIVRES : couleur rouge indiquée\n");
        balise_couleur_existe++;
#endif //FR_DEBUG_LIVRES
        strtod_securise((char*)couleur_rouge_str, &couleur_texte.rouge);
    }

    if ((couleur_vert_str = xmlGetProp(courant, (xmlChar*)"vert")) != NULL)
    {
#ifdef FR_DEBUG_LIVRES
        LOG_ERROR("Flag FR_LIVRES : couleur vert indiquée\n");
        balise_couleur_existe++;
#endif //FR_DEBUG_LIVRES
        strtod_securise((char*)couleur_vert_str, &couleur_texte.vert);
    }

    if ((couleur_bleu_str = xmlGetProp(courant, (xmlChar*)"bleu")) != NULL)
    {
#ifdef FR_DEBUG_LIVRES
        LOG_ERROR("Flag FR_LIVRES : couleur bleu indiquée\n");
        balise_couleur_existe++;
#endif //FR_DEBUG_LIVRES
        strtod_securise((char*)couleur_bleu_str, &couleur_texte.bleu);
    }

#ifdef FR_DEBUG_LIVRES
    if (balise_couleur_existe == 0)
    {
        LOG_ERROR("Flag FR_LIVRES : aucune couleur indiquée\n");
    }
#endif //FR_DEBUG_LIVRES
    return couleur_texte;
}

// Vérifie si la propriété d'alignement est indiqué. Si ce n'est pas le cas
// le retour est l'alignement de base
int verif_alignement (xmlNode *courant, int align_base)
{
    xmlChar *align_str = NULL;

    if ((align_str = xmlGetProp(courant, (xmlChar*)"align")) != NULL)
    {
#ifdef FR_DEBUG_LIVRES
        LOG_ERROR("Flag FR_LIVRES : alignement indiqué\n");
#endif //FR_DEBUG_LIVRES
        if (!strncmp((char *)align_str, "gauche", 6))
        {
            return 1;
        }
        else if (!strncmp((char *)align_str, "centre", 6))
        {
            return 2;
        }
        else if (!strncmp((char *)align_str, "droite", 6))
        {
            return 3;
        }
        else
        {
#ifdef FR_DEBUG_LIVRES
            LOG_ERROR("Flag FR_LIVRES : alignement indiqué, mais faux\n");
#endif //FR_DEBUG_LIVRES
        // On retourne l'alignement de base vu que l'indication est fausse
             return align_base;
        }
    }
    else
    {
#ifdef FR_DEBUG_LIVRES
        LOG_ERROR("Flag FR_LIVRES : alignement non indiqué\n");
#endif //FR_DEBUG_LIVRES
        // On retourne l'alignement de base vu qu'il n'est pas indiqué
        return align_base;
    }
}

// Vérifie si la propriété de retrait est indiqué. Si ce n'est pas le cas
// retourne le texte sans changement
char* verif_retrait (xmlNode *courant, char *texte)
{
    xmlChar *retrait_str = NULL;
    int retrait = 0;
    int i = 0;

    if ((retrait_str = xmlGetProp(courant, (xmlChar*)"retrait")) != NULL)
    {
#ifdef FR_DEBUG_LIVRES
        LOG_ERROR("Flag FR_LIVRES : retrait indiqué\n");
#endif //FR_DEBUG_LIVRES
        // On récupère la valeur numérique
		strtol_securise((char *)retrait_str, &retrait);

        // Le retrait est de zéro ou bien ce n'est pas un chiffre correct
        if (retrait == 0)
        {
#ifdef FR_DEBUG_LIVRES
            LOG_ERROR("Flag FR_LIVRES : retrait indiqué, mais la conversion en numérique donne 0\n");
#endif //FR_DEBUG_LIVRES
            return texte;
        }
        else
        {
#ifdef FR_DEBUG_LIVRES
            LOG_ERROR("Flag FR_LIVRES : retrait indiqué et correct\n");
#endif //FR_DEBUG_LIVRES
			texte = (char *)realloc(texte, (strlen(texte) + retrait + 1) * sizeof(char));
			memmove(texte + retrait, texte, strlen(texte)+1);
            for (i = 0; i < retrait; i++)
            {
                texte[i] = ' ';
            }
			return texte;
        }
    }
    else
    {
#ifdef FR_DEBUG_LIVRES
        LOG_ERROR("Flag FR_LIVRES : retrait non indiqué\n");
#endif //FR_DEBUG_LIVRES
        return texte;
    }
}

// Vérifie si on force le numéro du chapitre
void verif_num_chapitre(xmlNode *courant, struct_livres *livre_actuel)
{
    xmlChar *num_chapitre_str = NULL;
    int num_chapitre = 0;

    if ((num_chapitre_str = xmlGetProp(courant, (xmlChar*)"num")) != NULL)
    {
#ifdef FR_DEBUG_LIVRES
        LOG_ERROR("Flag FR_LIVRES : numéro du chapitre indiqué\n");
#endif //FR_DEBUG_LIVRES
        // On récupère la valeur numérique
        strtol_securise((char *)num_chapitre_str, &num_chapitre);

        // Le numéro est zéro ou bien le chiffre n'est pas correct
        if (num_chapitre == 0)
        {
#ifdef FR_DEBUG_LIVRES
            LOG_ERROR("Flag FR_LIVRES : numéro du chapitre indiqué, mais la conversion en numérique donne 0\n");
#endif //FR_DEBUG_LIVRES
        }
        else
        {
#ifdef FR_DEBUG_LIVRES
            LOG_ERROR("Flag FR_LIVRES : numéro du chapitre indiqué et correct\n");
#endif //FR_DEBUG_LIVRES
            livre_actuel->nombre_chapitres = num_chapitre;
        }
    }
#ifdef FR_DEBUG_LIVRES
    else
    {
        LOG_ERROR("Flag FR_LIVRES : numéro du chapitre non indiqué\n");
    }
#endif //FR_DEBUG_LIVRES
}
struct_livres * analyse_livre(xmlNode *noeud, int type, int num, int serveur)
{
	xmlNode * courant;
	struct_livres * livre_actuel = cree_livre(type, num, serveur);
    xmlChar * nombre_saut_ligne_str = NULL;
    char * texte_UTF8 = NULL;
    char * texte_dialogue = NULL;
    char titre_chapitre[200];
    int nombre_saut_ligne = 1;
    int align = 0;
    couleur_rvb couleur_du_texte = {0, 0, 0};
	int i, count = 0;

    // On fait le tour des differentes balises
	for (courant = noeud ; courant ; courant=courant->next)
    {
		if (courant->type == XML_ELEMENT_NODE)
        {
            // On a trouve une balise "texte"
			if(!xmlStrcasecmp(courant->name, (xmlChar*)"texte"))
            {
#ifdef FR_DEBUG_LIVRES
                LOG_ERROR("Flag FR_LIVRES : balise texte trouve\n");
#endif //FR_DEBUG_LIVRES
                texte_UTF8 = fromUTF8 ((xmlChar*)courant->children->content, strlen ((const char*) courant->children->content));

                texte_UTF8 = verif_retrait(courant, texte_UTF8);
                align = verif_alignement(courant, 1);
                couleur_du_texte = verif_couleur(courant);
                ajout_texte_xml(livre_actuel, texte_UTF8, align, couleur_du_texte, 0);
			}
            // On a trouvé une balise "titre"
            else if(!xmlStrcasecmp(courant->name, (xmlChar*)"titre"))
            {
#ifdef FR_DEBUG_LIVRES
                LOG_ERROR("Flag FR_LIVRES : balise titre trouvée\n");
#endif //FR_DEBUG_LIVRES
                texte_UTF8 = fromUTF8 ((xmlChar*)courant->children->content, strlen ((const char*) courant->children->content));

                snprintf(livre_actuel->titre, sizeof(livre_actuel->titre), "%s", texte_UTF8);
                // On crée 2 pages, pour que le titre soit sur la page de droite
                // si c'est un livre sur 2 pages
                if (type == 2)
                {
                    ajout_page(livre_actuel);
                    ajout_page(livre_actuel);
                }

                ajout_saut_de_ligne(livre_actuel, livre_actuel->max_lignes/3);
                align = verif_alignement(courant, 2);
                couleur_du_texte = verif_couleur(courant);
                ajout_texte_xml(livre_actuel, texte_UTF8, align, couleur_du_texte, 0);
            }
            // On a trouvé une balise "auteur"
            else if(!xmlStrcasecmp(courant->name, (xmlChar*)"auteur"))
            {
#ifdef FR_DEBUG_LIVRES
                LOG_ERROR("Flag FR_LIVRES : balise auteur trouvée\n");
#endif //FR_DEBUG_LIVRES
                texte_UTF8 = fromUTF8 ((xmlChar*)courant->children->content, strlen ((const char*) courant->children->content));

                // On passe des lignes avant d'écrire l'auteur du livre et on passe à la page suivante
                ajout_saut_de_ligne(livre_actuel, livre_actuel->max_lignes/4);
                align = verif_alignement(courant, 2);
                couleur_du_texte = verif_couleur(courant);
                ajout_texte_xml(livre_actuel, texte_UTF8, align, couleur_du_texte, 0);
                ajout_page(livre_actuel);
            }
            // On a trouvé une balise "saut_de_ligne"
            else if(!xmlStrcasecmp(courant->name, (xmlChar*)"saut_de_ligne"))
            {
                nombre_saut_ligne = 1;
#ifdef FR_DEBUG_LIVRES
                LOG_ERROR("Flag FR_LIVRES : balise saut_de_ligne trouvée\n");
#endif //FR_DEBUG_LIVRES
                // Le nombre de saut de ligne est renseigné dans le fichier du livre
	            if ((nombre_saut_ligne_str = xmlGetProp(courant, (xmlChar*)"nombre")) != NULL)
                {
                    strtol_securise((char*)nombre_saut_ligne_str, &nombre_saut_ligne);
#ifdef FR_DEBUG_LIVRES
                    LOG_ERROR("Flag FR_LIVRES : nombre de saut de ligne indiqué\n");
#endif //FR_DEBUG_LIVRES
                }
                ajout_saut_de_ligne(livre_actuel, nombre_saut_ligne);
            }
            // On a trouvé une balise "saut_de_page"
            else if(!xmlStrcasecmp(courant->name, (xmlChar*)"saut_de_page"))
            {
#ifdef FR_DEBUG_LIVRES
                LOG_ERROR("Flag FR_LIVRES : balise saut_de_page trouvée\n");
#endif //FR_DEBUG_LIVRES
                ajout_page(livre_actuel);
            }
            // On a trouvé une balise "chapitre"
            else if(!xmlStrcasecmp(courant->name, (xmlChar*)"chapitre"))
            {
#ifdef FR_DEBUG_LIVRES
                LOG_ERROR("Flag FR_LIVRES : balise chapitre trouvée\n");
#endif //FR_DEBUG_LIVRES
                // On incrémente le nombre de chapitres
                livre_actuel->nombre_chapitres++;
                // On vérifie si le numéro du chapitre n'est pas forcé
                verif_num_chapitre(courant, livre_actuel);
                snprintf(titre_chapitre, sizeof(titre_chapitre), "Chapitre %d\n",livre_actuel->nombre_chapitres);
                texte_UTF8 = fromUTF8 ((xmlChar*)courant->children->content, strlen ((const char*) courant->children->content));

                // La page actuelle n'est vide, on saute donc une page
	            if (*livre_actuel->pages[livre_actuel->nombre_pages-1]->lignes != NULL)
                {
                    ajout_page(livre_actuel);
                }

                // Le chapitre doit commencer sur la page de droite
                if (livre_actuel->nombre_pages%2 != 0)
                {
                    ajout_page(livre_actuel);
                }

                ajout_saut_de_ligne(livre_actuel, livre_actuel->max_lignes/3);
                couleur_du_texte = verif_couleur(courant);
                ajout_texte_xml(livre_actuel, (char *)titre_chapitre, 2, couleur_du_texte, 0);
                ajout_saut_de_ligne(livre_actuel, livre_actuel->max_lignes/4);
                align = verif_alignement(courant, 2);
                ajout_texte_xml(livre_actuel, texte_UTF8, align, couleur_du_texte, 0);
                ajout_page(livre_actuel);
            }
            // On a trouvé une balise "paragraphe"
            else if(!xmlStrcasecmp(courant->name, (xmlChar*)"paragraphe"))
            {
#ifdef FR_DEBUG_LIVRES
                LOG_ERROR("Flag FR_LIVRES : balise paragraphe trouvée\n");
#endif //FR_DEBUG_LIVRES
                texte_UTF8 = fromUTF8 ((xmlChar*)courant->children->content, strlen ((const char*) courant->children->content));

                texte_UTF8 = verif_retrait(courant, texte_UTF8);
                align = verif_alignement(courant, 1);
                couleur_du_texte = verif_couleur(courant);
                ajout_texte_xml(livre_actuel, texte_UTF8, align, couleur_du_texte, 0);
                ajout_saut_de_ligne(livre_actuel, nombre_saut_ligne);
            }
            // On a trouvé une balise "strophe"
            else if(!xmlStrcasecmp(courant->name, (xmlChar*)"strophe"))
            {
#ifdef FR_DEBUG_LIVRES
                LOG_ERROR("Flag FR_LIVRES : balise strophe trouvéee\n");
#endif //FR_DEBUG_LIVRES
                texte_UTF8 = fromUTF8 ((xmlChar*)courant->children->content, strlen ((const char*) courant->children->content));

                texte_UTF8 = verif_retrait(courant, texte_UTF8);
                align = verif_alignement(courant, 1);
                couleur_du_texte = verif_couleur(courant);
                ajout_texte_xml(livre_actuel, texte_UTF8, align, couleur_du_texte, 2);
                ajout_saut_de_ligne(livre_actuel, nombre_saut_ligne);
            }
            // On a trouvé une balise "dialogue"
            else if(!xmlStrcasecmp(courant->name, (xmlChar*)"dialogue"))
            {
#ifdef FR_DEBUG_LIVRES
                LOG_ERROR("Flag FR_LIVRES : balise dialogue trouvée\n");
#endif //FR_DEBUG_LIVRES
				texte_UTF8 = fromUTF8 ((xmlChar*)courant->children->content, strlen ((const char*) courant->children->content));

				// On compte les sauts de lignes pour avoir le nombre d'insertions
				for (i=0; texte_UTF8[i]; i++) { if (texte_UTF8[i] == '\n') count++; }
				texte_dialogue = (char *) calloc(strlen(texte_UTF8) + count*2 + 5, sizeof(char));

				// On ajoute des guillemets en début et fin ainsi que des tirets pour chaque tirade
				texte_dialogue[0] = 171; // guillemets ouvrants '«'
				texte_dialogue[1] = 160; // suivi d'un espace insécable
				for (i=2; *texte_UTF8; texte_UTF8++) {
					texte_dialogue[i++] = texte_UTF8[0];
					if (texte_UTF8[0] == '\n') {
						texte_dialogue[i++] = 172; // faux tiret cadratin '¬' (NOT)
						texte_dialogue[i++] = 160; // précédé d'un espace insécable
					}
				}
				texte_dialogue[i+1] = 187; // guillemets fermants '»'
				texte_dialogue[i+0] = 160; // précédé d'un espace insécable
				texte_dialogue[i+2] = '\0';
/*
				memmove(texte_dialogue+2, texte_UTF8, strlen(texte_UTF8));
				texte_dialogue[strlen(texte_UTF8)+3] = 187; // guillemets fermants '»'
				texte_dialogue[strlen(texte_UTF8)+2] = 160; // précédé d'un espace insécable
				texte_dialogue[strlen(texte_UTF8)+4] = '\0';
*/
				ajout_texte_xml(livre_actuel, texte_dialogue, align, couleur_du_texte, 1);
				ajout_saut_de_ligne(livre_actuel, 1);
            }
            // La balise est inconnue
            else
            {
                LOG_ERROR("Flag FR_LIVRES : balise inconnue %s\n", courant->name);
            }
		}
	}

	return livre_actuel;
}
#endif //FR_VERSION

#ifdef FR_VERSION
// Initialisation de la lecture du livre
struct_livres * lire_livre(char * fichier, int type, int num)
{
	xmlDoc * doc = NULL;
	xmlNode * root = NULL;
	struct_livres *livre_actuel = NULL;
	char chemin[1024];
#ifdef FR_DEBUG_LIVRES
    char info_debug[200];
#endif //R_DEBUG_LIVRES

	safe_snprintf (chemin, sizeof(chemin), "languages/%s/%s", lang, fichier);

#ifdef FR_DEBUG_LIVRES
    safe_snprintf (info_debug, sizeof(info_debug), "Chemin du livre local : %s", chemin);
    LOG_TO_CONSOLE(c_red1, info_debug);
#endif //R_DEBUG_LIVRES

	if ((doc = xmlReadFile(chemin, NULL, 0)) == NULL)
    {
        // Si on ne trouve pas le livre dans le bon repertoire, on va
        // le chercher dans le repertoire "en".
		safe_snprintf(chemin, sizeof(chemin), "languages/en/%s", fichier);
	}

	if (doc == NULL && ((doc = xmlReadFile(chemin, NULL, 0)) == NULL))
    {
        // Le fichier n'est pas trouve
		char str[200];

		safe_snprintf(str, sizeof(str), book_open_err_str, chemin);
		LOG_ERROR(str);
		LOG_TO_CONSOLE(c_red1,str);
	}
    else if ((root = xmlDocGetRootElement(doc)) == NULL)
    {
        // Il y a un soucis sur le xml du fichier
		LOG_ERROR("Erreur en analysant le fichier : %s", chemin);
	}
    else if (xmlStrcasecmp(root->name,(xmlChar*)"livre"))
    {
         // L'element <livre> qui doit etre la racine n'est pas present
		LOG_ERROR("L'element racine n'est pas <livre> dans le fichier %s", chemin);
	}
    else
    {
        // Tout se passe bien et donc on commence a analyser le fichier
		livre_actuel = analyse_livre (root->children, type, num, 0);
	}

	xmlFreeDoc(doc);

	return livre_actuel;
}
#else //FR_VERSION
book * read_book(char * file, int type, int id)
{
	xmlDoc * doc;
	xmlNode * root=NULL;
	xmlChar *title=NULL;
	book *b=NULL;
	char path[1024];

	safe_snprintf(path, sizeof(path), "languages/%s/%s", lang, file);

	if ((doc = xmlReadFile(path, NULL, 0)) == NULL) {
		safe_snprintf(path, sizeof(path), "languages/en/%s", file);
	}
	if(doc == NULL && ((doc = xmlReadFile(path, NULL, 0)) == NULL)) {
		char str[200];

		safe_snprintf(str, sizeof(str), book_open_err_str, path);
		LOG_ERROR(str);
		LOG_TO_CONSOLE(c_red1,str);
	} else if ((root = xmlDocGetRootElement(doc))==NULL) {
		LOG_ERROR("Error while parsing: %s", path);
	} else if(xmlStrcasecmp(root->name,(xmlChar*)"book")){
		LOG_ERROR("Root element in %s is not <book>", path);
	} else if((title=xmlGetProp(root,(xmlChar*)"title"))==NULL){
		LOG_ERROR("Root element in %s does not contain a title=\"<short title>\" property.", path);
	} else {
		b=parse_book(root->children, (char*)title, type, id);
	}

	if(title) {
		xmlFree(title);
	}

	xmlFreeDoc(doc);

	return b;
}
#endif //FR_VERSION

#ifdef ENGLISH
void parse_knowledge_item(xmlNode *in)
{
	xmlNode * cur;
	int id = -1;
	char * strID=NULL;
	char * string=NULL;

	for(cur=in;cur;cur=cur->next){
		if(cur->type == XML_ELEMENT_NODE){
			if(!xmlStrcasecmp(cur->name,(xmlChar*)"Knowledge")){
				if ((strID=(char*)xmlGetProp(cur,(xmlChar*)"ID"))==NULL){
					LOG_ERROR("Knowledge Item does not contain an ID property.");
				} else {
					id = atoi(strID);
					if(cur->children && cur->children->content && MY_XMLSTRCPY(&string, (char*)cur->children->content)!=-1){
						if (read_book(string, 2, id + KNOWLEDGE_BOOK_OFFSET) != NULL) {
							knowledge_list[id].has_book = 1;
						}
					} else {
#ifndef OSX
						LOG_ERROR("An error occured when parsing the content of the <%s>-tag on line %d - Check it for letters that cannot be translated into iso8859-1\n", cur->name, cur->line);
#else
						LOG_ERROR("An error occured when parsing the content of the <%s>-tag - Check it for letters that cannot be translated into iso8859-1\n", cur->name);
#endif
					}
					xmlFree(strID);
					free(string);
					string = NULL;
				}
			}
		}
	}

	return;
}

void read_knowledge_book_index()
{
	xmlDoc * doc;
	xmlNode * root=NULL;
	char path[1024];

	if ((doc = xmlReadFile("knowledge.xml", NULL, 0)) == NULL) {
			LOG_TO_CONSOLE(c_red1, "Can't open knowledge book index");
	} else if ((root = xmlDocGetRootElement(doc))==NULL) {
		LOG_ERROR("Error while parsing: %s", path);
	} else if(xmlStrcasecmp(root->name,(xmlChar*)"Knowledge_Books")){
		LOG_ERROR("Root element in %s is not <Knowledge_Books>", path);
	} else {
		parse_knowledge_item(root->children);
	}

	xmlFreeDoc(doc);

	return;
}
#endif //ENGLISH

#ifdef FR_VERSION
void init_livres()
{
	// évite un reload inutile alors qu'aucun livre n'est encore chargé
	// (à cause de l'initialisation au préalable de book_zoom dans elconfig.c)
	book_reload = 0;
#ifdef	NEW_TEXTURES
	texture_livre_1_page  = load_texture_cached ("textures/paper1.dds", tt_image);
	texture_livre_2_pages = load_texture_cached ("textures/book1.dds", tt_image);
#else	/* NEW_TEXTURES */
	texture_livre_1_page  = load_texture_cache_deferred ("./textures/paper1.bmp", 0);
	texture_livre_2_pages = load_texture_cache_deferred ("./textures/book1.bmp", 0);
#endif	/* NEW_TEXTURES */

	lire_livre("books/races/eldo.xml", 2, book_eldo);
	lire_livre("books/races/haut_elfe.xml", 2, book_haut_elfe);
	lire_livre("books/races/dwarf.xml", 2, book_dwarf);
	lire_livre("books/races/orchan.xml", 2, book_orchan);
	lire_livre("books/races/sinan.xml", 2, book_sinan);
	lire_livre("books/races/elfe_noir.xml", 2, book_elfe_noir);
	lire_livre("books/races/gnome.xml", 2, book_gnome);
	lire_livre("books/races/draegoni.xml", 2, book_draegoni);
}
#else //FR_VERSION
void init_books()
{
#ifdef	NEW_TEXTURES
	paper1_text = load_texture_cached ("textures/paper1.dds", tt_image);
	book1_text = load_texture_cached ("textures/book1.dds", tt_image);
#else	/* NEW_TEXTURES */
	paper1_text = load_texture_cache_deferred ("./textures/paper1.bmp", 0);
	book1_text = load_texture_cache_deferred ("./textures/book1.bmp", 0);
#endif	/* NEW_TEXTURES */

	read_book("books/races/human.xml", 2, book_human);
	read_book("books/races/dwarf.xml", 2, book_dwarf);
	read_book("books/races/elf.xml", 2, book_elf);
	read_book("books/races/gnome.xml", 2, book_gnome);
	read_book("books/races/orchan.xml", 2, book_orchan);
	read_book("books/races/draegoni.xml", 2, book_draegoni);

#ifdef ENGLISH
	read_knowledge_book_index();
#endif //ENGLISH
}
#endif //FR_VERSION

/*Network parser*/

#ifdef FR_VERSION
void ouvre_livre(int num_livre)
{
	struct_livres *livre_actuel;
	if (book_reload)
	{
		ferme_livre(livre_ouvert);
		libere_memoire_livres();
		book_reload = 0;
	}
	livre_actuel = recupere_livre(num_livre);
	if (!livre_actuel)
    {
		char str[3];

		str[0] = OUVRE_LIVRE;
		*((Uint16*)(str+1)) = SDL_SwapLE16((Uint16)num_livre);

		my_tcp_send(my_socket, (Uint8*)str, 3);
	}
    else
    {
        affiche_livre (livre_actuel);
	}
}

void lire_livre_local (const char *data, int len)
{
	char nom_fichier[200];
#ifdef FR_DEBUG_LIVRES
    char info_debug[200];
#endif //R_DEBUG_LIVRES
    struct_livres *livre_actuel;

	safe_snprintf (nom_fichier, sizeof(nom_fichier), "%.*s", len-3, data+3);

#ifdef FR_DEBUG_LIVRES
    safe_snprintf (info_debug, sizeof(info_debug), "Lecture du livre local : %s", nom_fichier);
    LOG_TO_CONSOLE(c_red1, info_debug);
#endif //R_DEBUG_LIVRES

	livre_actuel = recupere_livre (SDL_SwapLE16 (*((Uint16*)(data+1))));
	if (livre_actuel == NULL)
	{
		livre_actuel = lire_livre (nom_fichier, data[0], SDL_SwapLE16 (*((Uint16*)(data+1))));
		if (livre_actuel == NULL)
		{
			char str[200];
			safe_snprintf (str, sizeof(str), book_open_err_str, nom_fichier);
			LOG_TO_CONSOLE(c_red1, str);
			return;
		}
	}

    affiche_livre (livre_actuel);
}

void lire_livre_serveur (const char *donnees, int longueur_donnees)
{
	xmlDoc *doc = NULL;
    xmlNode *root = NULL;
	struct_livres *livre_actuel = NULL;
    int type = donnees[0];
    int num = SDL_SwapLE16 (*((Uint16*)(donnees+1)));
    int longueur = SDL_SwapLE16 (*((Uint16*)(donnees+3)));

    doc = xmlParseMemory(donnees+5, longueur);

    // On vérifie que les données sont bien du xml
    if (doc  == NULL)
    {
        LOG_TO_CONSOLE(c_red1, "Impossible de lire le livre");
        return;
    }
    else if ((root = xmlDocGetRootElement(doc)) == NULL)
    {
        // Il y a un soucis sur le xml du livre serveur
        LOG_ERROR("Erreur en analysant le livre serveur numéro %d", num);
        return;
    }
    // L'élément racine n'est pas <livre>
    else if (xmlStrcasecmp(root->name, (xmlChar*)"livre"))
    {
        LOG_ERROR("L'element racine n'est pas <livre> dans le livre serveur numéro %d", num);
        return;
    }
    // Tout est ok
    else
    {
#ifdef FR_DEBUG_LIVRES
        LOG_ERROR("Flag FR_LIVRES : on analyse le livre\n");
#endif //FR_DEBUG_LIVRES
        livre_actuel = analyse_livre (root->children, type, num, 1);
    }

	livre_actuel = recupere_livre (num);
	if (livre_actuel == NULL)
    {
#ifdef FR_DEBUG_LIVRES
        LOG_ERROR("Flag FR_LIVRES : on créé le livre\n");
#endif //FR_DEBUG_LIVRES
		livre_actuel = cree_livre (type, num, 1);
    }

    if (livre_actuel)
    {
#ifdef FR_DEBUG_LIVRES
        LOG_ERROR("Flag FR_LIVRES : on affiche le livre\n");
#endif //FR_DEBUG_LIVRES
        affiche_livre (livre_actuel);
    }
#ifdef FR_DEBUG_LIVRES
    LOG_ERROR("Flag FR_LIVRES : ok pour la lecture du livre serveur\n");
#endif //FR_DEBUG_LIVRES
}

void lire_livre_reseau (const char *donnees, int longueur_donnees)
{
    switch (*donnees)
    {
        case LOCAL:
#ifdef FR_DEBUG_LIVRES
        LOG_ERROR("Flag FR_LIVRES : réception d'un livre local\n");
#endif //FR_DEBUG_LIVRES
            lire_livre_local(&donnees[1], longueur_donnees-1);
            break;
        case SERVER:
#ifdef FR_DEBUG_LIVRES
        LOG_ERROR("Flag FR_LIVRES : réception d'un livre serveur\n");
#endif //FR_DEBUG_LIVRES
            lire_livre_serveur(&donnees[1], longueur_donnees-1);
            break;
    }
}
#else //FR_VERSION
void open_book(int id)
{
	book *b=get_book(id);

	if(!b) {
		char str[5];

		str[0]=SEND_BOOK;
		*((Uint16*)(str+1))=SDL_SwapLE16((Uint16)id);
		*((Uint16*)(str+3))=SDL_SwapLE16(0);

		my_tcp_send(my_socket, (Uint8*)str, 5);
	} else {
		display_book_window(b);
	}
}

void read_local_book (const char *data, int len)
{
	char file_name[200];
	book *b;

	safe_snprintf (file_name, sizeof(file_name), "%.*s", len-3, data+3);

	b = get_book (SDL_SwapLE16 (*((Uint16*)(data+1))));
	if (b == NULL)
	{
		b = read_book (file_name, data[0], SDL_SwapLE16 (*((Uint16*)(data+1))));
		if (b == NULL)
		{
			char str[200];
			safe_snprintf (str, sizeof(str), book_open_err_str, file_name);
			LOG_TO_CONSOLE(c_red1, str);
			return;
		}
	}

	display_book_window (b); // Otherwise there's no point...
}

page * add_image_from_server(char *data, book *b, page *p)
{
	int x, y;
	int w, h;
	int u_start, u_end;
	int v_start, v_end;
	char image_path[256];
	char text[512];
	int l=SDL_SwapLE16(*((Uint16*)(data)));
	_image *img;

	if(l>254)l=254;
	memcpy(image_path, data+2, l);
	image_path[l]=0;

	data+=l+2;

	l=SDL_SwapLE16(*((Uint16*)(data)));
	if(l>510)
		l=510;
	memcpy(text, data+2, l);
	text[l]=0;

	data+=l+2;

	x=SDL_SwapLE16(*((Uint16*)(data)));
	y=SDL_SwapLE16(*((Uint16*)(data+2)));
	w=SDL_SwapLE16(*((Uint16*)(data+4)));
	h=SDL_SwapLE16(*((Uint16*)(data+6)));

	u_start=data[8];
	u_end=data[9];
	v_start=data[10];
	v_end=data[11];

	img=create_image(image_path, x, y, w, h, u_start, v_start, u_end, v_end);
	if(add_image_to_page(text, img, b, p)==NULL) free(img);

	return p;
}

void read_server_book (const char *data, int len)
{
	char buffer[8192];
	book *b;
	page *p;
	int l = SDL_SwapLE16(*((Uint16*)(data+4)));
	int idx;

	if ( l >= sizeof (buffer) ) // Safer
		l = sizeof (buffer) - 1;
	memcpy (buffer, data+6, l);
	buffer[l] = '\0';

	b = get_book (SDL_SwapLE16 (*((Uint16*)(data+1))));
	if (b == NULL)
		b = create_book (buffer, data[0], SDL_SwapLE16 (*((Uint16*)(data+1))));

	b->server_pages = data[3];
	b->have_server_pages++;

	p=add_page(b);//Will create a page if pages is not found.

	idx = l + 6;
	while (idx <= len)
	{
		l = SDL_SwapLE16 (*((Uint16*)(&data[idx+1])));
		if ( l >= sizeof (buffer) ) // Safer.
			l = sizeof (buffer) - 1;
		memcpy (buffer, &data[idx+3], l);
		buffer[l]=0;

		switch (data[idx])
		{
			case _TEXT:
				p=add_str_to_page(buffer,_TEXT,b,p);
				break;
			case _AUTHOR:
				p=add_str_to_page(buffer,_AUTHOR,b,p);
				break;
			case _TITLE:
				p=add_str_to_page(buffer,_TITLE,b,p);
				break;
			case _IMAGE:
				p=add_image_from_server(buffer, b, p);
				break;
			case _PAGE:
				//p=add_page(b);
				break;
		}
		idx += l + 3;
	}

	b->active_page += b->pages_to_scroll;
	b->pages_to_scroll = 0;

	if (b) display_book_window (b); // Otherwise there's no point...
}


void read_network_book (const char *in_data, int data_length)
{
	switch (*in_data)
	{
		case LOCAL:
			read_local_book (&in_data[1], data_length-1);
			break;
		case SERVER:
			read_server_book (&in_data[1], data_length-1);
			break;
	}
}


/*Generic display*/

void display_image(_image *i)
{
	glColor4f(1.0f,1.0f,1.0f,0.5f);
#ifdef	NEW_TEXTURES
	bind_texture(i->texture);
#else	/* NEW_TEXTURES */
	get_and_set_texture_id(i->texture);
#endif	/* NEW_TEXTURES */
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.05f);
	glBegin(GL_QUADS);
		glTexCoord2f(i->u[0],i->v[1]); 	glVertex2i(i->x,i->y+i->h);
		glTexCoord2f(i->u[1],i->v[1]);	glVertex2i(i->x+i->w,i->y+i->h);
		glTexCoord2f(i->u[1],i->v[0]);	glVertex2i(i->x+i->w,i->y);
		glTexCoord2f(i->u[0],i->v[0]);	glVertex2i(i->x,i->y);
	glEnd();
	glDisable(GL_ALPHA_TEST);
#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
}
#endif //FR_VERSION

#ifdef FR_VERSION
void afficher_page(struct_livres * livre_actuel, struct_pages * page_actuelle)
{
    int numero_ligne = 0;
    char **ligne_texte;
    couleur_rvb *couleur_texte;

#ifdef FR_DEBUG_LIVRES
    LOG_ERROR("Flag FR_LIVRES : on affiche la page\n");
#endif //FR_DEBUG_LIVRES
    // La page n'existe pas donc on arrête ici.
    if (!page_actuelle)
    {
#ifdef FR_DEBUG_LIVRES
        LOG_ERROR("Flag FR_LIVRES : la page n'existe pas, bien qu'on veuille l'afficher\n");
#endif //FR_DEBUG_LIVRES
        return;
    }

	set_font(police_livre);

	for (ligne_texte = page_actuelle->lignes, couleur_texte = page_actuelle->couleur_texte; *ligne_texte; ligne_texte++, couleur_texte++)
	{
#ifdef FR_DEBUG_LIVRES
		LOG_ERROR("Flag FR_LIVRES : on affiche le texte (ligne %d) : %s\n", numero_ligne, ligne_texte[0]);
#endif //FR_DEBUG_LIVRES
		glColor3f(couleur_texte->rouge, couleur_texte->vert, couleur_texte->bleu);
		draw_string_zoomed(0, numero_ligne * (int)(DEFAULT_FONT_Y_LEN * book_zoom), (unsigned char*)ligne_texte[0], 0, book_zoom);
		numero_ligne++;
	}

	set_font(0);
}
#else //FR_VERSION
void display_page(book * b, page * p)
{
	char ** l;
	int i;
	char str[20];

	if(!p)
		return;

	set_font(2);
	if(p->image) {
		display_image(p->image);
	}

	for(i=0, l=p->lines; *l; i++,l++){
		glColor3f(0.34f,0.25f, 0.16f);
		draw_string_zoomed(10,i*18*0.9f,(unsigned char*)*l,0,1.0f);
	}

	glColor3f(0.385f,0.285f, 0.19f);

	safe_snprintf(str,sizeof(str),"%d",p->page_no);
	if(b->type==1)
		draw_string_zoomed(140,b->max_lines*18*0.9f+2,(unsigned char*)str,0,1.0);
	else if(b->type==2)
		draw_string_zoomed(110,b->max_lines*18*0.9f+2,(unsigned char*)str,0,1.0);
	set_font(0);
}
#endif //FR_VERSION

#ifdef FR_VERSION
void afficher_livre (struct_livres * livre_actuel)
{
    struct_pages ** page_actuelle = &livre_actuel->pages[livre_actuel->page_active];
	int x = 0;
	switch (livre_actuel->type)
    {
        // On a 2 pages a afficher
		case 2:
			glPushMatrix();
			glTranslatef(x, 0, 0);
			afficher_page(livre_actuel, *page_actuelle);
			glPopMatrix();
			// Si on est rendu a la derniere page
			// ne pas essayer d'afficher la page de droite qui n'existe pas
            if (livre_actuel->nombre_pages <= livre_actuel->page_active) break;
            x += page_largeur;
            page_actuelle++;
		case 1:
		default:
			glPushMatrix();
			glTranslatef(x, 0, 0);
			afficher_page(livre_actuel, *page_actuelle);
			glPopMatrix();
			break;
	}
#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
}
#else //FR_VERSION
void display_book(book * b, int type)
{
	page ** p=&b->pages[b->active_page];
	int x=0;
	switch(type){
		case 2:
			glPushMatrix();
			glTranslatef(x,0,0);
			display_page(b,*p);
			glPopMatrix();
			if(b->no_pages<=b->active_page)
				break;
			p++;
			x+=250;
		case 1:
		default:
			glPushMatrix();
			glTranslatef(x,0,0);
			display_page(b,*p);
			glPopMatrix();
			break;
	}
#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
}
#endif //FR_VERSION

/*Book window*/

int book_win=-1;
int paper_win=-1;
#ifdef FR_VERSION
int souris_livre_x = 0;
int souris_livre_y = 0;
#else //FR_VERSION
int book_win_x=100;
int book_win_y=100;
int book_win_x_len=400;
int book_win_y_len=300;

int book_mouse_x=0;
int book_mouse_y=0;
#endif //FR_VERSION

#ifdef FR_VERSION
int affiche_fenetre_livre(window_info *win)
{
   char strnum[20];
   struct_livres *livre_actuel=win->data;

    // Le livre n'existe pas
	if(!livre_actuel)
    {
		toggle_window(book_win);
		return 1;
	}

	switch(livre_actuel->type)
    {
		case 1:
#ifdef	NEW_TEXTURES
			bind_texture(texture_livre_1_page);
#else //NEW_TEXTURES
			get_and_set_texture_id(texture_livre_1_page);
#endif //NEW_TEXTURES
			safe_snprintf(strnum, sizeof(strnum), "%d/%d", livre_actuel->pages[livre_actuel->page_active]->page_numero, livre_actuel->nombre_pages);
			break;
		case 2:
#ifdef	NEW_TEXTURES
			bind_texture(texture_livre_2_pages);
#else //NEW_TEXTURES
			get_and_set_texture_id(texture_livre_2_pages);
#endif //NEW_TEXTURES
			safe_snprintf(strnum, sizeof(strnum), "%d-%d / %d", livre_actuel->pages[livre_actuel->page_active]->page_numero, livre_actuel->pages[livre_actuel->page_active]->page_numero+1, livre_actuel->nombre_pages);
			break;
	}

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER,0.03f);
	glBegin(GL_QUADS);
#ifdef	NEW_TEXTURES
		glTexCoord2f(0.0f,0.0f); glVertex3i(0,0,0);
		glTexCoord2f(0.0f,1.0f); glVertex3i(0,win->len_y-20,0);
		glTexCoord2f(1.0f,1.0f); glVertex3i(win->len_x,win->len_y-20,0);
		glTexCoord2f(1.0f,0.0f); glVertex3i(win->len_x,0,0);
#else //NEW_TEXTURES
		glTexCoord2f(0.0f,1.0f); glVertex3i(0,0,0);
		glTexCoord2f(0.0f,0.0f); glVertex3i(0,win->len_y-20,0);
		glTexCoord2f(1.0f,0.0f); glVertex3i(win->len_x,win->len_y-20,0);
		glTexCoord2f(1.0f,1.0f); glVertex3i(win->len_x,0,0);
#endif //NEW_TEXTURES
	glEnd();
	glDisable(GL_ALPHA_TEST);

	glPushMatrix();
	// application des marges d'une page (idem pour type 1 et 2) : arrondir éviterait l'effet "gribouilli" !
	glTranslatef(((int)(0.5f+page_largeur*PAGE_MARGE_X)), ((int)(0.5f+page_hauteur*PAGE_MARGE_Y)), 0);

    afficher_livre(livre_actuel);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0, win->len_y - 18, 0);
	souris_livre_y -= (win->len_y - 18);

	// on force un fond opaque derrière les boutons
	glColor3f(0.0f,0.0f,0.0f);
	glBegin(GL_QUADS);
		glVertex3i(0, 0, 0);
		glVertex3i(win->len_x, 0, 0);
		glVertex3i(win->len_x, 18, 0);
		glVertex3i(0, 18, 0);
	glEnd();

	// affichage du numéro de page, centré en bas de page
	glColor3f(0.95f, 0.76f, 0.52f);
	draw_string(90, 0, (unsigned char*)strnum, 0);

    // La souris est sur "<<"
	if (souris_livre_y>0 && souris_livre_y<18 && souris_livre_x>10 && souris_livre_x<(get_string_width((unsigned char*)"<<")*11.0f/12.0f)+10)
    {
		glColor3f(0.95f, 0.76f, 0.52f);
		draw_string(10, -2, (unsigned char*)"<<", 0);

		glColor3f(0.77f,0.59f, 0.38f);
		draw_string(win->len_x-75, -2, (unsigned char*)"->", 0);
		draw_string(50, -2, (unsigned char*)"<-", 0);
		draw_string(win->len_x-33, -2, (unsigned char*)">>", 0);
	}
    // La souris est sur ">>"
    else if(souris_livre_y>0 && souris_livre_y<18 && souris_livre_x>win->len_x-38 && souris_livre_x<win->len_x-33+(get_string_width((unsigned char*)">>")*11.0f/12.0f))
    {
		glColor3f(0.95f, 0.76f, 0.52f);
		draw_string(win->len_x-33, -2, (unsigned char*)">>", 0);

		glColor3f(0.77f, 0.59f, 0.38f);
		draw_string(win->len_x-75, -2, (unsigned char*)"->", 0);
		draw_string(50, -2, (unsigned char*)"<-", 0);
		draw_string(10, -2, (unsigned char*)"<<", 0);
	}
    // La souris est sur "->"
    else if(souris_livre_y>0 && souris_livre_y<18 && souris_livre_x>win->len_x-(55+get_string_width((unsigned char*)">>")*11.0f/12.0f) && souris_livre_x<win->len_x-(45+get_string_width((unsigned char*)">>")*11.0f/12.0f)+(get_string_width((unsigned char*)">>")*11.0f/12.0f))
    {
		glColor3f(0.95f, 0.76f, 0.52f);
		draw_string(win->len_x-75, -2, (unsigned char*)"->", 0);

		glColor3f(0.77f, 0.59f, 0.38f);
		draw_string(50, -2, (unsigned char*)"<-", 0);
		draw_string(10, -2, (unsigned char*)"<<", 0);
		draw_string(win->len_x-33, -2, (unsigned char*)">>", 0);
	}
    // La souris est sur "<-"
    else if(souris_livre_y>0 && souris_livre_y<18 && souris_livre_x>(20+get_string_width((unsigned char*)"<<")*11.0f/12.0f) && souris_livre_x<(35+get_string_width((unsigned char*)"<<")*11.0f/12.0f)+(get_string_width((unsigned char *)"<-")*11.0f/12.0f))
    {
		glColor3f(0.95f, 0.76f, 0.52f);
		draw_string(50, -2, (unsigned char*)"<-", 0);

		glColor3f(0.77f, 0.59f, 0.38f);
		draw_string(win->len_x-75, -2, (unsigned char*)"->", 0);
		draw_string(10, -2, (unsigned char*)"<<", 0);
		draw_string(win->len_x-33, -2, (unsigned char*)">>", 0);
	}
    // La souris n'est sur aucune des flèches
    else
    {
		glColor3f(0.77f, 0.59f, 0.38f);
		draw_string(50, -2, (unsigned char*)"<-", 0);
		draw_string(win->len_x-75, -2, (unsigned char*)"->", 0);
		draw_string(10, -2, (unsigned char*)"<<", 0);
		draw_string(win->len_x-33, -2, (unsigned char*)">>", 0);
	}

    // La souris est sur le [X]
	if(souris_livre_y>0 && souris_livre_y<18 && souris_livre_x>win->len_x/2-15 && souris_livre_x<win->len_x/2+15)
    {
		glColor3f(0.95f, 0.76f, 0.52f);
    }

	draw_string(win->len_x/2-15,0,(unsigned char*)"[X]",0);
	glPopMatrix();
#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
	return 1;
}
#else //FR_VERSION
int display_book_handler(window_info *win)
{
	int x=32,i,p;
	char str[20];
	book *b=win->data;

	if(!b) {
		toggle_window(book_win);
		return 1;
	}
	switch(b->type){
		case 1:
#ifdef	NEW_TEXTURES
			bind_texture(paper1_text);
#else	/* NEW_TEXTURES */
			get_and_set_texture_id(paper1_text);
#endif	/* NEW_TEXTURES */
			break;
		case 2:
#ifdef	NEW_TEXTURES
			bind_texture(book1_text);
#else	/* NEW_TEXTURES */
			get_and_set_texture_id(book1_text);
#endif	/* NEW_TEXTURES */
			break;
	}
	glBegin(GL_QUADS);
		glTexCoord2f(0.0f,1.0f); glVertex3i(0,0,0);
		glTexCoord2f(0.0f,0.0f); glVertex3i(0,win->len_y-20,0);
		glTexCoord2f(1.0f,0.0f); glVertex3i(win->len_x,win->len_y-20,0);
		glTexCoord2f(1.0f,1.0f); glVertex3i(win->len_x,0,0);
	glEnd();
	glPushMatrix();
	if(b->type==1)
		glTranslatef(15,25,0);
	else if(b->type==2)
		glTranslatef(30,15,0);
	display_book(b, b->type);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0,win->len_y-18,0);
	book_mouse_y-=(win->len_y-18);
	x=10;
	if(book_mouse_y>0 && book_mouse_y<18 && book_mouse_x>10 && book_mouse_x<(get_string_width((unsigned char*)"<-")*11.0f/12.0f)){
		glColor3f(0.95f, 0.76f, 0.52f);
		draw_string(10,-2,(unsigned char*)"<-",0);

		glColor3f(0.77f,0.59f, 0.38f);
		draw_string(win->len_x-33,-2,(unsigned char*)"->",0);
	} else if(book_mouse_y>0 && book_mouse_y<18 && book_mouse_x>win->len_x-33 && book_mouse_x<win->len_x-33+(get_string_width((unsigned char*)"->")*11.0f/12.0f)){
		glColor3f(0.95f, 0.76f, 0.52f);
		draw_string(win->len_x-33,-2,(unsigned char*)"->",0);

		glColor3f(0.77f,0.59f, 0.38f);
		draw_string(10,-2,(unsigned char*)"<-",0);
	} else {
		glColor3f(0.77f,0.59f, 0.38f);
		draw_string(10,-2,(unsigned char*)"<-",0);
		draw_string(win->len_x-33,-2,(unsigned char*)"->",0);
	}
	if(b->type==1) {
		x=50;
		p=b->active_page-5;
		if(p>=0){
			safe_snprintf(str,sizeof(str),"%d",p+1);

			if(book_mouse_y>0 && book_mouse_y<18 && book_mouse_x>x && book_mouse_x<x+(get_string_width((unsigned char*)str)*11.0f/12.0f)){
				glColor3f(0.95f, 0.76f, 0.52f);
				draw_string(x,0,(unsigned char*)str,0);
				glColor3f(0.77f,0.59f, 0.38f);
			} else
				draw_string(x,0,(unsigned char*)str,0);
		}
		x=100;
		p=b->active_page-2;
		if(p>=0){
			safe_snprintf(str,sizeof(str),"%d",p+1);

			if(book_mouse_y>0 && book_mouse_y<18 && book_mouse_x>x && book_mouse_x<x+(get_string_width((unsigned char*)str)*11.0f/12.0f)){
				glColor3f(0.95f, 0.76f, 0.52f);
				draw_string(x,0,(unsigned char*)str,0);
				glColor3f(0.77f,0.59f, 0.38f);
			} else
				draw_string(x,0,(unsigned char*)str,0);
		}
		x=win->len_x-120;
		p=b->active_page+2;
		if(p<b->no_pages){
			safe_snprintf(str,sizeof(str),"%d",p+1);

			if(book_mouse_y>0 && book_mouse_y<18 && book_mouse_x>x && book_mouse_x<x+(get_string_width((unsigned char*)str)*11.0f/12.0f)){
				glColor3f(0.95f, 0.76f, 0.52f);
				draw_string(x,0,(unsigned char*)str,0);
				glColor3f(0.77f,0.59f, 0.38f);
			} else
				draw_string(x,0,(unsigned char*)str,0);
		}
		x=win->len_x-70;
		p=b->active_page+5;
		if(p<b->no_pages){
			safe_snprintf(str,sizeof(str),"%d",p+1);

			if(book_mouse_y>0 && book_mouse_y<18 && book_mouse_x>x && book_mouse_x<x+(get_string_width((unsigned char*)str)*11.0f/12.0f)){
				glColor3f(0.95f, 0.76f, 0.52f);
				draw_string(x,0,(unsigned char*)str,0);
				glColor3f(0.77f,0.59f, 0.38f);
			} else
				draw_string(x,0,(unsigned char*)str,0);
		}
	} else if(b->type==2) {
		x=win->len_x/2-60;
		for(i=1;i<5;i++){
			p=b->active_page-i*b->type;
			if(p>=0){
				safe_snprintf(str,sizeof(str),"%d",p+1);

				if(book_mouse_y>0 && book_mouse_y<18 && book_mouse_x>x && book_mouse_x<x+(get_string_width((unsigned char*)str)*11.0f/12.0f)){
					glColor3f(0.95f, 0.76f, 0.52f);
					draw_string(x,0,(unsigned char*)str,0);
					glColor3f(0.77f,0.59f, 0.38f);
				} else
					draw_string(x,0,(unsigned char*)str,0);
			}
			x-=40;
		}
		x=win->len_x/2+50;
		for(i=1;i<5;i++){
			p=b->active_page+i*b->type;
			if(p<b->no_pages){
				safe_snprintf(str,sizeof(str),"%d",p+1);

				if(book_mouse_y>0 && book_mouse_y<18 && book_mouse_x>x && book_mouse_x<x+(get_string_width((unsigned char*)str)*11.0f/12.0f)){
					glColor3f(0.95f, 0.76f, 0.52f);
					draw_string(x,0,(unsigned char*)str,0);
					glColor3f(0.77f,0.59f, 0.38f);
				} else
					draw_string(x,0,(unsigned char*)str,0);
			}
			x+=40;
		}
	}

	if(book_mouse_y>0 && book_mouse_y<18 && book_mouse_x>win->len_x/2-15 && book_mouse_x<win->len_x/2+15)
		glColor3f(0.95f, 0.76f, 0.52f);

	draw_string(win->len_x/2-15,0,(unsigned char*)"[X]",0);
	glPopMatrix();
#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
	return 1;
}
#endif //FR_VERSION

#ifdef FR_VERSION
// On change de page
void change_page (int num_livre, int nb_pages, int positif)
{
	struct_livres *livre_actuel = recupere_livre(num_livre);
    int i = 0;

    // On avance dans les pages
    if (positif == 1)
    {
        for (i = 0 ; i < nb_pages ; i++)
        {
            if (livre_actuel->page_active+livre_actuel->type < livre_actuel->nombre_pages)
            {
                livre_actuel->page_active += livre_actuel->type;
            }
        }
    }
    // On revient sur les anciennes pages
    else
    {
        for (i = 0 ; i < nb_pages ; i++)
        {
            if (livre_actuel->page_active - livre_actuel->type >= 0)
            {
                livre_actuel->page_active -= livre_actuel->type;
            }
        }
    }
}

int clique_fenetre_livre(window_info *win, int mx, int my, Uint32 flags)
{
    struct_livres *livre_actuel=win->data;
    int nb_pages = 0;
    int positif = 0;

	my -= win->len_y;

    // On ne prends pas en compte la molette de la souris
    if ( (flags & ELW_MOUSE_BUTTON) == 0)
    {
		nb_pages = 1;
		positif = (flags & ELW_WHEEL_UP) ? 0 : 1;
    }

    // On clique dans la barre du bas
	else if (my<-2 && my>-25)
	{
		// Flèches de gauche : rapide (5 pages) ou lente (1 page)
		if ((mx > 7 && mx < 35) || (mx > 45 && mx < 75))
		{
			nb_pages = (mx > 7 && mx < 35) ? 5 : 1;
			positif = 0;
		}
		// Flèches de droite : rapide (5 pages) ou lente (1 page)
		else if ((mx > win->len_x-40 && mx < win->len_x-7) || (mx > win->len_x-80 && mx < win->len_x-50))
		{
			nb_pages = ((mx > win->len_x-40) && (mx < win->len_x-7)) ? 5 : 1;
			positif = 1;
		}

		// On a cliqué sur le X qui permet de fermer le livre
		else if (mx>win->len_x/2-15 && mx < win->len_x/2+15)
		{
			ferme_livre(livre_ouvert);
			hide_window(win->window_id);
		}
	}

	if (nb_pages != 0)
	{
		// C'est un livre serveur, on demande donc l'autorisation
		// au serveur d'afficher la suite
		if (livre_actuel->serveur)
		{
		    char str[6];
			str[0]=CONTINUE_LIVRE;
			*((Uint16*)(str+1)) = SDL_SwapLE16(livre_actuel->num);
			*((Uint16*)(str+3)) = SDL_SwapLE16(nb_pages);
			*((Uint16*)(str+5)) = SDL_SwapLE16(positif);
			my_tcp_send(my_socket, (Uint8*)str, 6);
		}
		else
		{
			change_page (livre_actuel->num, nb_pages, positif);
		}
	}
	return 1;
}
#else //FR_VERSION
int click_book_handler(window_info *win, int mx, int my, Uint32 flags)
{
	int i,x,p;
	book *b=win->data;

	// only handle mouse button clicks, not scroll wheels moves
	if ( (flags & ELW_MOUSE_BUTTON) == 0) return 0;

	my -= win->len_y;
	if(my<-2 && my>-18) {
		if(mx>10 && mx < 20){
			if(b->have_server_pages<b->server_pages) {
				if(b->active_page-b->type >= 0)
					b->active_page -= b->type;
				//We'll always have the first pages, you can't advance from 0-20 in 1 jump but must get them all.
				//TODO: Make it possible to jump that many pages.
			} else if(b->active_page-b->type >= 0) {
				b->active_page-=b->type;
			}
		} else if(mx>win->len_x-20 && mx<win->len_x-10){
			if(b->have_server_pages<b->server_pages){
				//Get a 2 new pages...
				char str[5];
				int id=b->id;
				int pages=b->have_server_pages;

				str[0]=SEND_BOOK;
				*((Uint16*)(str+1))=SDL_SwapLE16(id);
				*((Uint16*)(str+3))=SDL_SwapLE16(pages);
				my_tcp_send(my_socket, (Uint8*)str, 5);

				if(b->active_page+b->type<b->no_pages)
					b->active_page+=b->type;
				else
					b->pages_to_scroll=b->type;
			} else if(b->active_page+b->type<b->no_pages) {
				b->active_page+=b->type;
			}
		}
		if(b->type==1){
			x=50;
			p=b->active_page-5;
			if(p>=0 && mx>=x&&mx<x+25){
				b->active_page-=5;
			}
			x=100;
			p=b->active_page-2;
			if(p>=0 && mx>=x&&mx<x+25){
				b->active_page-=2;
			}
			x=140;

			if(mx>win->len_x/2-15 && mx < win->len_x/2+15) {
//				char str[5];
//				int id=b->id;

//				// Lachesis: Please either fix this branching condition or remove it.
//				if (10000 > id > 11000) {
//					str[0]=SEND_BOOK;
//					*((Uint16*)(str+1))=SDL_SwapLE16(id);
//					*((Uint16*)(str+3))=SDL_SwapLE16(0xFFFF); // Swap not actually necessary.. But it's cleaner.
//					my_tcp_send(my_socket, str, 5);
//				}

				hide_window(win->window_id);
				book_opened=-1;
			}

			x=win->len_x-120;
			p=b->active_page+2;
			if(p<b->no_pages && mx>=x && mx<x+25){
				b->active_page+=2;
			}
			x=win->len_x-70;
			p=b->active_page+5;
			if(p<b->no_pages && mx>=x && mx<x+25){
				b->active_page+=5;
			}
		} else if(b->type==2){
			x=win->len_x/2-60;
			for(i=1;i<5;i++){
				p=b->active_page-i*b->type;
				if(mx>=x && mx<x+25 && p>=0){
					b->active_page-=i*b->type;
				}
				x-=40;
			}

			if(mx>win->len_x/2-15 && mx < win->len_x/2+15) {
//				char str[5];
//				int id=b->id;

//				// Lachesis: Please either fix this branching condition or remove it.
//				if (10000 > id > 11000) {
//					str[0]=SEND_BOOK;
//					*((Uint16*)(str+1))=SDL_SwapLE16(id);
//					*((Uint16*)(str+3))=SDL_SwapLE16(0xFFFF);
//					my_tcp_send(my_socket, str, 5);
//				}

				hide_window(win->window_id);
				book_opened=-1;
			}

			x=win->len_x/2+50;
			for(i=1;i<5;i++){
				p=b->active_page+i*b->type;
				if(mx>=x && mx<x+25 && p<b->no_pages){
					b->active_page+=i*b->type;
				}
				x+=40;
			}
		}
	}
	return 1;
}
#endif //FR_VERSION

#ifdef FR_VERSION
int souris_fenetre_livre (window_info * win, int mx, int my)
{
	souris_livre_x = mx;
	souris_livre_y = my;

	return 0;
}
#else //FR_VERSION
int mouseover_book_handler(window_info * win, int mx, int my)
{
	//Save for later
	book_mouse_x=mx;
	book_mouse_y=my;

	return 0;
}
#endif //FR_VERSION


#ifdef FR_VERSION
void affiche_livre(struct_livres *livre_actuel)
{
	int *fenetre;

	if(!livre_actuel)
    {
#ifdef FR_DEBUG_LIVRES
        LOG_ERROR("Flag FR_LIVRES : le livre n'existe pas, bien qu'on veuille l'afficher\n");
#endif //FR_DEBUG_LIVRES
		return;
    }

	if(livre_actuel->type==1)
    {
		fenetre = &paper_win;
		if(book_win!=-1)
        {
			hide_window(book_win);
        }
	}
    else
    {
		fenetre = &book_win;
		if(paper_win!=-1)
        {
			hide_window(paper_win);
        }
	}

	livre_ouvert = livre_actuel->num;

	if(*fenetre < 0)
    {
        // Une seule page
		if(livre_actuel->type == 1)
        {
#ifdef FR_DEBUG_LIVRES
        LOG_ERROR("Flag FR_LIVRES : creation d'une fenêtre pour un livre d'une page\n");
#endif //FR_DEBUG_LIVRES
			// large comme 1 page et haute comme 1 page + 20 pixels pour les boutons de navigation
			*fenetre = create_window(livre_actuel->titre, (windows_on_top)?-1:game_root_win, 0, book_win_x, book_win_y, page_largeur, page_hauteur+20, ELW_WIN_DEFAULT^ELW_CLOSE_BOX);
        }
        // Deux pages
		else if(livre_actuel->type == 2)
        {
#ifdef FR_DEBUG_LIVRES
        LOG_ERROR("Flag FR_LIVRES : creation d'une fenêtre pour un livre de 2 pages\n");
#endif //FR_DEBUG_LIVRES
			// large comme 2 pages et haute comme 1 page + 20 pixels pour les boutons de navigation
			*fenetre = create_window(livre_actuel->titre, (windows_on_top)?-1:game_root_win, 0, book_win_x, book_win_y, page_largeur*2, page_hauteur+20, ELW_WIN_DEFAULT^ELW_CLOSE_BOX);
        }
#ifdef FR_DEBUG_LIVRES
        LOG_ERROR("Flag FR_LIVRES : creation de la fenêtre réussit\n");
#endif //FR_DEBUG_LIVRES

		set_window_handler(*fenetre, ELW_HANDLER_DISPLAY, &affiche_fenetre_livre);
		set_window_handler(*fenetre, ELW_HANDLER_MOUSEOVER, &souris_fenetre_livre);
		set_window_handler(*fenetre, ELW_HANDLER_CLICK, &clique_fenetre_livre);
		windows_list.window[*fenetre].data = livre_actuel;
#ifdef FR_DEBUG_LIVRES
        LOG_ERROR("Flag FR_LIVRES : creation de la fenêtre entièrement réussit\n");
#endif //FR_DEBUG_LIVRES
	}
    else
    {
		if ((point)windows_list.window[*fenetre].data != (point)livre_actuel)
        {
			safe_snprintf(windows_list.window[*fenetre].window_name, sizeof(windows_list.window[*fenetre].window_name), livre_actuel->titre);
			windows_list.window[*fenetre].data = livre_actuel;
			if(!get_show_window(*fenetre))
            {
				show_window(*fenetre);
            }
			select_window(*fenetre);
		}
        else if (!get_show_window(*fenetre))
        {
			show_window(*fenetre);
			select_window(*fenetre);
		}
	}
}
#else //FR_VERSION
void display_book_window(book *b)
{
	int *p;

	if(!b)
		return;

	if(b->type==1){
		p=&paper_win;
		if(book_win!=-1)
			hide_window(book_win);
	} else {
		p=&book_win;
		if(paper_win!=-1)
			hide_window(paper_win);
	}
	book_opened = b->id;
	if(*p<0){
		if(b->type==1)
			*p=create_window(b->title, -1, 0, book_win_x, book_win_y, 320, 400, ELW_WIN_DEFAULT^ELW_CLOSE_BOX);
		else if(b->type==2)
			*p=create_window(b->title, -1, 0, book_win_x, book_win_y, 528, 320, ELW_WIN_DEFAULT^ELW_CLOSE_BOX); //width/height are different

		set_window_handler(*p, ELW_HANDLER_DISPLAY, &display_book_handler);
		set_window_handler(*p, ELW_HANDLER_MOUSEOVER, &mouseover_book_handler);
		set_window_handler(*p, ELW_HANDLER_CLICK, &click_book_handler);
		windows_list.window[*p].data=b;
	} else {
		if((point)windows_list.window[*p].data!=(point)b) {
			safe_snprintf(windows_list.window[*p].window_name, sizeof(windows_list.window[*p].window_name), "%s", b->title);
			windows_list.window[*p].data=b;
			if(!get_show_window(*p))
				show_window(*p);
			select_window(*p);
		} else if(!get_show_window(*p)) {
			show_window(*p);
			select_window(*p);
		}
	}
}
#endif //FR_VERSION

#ifdef FR_VERSION
void libere_memoire_page(struct_pages *page)
{
    free(page->lignes);
    free(page->couleur_texte);
}

void libere_memoire_livre(struct_livres *livre_actuel)
{
    int i;
    struct_pages **page;

    page = livre_actuel->pages;

    for (i = 0; i < livre_actuel->nombre_pages; i++, page++)
    {
        libere_memoire_page(*page);
    }
    free(livre_actuel->pages);
    free(livre_actuel);
}

void libere_memoire_livres()
{
    struct_livres *livre_libere, *livre_actuel = NULL;
    livre_libere = liste_livres;
    for (livre_actuel = liste_livres ; livre_actuel ; livre_libere = livre_actuel)
    {
        livre_actuel = livre_actuel->suivant;
        libere_memoire_livre(livre_libere);
    }
    liste_livres = NULL;
    page_hauteur = 0;
    page_largeur = 0;
}

int ferme_livre(int num_livre)
{
	struct_livres *livre_actuel = recupere_livre(num_livre);
	char str[5];

	if(!livre_actuel)
    {
		livre_ouvert = -1;
		return 0;
    }
	if(book_win!=-1)
    {
		if((point)windows_list.window[book_win].data==(point)livre_actuel)
        {
			hide_window(book_win);
		}
	}
	if(paper_win!=-1)
    {
		if((point)windows_list.window[paper_win].data == (point)livre_actuel)
        {
			hide_window(paper_win);
		}
	}

	str[0]=CLOSE_BOOK_FROM_CLIENT;
	my_tcp_send(my_socket, (Uint8*)str, 1);

	livre_ouvert = -1;
    return 1;
}
#else //FR_VERSION
void close_book(int book_id)
{
	book *b=get_book(book_id);

	if(!b)
		return;
	if(book_win!=-1) {
		if((point)windows_list.window[book_win].data==(point)b) {
			hide_window(book_win);
		}
	}
	if(paper_win!=-1) {
		if((point)windows_list.window[paper_win].data == (point)b) {
			hide_window(paper_win);
		}
	}

	book_opened=-1;
}
#endif //FR_VERSION

#ifdef FR_VERSION
int affiche()
{
    char chemin[256];
    static int texture_image = -1;

    safe_snprintf(chemin, sizeof(chemin),"./textures/%s",nom_fichier);
#ifdef	NEW_TEXTURES
    texture_image  = load_texture_cached (chemin, tt_image);
	bind_texture(texture_image);
#else	/* NEW_TEXTURES */
    texture_image  = load_texture_cache_deferred (chemin, 0);
	get_and_set_texture_id(texture_image);
#endif	/* NEW_TEXTURES */

#ifdef OPENGL_TRACE
    CHECK_GL_ERRORS();
#endif //OPENGL_TRACE

    glBegin(GL_QUADS);
    draw_2d_thing (0.0f, 0.0f, 1.0f, 1.0f, 0, 256, 256, 0);
    glEnd();

#ifdef OPENGL_TRACE
    CHECK_GL_ERRORS();
#endif //OPENGL_TRACE

    return 1;
}

int ferme_image()
{
    fenetre_image = -1;
    return 1;
}

int affiche_image(char* texte)
{

    safe_strncpy(nom_fichier, texte, sizeof(nom_fichier));

    if(fenetre_image < 0)
    {
        fenetre_image = create_window((char*)"Image",  -1, 0, 50, 50, 256+20, 256, ELW_WIN_DEFAULT);
        set_window_handler (fenetre_image, ELW_HANDLER_DISPLAY, &affiche);
        set_window_handler (fenetre_image, ELW_HANDLER_CLOSE, &ferme_image);
    }
    else
    {
        show_window(fenetre_image);
        select_window(fenetre_image);
    }

    return 1;
}

#endif //FR_VERSION

#ifdef ENGLISH
void free_books()
{
	book *b,*l=NULL;
	for(b=books;b;l=b){
		b=b->next;
		if (l)
		free_book(l);
	}
	if(books)
		free_book(books);
}
#endif //ENGLISH

/* currently UNUSED
int have_book(int id)
{
	book *b=books;
	if(b){
		for(;b->next;b=b->next){
			if(b->id==id) return 1;
		}
	}
	return 0;
}
*/
