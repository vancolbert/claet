#include <stdlib.h>
#include <string.h>
#ifdef WINDOWS
	#include <io.h>
#else //!WINDOWS
	#include <glob.h>
	#include <unistd.h>
#endif //WINDOWS
#include "font.h"
#include "asc.h"
#include "chat.h"
#include "client_serv.h"
#include "colors.h"
#include "elconfig.h"
#include "errors.h"
#include "init.h"
#include "gl_init.h"
#include "global.h"
#include "interface.h"
#include "misc.h"
#include "textures.h"
#ifndef SKY_FPV_OPTIONAL
#include "draw_scene.h"
#endif // SKY_FPV_OPTIONAL
#ifdef FR_VERSION
#include "themes.h"
#endif //FR_VERSION

#ifdef FR_VERSION
/* ========================================================================== *
 *                           CONSTANTS AND VARIABLES                          *
 * ========================================================================== */

// Disposition des caractères sur les planches de fontes

#define FONT_CHARS_PER_LINE  16  // nombre de colonnes
#define FONT_CHARS_MAX_LINE  12  // nombre de lignes
#define FONT_X_SPACING       32  // largeur des "cases"
#define FONT_Y_SPACING       42  // hauteur des "cases"
#define FONTS_ARRAY_SIZE     20  // nombre max de fontes

static const char texture_dir[] = "fontes/";

static int font_text = 0;  // id de la texture en cours
int cur_font_num = 0;      // id de la fonte en cours


// Table mémorisant l'UV-Mapping des caractères sur les planches de fontes

typedef struct {
	float u_start;
	float u_end;
	float v_start;
	float v_end;
} char_uv;

char_uv *font_char_uv[FONT_CHARS_PER_LINE * FONT_CHARS_MAX_LINE];


// Liste des fontes disponibles

typedef struct {
	char name[28]; // pas plus long pour rentrer dans les boutons
	char texture_file[32];
	int texture_id;
	float adjust;
	int spacing;
	int baseline;
	int widths[FONT_CHARS_MAX_LINE * FONT_CHARS_PER_LINE];
} font_info;

font_info *fonts[FONTS_ARRAY_SIZE];


int chat_font = 0;
int name_font = 0;
int book_font = 0;


/* ========================================================================== *
 *  Fonctions de conversion d'un caractère en numéro ou en couleur            *
 * ========================================================================== */

// Retourne le numéro d'un caractère (ou -1 si non affichable)
int get_font_char(unsigned char cur_char)
{
	// caractère non affichable
	if (cur_char < 32) return -1;
	// code couleur non affichable
	if (is_color(cur_char)) return -1;
	// caractère ascii standart
	if (cur_char < 128) return ((int)cur_char - 32);
	// caractère ascii étendu
	return ((int)cur_char - 64);
}


// Applique une couleur si le caractère correspond, sinon retourne faux
int apply_color(unsigned char cur_char)
{
	if (is_color(cur_char))
	{
		float r, g, b;
		int color = from_color_char(cur_char);
		r = (float) colors_list[color].r1 / 255.0f;
		g = (float) colors_list[color].g1 / 255.0f;
		b = (float) colors_list[color].b1 / 255.0f;
		//This fixes missing letters in the font on some clients
		//No idea why going from 3f to 4f helps, but it does
		glColor4f(r, g, b, 1.0);
		return 1;
	}
	return 0;
}

// Convertit un caractère en numéro pour la texture (ou applique la couleur)
int find_font_char(unsigned char cur_char)
{
	if (apply_color(cur_char)) return -1;
	return get_font_char(cur_char);
}


/* ========================================================================== *
 *  Fonctions retournant la largeur d'un caractère ou d'une chaine            *
 * ========================================================================== */

// Retourne la largeur correspondant au numéro de caractère (spacing compris)
int get_font_width(int chr)
{
    // ignore unknown characters
	if (chr < 0) return 0;
	// return width of character + spacing between chars (supports variable width fonts)
	return fonts[cur_font_num]->widths[chr] * fonts[cur_font_num]->adjust + fonts[cur_font_num]->spacing;
}

// Retourne la largeur correspondant au caractère (spacing compris)
int get_char_width(unsigned char cur_char)
{
	return get_font_width(get_font_char(cur_char));
}

// Retourne la largeur d'une chaine (longueur fournie)
int get_nstring_width(const unsigned char *str, int len)
{
	int i, wdt = 0;
	for (i=0; i<len; i++) wdt+= get_char_width(str[i]);
	wdt-= fonts[cur_font_num]->spacing; // adjust to ignore the final spacing
	return wdt;
}

// Retourne la largeur d'une chaine
int get_string_width(const unsigned char *str)
{
	return get_nstring_width(str, strlen((char*)str));
}


/* ========================================================================== *
 *  Fonction d'affichage d'un caractère (aux position et taille spécifiées)   *
 * ========================================================================== */

float X_RATIO = 0.5f * (FONT_X_SPACING-1) / DEFAULT_FONT_X_LEN;
float Y_RATIO = 0.5f * (FONT_Y_SPACING-1) / (DEFAULT_FONT_Y_LEN + 1.0f);

// Affichage 2D en retournant le décalage pour positionner le caractère suivant
int draw_char_scaled(unsigned char cur_char, int cur_x, int cur_y, float displayed_font_x_size, float displayed_font_y_size)
{
	int displayed_font_x_width;

	// watch for illegal/non-display characters
	int chr = find_font_char(cur_char);
	if (chr < 0) return 0;

	// récupère la largeur et calcul les dimensions à appliquer
	displayed_font_x_width = round(((float)get_font_width(chr)) * displayed_font_x_size / 12.0f);
	displayed_font_x_size = floor(displayed_font_x_size * fonts[cur_font_num]->adjust * X_RATIO);
	displayed_font_y_size = floor(displayed_font_y_size * fonts[cur_font_num]->adjust * Y_RATIO) + 1;
	cur_x-= floor((displayed_font_x_size - displayed_font_x_width) / 2);
	cur_y+= floor(fonts[cur_font_num]->baseline * displayed_font_y_size / FONT_Y_SPACING);

	// place the text from the graphics on the map
	glTexCoord2f(font_char_uv[chr]->u_start, font_char_uv[chr]->v_start);
	glVertex3i(cur_x, cur_y, 0);
	glTexCoord2f(font_char_uv[chr]->u_start, font_char_uv[chr]->v_end);
	glVertex3i(cur_x, cur_y+displayed_font_y_size, 0);
	glTexCoord2f(font_char_uv[chr]->u_end,   font_char_uv[chr]->v_end);
	glVertex3i(cur_x+displayed_font_x_size, cur_y+displayed_font_y_size, 0);
	glTexCoord2f(font_char_uv[chr]->u_end,   font_char_uv[chr]->v_start);
	glVertex3i(cur_x+displayed_font_x_size, cur_y, 0);

	// return how far to move for the next character
	return displayed_font_x_width;
}


// Affichage 3D (ortho) en retournant le décalage pour positionner le caractère suivant
float draw_ortho_char_scaled(unsigned char cur_char, float cur_x, float cur_y, float z, float displayed_font_x_size, float displayed_font_y_size)
{
	float displayed_font_x_width;

	// watch for illegal/non-display characters
	int chr = get_font_char(cur_char);
	if (chr < 0) return 0;

	// récupère la largeur et calcul les dimensions à appliquer
	displayed_font_x_width = ((float)get_font_width(chr)) * displayed_font_x_size / 12.0f;
	displayed_font_x_size = displayed_font_x_size * fonts[cur_font_num]->adjust * X_RATIO;
	displayed_font_y_size = displayed_font_y_size * fonts[cur_font_num]->adjust * Y_RATIO;
	cur_x-= (displayed_font_x_size - displayed_font_x_width) * 0.5f;
	cur_y+= fonts[cur_font_num]->baseline * displayed_font_y_size / FONT_Y_SPACING;

	// place the text from the graphics on the map
	glTexCoord2f(font_char_uv[chr]->u_start, font_char_uv[chr]->v_start);
	glVertex3f(cur_x, cur_y+displayed_font_y_size, z);
	glTexCoord2f(font_char_uv[chr]->u_start, font_char_uv[chr]->v_end);
	glVertex3f(cur_x, cur_y, z);
	glTexCoord2f(font_char_uv[chr]->u_end,   font_char_uv[chr]->v_end);
	glVertex3f(cur_x+displayed_font_x_size, cur_y, z);
	glTexCoord2f(font_char_uv[chr]->u_end,   font_char_uv[chr]->v_start);
	glVertex3f(cur_x+displayed_font_x_size, cur_y+displayed_font_y_size, z);

	// return how far to move for the next character
	return displayed_font_x_width;
}


// Affichage 3D (non ortho) en retournant le décalage pour positionner le caractère suivant
float draw_ingame_char_scaled(unsigned char cur_char, float cur_x, float cur_y, float displayed_font_x_size, float displayed_font_y_size)
{
	float displayed_font_x_width;

	// watch for illegal/non-display characters
	int chr = get_font_char(cur_char);
	if (chr < 0) return 0;

	// récupère la largeur et calcul les dimensions à appliquer
	displayed_font_x_width = ((float)get_font_width(chr)) * displayed_font_x_size / 12.0f;
	displayed_font_x_size = displayed_font_x_size * fonts[cur_font_num]->adjust * X_RATIO;
	displayed_font_y_size = displayed_font_y_size * fonts[cur_font_num]->adjust * Y_RATIO;
	cur_x-= (displayed_font_x_size - displayed_font_x_width) * 0.5f;
	cur_y+= fonts[cur_font_num]->baseline * displayed_font_y_size / FONT_Y_SPACING;

	// place the text from the graphics on the map
	glTexCoord2f(font_char_uv[chr]->u_start, font_char_uv[chr]->v_start);
	glVertex3f(cur_x, 0, cur_y+displayed_font_y_size);
	glTexCoord2f(font_char_uv[chr]->u_start, font_char_uv[chr]->v_end);
	glVertex3f(cur_x, 0, cur_y);
	glTexCoord2f(font_char_uv[chr]->u_end,   font_char_uv[chr]->v_end);
	glVertex3f(cur_x+displayed_font_x_size, 0, cur_y);
	glTexCoord2f(font_char_uv[chr]->u_end,   font_char_uv[chr]->v_start);
	glVertex3f(cur_x+displayed_font_x_size, 0, cur_y+displayed_font_y_size);

	// return how far to move for the next character
	return displayed_font_x_width;
}


/* ========================================================================== *
 *  Fonctions d'affichage d'un texte (2D)                                     *
 * ========================================================================== */

// Fonction principale avec position, texte, largeur max, nb lignes max, dimension des caractères
int draw_string_scaled(int x, int y, const unsigned char * our_string, int max_width, int max_lines, float displayed_font_x_size, float displayed_font_y_size)
{
	unsigned char cur_char;
	int cur_x, cur_y;
	int current_lines = 1;
	int i = 0;

#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
	//enable alpha filtering, so we have some alpha key
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// ancien seuil alpha inutile mais conservé à cause d'un bug des cartes ATI
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.0f);
#ifdef	NEW_TEXTURES
	bind_texture(font_text);
#else //NEW_TEXTURES
	get_and_set_texture_id(font_text);
#endif //NEW_TEXTURES

	cur_x = x;
	cur_y = y;
	glBegin(GL_QUADS);
	while (1)
	{
		cur_char = our_string[i++];
		if (! cur_char) break;

		// passe à une nouvelle ligne (retour chariot ou largeur max)
		if ((cur_char == '\n') || (cur_char == '\r')
		 || (cur_x - x + displayed_font_x_size >= max_width))
		{
			//TODO: si on traite le \n n'est-ce pas mieux d'ignorer le \r ?
			if (current_lines < max_lines)
			{
				current_lines++;
				cur_y+= displayed_font_y_size;
				cur_x = x;
				continue;
			}

			// texte tronqué : ajoute de points de suspension (code 173)
			if (our_string[i])
			{
				if ((i>1) && (our_string[i-2]==' '))
				{
					// on recule si le caractère précédent était un espace
					cur_x-= (int)(0.5f + get_font_width(32) * displayed_font_x_size/12.0f);
				}
				draw_char_scaled(173, cur_x, cur_y, displayed_font_x_size, displayed_font_y_size);
			}
			break;
		}

		// affichage du caractère
		cur_x+= draw_char_scaled(cur_char, cur_x, cur_y, displayed_font_x_size, displayed_font_y_size);
	}

	glEnd();
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
	return current_lines;
}

/* -------------------------------------------------------------------------- */

// Affichage avec niveau de zoom + largeur max et nb lignes max
int draw_string_zoomed_width(int x, int y, const unsigned char * our_string, int max_width, int max_lines, float text_zoom)
{
	return draw_string_scaled(x, y, our_string, max_width, max_lines, DEFAULT_FONT_X_LEN*text_zoom, DEFAULT_FONT_Y_LEN*text_zoom);
}

