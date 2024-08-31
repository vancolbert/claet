/*!
 * \file
 * \ingroup quest_window
 * \brief handles the data and the display of the quest log.
 */
#ifndef __QUESTLOG_H__
#define __QUESTLOG_H__
#ifdef __cplusplus
extern "C" {
#endif
/*!
 * the _logdata structure is a linked list with a string as its data.
 */
typedef struct ld {
	char *msg; /*!< the message to log */
	struct ld *Next; /*!< link to the element in the list. */
} _logdata;
typedef struct lq {
	char *Title;
	_logdata *Data;
	_logdata *Last;
	int Count;
	int Finish;
	struct lq *Next;
} _logquest;
/*!
 * \name windows handlers
 */
/*! @{ */
extern int questlog_win; /*!< handle for the questlog window */
/*! @} */
extern int questlog_menu_x;
extern int questlog_menu_y;
extern int questlog_menu_x_len;
extern int questlog_menu_y_len;
/*!
 * \ingroup quest_window
 * \brief Displays the questlog window
 *
 *      Displays the questlog window
 *
 * \callgraph
 */
void display_questlog();
/*!
 * \ingroup quest_window
 * \brief Loads the questlog from the users filesystem.
 *
 *      Loads the questlog from the users filesystem.
 *
 * \callgraph
 */
void load_questlog();
/*!
 * \ingroup quest_window
 * \brief Unloads the questlog and frees up the memory used.
 *
 *      Unloads the questlog and frees up the memory used.
 *
 */
void unload_questlog();
/*!
 * \ingroup quest_window
 * \brief Adds the log specified in t up to the specified length to the users questlog.
 *
 *      Adds the log specified in the parameter t up to the specified length len to the users questlog.
 *
 * \param t     the log to add
 * \param len   the length of t
 *
 * \callgraph
 */
void add_questlog(char *t, int len);
/*!
 * \ingroup quest_win
 * \brief adds the string t up to the given length as a new line to the questlog.
 *
 *      Adds the string t up to the given length as a new line to the questlog
 *
 * \param t     the log to add
 * \param len   the length of t
 *
 * \callgraph
 */
int add_questlog_line(char *t, int len);
int add_questlog_line2(char *q, char *t, int len, int finish);
/*!
 * \ingroup misc_utils
 * \brief string_fix
 *
 *      string_fix(char*,int)
 *
 * \param t
 * \param len
 */
void string_fix(char *t, int len);
/*!
 * \ingroup quest_window
 * \brief Goes to the entry in the questlog with the specified index.
 *
 *      Goes to the entry in the questlog with the specified index.
 *
 * \param ln    the index for the entry to search for.
 */
void goto_questlog_entry(int ln);
/*!
 * \ingroup quest_window
 * \brief Sets the window handler functions for the quest log window
 *
 *      Sets the window handler functions for the quest log window
 *
 * \callgraph
 */
void fill_questlog_win();
/*!
 * \ingroup quest_window
 * \brief  Draw a context menu like highlight.
 *
 *        Draw a context menu like highlight, i,e, shaded from top to bottom.
 *
 * \callgraph
 */
void draw_highlight(int topleftx, int toplefty, int widthx, int widthy, size_t col);
#ifdef __cplusplus
} // extern "C"
#endif
#endif  //__QUESTLOG_H__
