/*!
 * \file
 * \ingroup quickbars
 * \brief handling and displaying the items quickbar
 */
#ifndef	__FR_QUICKITEMS_H
#define	__FR_QUICKITEMS_H

#include "elwindows.h"
#include "items.h"

#ifdef __cplusplus
extern "C" {
#endif


#define FR_QUICKITEMS_MAXSIZE 36
extern item fr_quickitem_list[FR_QUICKITEMS_MAXSIZE];
extern int fr_quickitem_dragged;

extern int cm_quickbar_enabled;
extern int cm_quickbar_protected;
extern int fr_quickitems_insertmode;
extern int fr_quickitems_autocompress;
extern int fr_quickitems_autocomplete;
extern int fr_quickitems_autoremove;

extern int quickbar_x;
extern int quickbar_y;
extern int quickbar_x_len;
extern int quickbar_y_len;
extern int quickbar_dir;
extern int quickbar_on_top;
extern int quickbar_draggable;
extern int quickitems_size; /*!< current number of cases in the quickbar */


void init_fr_quickitems();
void resize_fr_quickitems();
int get_fr_quickitems_y_base();
void drag_quickitem(int item);

int display_fr_quickitems_handler(window_info *win);
int mouseover_fr_quickitems_handler(window_info *win, int mx, int my);
int click_fr_quickitems_handler(window_info *win, int mx, int my, Uint32 flags);

void toggle_fr_quickitems_draggable();
void flip_fr_quickitems();
void reset_fr_quickitems();

void sync_fr_quickitems();
void build_fr_quickitems(int force_complete);
int affect_fr_quickitems(int pos);


/*!
 * \brief Sauvegarde la barre rapide d'inventaire.
 *
 * \callgraph
*/
void load_fr_quickitems();

/*!
 * \brief Charge la barre rapide d'inventaire.
 *
 * \callgraph
*/
void save_fr_quickitems();



#ifdef __cplusplus
} // extern "C"
#endif

#endif