// Affichage avec niveau de zoom + nb lignes max (sans largeur max)
int draw_string_zoomed(int x, int y, const unsigned char * our_string, int max_lines, float text_zoom)
{
	return draw_string_scaled(x, y, our_string, window_width, max_lines, DEFAULT_FONT_X_LEN*text_zoom, DEFAULT_FONT_Y_LEN*text_zoom);
}


// Affichage avec taille standard "DEFAULT" + largeur max et nb lignes max
int draw_string_width(int x, int y, const unsigned char * our_string, int max_width, int max_lines)
{
	return draw_string_scaled(x, y, our_string, max_width, max_lines, DEFAULT_FONT_X_LEN, DEFAULT_FONT_Y_LEN);
}

// Affichage avec taille standard "DEFAULT" + nb lignes max (sans largeur max)
int draw_string(int x, int y, const unsigned char * our_string, int max_lines)
{
	return draw_string_scaled(x, y, our_string, window_width, max_lines, DEFAULT_FONT_X_LEN, DEFAULT_FONT_Y_LEN);
}

// Affichage avec taille standard "SMALL" (sans largeur max)
int draw_string_small(int x, int y, const unsigned char * our_string, int max_lines)
{
	return draw_string_scaled(x, y, our_string, window_width, max_lines, SMALL_FONT_X_LEN, SMALL_FONT_Y_LEN);
}

// Affichage avec taille standard "LEGENDE" (sans largeur max)
int draw_string_legende(int x, int y, const unsigned char * our_string, int max_lines)
{
	return draw_string_scaled(x, y, our_string, window_width, max_lines, LEGENDE_FONT_X_LEN, LEGENDE_FONT_Y_LEN);
}


int draw_string_scaled_shadowed(int x, int y, const unsigned char * our_string, int max_lines, float displayed_font_x_size, float displayed_font_y_size, float fr,float fg,float fb, float br,float bg,float bb)
{
	int px, py;
	glColor3f(br, bg, bb); //set shadow colour
	for (px=-1; px<=1; px+=2) for (py=-1; py<=1; py+=2) draw_string_scaled(x+px, y+py, our_string, window_width, max_lines, displayed_font_x_size, displayed_font_y_size);
	glColor3f(fr, fg, fb); //set foreground colour
	return draw_string_scaled(x, y, our_string, window_width, max_lines, displayed_font_x_size, displayed_font_y_size);
}

// Affichage avec taille standart "DEFAULT" avec contour + nb lignes max (avec couleurs, sans largeur max)
int draw_string_shadowed(int x, int y, const unsigned char * our_string, int max_lines, float fr,float fg,float fb, float br,float bg,float bb)
{
	return draw_string_scaled_shadowed(x, y, our_string, max_lines, DEFAULT_FONT_X_LEN, DEFAULT_FONT_Y_LEN, fr, fg, fb, br, bg, bb);
}

// Affichage avec taille standart "SMALL" avec contour + nb lignes max (avec couleurs, sans largeur max)
int draw_string_small_shadowed(int x, int y,const unsigned char * our_string,int max_lines, float fr, float fg, float fb, float br, float bg, float bb)
{
	return draw_string_scaled_shadowed(x, y, our_string, max_lines, SMALL_FONT_X_LEN, SMALL_FONT_Y_LEN, fr, fg, fb, br, bg, bb);
}

// Affichage avec taille adaptée à la largeur max + contour + nb lignes max!? (avec couleurs)
int draw_string_shadowed_width(int x, int y, const unsigned char * our_string, int max_width, int max_lines, float fr,float fg,float fb, float br,float bg,float bb)
{
	float zoom = ((float)max_width*12.0f)/((float)get_string_width(our_string)*DEFAULT_FONT_X_LEN);
	return draw_string_scaled_shadowed(x, y, our_string, max_lines, DEFAULT_FONT_X_LEN*zoom, DEFAULT_FONT_Y_LEN*zoom, fr, fg, fb, br, bg, bb);
}

int draw_string_zoomed_shadowed(int x, int y, const unsigned char * our_string, int max_lines, float text_zoom, float fr,float fg,float fb, float br,float bg,float bb)
{
	float displayed_font_x_size = DEFAULT_FONT_X_LEN * text_zoom;
	float displayed_font_y_size = DEFAULT_FONT_Y_LEN * text_zoom;
//	return draw_string_scaled_shadowed(x, y, our_string, max_lines, displayed_font_x_size, displayed_font_y_size, fr, fg, fb, br, bg, bb);
	glColor3f(br, bg, bb); //set shadow colour
	draw_string_scaled(x+2, y,   our_string, window_width, max_lines, displayed_font_x_size, displayed_font_y_size);
	draw_string_scaled(x+1, y+1, our_string, window_width, max_lines, displayed_font_x_size, displayed_font_y_size);
	draw_string_scaled(x,   y+2, our_string, window_width, max_lines, displayed_font_x_size, displayed_font_y_size);
	glColor3f(fr, fg, fb); //set foreground colour
	return draw_string_scaled(x, y, our_string, window_width, max_lines, displayed_font_x_size, displayed_font_y_size);
}


/* -------------------------------------------------------------------------- */

// Affichage d'un texte contenu dans une box (tronqué) avec zoom et curseur
void draw_string_zoomed_clipped (int x, int y, const unsigned char* our_string, int cursor_pos, int width, int height, float text_zoom)
{
	float displayed_font_x_size = DEFAULT_FONT_X_LEN * text_zoom;
	float displayed_font_y_size = DEFAULT_FONT_Y_LEN * text_zoom;

	unsigned char cur_char;
	int cur_x, cur_y;
	int cursor_x = x-1;
	int cursor_y = y-1;
	int i = 0;

	if (width < displayed_font_x_size || height < displayed_font_y_size) return;

	// enable alpha filtering, so we have some alpha key
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//	glEnable(GL_ALPHA_TEST);
//	glAlphaFunc(GL_GREATER, 0.1f);
#ifdef	NEW_TEXTURES
	bind_texture(font_text);
#else //NEW_TEXTURES
	get_and_set_texture_id(font_text);
#endif //NEW_TEXTURES

	cur_x = x;
	cur_y = y;
	glBegin(GL_QUADS);
	while (1)
	{
		// récupère la position du curseur
		if (i == cursor_pos) {
			cursor_x = cur_x;
			cursor_y = cur_y;
		}

		cur_char = our_string[i++];
		if (! cur_char) break;

		// passe à une nouvelle ligne (retour chariot)
		if (cur_char == '\n' || cur_char == '\r')
		{
			cur_y+= displayed_font_y_size;
			cur_x = x;
			// en cas de dépassement de la hauteur : fin de l'affichage
			if (cur_y - y > height - displayed_font_y_size) break;
			continue;
		}

		// affichage du caractère
		cur_x+= draw_char_scaled(cur_char, cur_x, cur_y, displayed_font_x_size, displayed_font_y_size);

		// en cas de dépassement de la largeur : on saute la fin de la ligne
		if (cur_x - x > width - displayed_font_x_size)
		{
			while (our_string[i] != '\0' && our_string[i] != '\n' && our_string[i] != '\r') i++;
		}
	}

	if (cursor_x >= x && cursor_y >= y && cursor_y - y <= height - displayed_font_y_size)
	{
		draw_char_scaled('_', cursor_x, cursor_y, displayed_font_x_size, displayed_font_y_size);
	}

	glEnd();
	glDisable(GL_BLEND);
//	glDisable(GL_ALPHA_TEST);
#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
}

// Affichage d'un texte contenu dans une box (tronqué) sans zoom ni curseur
void draw_string_clipped(int x, int y, const unsigned char * our_string, int width, int height)
{
	draw_string_zoomed_clipped(x, y, our_string, -1, width, height, 1.0f);
}


/* ========================================================================== *
 *  Fonctions d'affichage d'un texte (3D)                                     *
 * ========================================================================== */

#ifdef	ELC
#ifndef MAP_EDITOR2

// Fonction principale avec position, texte, nb lignes max, dimension des caractères
void draw_ortho_ingame_scaled(float x, float y, float z, const unsigned char * our_string,
	              int max_lines, float displayed_font_x_size, float displayed_font_y_size)
{
	unsigned char cur_char;
	float cur_x, cur_y;
	int current_lines = 1;
	int i = 0;

	//enable alpha filtering, so we have some alpha key
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.1f);
#ifdef	NEW_TEXTURES
	bind_texture(font_text);
#else //NEW_TEXTURES
	get_and_set_texture_id(font_text);
#endif //NEW_TEXTURES

	cur_x = x;
	cur_y = y;
	glBegin(GL_QUADS);
	while (1)
	{
		cur_char = our_string[i++];
		if (! cur_char) break;

		// application d'un code couleur (NV bug fix!)
		if (is_color(cur_char))
		{
			glEnd();
			apply_color(cur_char);
			glBegin(GL_QUADS);
			continue;
		}

		// passe à une nouvelle ligne (retour chariot)
		if (cur_char == '\n')
		{
			if (++current_lines > max_lines) break;
			cur_y+= displayed_font_y_size;
			cur_x = x;
			continue;
		}

		// affichage du caractère
		cur_x+= draw_ortho_char_scaled(cur_char, cur_x, cur_y, z, displayed_font_x_size, displayed_font_y_size);
	}

	glEnd();
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
}

// Affichage avec mise à échelle (sur name_zoom + zoom_level sur SMALL) + nb lignes max
void draw_ortho_ingame_string_small(float x, float y, float z, const unsigned char * our_string, int max_lines)
{
	float displayed_font_x_size = SMALL_INGAME_FONT_X_LEN * zoom_level * name_zoom / 3.0f;
	float displayed_font_y_size = SMALL_INGAME_FONT_Y_LEN * zoom_level * name_zoom / 3.0f;
	draw_ortho_ingame_scaled(x, y, z, our_string, max_lines, displayed_font_x_size, displayed_font_y_size);
}

// Affichage avec mise à échelle (sur name_zoom) + nb lignes max
void draw_ortho_ingame_string(float x, float y, float z, const unsigned char * our_string,
						int max_lines, float font_x_scale, float font_y_scale)
{
	float displayed_font_x_size = font_x_scale * name_zoom * 12.0f;
	float displayed_font_y_size = font_y_scale * name_zoom * 12.0f;
	draw_ortho_ingame_scaled(x, y, z, our_string, max_lines, displayed_font_x_size, displayed_font_y_size);
}

// Affichage avec mise à échelle (en spécifiant le zoom) + nb lignes max
void draw_ortho_ingame_string_zoomed(float x, float y, float z, const unsigned char * our_string,
						int max_lines, float font_x_scale, float font_y_scale, float zoom)
{
	float displayed_font_x_size = font_x_scale * zoom * 12.0f;
	float displayed_font_y_size = font_y_scale * zoom * 12.0f;
	draw_ortho_ingame_scaled(x, y, z, our_string, max_lines, displayed_font_x_size, displayed_font_y_size);
}

/* -------------------------------------------------------------------------- */

// Affichage utilisé pour les bulles des messages locaux
void draw_ingame_string(float x, float y, const unsigned char * our_string,
						int max_lines, float font_x_scale, float font_y_scale)
{
#ifdef SKY_FPV_OPTIONAL
	float displayed_font_x_size = font_x_scale * chat_zoom * 12.0f * font_scale;
	float displayed_font_y_size = font_y_scale * chat_zoom * 12.0f * font_scale;

	double model[16], proj[16], hx,hy,hz;
	int view[4];

	glGetDoublev(GL_MODELVIEW_MATRIX, model);
	glGetDoublev(GL_PROJECTION_MATRIX, proj);
	glGetIntegerv(GL_VIEWPORT, view);
	gluProject((double)x, 0.0f, (double)y, model, proj, view, &x, &y, &hz);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(view[0], view[2]+view[0], view[1], view[3]+view[1], 0.0f, -1.0f);

	draw_ortho_ingame_scaled(x, y, 0, our_string, max_lines, displayed_font_x_size, displayed_font_y_size);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
#else //SKY_FPV_OPTIONAL
	float displayed_font_x_size = font_x_scale * zoom_level * chat_zoom / 3.0f;
	float displayed_font_y_size = font_y_scale * zoom_level * chat_zoom / 3.0f;

	unsigned char cur_char;
	float cur_x, cur_y;
	int current_lines = 1;
	int i = 0;

	//enable alpha filtering, so we have some alpha key
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.1f);
#ifdef	NEW_TEXTURES
	bind_texture(font_text);
#else //NEW_TEXTURES
	get_and_set_texture_id(font_text);
