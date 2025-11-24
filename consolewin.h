/*!
 * \file
 * \ingroup interface_console
 * \brief Declares functions to create and handle the console window
 */
#ifndef __CONSOLE_WIN__
#define __CONSOLE_WIN__

#ifndef MAP_EDITOR
#include "widgets.h"
#endif
#ifdef FR_VERSION
#include "elwindows.h"
#endif //FR_VERSION

#ifdef __cplusplus
extern "C" {
#endif

#ifdef FR_VERSION
#define CONSOLE_SEP_HEIGHT	18
#endif //FR_VERSION

/*! \name windows handlers
 * @{ */
extern int console_root_win; /*!< handler for the console window */
/*! @} */

#ifdef FR_VERSION
extern int console_in_id; /*!< ID of the console input widget */
extern int console_out_id; /*!< ID of the console output widget */

extern int locked_to_console; /*!< indicates whether the console win is locked. Used when we don't have any maps. */
extern int nr_console_lines;
extern int console_text_width;

int display_console_handler (window_info *win);
extern int console_text_changed;
#else //FR_VERSION
extern int locked_to_console; /*!< indicates whether the console win is locked. Used when we don't have any maps. */
extern int console_scrollbar_enabled;  /*!< config option, if true, a scroll bar is displayed for the console */
#endif //FR_VERSION


void clear_console (void);
#ifdef ENGLISH
void toggle_console_scrollbar(int *enable);
void console_font_resize(float fond_size);
#endif //ENGLISH
int get_console_text_width(void);
int get_total_nr_lines(void);


/*!
 * \ingroup interface_console
 * \brief Signals the console window that the text buffer has changed
 *
 *      Signals the console window that the text buffer has changed
 *
 * \callgraph
 */
#ifdef FR_VERSION
void update_console_win ();
#else //FR_VERSION
void update_console_win ();
#endif //FR_VERSION

/*!
 * \ingroup interface_console
 * \brief Creates and initializes the console window
 *
 *      Creates and initializes the console window
 *
 * \param width the width of the window
 * \param height the height of the window
 * \callgraph
 */
void create_console_root_window (int width, int height);

#ifndef	MAP_EDITOR
int input_field_resize(widget_list *w, Uint32 x, Uint32 y);
#endif	//MAP_EDITOR

int history_grep (const char* text, int len);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // def __CONSOLE_WIN__
