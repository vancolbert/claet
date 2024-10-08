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

#ifndef ENGLISH
/*!
 * the _logdata structure is a linked list with a string as its data.
 */
typedef struct ld
{
	char *msg; /*!< the message to log */
	struct ld *Next; /*!< link to the element in the list. */
}_logdata;

typedef struct lq
{
	char      *Title;
	_logdata  *Data;
	_logdata  *Last;
	int       Count;
	int       Finish;
	struct lq *Next;
}_logquest;
#endif //ENGLISH

/*!
 * \name windows handlers
 */
/*! @{ */
extern int questlog_win; /*!< handle for the questlog window */
/*! @} */

extern int questlog_menu_x;
extern int questlog_menu_y;
#ifndef ENGLISH
extern int questlog_menu_x_len;
extern int questlog_menu_y_len;
#endif //ENGLISH

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

#ifdef ENGLISH
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
void fill_questlog_win ();
#endif //ENGLISH

#ifndef ENGLISH
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
void fill_questlog_win ();
#endif //ENGLISH

#ifdef NEW_QUESTLOG

/*!
 * \ingroup quest_window
 * \brief Sets the quest id for the next quest log entry which will be sent.
 *
 *      Sets the quest id for the next quest log entry which will be sent.
 *
 * \param id	the quest id
 *
 * \callgraph
 */
void set_next_quest_entry_id(Uint16 id);

/*!
 * \ingroup quest_window
 * \brief Set the title for the specified quest.
 *
 *      Set the title for the specified quest.
 *
 * \param data	pointer to non null terminated string
 * \param len	the length in bytes of the title
 *
 * \callgraph
 */
void set_quest_title(const char *data, int len);

/*!
 * \ingroup quest_window
 * \brief Set the specified quest as completed
 *
 *      Set the specified quest as completed, the user interface
 * shows completed and not completed quests differently.
 *
 * \param id	the quest id
 *
 * \callgraph
 */
void set_quest_finished(Uint16 id);


/*!
 * \ingroup quest_window
 * \brief Check if we have a quest id and waiting for an entry.
 *
 *      Check if we have a quest id and waiting for an entry.
 *
 * return true if if answer is yes.
 * \callgraph
 */
int waiting_for_questlog_entry(void);


/*!
 * \ingroup quest_window
 * \brief Clear the state that we are waiting for an entry.
 *
 *      Clear the state that we are waiting for an entry..
 *
 * \callgraph
 */
void clear_waiting_for_questlog_entry(void);


/*!
 * \ingroup quest_window
 * \brief Write the questlog options to the el.cfg structure.
 *
 *      Write the questlog options to the el.cfg structure.
 *
 * \callgraph
 */
unsigned int get_options_questlog(void);


/*!
 * \ingroup quest_window
 * \brief  Read the questlog options from the el.cfg structure.
 *
 *      Read the questlog options from the el.cfg structure.
 *
 * return true if if answer is yes.
 * \callgraph
 */
void set_options_questlog(unsigned int cfg_options);

#endif // NEW_QUESTLOG

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

#endif	//__QUESTLOG_H__