#endif //NEW_TEXTURES

	cur_x = x;
	cur_y = y;
	glBegin(GL_QUADS);
	while (1)
	{
		cur_char = our_string[i++];
		if (! cur_char) break;

		// couleur ignorée (on garde le texte noir dans les bulles)
		if (is_color(cur_char)) continue;

		// passe à une nouvelle ligne (retour chariot)
		if (cur_char == '\n')
		{
			if (++current_lines > max_lines) break;
			cur_y+= displayed_font_y_size;
			cur_x = x;
			continue;
		}

		// affichage du caractère
		cur_x+= draw_ingame_char_scaled(cur_char, cur_x, cur_y, displayed_font_x_size, displayed_font_y_size);
	}

	glEnd();
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
#endif //SKY_FPV_OPTIONAL
}

#endif //!MAP_EDITOR_2
#endif //ELC


/* ========================================================================== *
 *                                                                            *
 * ========================================================================== */

int reset_soft_breaks (char *str, int len, int size, float zoom, int width, int *cursor, float *max_line_width)
{
	char *buf;
	int ibuf;
	int nchar;
	int font_bit_width;
	int nlines;
	float line_width;
	int isrc, idst;
	int lastline;
	int dcursor = 0;
	/* the generic special text window code needs to know the
	   maximum line length in pixels.  This information is used
	   but was previously throw away in this function.  Others
	   may fine it useful for setting the winow size, so pass back
	   to the caller if they provide somewhere to store it. */
	float local_max_line_width = 0;

	// error checking
	if (str == NULL || width <= 0 || size <= 0) {
		return 0;
	}

	/* strip existing soft breaks before we start,
		to avoid complicated code later */
	for (isrc=0, idst=0; isrc<len; isrc++)
	{
		if (str[isrc] == '\r')
		{
			/* move the cursor back if after this point */
			if ((cursor != NULL) && (isrc < *cursor))
				dcursor--;
		}
		else
			str[idst++] = str[isrc];
	}
	len = idst;
	str[len] = 0;

	/* allocate the working buffer so it can hold the maximum
	   the source string can take.  Previously, the fixed length
	   buffer was sometimes not big enough.  The code looked
	   to attempt to cope but was floored.  When ever the wrap
	   caused more characters to be in the output, some of the
	   source would be lost.  This is still possable if the source
	   size cannot take the extra characters.  For example, try
	   #glinfo and watch as the end characters are lost.  At least
	   characters are no longer lost wrap process. If you make
	   size large enough for no character will be lost.  Twice
	   the actual string length is probably enough */
	buf = (char *)calloc(size, sizeof(char));

	nlines = 1;
	isrc = ibuf = idst = 0;
	line_width = 0;
	lastline = 0;

	// fill the buffer
	while (isrc < len && str[isrc] != '\0')
	{
		// see if it's an explicit line break
		if (str[isrc] == '\n') {
			nlines++;
			if (line_width > local_max_line_width)
				local_max_line_width = line_width;
			line_width = 0;
		} else {
			font_bit_width = (int) (0.5f + get_char_width (str[isrc]) * DEFAULT_FONT_X_LEN * zoom / 12.0f);
			if (line_width + font_bit_width > width)
			{
				// search back for a space
				for (nchar = 0; ibuf-nchar-1 > lastline; nchar++) {
					if (buf[ibuf-nchar-1] == ' ') {
						break;
					}
				}
				if (ibuf-nchar-1 <= lastline)
					// no space found, introduce a break in
					// the middle of the word
					nchar = 0;

				// introduce the break, and reset the counters
				ibuf -= nchar;
				isrc -= nchar;

				buf[ibuf] = '\r';
				nlines++; ibuf++;
				if (cursor && isrc < *cursor) {
					dcursor++;
				}
				if (ibuf >= size - 1) {
					break;
				}

				if (line_width >= local_max_line_width)
					local_max_line_width = line_width;

				lastline = ibuf;
				line_width = font_bit_width;
			} else {
				line_width += font_bit_width;
			}
		}

		// copy the character into the buffer
		buf[ibuf] = str[isrc];
		isrc++; ibuf++;

		if (ibuf >= size - 1) {
			break;
		}
	}

	safe_strncpy(str, buf, size * sizeof(char));
	str[size-1] = '\0';

	if (cursor) {
		*cursor += dcursor;
		if(*cursor > size-1) {
			/* If there's a better way to detect this, please do */
			*cursor = size-1;
		}
	}
	free(buf);
	if (line_width > local_max_line_width)
		local_max_line_width = line_width;
	if (max_line_width!=NULL)
		*max_line_width = local_max_line_width;
	return nlines;
}


/* ========================================================================== *
 *                              CHANNELS MESSAGES                             *
 * ========================================================================== */

//
int pos_selected(int msg, int ichar, select_info* select)
{
	int d = 0;
	if (select == NULL) return 0;
	if (TEXT_FIELD_SELECTION_EMPTY(select)) return 0;
	if (select->em > select->sm) d = 1;
	else if ((select->em == select->sm) && (select->ec >= select->sc)) d = 1;
	else d = -1;
	if (d * (msg - select->sm) > d * (select->em - select->sm)) return 0;
	if ((msg == select->em) && (d * ichar > d * select->ec)) return 0;
	if (d * msg < d * select->sm) return 0;
	if ((msg == select->sm) && (d * ichar < d * select->sc)) return 0;
	return 1;
}


//
void recolour_message(text_message *msg){
#ifndef ENGLISH
	if (msg->chan_idx >= CHAT_CHANNEL1 && msg->chan_idx <= CHAT_CHANNEL5 && msg->len > 0 && msg->data[0] && !msg->deleted){
#else //ENGLISH
	if (msg->chan_idx >= CHAT_CHANNEL1 && msg->chan_idx <= CHAT_CHANNEL3 && msg->len > 0 && msg->data[0] && !msg->deleted){
#endif //ENGLISH
		int i;
		for(i=0; i< MAX_CHANNEL_COLORS; i++)
		{
			if(channel_colors[i].nr == msg->channel)
				break;
		}
		if(i< MAX_CHANNEL_COLORS && channel_colors[i].color != -1) {
			msg->data[0] = to_color_char (channel_colors[i].color);
		} else if (active_channels[current_channel] != msg->channel){
#ifdef FR_VERSION
			msg->data[0] = to_color_char (couleur_canaux);
		} else {
			msg->data[0] = to_color_char (couleur_canal_actif);
#else //FR_VERSION
			msg->data[0] = to_color_char (c_grey2);
		} else {
			msg->data[0] = to_color_char (c_grey1);
#endif //FR_VERSION
		}
	}
}

void recolour_messages(text_message *msgs)
{
       int i;
       for (i=0; i < DISPLAY_TEXT_BUFFER_SIZE && msgs[i].data; ++i)
               recolour_message(&msgs[i]);
}

/* -------------------------------------------------------------------------- */

//
void draw_messages(int x, int y, text_message *msgs, int msgs_size, Uint8 filter, int msg_start, int offset_start, int cursor, int width, int height, float text_zoom, select_info* select)
{
	float displayed_font_x_size = DEFAULT_FONT_X_LEN * text_zoom;
	float displayed_font_y_size = DEFAULT_FONT_Y_LEN * text_zoom;

	float selection_red = 255 / 255.0f;
	float selection_green = 162 / 255.0f;
	float selection_blue = 0;

	unsigned char cur_char;
	int i;
	int imsg, ichar;
	int cur_x, cur_y;
	int cursor_x = x-1, cursor_y = y-1;
	unsigned char ch;
	int cur_line = 0;
	int cur_col = 0;
	unsigned char last_color_char = 0;
	int in_select = 0;

	imsg = msg_start;
	ichar = offset_start;
	if (msgs[imsg].data == NULL || msgs[imsg].deleted) return;
	if (width < displayed_font_x_size || height < displayed_font_y_size) return;

#ifndef MAP_EDITOR2
	#ifndef FR_VERSION
	if (filter != FILTER_ALL)
	#endif //FR_VERSION
	{
		// skip all messages of the wrong channel
		while (1)
		{
			if (skip_message(&msgs[imsg], filter))
			{
				ichar = 0;
				if (++imsg >= msgs_size) imsg = 0;
				if (msgs[imsg].data == NULL || imsg == msg_start || msgs[imsg].deleted)
					// nothing to draw
					return;
			}
			else
			{
				break;
			}
		}
		if (msgs[imsg].data == NULL || msgs[imsg].deleted) return;
	}
#endif //! MAP_EDITOR2

	ch = msgs[imsg].data[ichar];
	if (!is_color (ch))
	{
		// search backwards for the last color
		for (i = ichar-1; i >= 0; i--)
		{
			ch = msgs[imsg].data[i];
			if (is_color (ch))
			{
				find_font_char (ch);
				last_color_char = ch;
				break;
			}
		}

		if (i < 0)
		{
			// no color character found, try the message color
			if (msgs[imsg].r >= 0)
				glColor3f (msgs[imsg].r, msgs[imsg].g, msgs[imsg].b);
		}
	}

	// enable alpha filtering, so we have some alpha key
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
// 	glEnable (GL_ALPHA_TEST);
//	glAlphaFunc (GL_GREATER, 0.1f);
#ifdef	NEW_TEXTURES
	bind_texture(font_text);
#else //NEW_TEXTURES
	get_and_set_texture_id(font_text);
#endif //NEW_TEXTURES

	i = 0;
	cur_x = x;
	cur_y = y;

	glBegin (GL_QUADS);
	while (1)
	{
		if (i == cursor)
		{
			cursor_x = cur_x;
			cursor_y = cur_y;
			if (cursor_x - x > width - displayed_font_x_size)
			{
				cursor_x = x;
				cursor_y = cur_y + displayed_font_y_size;
			}

		}

		cur_char = msgs[imsg].data[ichar];

		// watch for special characters
		if (cur_char == '\0')
		{
			// end of message
			if (++imsg >= msgs_size) {
				imsg = 0;
			}
#ifndef MAP_EDITOR2
	#ifndef FR_VERSION
			if (filter != FILTER_ALL)
	#endif //FR_VERSION
			{
				// skip all messages of the wrong channel
				while (skip_message (&msgs[imsg], filter))
				{
					if (++imsg >= msgs_size) imsg = 0;
					if (msgs[imsg].data == NULL || imsg == msg_start) break;
				}
			}
#endif
			if (msgs[imsg].data == NULL || imsg == msg_start || msgs[imsg].deleted) break;
			rewrap_message (&msgs[imsg], text_zoom, chat_font, width, NULL);
			ichar = 0;
			last_color_char = 0;
		}

		if (select != NULL && select->lines && select->lines[cur_line].msg == -1)
		{
			select->lines[cur_line].msg = imsg;
			select->lines[cur_line].chr = ichar;
		}

		if (cur_char == '\n' || cur_char == '\r' || cur_char == '\0')
		{
			// newline
			cur_y += displayed_font_y_size;
			if (cur_y - y > height - displayed_font_y_size) break;
			cur_x = x;
			if (cur_char != '\0') ichar++;
			i++;
			cur_line++;
			cur_col = 0;
			continue;
		}

		if (pos_selected(imsg, ichar, select))
		{
			if (!in_select)
			{
				glColor3f (selection_red, selection_green, selection_blue);
				in_select = 1;
			}
		}
		else
		{
			if (in_select)
			{
				if (last_color_char)
					find_font_char (last_color_char);
				else if (msgs[imsg].r < 0)
					find_font_char (to_color_char (c_grey1));
				else
					glColor3f (msgs[imsg].r, msgs[imsg].g, msgs[imsg].b);

				in_select = 0;
			}
		}

		if (is_color (cur_char))
		{
			last_color_char = cur_char;
			if (in_select)
			{
				// don't draw color characters in a selection
				i++;
				ichar++;
				continue;
			}
		}

		cur_x+= draw_char_scaled(cur_char, cur_x, cur_y, displayed_font_x_size, displayed_font_y_size);
		cur_col++;

		ichar++;
		i++;
		if (cur_x - x > width - displayed_font_x_size)
		{
			// ignore rest of this line, but keep track of
			// color characters
			while (1)
			{
				ch = msgs[imsg].data[ichar];
				if (ch == '\0' || ch == '\n' || ch == '\r')
					break;
				if (is_color (ch))
					last_color_char = ch;
				ichar++;
				i++;
			}
		}
	}

	if (cursor_x >= x && cursor_y >= y && cursor_y - y <= height - displayed_font_y_size)
	{
		draw_char_scaled('_', cursor_x, cursor_y, displayed_font_x_size, displayed_font_y_size);
	}

	glEnd();
	glDisable(GL_BLEND);
//	glDisable(GL_ALPHA_TEST);
#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
}


/* ========================================================================== *
 *  Fonction d'affichage dans le terminal (mode debug)                        *
 * ========================================================================== */

#ifdef DEBUG
void print_string_escaped(const char *str)
{
	int i;

	for (i=0; str[i]; i++)
	{
		if (str[i]=='\r')
			printf ("\\r");
		else if (str[i] == '\n')
			printf ("\\n\n");
		else if (!is_color (str[i]))
			printf ("%c", str[i]);
	}

	printf ("\nlen = %d\n", i);
}
#endif


/* ========================================================================== *
 *  Fonctions de gestion des fontes                                           *
 * ========================================================================== */

// Sélectionne la fonte indiquée comme la fonte courante
int	set_font(int num)
{
	if ((num >= 0) && (num < FONTS_ARRAY_SIZE) && fonts[num] && (fonts[num]->texture_id >= 0))
	{
		cur_font_num = num;
		font_text = fonts[cur_font_num]->texture_id;
	}
	return cur_font_num;
}

#ifndef NEW_TEXTURES
// Recharge toutes les textures (appelé lors d'un changement de mode video)
void reload_fonts()
{
	int i;
	int poor_man_save = poor_man;
	int use_mipmaps_save = use_mipmaps;

	poor_man = 0;
	use_mipmaps = 0;

	for (i = 0; i < FONTS_ARRAY_SIZE; i++)
	{
		if (fonts[i] == NULL) continue;
		if (fonts[i]->texture_id >= 0)
		{
			glDeleteTextures(1, (GLuint*)&texture_cache[fonts[i]->texture_id].texture_id);
			texture_cache[fonts[i]->texture_id].texture_id = 0;
			get_texture_id(fonts[i]->texture_id);
		}
	}

	poor_man = poor_man_save;
	use_mipmaps = use_mipmaps_save;
#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
}
#endif //NEW_TEXTURES

// Libère les fontes en mémoire (appelé avant la fin du programme)
void cleanup_fonts()
{
	int i;
	for (i = 0; i < FONTS_ARRAY_SIZE; i++)
	{
		if (fonts[i] != NULL) free(fonts[i]);
	}
	for (i = 0; i < FONT_CHARS_MAX_LINE*FONT_CHARS_PER_LINE; i++)
	{
		if (font_char_uv[i] != NULL) free(font_char_uv[i]);
	}
}


/* ========================================================================== *
 *  Fonctions d'initialisation de la liste des fontes disponibles             *
 * ========================================================================== */

//
int load_font_textures()
{
	int i;
#ifndef	NEW_TEXTURES
	int poor_man_save=poor_man;
	int use_mipmaps_save=use_mipmaps;
#endif	/* NEW_TEXTURES */

/*
	if (fonts[0] == NULL || fonts[1] == NULL || fonts[2] == NULL || fonts[3]==NULL )
	{
		for (i = 0; i < FONTS_ARRAY_SIZE; i++) {
			if (fonts[i] != NULL) free (fonts[i]);
			fonts[i] = NULL;
		}
		if ( ! init_fonts() ) return 0;
	}
*/
#ifndef NEW_TEXTURES
	poor_man = 0;
	use_mipmaps = 0;
#endif	//NEW_TEXTURES

	for (i = 0; i < FONTS_ARRAY_SIZE; i++)
	{
		char str[60] = "";
		if (fonts[i]->texture_file[0] == '\0') continue;

#ifdef NEW_TEXTURES
		safe_snprintf(str, sizeof(str), "./%s%s.dds", texture_dir, fonts[i]->texture_file);
		if (i > 0)
		{
			fonts[i]->texture_id = load_texture_cached(str, tt_font);
		}
		else
		{
			fonts[i]->texture_id = load_texture_cached(str, tt_font);
		}
#else //NEW_TEXTURES
		safe_snprintf(str, sizeof(str), "./%s%s.bmp", texture_dir, fonts[i]->texture_file);
		if (i > 0)
		{
			fonts[i]->texture_id = load_texture_cache_deferred(str, 0);
		}
		else
		{
			fonts[i]->texture_id = load_texture_cache(str, 0);
		}
#endif //NEW_TEXTURES

		if (fonts[i]->texture_id >= 0)
		{
			add_multi_option("chat_font", fonts[i]->name);
			add_multi_option("name_font", fonts[i]->name);
		}
	}

#ifndef	NEW_TEXTURES
	poor_man=poor_man_save;
	use_mipmaps=use_mipmaps_save;
#endif	/* NEW_TEXTURES */

	//set the default font
	set_font(0);
	return 1;
}


// Préchargement en mémoire de la table de correspondance num_char <-> uv_map
int init_char_uv()
{
	int row, col;
	for (row = 0; row < FONT_CHARS_MAX_LINE; row++)
	{
		for (col = 0; col < FONT_CHARS_PER_LINE; col++)
		{
			int num = row * FONT_CHARS_PER_LINE + col;
			// allocation mémoire si nécessaire
			if (font_char_uv[num] == NULL)
			{
				font_char_uv[num] = (char_uv *)calloc(1, sizeof(char_uv));
				if (font_char_uv[num] == NULL) return -1;
			}
			font_char_uv[num]->u_start = (float) (col*FONT_X_SPACING)/512.0f;
			font_char_uv[num]->u_end = font_char_uv[num]->u_start + (FONT_X_SPACING-1)/512.0f;
#ifdef NEW_TEXTURES
			font_char_uv[num]->v_start = (float) (4.0f + row*FONT_Y_SPACING)/512.0f;
			font_char_uv[num]->v_end = font_char_uv[num]->v_start + (FONT_Y_SPACING-1)/512.0f;
#else //NEW_TEXTURES
			font_char_uv[num]->v_start = (float) 1.0f - (4.0f + row*FONT_Y_SPACING)/512.0f;
			font_char_uv[num]->v_end = font_char_uv[num]->v_start - (FONT_Y_SPACING-1)/512.0f;
#endif //NEW_TEXTURES
		}
	}
	return 1;
}

/* -------------------------------------------------------------------------- */

void set_font_widths(xmlNode * widthsnode, int num)
{
	int n = 0;
	char tmp[3];
	xmlNode* node;
	for (node = widthsnode->children; node; node = node->next)
	{
		if (xmlStrEqual(node->name, (xmlChar*)"text"))
		{
			int i = 0;
			const Uint8 * widths = node->content;
			if (! widths) continue;
			while (*widths) {
				if (*widths>47 && *widths<58 && i<2) tmp[i++] = *widths;
				else
				{
					if (i > 0)
					{
						tmp[i] = '\0';
						fonts[num]->widths[n++] = atoi(tmp);
					}
					tmp[0] = '\0';
					i = 0;
				}
				if (n >= FONT_CHARS_PER_LINE*FONT_CHARS_MAX_LINE) break;
				widths++;
			}
		}
	}
}


void set_font_parameters(xmlNode * node)
{
	int num;
	if (! xmlStrEqual(node->name, (xmlChar*)"font")) return;

	num = xmlGetInt(node, (xmlChar*)"num");
	if (num >= FONTS_ARRAY_SIZE) return;

    for (node = node->children; node; node = node->next)
    {
		char* tmp;
        if (node->type != XML_ELEMENT_NODE) continue;

		if (xmlStrEqual(node->name, (xmlChar*)"name"))
		{
			tmp = fromUTF8(node->children->content, strlen((const char*)node->children->content));
			safe_strncpy(fonts[num]->name, tmp, sizeof(fonts[num]->name));
			if (tmp) free(tmp);
		}
		else if (xmlStrEqual(node->name, (xmlChar*)"adjust"))
		{
			fonts[num]->adjust = get_float_value(node);
		}
		else if (xmlStrEqual(node->name, (xmlChar*)"texture"))
		{
			get_string_value(fonts[num]->texture_file, sizeof(fonts[num]->texture_file), node);
		}
		else if (xmlStrEqual(node->name, (xmlChar*)"spacing"))
		{
			fonts[num]->spacing = get_int_value(node);
		}
		else if (xmlStrEqual(node->name, (xmlChar*)"baseline"))
		{
			fonts[num]->baseline = get_int_value(node);
		}
		else if (xmlStrEqual(node->name, (xmlChar*)"widths"))
		{
			int n;
			int width = xmlGetInt(node, (xmlChar*)"fixed");
//			fonts[i]->fixed_width = width;
			if (width>0) for (n=0; n<FONT_CHARS_PER_LINE*FONT_CHARS_MAX_LINE; n++) fonts[num]->widths[n] = width;
			set_font_widths(node, num);
		}
	}
}


int init_fonts()
{
	int i, j;
	int ok = 1;
	char *fname = "./fontes.xml";
	xmlDoc *doc;
	xmlNode *root;

	//
	init_char_uv();

	//
	for (i = 0; i < FONTS_ARRAY_SIZE; i++)
	{
		fonts[i] = NULL;
		fonts[i] = (font_info *)calloc(1, sizeof(font_info));
		if (fonts[i] == NULL)
		{
			LOG_ERROR(cant_load_font);
			return 0;
		}
		// set default font info
		safe_snprintf(fonts[i]->name, sizeof(fonts[i]->name), "Fonte %i", i);
		my_strcp(fonts[i]->texture_file, "");
		fonts[i]->adjust = 1.0f;
		fonts[i]->spacing = 0;
		fonts[i]->baseline = 0;
//		fonts[i]->fixed_width = 12;
		for (j=0; j<FONT_CHARS_PER_LINE*FONT_CHARS_MAX_LINE; j++) fonts[i]->widths[j] = 12;
	}

	// lecture du fichier XML
	doc = xmlReadFile(fname, NULL, 0);
	if (doc == NULL)
	{
		LOG_ERROR("Impossible de lire le fichier de définition des fontes %s", fname);
		return 0;
	}

	// parcours du fichier XML
	root = xmlDocGetRootElement(doc);
	if (root == NULL)
	{
		LOG_ERROR("Impossible de parcourir le fichier de définition des fontes %s", fname);
		ok = 0;
	}
	else if (! xmlStrEqual(root->name, (xmlChar*)"Fonts"))
	{
		LOG_ERROR("Fichier XML non valide \"%s\" (\"Fonts\" attendu).", root->name);
		ok = 0;
	}
	else
	{
		xmlNode *node;
		for (node = root->children; node; node = node->next)
		{
			if (node->type == XML_ELEMENT_NODE) set_font_parameters(node);
		}
	}
	xmlFreeDoc(doc);

	//TODO: vérifier que la fonte 0 a été définie
	set_font(0);
	return ok;
}


/* -------------------------------------------------------------------------- */
#else //FR_VERSION

/* NOTE: This file contains implementations of the following, currently unused, and commented functions:
 *          Look at the end of the file.
 *
 * void remove_font(int);
 */

#define FONT_START_CHAR	32
#define FONT_CHARS_PER_LINE	14
#define	FONT_X_SPACING	18
#define	FONT_Y_SPACING	21
#define FONTS_ARRAY_SIZE	10

typedef struct	{
	int	spacing;
	int texture_id;
	int	widths[10*FONT_CHARS_PER_LINE];
	char name[32];
} font_info;

static int font_text = 0;

int	cur_font_num=0;
int	max_fonts=0;
font_info	*fonts[FONTS_ARRAY_SIZE];
char font_names[FONTS_ARRAY_SIZE][30];
int	chat_font=0;
int	name_font=0;
int	book_font=0;

int get_font_width(int cur_char);
int get_nstring_width(const unsigned char *str, int len);
int set_font_parameters (int num);

// CHECK
int pos_selected(int msg, int ichar, select_info* select)
{
	int d = 0;
	if (select == NULL) return 0;
	if (TEXT_FIELD_SELECTION_EMPTY(select)) return 0;
	if (select->em > select->sm) d = 1;
	else if ((select->em == select->sm) && (select->ec >= select->sc)) d = 1;
	else d = -1;
	if (d * (msg - select->sm) > d * (select->em - select->sm)) return 0;
	if ((msg == select->em) && (d * ichar > d * select->ec)) return 0;
	if (d * msg < d * select->sm) return 0;
	if ((msg == select->sm) && (d * ichar < d * select->sc)) return 0;
	return 1;
}


int get_font_char(unsigned char cur_char)
{
	if(cur_char<FONT_START_CHAR)	//null or invalid character
		{
			return -1;
		}
	else if(cur_char>=127)
		{
			if(cur_char<=127+c_grey4)
				{
					//color, won't show
					return -1;
				}
			else if(cur_char>127+c_grey4)
				{
					switch(cur_char) {
					case 193:
						cur_char=AACCENT;break;
					case 196:
						cur_char=AUMLAUT;break;
					case 197:
						cur_char=ARING;break;
					case 198:
						cur_char=AELIG;break;
					case 201:
						cur_char=EACCENT;break;
					case 205:
						cur_char=IACCENT;break;
					case 209:
						cur_char=ENYE;break;
					case 211:
						cur_char=OACCENT;break;
					case 214:
						cur_char=OUMLAUT;break;
					case 216:
						cur_char=OSLASH;break;
					case 218:
						cur_char=UACCENT;break;
					case 220:
						cur_char=UUMLAUT;break;
					case 223:
						cur_char=DOUBLES;break;
					case 224:
						cur_char=AGRAVE;break;
					case 225:
						cur_char=aACCENT;break;
					case 226:
						cur_char=ACIRC;break;
					case 228:
						cur_char=aUMLAUT;break;
					case 229:
						cur_char=aRING;break;
					case 230:
						cur_char=aELIG;break;
					case 231:
						cur_char=CCEDIL;break;
					case 232:
						cur_char=EGRAVE;break;
					case 233:
						cur_char=EACUTE;break;
					case 234:
						cur_char=ECIRC;break;
					case 235:
						cur_char=EUML;break;
					case 237:
						cur_char=iACCENT;break;
					case 238:
						cur_char=iCIRC;break;
					case 239:
						cur_char=IUML;break;
					case 241:
						cur_char=EnyE;break;
					case 243:
						cur_char=oACCENT;break;
					case 244:
						cur_char=OCIRC;break;
					case 246:
						cur_char=oUMLAUT;break;
					case 248:
						cur_char=oSLASH;break;
					case 249:
						cur_char=UGRAVE;break;	// bonne lettre
					case 250:
						cur_char=uACCENT;break;
					case 251:
						cur_char=uCIRC;break;
					case 252:
						cur_char=UUML;break;
					/* this case is redundant. which one is right?
					case 252:
						cur_char=uUMLAUT;break; */
					default:
                    printf("%d\n",cur_char);
						return -1;	//ignore it
					}
					if(cur_char>=UUML && cur_char<=iCIRC)
						{
							cur_char-=(UUML-127);
						}
				}
		}

	// finally, adjust for the missing lead characters
	cur_char-=FONT_START_CHAR;

	return((int)cur_char);
}

// converts a character into which entry in font.bmp to use, negative on error or no output
int find_font_char (unsigned char cur_char)
{
	if (is_color (cur_char))
	{
		float r,g,b;
		//must be a color
		int color = from_color_char (cur_char);
		r = (float) colors_list[color].r1 / 255.0f;
		g = (float) colors_list[color].g1 / 255.0f;
		b = (float) colors_list[color].b1 / 255.0f;
		//This fixes missing letters in the font on some clients
		//No idea why going from 3f to 4f helps, but it does
		glColor4f(r,g,b,1.0);
		return(-1);	// nothing to do
	}
	return(get_font_char(cur_char));
}

// returns how far to move for the next char, or negative on error
int	draw_char_scaled(unsigned char cur_char, int cur_x, int cur_y, float displayed_font_x_size, float displayed_font_y_size)
{
	float u_start,u_end,v_start,v_end;
	int chr,col,row;
	int displayed_font_x_width;
	int	font_bit_width, ignored_bits;

	chr= find_font_char(cur_char);
	if(chr < 0)	// watch for illegal/non-display characters
		{
			return 0;
		}

	// first, see where that char is, in the font.bmp
	col= chr/FONT_CHARS_PER_LINE;
	row= chr%FONT_CHARS_PER_LINE;

	//displayed_font_x_width=(int)displayed_font_x_size;
	font_bit_width= get_font_width(chr);
	displayed_font_x_width= (int)(0.5f+((float)font_bit_width)*displayed_font_x_size/12.0);
	ignored_bits= (12-font_bit_width)/2;	// how many bits on each side of the char are ignored?

	//now get the texture coordinates
	u_start= (float)(row*FONT_X_SPACING+ignored_bits)/256.0f;
	u_end= (float)(row*FONT_X_SPACING+FONT_X_SPACING-7-ignored_bits)/256.0f;
#ifdef NEW_TEXTURES
	v_start= (float)(1+col*FONT_Y_SPACING)/256.0f;
	v_end= (float)(col*FONT_Y_SPACING+FONT_Y_SPACING-1)/256.0f;
#else
	v_start= (float)1.0f-(1+col*FONT_Y_SPACING)/256.0f;
	v_end= (float)1.0f-(col*FONT_Y_SPACING+FONT_Y_SPACING-1)/256.0f;
#endif //NEW_TEXTURES

	// and place the text from the graphics on the map
	glTexCoord2f(u_start,v_start);
	glVertex3i(cur_x,cur_y,0);

	glTexCoord2f(u_start,v_end);
	glVertex3i(cur_x,cur_y+(displayed_font_y_size+1),0);

	glTexCoord2f(u_end,v_end);
	glVertex3i(cur_x+displayed_font_x_width,cur_y+(displayed_font_y_size+1),0);

	glTexCoord2f(u_end,v_start);
	glVertex3i(cur_x+displayed_font_x_width,cur_y,0);

	return(displayed_font_x_width);	// return how far to move for the next character
}

#ifndef MAP_EDITOR2
void recolour_message(text_message *msg){
#ifndef ENGLISH
	if (msg->chan_idx >= CHAT_CHANNEL1 && msg->chan_idx <= CHAT_CHANNEL5 && msg->len > 0 && msg->data[0] && !msg->deleted){
#else //ENGLISH
	if (msg->chan_idx >= CHAT_CHANNEL1 && msg->chan_idx <= CHAT_CHANNEL3 && msg->len > 0 && msg->data[0] && !msg->deleted){
#endif //ENGLISH
	{
		int i;
		for(i=0; i< MAX_CHANNEL_COLORS; i++)
		{
			if(channel_colors[i].nr == msg->channel)
				break;
		}
		if(i< MAX_CHANNEL_COLORS && channel_colors[i].color != -1) {
			msg->data[0] = to_color_char (channel_colors[i].color);
		} else if (active_channels[current_channel] != msg->channel){
#ifdef FR_VERSION
			msg->data[0] = to_color_char (couleur_canaux);
		} else {
			msg->data[0] = to_color_char (couleur_canal_actif);
#else //FR_VERSION
			msg->data[0] = to_color_char (c_grey2);
		} else {
			msg->data[0] = to_color_char (c_grey1);
#endif //FR_VERSION
		}
	}
}
#endif

void recolour_messages(text_message *msgs){
	int i;
	for(i=0;i<DISPLAY_TEXT_BUFFER_SIZE && msgs[i].data;++i){
		recolour_message(&msgs[i]);
	}
}

void draw_messages (int x, int y, text_message *msgs, int msgs_size, Uint8 filter, int msg_start, int offset_start, int cursor, int width, int height, float text_zoom, select_info* select)
{
	float displayed_font_x_size = DEFAULT_FONT_X_LEN * text_zoom;
	float displayed_font_y_size = DEFAULT_FONT_Y_LEN * text_zoom;

	float selection_red = 255 / 255.0f;
	float selection_green = 162 / 255.0f;
	float selection_blue = 0;

	unsigned char cur_char;
	int i;
	int imsg, ichar;
	int cur_x, cur_y;
	int cursor_x = x-1, cursor_y = y-1;
	unsigned char ch;
	int cur_line = 0;
	int cur_col = 0;
	unsigned char last_color_char = 0;
	int in_select = 0;

	imsg = msg_start;
	ichar = offset_start;
	if (msgs[imsg].data == NULL || msgs[imsg].deleted) return;

	if (width < displayed_font_x_size || height < displayed_font_y_size)
		// no point in trying
		return;

#ifndef MAP_EDITOR2
	#ifndef FR_VERSION
	if (filter != FILTER_ALL)
	#endif //FR_VERSION
	{
		// skip all messages of the wrong channel
		while (1)
		{
			if (skip_message(&msgs[imsg], filter))
			{
				ichar = 0;
				if (++imsg >= msgs_size) imsg = 0;
				if (msgs[imsg].data == NULL || imsg == msg_start || msgs[imsg].deleted)
					// nothing to draw
					return;
			}
			else
			{
				break;
			}
		}
		if (msgs[imsg].data == NULL || msgs[imsg].deleted) return;
	}
#endif //! MAP_EDITOR2

	ch = msgs[imsg].data[ichar];
	if (!is_color (ch))
	{
		// search backwards for the last color
		for (i = ichar-1; i >= 0; i--)
		{
			ch = msgs[imsg].data[i];
			if (is_color (ch))
			{
				find_font_char (ch);
				last_color_char = ch;
				break;
			}
		}

		if (i < 0)
		{
			// no color character found, try the message color
			if (msgs[imsg].r >= 0)
				glColor3f (msgs[imsg].r, msgs[imsg].g, msgs[imsg].b);
		}
	}

 	glEnable (GL_ALPHA_TEST);	// enable alpha filtering, so we have some alpha key
	glAlphaFunc (GL_GREATER, 0.1f);
#ifdef	NEW_TEXTURES
	bind_texture(font_text);
#else	/* NEW_TEXTURES */
	get_and_set_texture_id(font_text);
#endif	/* NEW_TEXTURES */

	i = 0;
	cur_x = x;
	cur_y = y;
	glBegin (GL_QUADS);
	while (1)
	{
		if (i == cursor)
		{
			cursor_x = cur_x;
			cursor_y = cur_y;
			if (cursor_x - x > width - displayed_font_x_size)
			{
				cursor_x = x;
				cursor_y = cur_y + displayed_font_y_size;
			}

		}

		cur_char = msgs[imsg].data[ichar];

		// watch for special characters
		if (cur_char == '\0')
		{
			// end of message
			if (++imsg >= msgs_size) {
				imsg = 0;
			}
#ifndef MAP_EDITOR2
	#ifndef FR_VERSION
			if (filter != FILTER_ALL)
	#endif //FR_VERSION
			{
				// skip all messages of the wrong channel
				while (skip_message (&msgs[imsg], filter))
				{
					if (++imsg >= msgs_size) imsg = 0;
					if (msgs[imsg].data == NULL || imsg == msg_start) break;
				}
			}
#endif
			if (msgs[imsg].data == NULL || imsg == msg_start || msgs[imsg].deleted) break;
			rewrap_message (&msgs[imsg], text_zoom, width, NULL);
			ichar = 0;
			last_color_char = 0;
		}

		if (select != NULL && select->lines && select->lines[cur_line].msg == -1)
		{
			select->lines[cur_line].msg = imsg;
			select->lines[cur_line].chr = ichar;
		}

		if (cur_char == '\n' || cur_char == '\r' || cur_char == '\0')
		{
			// newline
			cur_y += displayed_font_y_size;
			if (cur_y - y > height - displayed_font_y_size) break;
			cur_x = x;
			if (cur_char != '\0') ichar++;
			i++;
			cur_line++;
			cur_col = 0;
			continue;
		}

		if (pos_selected(imsg, ichar, select))
		{
			if (!in_select)
			{
				glColor3f (selection_red, selection_green, selection_blue);
				in_select = 1;
			}
		}
		else
		{
			if (in_select)
			{
				if (last_color_char)
					find_font_char (last_color_char);
				else if (msgs[imsg].r < 0)
					find_font_char (to_color_char (c_grey1));
				else
					glColor3f (msgs[imsg].r, msgs[imsg].g, msgs[imsg].b);

				in_select = 0;
			}
		}

		if (is_color (cur_char))
		{
			last_color_char = cur_char;
			if (in_select)
			{
				// don't draw color characters in a selection
				i++;
				ichar++;
				continue;
			}
		}

		cur_x += draw_char_scaled (cur_char, cur_x, cur_y, displayed_font_x_size, displayed_font_y_size);
		cur_col++;

		ichar++;
		i++;
		if (cur_x - x > width - displayed_font_x_size)
		{
			// ignore rest of this line, but keep track of
			// color characters
			while (1)
			{
				ch = msgs[imsg].data[ichar];
				if (ch == '\0' || ch == '\n' || ch == '\r')
					break;
				if (is_color (ch))
					last_color_char = ch;
				ichar++;
				i++;
			}
		}
	}

	if (cursor_x >= x && cursor_y >= y && cursor_y - y <= height - displayed_font_y_size)
	{
		draw_char_scaled ('_', cursor_x, cursor_y, displayed_font_x_size, displayed_font_y_size);
	}

	glEnd();
	glDisable(GL_ALPHA_TEST);
#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
}

int draw_string (int x, int y, const unsigned char * our_string, int max_lines)
{
	return draw_string_zoomed_width (x, y, our_string, window_width, max_lines, 1.0f);
}

int draw_string_shadowed (int x, int y, const unsigned char * our_string, int max_lines, float fr,float fg,float fb, float br,float bg,float bb)
{
 	 int px,py,r;
 	 //set shadow colour
	 glColor3f(br, bg, bb);
	 for(px=-1;px<2;px++)
  	     for(py=-1;py<2;py++)
  	         if(px!=0 || py!=0)
  	             r=draw_string(x+px, y+py, our_string, max_lines);
 	 //set foreground colour
	 glColor3f(fr, fg, fb);
     r=draw_string(x, y, our_string, max_lines);
     return r;
}

int draw_string_shadowed_width (int x, int y, const unsigned char * our_string, int max_width, int max_lines, float fr,float fg,float fb, float br,float bg,float bb)
{
 	 int px,py,r;
	 float zoom = ((float)max_width*12.0)/((float)get_string_width(our_string)*11.0);
 	 //set shadow colour
	 glColor3f(br, bg, bb);
	 for(px=-1;px<2;px++)
  	     for(py=-1;py<2;py++)
  	         if(px!=0 || py!=0)
  	             r=draw_string_zoomed(x+px, y+py, our_string, max_lines, zoom);
 	 //set foreground colour
	 glColor3f(fr, fg, fb);
     r=draw_string_zoomed(x, y, our_string, max_lines, zoom);
     return r;
}

int draw_string_width(int x, int y, const unsigned char * our_string, int max_width, int max_lines)
{
	return draw_string_zoomed_width (x, y, our_string, max_width, max_lines, 1.0f);
}

int draw_string_zoomed (int x, int y, const unsigned char * our_string, int max_lines, float text_zoom)
{
	return draw_string_zoomed_width (x, y, our_string, window_width, max_lines, text_zoom);
}

int draw_string_zoomed_width (int x, int y, const unsigned char * our_string, int max_width, int max_lines, float text_zoom)
{
	float displayed_font_x_size= 11.0*text_zoom;
	float displayed_font_y_size= 18.0*text_zoom;

	unsigned char cur_char;
	int i;
	int cur_x,cur_y;
	int current_lines= 1;

#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
	glEnable(GL_ALPHA_TEST);//enable alpha filtering, so we have some alpha key
	glAlphaFunc(GL_GREATER,0.1f);
#ifdef	NEW_TEXTURES
	bind_texture(font_text);
#else	/* NEW_TEXTURES */
	get_and_set_texture_id(font_text);
#endif	/* NEW_TEXTURES */

	i=0;
	cur_x=x;
	cur_y=y;
	glBegin(GL_QUADS);
	while(1)
		{
			cur_char=our_string[i];
			// watch for special characters
			if(!cur_char)	// end of line
				{
					break;
				}
			else if (cur_char == '\n' || cur_char == '\r')	// newline
				{
					cur_y+=displayed_font_y_size;
					cur_x=x;
					i++;
					current_lines++;
					if(current_lines>max_lines)break;
					continue;
				}
			else if (cur_x+displayed_font_x_size-x>=max_width){
				cur_y+=displayed_font_y_size;
				cur_x=x;
				current_lines++;
				if(current_lines>max_lines)break;
			}

			cur_x+=draw_char_scaled(cur_char, cur_x, cur_y, displayed_font_x_size, displayed_font_y_size);

			i++;
		}

	glEnd();
	glDisable(GL_ALPHA_TEST);
#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
	return current_lines;
}

void draw_string_clipped(int x, int y, const unsigned char * our_string, int width, int height)
{
	draw_string_zoomed_clipped (x, y, our_string, -1, width, height, 1.0f);
}

void draw_string_zoomed_clipped (int x, int y, const unsigned char* our_string, int cursor_pos, int width, int height, float text_zoom)
{
	float displayed_font_x_size = DEFAULT_FONT_X_LEN * text_zoom;
	float displayed_font_y_size = DEFAULT_FONT_Y_LEN * text_zoom;

	unsigned char cur_char;
	int i;
	int cur_x, cur_y;
	int cursor_x = x-1, cursor_y = y-1;

	if (width < displayed_font_x_size || height < displayed_font_y_size)
		// no point in trying
		return;

	glEnable (GL_ALPHA_TEST);	// enable alpha filtering, so we have some alpha key
	glAlphaFunc (GL_GREATER, 0.1f);
#ifdef	NEW_TEXTURES
	bind_texture(font_text);
#else	/* NEW_TEXTURES */
	get_and_set_texture_id(font_text);
#endif	/* NEW_TEXTURES */

	i = 0;
	cur_x = x;
	cur_y = y;
	glBegin (GL_QUADS);
	while (1)
	{
		if (i == cursor_pos)
		{
			cursor_x = cur_x;
			cursor_y = cur_y;
			if (cursor_x - x > width - displayed_font_x_size)
			{
				cursor_x = x;
				cursor_y = cur_y + displayed_font_y_size;
			}

		}

		cur_char = our_string[i];
		// watch for special characters
		if (!cur_char)
		{
			// end of string
			break;
		}
		else if (cur_char == '\n' || cur_char == '\r')
		{
			// newline
			cur_y += displayed_font_y_size;
			if (cur_y - y > height - displayed_font_y_size) break;
			cur_x = x;
			i++;
			continue;
		}

		cur_x += draw_char_scaled (cur_char, cur_x, cur_y, displayed_font_x_size, displayed_font_y_size);

		i++;
		if (cur_x - x > width - displayed_font_x_size)
		{
			// ignore rest of this line
			while (our_string[i] != '\0' && our_string[i] != '\n' && our_string[i] != '\r') i++;
		}
	}

	if (cursor_x >= x && cursor_y >= y && cursor_y - y <= height - displayed_font_y_size)
	{
		draw_char_scaled ('_', cursor_x, cursor_y, displayed_font_x_size, displayed_font_y_size);
	}

	glEnd();
	glDisable(GL_ALPHA_TEST);
#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
}

#ifdef DEBUG
void print_string_escaped (const char *str)
{
	int i;

	for (i=0; str[i]; i++)
	{
		if (str[i]=='\r')
			printf ("\\r");
		else if (str[i] == '\n')
			printf ("\\n\n");
		else if (!is_color (str[i]))
			printf ("%c", str[i]);
	}

	printf ("\nlen = %d\n", i);
}
#endif

int reset_soft_breaks (char *str, int len, int size, float zoom, int width, int *cursor, float *max_line_width)
{
	char *buf;
	int ibuf;
	int nchar;
	int font_bit_width;
	int nlines;
	float line_width;
	int isrc, idst;
	int lastline;
	int dcursor = 0;
	/* the generic special text window code needs to know the
	   maximum line length in pixels.  This information is used
	   but was previously throw away in this function.  Others
	   may fine it useful for setting the winow size, so pass back
	   to the caller if they provide somewhere to store it. */
	float local_max_line_width = 0;

	// error checking
	if (str == NULL || width <= 0 || size <= 0) {
		return 0;
	}

	/* strip existing soft breaks before we start,
		to avoid complicated code later */
	for (isrc=0, idst=0; isrc<len; isrc++)
	{
		if (str[isrc] == '\r')
		{
			/* move the cursor back if after this point */
			if ((cursor != NULL) && (isrc < *cursor))
				dcursor--;
		}
		else
			str[idst++] = str[isrc];
	}
	len = idst;
	str[len] = 0;

	/* allocate the working buffer so it can hold the maximum
	   the source string can take.  Previously, the fixed length
	   buffer was sometimes not big enough.  The code looked
	   to attempt to cope but was floored.  When ever the wrap
	   caused more characters to be in the output, some of the
	   source would be lost.  This is still possable if the source
	   size cannot take the extra characters.  For example, try
	   #glinfo and watch as the end characters are lost.  At least
	   characters are no longer lost wrap process. If you make
	   size large enough for no character will be lost.  Twice
	   the actual string length is probably enough */
	buf = (char *)calloc(size, sizeof(char));

	nlines = 1;
	isrc = ibuf = idst = 0;
	line_width = 0;
	lastline = 0;

	// fill the buffer
	while (isrc < len && str[isrc] != '\0')
	{
		// see if it's an explicit line break
		if (str[isrc] == '\n') {
			nlines++;
			if (line_width > local_max_line_width)
				local_max_line_width = line_width;
			line_width = 0;
		} else {
			font_bit_width = (int) (0.5f + get_char_width (str[isrc]) * 11.0f * zoom / 12.0f);
			if (line_width + font_bit_width > width)
			{
				// search back for a space
				for (nchar = 0; ibuf-nchar-1 > lastline; nchar++) {
					if (buf[ibuf-nchar-1] == ' ') {
						break;
					}
				}
				if (ibuf-nchar-1 <= lastline)
					// no space found, introduce a break in
					// the middle of the word
					nchar = 0;

				// introduce the break, and reset the counters
				ibuf -= nchar;
				isrc -= nchar;

				buf[ibuf] = '\r';
				nlines++; ibuf++;
				if (cursor && isrc < *cursor) {
					dcursor++;
				}
				if (ibuf >= size - 1) {
					break;
				}

				if (line_width > local_max_line_width)
					local_max_line_width = line_width;

				lastline = ibuf;
				line_width = font_bit_width;
			} else {
				line_width += font_bit_width;
			}
		}

		// copy the character into the buffer
		buf[ibuf] = str[isrc];
		isrc++; ibuf++;

		if (ibuf >= size - 1) {
			break;
		}
	}

	safe_strncpy(str, buf, size * sizeof(char));
	str[size-1] = '\0';

	if (cursor) {
		*cursor += dcursor;
		if(*cursor > size-1) {
			/* If there's a better way to detect this, please do */
			*cursor = size-1;
		}
	}
	free(buf);
	if (line_width > local_max_line_width)
		local_max_line_width = line_width;
	if (max_line_width!=NULL)
		*max_line_width = local_max_line_width;
	return nlines;
}

void draw_string_small_shadowed(int x, int y,const unsigned char * our_string,int max_lines, float fr, float fg, float fb, float br, float bg, float bb)
{
 	 int px,py;
 	 //set shadow colour
	 glColor4f(br, bg, bb, 0.25f);
	 for(px=-1;px<2;px++)
  	     for(py=-1;py<2;py++)
  	         if(px!=0 || py!=0)
  	             draw_string_small(x+px, y+py, our_string, max_lines);
 	 //set foreground colour
	 glColor4f(fr, fg, fb, 1.0f);
     draw_string_small(x, y, our_string, max_lines);
}

void draw_string_small(int x, int y,const unsigned char * our_string,int max_lines)
{
	//int displayed_font_x_size=SMALL_FONT_X_LEN;
	//int displayed_font_y_size=SMALL_FONT_Y_LEN;

	unsigned char cur_char;
	int i;
	int cur_x,cur_y;
	int current_lines=0;

#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
	glEnable(GL_ALPHA_TEST);//enable alpha filtering, so we have some alpha key
	glAlphaFunc(GL_GREATER,0.1f);
#ifdef	NEW_TEXTURES
	bind_texture(font_text);
#else	/* NEW_TEXTURES */
	get_and_set_texture_id(font_text);
#endif	/* NEW_TEXTURES */

	i=0;
	cur_x=x;
	cur_y=y;
	glBegin(GL_QUADS);
	while(1)
		{
			cur_char=our_string[i];
			if(!cur_char)
				{
					break;
				}
			else if(cur_char=='\n')
				{
#ifdef ENGLISH
					cur_y+=SMALL_FONT_Y_LEN;
					cur_x=x;
					i++;
					current_lines++;
					if(current_lines>=max_lines)break;
#else //ENGLISH
					/* affiche des points de suspension en fin de texte s'il est tronqué */
					if ((++current_lines>=max_lines) && (our_string[i+1]))
					{
						if ((i>0) && (our_string[i-1]==' ')) cur_x-=8;
						draw_char_scaled('.', cur_x+1, cur_y, SMALL_FONT_X_LEN, SMALL_FONT_Y_LEN);
						draw_char_scaled('.', cur_x+4, cur_y, SMALL_FONT_X_LEN, SMALL_FONT_Y_LEN);
						draw_char_scaled('.', cur_x+7, cur_y, SMALL_FONT_X_LEN, SMALL_FONT_Y_LEN);
						break;
					}
					cur_x=x;
					cur_y+=SMALL_FONT_Y_LEN;
					i++;
#endif //ENGLISH
					continue;
				}

			cur_x+=draw_char_scaled(cur_char, cur_x, cur_y, SMALL_FONT_X_LEN, SMALL_FONT_Y_LEN);

			i++;
		}


	glEnd();
	glDisable(GL_ALPHA_TEST);
#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
}

#ifdef	ELC
#ifndef MAP_EDITOR2

void draw_ortho_ingame_string(float x, float y,float z, const unsigned char * our_string,
						int max_lines, float font_x_scale, float font_y_scale)
{
	float u_start,u_end,v_start,v_end;
	int col,row;
	float displayed_font_x_size;
	float displayed_font_y_size;

	float displayed_font_x_width;
#ifndef SKY_FPV_OPTIONAL
	int font_x_size=FONT_X_SPACING;
	int font_y_size=FONT_Y_SPACING;
#endif // not SKY_FPV_OPTIONAL
	int	font_bit_width, ignored_bits;

	unsigned char cur_char;
	int chr;
	int i;
	float cur_x,cur_y;
	int current_lines=0;

#ifndef SKY_FPV_OPTIONAL
	/*
	if(big)
		{
			displayed_font_x_size=0.17*zoom_level*name_zoom/3.0;
			displayed_font_y_size=0.25*zoom_level*name_zoom/3.0;
		}
	else
		{
			displayed_font_x_size=SMALL_INGAME_FONT_X_LEN*zoom_level*name_zoom/3.0;
			displayed_font_y_size=SMALL_INGAME_FONT_Y_LEN*zoom_level*name_zoom/3.0;
		}
	*/
#endif // not SKY_FPV_OPTIONAL
	displayed_font_x_size=font_x_scale*name_zoom*12.0;
	displayed_font_y_size=font_y_scale*name_zoom*12.0;

	glEnable(GL_ALPHA_TEST);//enable alpha filtering, so we have some alpha key
	glAlphaFunc(GL_GREATER,0.1f);
#ifdef	NEW_TEXTURES
	bind_texture(font_text);
#else	/* NEW_TEXTURES */
	get_and_set_texture_id(font_text);
#endif	/* NEW_TEXTURES */

	i=0;
	cur_x=x;
	cur_y=y;
	glBegin(GL_QUADS);
#ifndef SKY_FPV_OPTIONAL
	while(1)
		{
			cur_char=our_string[i];
			if(!cur_char)
				{
					break;
				}
			else if(cur_char=='\n')
				{
					cur_y+=displayed_font_y_size;
					cur_x=x;
					i++;
					current_lines++;
					if(current_lines>=max_lines)break;
					continue;
				}
			else if (is_color (cur_char))
				{
					glEnd();	//Ooops - NV bug fix!!
				}
			chr=find_font_char(cur_char);
			if(chr >= 0)
				{
					col=chr/FONT_CHARS_PER_LINE;
					row=chr%FONT_CHARS_PER_LINE;


					font_bit_width=get_font_width(chr);
					displayed_font_x_width=((float)font_bit_width)*displayed_font_x_size/12.0;
					ignored_bits=(12-font_bit_width)/2;	// how many bits on each side of the char are ignored?
					if(ignored_bits < 0)ignored_bits=0;

					//now get the texture coordinates
					u_start=(float)(row*font_x_size+ignored_bits)/256.0f;
					u_end=(float)(row*font_x_size+font_x_size-7-ignored_bits)/256.0f;
					v_start=(float)(1+col*font_y_size)/256.0f;
					v_end=(float)(col*font_y_size+font_y_size-1)/256.0f;
					//v_end=(float)1.0f-(col*font_y_size+font_y_size-2)/256.0f;

					glTexCoord2f(u_start,v_start);
					glVertex3f(cur_x,cur_y+displayed_font_y_size,z);

					glTexCoord2f(u_start,v_end);
					glVertex3f(cur_x,cur_y,z);

					glTexCoord2f(u_end,v_end);
					glVertex3f(cur_x+displayed_font_x_width,cur_y,z);

					glTexCoord2f(u_end,v_start);
					glVertex3f(cur_x+displayed_font_x_width,cur_y+displayed_font_y_size,z);


					//cur_x+=displayed_font_x_size;
					cur_x+=displayed_font_x_width;
				}
			else if (is_color (cur_char))
				{
					glBegin(GL_QUADS);	//Ooops - NV bug fix!!
				}

#else // SKY_FPV_OPTIONAL
	while(1){
		cur_char=our_string[i];
		if(!cur_char){
			break;
		} else if(cur_char=='\n'){
			cur_y+=displayed_font_y_size;
			cur_x=x;
#endif // SKY_FPV_OPTIONAL
			i++;
#ifdef SKY_FPV_OPTIONAL
			current_lines++;
			if(current_lines>=max_lines)break;
			continue;
		} else if (is_color (cur_char)){
			glEnd();	//Ooops - NV bug fix!!
			glBegin(GL_QUADS);
		}
		chr=find_font_char(cur_char);
		if(chr >= 0){
			col=chr/FONT_CHARS_PER_LINE;
			row=chr%FONT_CHARS_PER_LINE;

			font_bit_width=get_font_width(chr);
			displayed_font_x_width=((float)font_bit_width)*displayed_font_x_size/12.0;
			ignored_bits=(12-font_bit_width)/2;	// how many bits on each side of the char are ignored?
			if(ignored_bits < 0)ignored_bits=0;

			//now get the texture coordinates
			u_start=(float)(row*FONT_X_SPACING+ignored_bits)/256.0f;
			u_end=(float)(row*FONT_X_SPACING+FONT_X_SPACING-7-ignored_bits)/256.0f;
#ifdef NEW_TEXTURES
			v_start=(float)(1+col*FONT_Y_SPACING)/256.0f;
			v_end=(float)(col*FONT_Y_SPACING+FONT_Y_SPACING-1)/256.0f;
#else
			v_start=(float)1.0f-(1+col*FONT_Y_SPACING)/256.0f;
			v_end=(float)1.0f-(col*FONT_Y_SPACING+FONT_Y_SPACING-1)/256.0f;
#endif //NEW_TEXTURES

			glTexCoord2f(u_start,v_start);
			glVertex3f(cur_x,cur_y+displayed_font_y_size,z);

			glTexCoord2f(u_start,v_end);
			glVertex3f(cur_x,cur_y,z);

			glTexCoord2f(u_end,v_end);
			glVertex3f(cur_x+displayed_font_x_width,cur_y,z);

			glTexCoord2f(u_end,v_start);
			glVertex3f(cur_x+displayed_font_x_width,cur_y+displayed_font_y_size,z);

			cur_x+=displayed_font_x_width;
		} else if(is_color (cur_char)){
			glEnd();	//Ooops - NV bug fix!!
			glBegin(GL_QUADS);
#endif // SKY_FPV_OPTIONAL
		}
#ifdef SKY_FPV_OPTIONAL
	i++;
	}
#endif // SKY_FPV_OPTIONAL

	glEnd();
	glDisable(GL_ALPHA_TEST);
}

void draw_ingame_string(float x, float y,const unsigned char * our_string,
						int max_lines, float font_x_scale, float font_y_scale)
{
	float u_start,u_end,v_start,v_end;
	int col,row;
	float displayed_font_x_size;
	float displayed_font_y_size;

	float displayed_font_x_width;
#ifndef SKY_FPV_OPTIONAL
	//int font_x_size=FONT_X_SPACING;
	//int font_y_size=FONT_Y_SPACING;
#endif // not SKY_FPV_OPTIONAL
	int	font_bit_width, ignored_bits;

	unsigned char cur_char;
	int chr;
	int i;
	float cur_x,cur_y;
	int current_lines=0;
#ifdef SKY_FPV_OPTIONAL
	double model[16], proj[16],hx,hy,hz;
	int view[4];

	displayed_font_x_size=font_x_scale*name_zoom*12.0*font_scale;
	displayed_font_y_size=font_y_scale*name_zoom*12.0*font_scale;

	glGetDoublev(GL_MODELVIEW_MATRIX, model);
	glGetDoublev(GL_PROJECTION_MATRIX, proj);
	glGetIntegerv(GL_VIEWPORT, view);
	gluProject((double)x,0.0f,(double)y,model, proj, view, &hx,&hy,&hz);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(view[0],view[2]+view[0],view[1],view[3]+view[1],0.0f,-1.0f);
#endif // SKY_FPV_OPTIONAL

#ifndef SKY_FPV_OPTIONAL
	displayed_font_x_size=font_x_scale*zoom_level*name_zoom/3.0;
	displayed_font_y_size=font_y_scale*zoom_level*name_zoom/3.0;
#endif // not SKY_FPV_OPTIONAL

	glEnable(GL_ALPHA_TEST);//enable alpha filtering, so we have some alpha key
	glAlphaFunc(GL_GREATER,0.1f);
#ifdef	NEW_TEXTURES
	bind_texture(font_text);
#else	/* NEW_TEXTURES */
	get_and_set_texture_id(font_text);
#endif	/* NEW_TEXTURES */

	i=0;
#ifndef SKY_FPV_OPTIONAL
	cur_x=x;
	cur_y=y;
#else // SKY_FPV_OPTIONAL
	cur_x=hx;
	cur_y=hy;
#endif // SKY_FPV_OPTIONAL
	glBegin(GL_QUADS);
#ifndef SKY_FPV_OPTIONAL
	while(1)
		{
			cur_char=our_string[i];
			if(!cur_char)
				{
					break;
				}
			else if(cur_char=='\n')
				{
					cur_y+=displayed_font_y_size;
					cur_x=x;
					i++;
					current_lines++;
					if(current_lines>=max_lines)break;
					continue;
				}
			else if(is_color (cur_char))
				{
					glEnd();	//Ooops - NV bug fix!!
				}
			chr=find_font_char(cur_char);
			if(chr >= 0)
				{
					col=chr/FONT_CHARS_PER_LINE;
					row=chr%FONT_CHARS_PER_LINE;

					font_bit_width=get_font_width(chr);
					displayed_font_x_width=((float)font_bit_width)*displayed_font_x_size/12.0;
					ignored_bits=(12-font_bit_width)/2;	// how many bits on each side of the char are ignored?
					if(ignored_bits < 0)ignored_bits=0;

					//now get the texture coordinates
					u_start=(float)(row*FONT_X_SPACING+ignored_bits)/256.0f;
					u_end=(float)(row*FONT_X_SPACING+FONT_X_SPACING-7-ignored_bits)/256.0f;
#ifdef NEW_TEXTURES
					v_start=(float)(1+col*FONT_Y_SPACING)/256.0f;
					v_end=(float)(col*FONT_Y_SPACING+FONT_Y_SPACING-1)/256.0f;
#else
					v_start=(float)1.0f-(1+col*FONT_Y_SPACING)/256.0f;
					v_end=(float)1.0f-(col*FONT_Y_SPACING+FONT_Y_SPACING-1)/256.0f;
#endif //NEW_TEXTURES

					glTexCoord2f(u_start,v_start);
					glVertex3f(cur_x,0,cur_y+displayed_font_y_size);

					glTexCoord2f(u_start,v_end);
					glVertex3f(cur_x,0,cur_y);

					glTexCoord2f(u_end,v_end);
					glVertex3f(cur_x+displayed_font_x_width,0,cur_y);

					glTexCoord2f(u_end,v_start);
					glVertex3f(cur_x+displayed_font_x_width,0,cur_y+displayed_font_y_size);

					cur_x+=displayed_font_x_width;
				}
			else if (is_color (cur_char))
				{
					glBegin(GL_QUADS);	//Ooops - NV bug fix!!
				}
#else // SKY_FPV_OPTIONAL
	while(1){
		cur_char=our_string[i];
		if(!cur_char){
			break;
		}else if(cur_char=='\n'){
			cur_y+=displayed_font_y_size;
			cur_x=hx;
#endif // SKY_FPV_OPTIONAL
			i++;
#ifdef SKY_FPV_OPTIONAL
			current_lines++;
			if(current_lines>=max_lines)break;
			continue;
		}
		else if (is_color (cur_char))
		{
			glEnd();	//Ooops - NV bug fix!!
			glBegin(GL_QUADS);
		}
		chr=find_font_char(cur_char);
		if(chr >= 0){
			col=chr/FONT_CHARS_PER_LINE;
			row=chr%FONT_CHARS_PER_LINE;

			font_bit_width=get_font_width(chr);
			displayed_font_x_width=((float)font_bit_width)*displayed_font_x_size/12.0;
			ignored_bits=(12-font_bit_width)/2;	// how many bits on each side of the char are ignored?
			if(ignored_bits < 0)ignored_bits=0;

			//now get the texture coordinates
			u_start=(float)(row*FONT_X_SPACING+ignored_bits)/256.0f;
			u_end=(float)(row*FONT_X_SPACING+FONT_X_SPACING-7-ignored_bits)/256.0f;
#ifdef NEW_TEXTURES
			v_start=(float)(1+col*FONT_Y_SPACING)/256.0f;
			v_end=(float)(col*FONT_Y_SPACING+FONT_Y_SPACING-1)/256.0f;
#else
			v_start=(float)1.0f-(1+col*FONT_Y_SPACING)/256.0f;
			v_end=(float)1.0f-(col*FONT_Y_SPACING+FONT_Y_SPACING-1)/256.0f;
#endif // NEW_TEXTURES

			glTexCoord2f(u_start,v_start);
			glVertex3f(cur_x,cur_y+displayed_font_y_size,0);

			glTexCoord2f(u_start,v_end);
			glVertex3f(cur_x,cur_y,0);

			glTexCoord2f(u_end,v_end);
			glVertex3f(cur_x+displayed_font_x_width,cur_y,0);

			glTexCoord2f(u_end,v_start);
			glVertex3f(cur_x+displayed_font_x_width,cur_y+displayed_font_y_size,0);

			cur_x+=displayed_font_x_width;
		}
		else if (is_color (cur_char))
		{
			glEnd();	//Ooops - NV bug fix!!
			glBegin(GL_QUADS);
#endif // SKY_FPV_OPTIONAL
		}
#ifdef SKY_FPV_OPTIONAL
		i++;
	}
#endif // SKY_FPV_OPTIONAL

	glEnd();
	glDisable(GL_ALPHA_TEST);
#ifdef SKY_FPV_OPTIONAL
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

#endif // SKY_FPV_OPTIONAL
}
#endif //!MAP_EDITOR_2
#endif	//ELC


#ifndef ENGLISH
//@tosh : optimisation de get_font_width
int get_font_width(int cur_char)
{
	int width=0;
	if(cur_char < 0)
	{
		switch(cur_char)
		{
			case -23: // é == e
				return (fonts[cur_font_num]->widths[101] + fonts[cur_font_num]->spacing);
			case -24: // è == e
				return (fonts[cur_font_num]->widths[101] + fonts[cur_font_num]->spacing);
			case -25: // ç == c
				return (fonts[cur_font_num]->widths[99] + fonts[cur_font_num]->spacing);
			case -32: // à == a
				return (fonts[cur_font_num]->widths[97] + fonts[cur_font_num]->spacing);
			case -7: // ù == u
				return (fonts[cur_font_num]->widths[117] + fonts[cur_font_num]->spacing);
			case -55: // É == E
			case -56: // È == E
				return (fonts[cur_font_num]->widths[69] + fonts[cur_font_num]->spacing);
			case -57: // Ç == C
				return (fonts[cur_font_num]->widths[67] + fonts[cur_font_num]->spacing);
			case -64: // À == A
				return (fonts[cur_font_num]->widths[65] + fonts[cur_font_num]->spacing);
			case -39: // Ù == U
				return (fonts[cur_font_num]->widths[85] + fonts[cur_font_num]->spacing);
		}
	}
	else
	{
		return (fonts[cur_font_num]->widths[cur_char] + fonts[cur_font_num]->spacing);
	}
	return width;
}
#else //ENGLISH
// font handling
int get_font_width(int cur_char)
{

    // ignore unknown characters
	if (cur_char < 0) {
		return 0;
	}
	// return width of character + spacing between chars (supports variable width fonts)
	return (fonts[cur_font_num]->widths[cur_char] + fonts[cur_font_num]->spacing);
}
#endif //ENGLISH
int get_char_width(unsigned char cur_char)
{
	return get_font_width(get_font_char(cur_char));
}


int get_string_width(const unsigned char *str)
{
	return get_nstring_width(str, strlen((char*)str));
}


int get_nstring_width(const unsigned char *str, int len)
{
	int	i, wdt=0;

	for(i=0; i<len; i++) {
		wdt+= get_char_width(str[i]);
	}

	// adjust to ignore the final spacing
	wdt-= fonts[cur_font_num]->spacing;

	return wdt;
}


int init_fonts ()
{
	int	i;

	max_fonts = 0;
	for(i = 0; i < FONTS_ARRAY_SIZE; i++) {
		fonts[i] = NULL;
		if (set_font_parameters (i) < 0) return 0;
	}

	cur_font_num = 0;


	return 1;
}

void cleanup_fonts(void)
{
	int i;

	for(i = 0; i < FONTS_ARRAY_SIZE; i++) {
		if(fonts[i] != NULL) {
			free(fonts[i]);
		}
	}
}

#ifndef	NEW_TEXTURES
void reload_fonts()
{
	int i;
	int poor_man_save=poor_man;
	int use_mipmaps_save=use_mipmaps;

	poor_man=0;
	use_mipmaps=0;

	for(i=0;i < FONTS_ARRAY_SIZE; i++){
		if(fonts[i] != NULL){
			if(fonts[i]->texture_id>=0){
				glDeleteTextures(1, (GLuint*)&texture_cache[fonts[i]->texture_id].texture_id);
				texture_cache[fonts[i]->texture_id].texture_id=0;
				get_texture_id(fonts[i]->texture_id);
			}
		}
	}

	poor_man=poor_man_save;
	use_mipmaps=use_mipmaps_save;
#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
}
#endif	/* NEW_TEXTURES */

static const char texture_dir[] = "textures/";
int load_font_textures ()
{
#ifndef	NEW_TEXTURES
	int poor_man_save=poor_man;
	int use_mipmaps_save=use_mipmaps;
#endif	/* NEW_TEXTURES */
	size_t i = 0;
	char *glob_pattern;
#ifdef WINDOWS
	struct _finddata_t c_file;
	long hFile;
#else //WINDOWS
	int ret;
	glob_t glob_res;
	size_t j;
#endif //WINDOWS
	char file[60] = "";
	char str[60] = "";

	if (fonts[0] == NULL || fonts[1] == NULL || fonts[2] == NULL || fonts[3]==NULL )
	{
		for (i = 0; i < FONTS_ARRAY_SIZE; i++) {
			if (fonts[i] != NULL)
				free (fonts[i]);
			fonts[i] = NULL;
		}
		if ( !init_fonts () ) return 0;
	}

#ifndef	NEW_TEXTURES
	poor_man=0;
	use_mipmaps=0;
#endif	/* NEW_TEXTURES */

#ifdef	NEW_TEXTURES
	fonts[0]->texture_id = load_texture_cached("textures/font.dds", tt_font);
#else	/* NEW_TEXTURES */
	fonts[0]->texture_id = load_texture_cache("./textures/font.bmp", 0);
#endif	/* NEW_TEXTURES */
	i = 1;
	// Force the selection of the base font.
	add_multi_option("chat_font", "Type 1");
	add_multi_option("name_font", "Type 1");
	// Find what font's exist and load them
	glob_pattern = malloc(strlen(datadir)+sizeof(texture_dir)+10+1); //+10 = font*.bmp*
#ifdef	NEW_TEXTURES
	sprintf(glob_pattern, "%s%sfont*.dds", datadir, texture_dir);
#else	/* NEW_TEXTURES */
	sprintf(glob_pattern, "%s%sfont*.bmp*", datadir, texture_dir);
#endif	/* NEW_TEXTURES */
#ifdef WINDOWS
	if( (hFile = _findfirst( glob_pattern, &c_file )) == -1L ){
		free(glob_pattern);
		return 0;
	}
	do {
		int	len;

		safe_strncpy(file, c_file.name, sizeof(file));
#else //!WINDOWS
	ret = glob(glob_pattern, 0, NULL, &glob_res);
	if(ret != 0) {
		LOG_ERROR("Unable to find any font textures\n");
		free(glob_pattern);
		return 0;
	}
	j = 0;
	while (j < glob_res.gl_pathc && i < FONTS_ARRAY_SIZE) {
		int	len;

		safe_strncpy(file, glob_res.gl_pathv[j]+sizeof(texture_dir)-1+strlen(datadir), sizeof(file));
#endif //WINDOWS
		len= strlen(file);
#ifdef	NEW_TEXTURES
		if (((len + sizeof(texture_dir) - 1) < sizeof(str)) && !strncasecmp(file, "font", 4)
			&& has_suffix(file, len, ".dds", 4))
		{
			safe_snprintf(str, sizeof(str), "./textures/%s", file); //Use a relative path here, load_texture_cache_deferred() is using the path wrappers.
			file[len - 4] = 0;
			fonts[i]->texture_id = load_texture_cached(str, tt_font);
#else	/* NEW_TEXTURES */
		if (len+sizeof(texture_dir)-1 < sizeof(str) && !strncasecmp(file, "font", 4)
				&& (has_suffix(file, len, ".bmp", 4) || has_suffix(file, len, ".bmp.gz", 7))
				&& (!has_suffix(file, len, "_alpha.bmp", 10)) && (!has_suffix(file, len, "_alpha.bmp.gz", 13))) {
			// Get the filename, remove the .bmp and add _alpha.bmp to a copy, then replace the .bmp
			safe_snprintf(str, sizeof(str), "./textures/%s", file); //Use a relative path here, load_texture_cache_deferred() is using the path wrappers.
			if(has_suffix(file, len, ".bmp.gz", 7)){
				file[len - 7]= 0;
			} else {
				file[len - 4]= 0;
			}
			fonts[i]->texture_id = load_texture_cache_deferred(str, 0);
#endif	/* NEW_TEXTURES */
			safe_snprintf(font_names[i], sizeof(font_names[i]), "Type %i - %s", i + 1, file);
			add_multi_option("chat_font", font_names[i]);
			add_multi_option("name_font", font_names[i]);
			i++;
		}
#ifndef WINDOWS
		j++;
#endif //WINDOWS
	}
#ifdef WINDOWS
	while ( _findnext( hFile, &c_file ) == 0 );
	_findclose( hFile );
#else //!WINDOWS
	globfree(&glob_res);
#endif //WINDOWS
	free(glob_pattern);

#ifndef	NEW_TEXTURES
	poor_man=poor_man_save;
	use_mipmaps=use_mipmaps_save;
#endif	/* NEW_TEXTURES */

	//set the default font
	cur_font_num = 0;
	font_text = fonts[0]->texture_id;

	return 1;
}

int set_font_parameters (int num)
{
	int	i;

	// error checking
	if(num < 0 || num >= FONTS_ARRAY_SIZE)
		{
			return -1;
		}
	// allocate space if needed
	if(fonts[num] == NULL)
		{
			fonts[num]=(font_info *)calloc(1, sizeof(font_info));
			if(fonts[num] == NULL)
				{
					LOG_ERROR(cant_load_font);
					return -1;
				}
		}
	//watch the highest font
	if(num >= max_fonts)
		{
			max_fonts=num+1;
		}
	// set default font info
	my_strcp (fonts[num]->name, "default");
	fonts[num]->spacing=0;

	// load font information
	// TODO: write this and remove the hack!
	if(num!=1||num!=2)for(i=0; i<FONTS_ARRAY_SIZE*FONT_CHARS_PER_LINE; i++) fonts[num]->widths[i]=12;
	if(num==1){
#ifdef ENGLISH
		static int widths[]={
			4,2,7,11,8,12,12,2,7,7,9,10,3,8,
			2,10,10,10,8,8,10,7,9,9,9,9,3,3,
			10,10,10,9,12,12,9,10,10,9,9,10,9,8,
			7,11,8,11,10,11,9,11,11,9,10,9,12,12,
			12,12,10,6,10,6,10,12,3,11,9,9,9,9,
			8,9,9,4,6,10,4,11,9,10,9,9,8,8,
			8,9,10,12,10,10,9,8,2,8,10,8,12,12,
			12,12,12,12,12,12,12,12,12,12,12,12,12,12,
			12,12,12,12,12,12,12,12,12,12,12,12,12,12,
			12,12,12,12,12,12,12,12,12,12,12,12,12,12,
		};
		fonts[num]->spacing=4;
#else //ENGLISH
		static int widths[]={
			 8,  8,  8, 10,  8, 10, 10,  8,  8,  8,  8, 10,  8,  8,
			 8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
			10, 10, 10,  8, 12, 10, 10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10,  8, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10,  8,  8,  8,  8,  8,  8,  8,
			10,  8,  8,  8,  8,  8,  8, 10,  8,  8,  8,  8,  8,  8,
			 8,  8,  8, 10,  8,  8,  8, 10,  8, 10, 10,  8, 10,  8,
			 8,  8, 10, 10, 10,  8, 10, 10,  8,  8,  8, 12, 12, 12,
			10, 10, 12, 10, 12, 12, 12,
		};
		fonts[num]->spacing=2;
#endif //ENGLISH
		memcpy(fonts[num]->widths, widths, sizeof(widths));
	}
	if(num==2){
		static int widths[]={
			 8,  8,  8, 10,  8, 10, 10,  8,  8,  8,  8, 10,  8,  8,
			 8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,
			10, 10, 10,  8, 12, 10, 10, 10, 10, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10, 10,  8, 10, 10, 10, 10, 10,
			10, 10, 10, 10, 10, 10, 10,  8,  8,  8,  8,  8,  8,  8,
			10,  8,  8,  8,  8,  8,  8, 10,  8,  8,  8,  8,  8,  8,
			 8,  8,  8, 10,  8,  8,  8, 10,  8, 10, 10,  8, 10,  8,
			 8,  8, 10, 10, 10,  8, 10, 10,  8,  8,  8, 12, 12, 12,
			10, 10, 12, 10, 12, 12, 12,
		};
		memcpy(fonts[num]->widths, widths, sizeof(widths));
		fonts[num]->spacing=2;
	}

	//and return
	return num;
}


int	set_font(int num)
{
	if(num >= 0 && num < max_fonts && fonts[num] && fonts[num]->texture_id >= 0)
		{
			cur_font_num=num;
			font_text=fonts[cur_font_num]->texture_id;
		}

	return cur_font_num;
}

/* currently UNUSED
void remove_font(int num)
{
	if(num < max_fonts && fonts[num])
		{
			free(fonts[num]);
			fonts[num]=NULL;
			if (num == max_fonts-1)
				{
					max_fonts--;
				}
		}
}
*/
#endif //FR_VERSION

