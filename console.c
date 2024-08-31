#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "new_actors.h"
#include <dirent.h>
#include "console.h"
#include "asc.h"
#include "buddy.h"
#include "cache.h"
#include "chat.h"
#include "consolewin.h"
#include "elconfig.h"
#include "filter.h"
#include "gamewin.h"
#include "global.h"
#include "hud.h"
#include "ignore.h"
#include "icon_window.h"
#include "init.h"
#include "item_lists.h"
#include "interface.h"
#include "knowledge.h"
#include "list.h"
#include "mapwin.h"
#include "manufacture.h"
#include "misc.h"
#include "multiplayer.h"
#include "notepad.h"
#include "pm_log.h"
#include "platform.h"
#include "sound.h"
#include "spells.h"
#include "tabs.h"
#include "translate.h"
#include "url.h"
#include "command_queue.h"
#include "counters.h"
#include "map.h"
#include "minimap.h"
#include "errors.h"
#include "io/elpathwrapper.h"
#include "io/elfilewrapper.h"
#include "calc.h"
#include "text_aliases.h"
#include "encyclopedia.h"
#include "themes.h"
#include "pathfinder.h"
#include "fr_quickitems.h"
typedef char name_t[32];
char auto_open_encyclopedia = 1;
/* Pointer to the array holding the commands */
command_t *commands;
/* Array holding names we have seen (for completion) */
name_t *name_list = NULL;
/* Counts how many commands we have */
Uint16 command_count = 0;
Uint16 name_count = 0;
/* The command buffer head pointer */
list_node_t *command_buffer = NULL;
/* Pointer to our position in the buffer */
list_node_t *command_buffer_offset = NULL;
/* The input line before we started moving around in the buffer. */
char first_input[256] = {0};
int time_warn_h, time_warn_s, time_warn_d;
void add_line_to_history(const char *line, int len) {
	char *copy;
	copy = malloc(len + 1);
	safe_snprintf(copy, len + 1, "%.*s", len, line);
	list_push(&command_buffer, copy);
	command_buffer_offset = NULL;
}
char *history_get_line_up(void) {
	if (command_buffer_offset == NULL) {
		/* This is the first up keypress.
		 * Return the first line we have, if there's one and remember what we had. */
		command_buffer_offset = command_buffer;
		safe_snprintf(first_input, sizeof(first_input), "%.*s", input_text_line.len, input_text_line.data);
	} else if (command_buffer_offset->next != NULL) {
		command_buffer_offset = command_buffer_offset->next;
	}
	if (command_buffer_offset != NULL) {
		return command_buffer_offset->data;
	} else {
		return NULL;
	}
}
char *history_get_line_down(void) {
	if (command_buffer_offset != NULL) {
		command_buffer_offset = command_buffer_offset->prev;
		if (command_buffer_offset != NULL) {
			return command_buffer_offset->data;
		} else {
			/* We're at the bottom of the list, return what we initially had. */
			return first_input;
		}
	}
	return NULL;
}
void history_reset(void) {
	command_buffer_offset = NULL;
}
void history_destroy(void) {
	list_destroy(command_buffer);
}
void command_cleanup(void) {
	if (command_count > 0) {
		free(commands);
	}
	if (name_count > 0) {
		free(name_list);
	}
}
void add_command(const char *command, int (*callback)()) {
	static int commands_size = 0;
	int i;
	/* Check if the command already is in the list. */
	for (i = 0; i < command_count; i++) {
		if (strcasecmp(commands[i].command, command) == 0) {
			/* It exists, just update the callback and return */
			commands[i].callback = callback;
			return;
		}
	}
	if (command_count >= commands_size) {
		if (commands_size == 0) {
			commands_size += 10;
			commands = malloc(commands_size * sizeof(*commands));
		} else {
			commands_size *= 2;
			commands = realloc(commands, commands_size * sizeof(*commands));
		}
	}
	safe_snprintf(commands[command_count].command, sizeof(commands[command_count].command), "%s", command);
	commands[command_count].callback = callback;
	command_count++;
}
void add_name_to_tablist(const char *name) {
	static int list_size = 0;
	int i;
	for (i = 0; i < name_count; i++) {
		if (strcasecmp(name_list[i], name) == 0) {
			/* Name is already in the list, do nothing */
			return;
		}
	}
	if (name_count >= list_size) {
		if (list_size == 0) {
			list_size += 10;
		}
		list_size *= 2;
		name_list = realloc(name_list, list_size * sizeof(*name_list));
	}
	safe_snprintf(name_list[name_count], sizeof(name_list[name_count]), "%s", name);
	name_count++;
}
/* strmrchr: returns a pointer to the last occurence of c in s,
 * beginning the (reversed) search at begin */
const char *strmrchr(const char *s, const char *begin, int c) {
	char *copy = strdup(s);
	char *cbegin = copy + (begin - s);
	const char *result;
	*cbegin = '\0';
	if ((result = strrchr(copy, c))) {
		result = s + (result - copy);
	}
	free(copy);
	return result;
}
enum compl_type {
	COMMAND = 1, NAME, NAME_PM, CHANNEL,
};
struct compl_str {
	const char *str;
	enum compl_type type;
};
struct compl_str tab_complete(const text_message *input, unsigned int cursor_pos) {
	static char last_complete[48] = {0};
	static int have_last_complete = 0;
	static int last_str_count = -1;
	static enum compl_type last_type = NAME;
	struct compl_str return_value = {NULL, 0};
	if (input != NULL && input->len > 0 ) {
		const char *input_string = (char *)input->data;
		int count;
		short retries;
		node_t *step;
		if (!have_last_complete) {
			if (cursor_pos > 0 && strmrchr(input_string, input_string + cursor_pos - 1, ' ') != NULL) {
				/* If we have a space in the input string, we're pretty certain
				 * it's not a PM-name, command or channel name. */
				return_value.type = NAME;
			} else if (*input_string == '/' || *input_string == *char_slash_str) {
				return_value.type = NAME_PM;
			} else if (*input_string == '@' || *input_string == *char_at_str) {
				return_value.type = CHANNEL;
			} else if (*input_string == '#' || *input_string == *char_cmd_str) {
				return_value.type = COMMAND;
			} else {
				return_value.type = NAME;
			}
		} else {
			return_value.type = last_type;
		}
		switch (return_value.type) {
		case CHANNEL:
			input_string++;
		/* No break, increment twice for channel */
		case NAME_PM:
		case COMMAND:
			input_string++;
			break;
		case NAME: {
			const char *last_space = strmrchr(input_string, input_string + cursor_pos, ' ');
			if (last_space != NULL) {
				/* Update the cursor position to be relative to last_complete */
				cursor_pos -= last_space + 1 - input_string;
				input_string = last_space + 1;
			}
			break;
		}
		}
		if (!have_last_complete || (*input_string && strncasecmp(input_string, last_complete, strlen(last_complete)) != 0)) {
			/* New input string, start over */
			size_t i;
			last_str_count = -1;
			/* Isolate the word we're currently typing (and completing) */
			for (i = 0;
			     i < sizeof(last_complete) && input_string[i] && i < cursor_pos && !isspace((unsigned char)input_string[i]);
			     i++) {
				last_complete[i] = input_string[i];
			}
			last_complete[i] = '\0';
			have_last_complete = 1;
		}
		/* Look through the list */
		for (retries = 0; retries < 2 && !return_value.str; retries++) {
			size_t i;
			switch (return_value.type) {
			case NAME:
			case NAME_PM:
				for (i = 0, count = 0; i < name_count; i++) {
					if (strncasecmp(name_list[i], last_complete, strlen(last_complete)) == 0) {
						/* We have a match! */
						if (count > last_str_count) {
							/* This hasn't been returned yet, let's return it */
							last_str_count = count++;
							return_value.str = name_list[i];
							break;
						}
						count++;
					}
				}
				break;
			case CHANNEL:
				for (step = queue_front_node(chan_name_queue), count = 0; step->next != NULL; step = step->next) {
					if (strncasecmp(((chan_name *)(step->data))->name, last_complete, strlen(last_complete)) == 0) {
						/* Yay! The chan-name begins with the string we're searching for. */
						if (count > last_str_count) {
							/* We found something we haven't returned earlier, let's return it. */
							last_str_count = count++;
							return_value.str = ((chan_name *)(step->data))->name;
							break;
						}
						count++;
					}
				}
				break;
			case COMMAND:
			default:
				for (i = 0, count = 0; i < command_count; i++) {
					if (strncasecmp(commands[i].command, last_complete, strlen(last_complete)) == 0) {
						/* Yay! The command begins with the string we're searching for. */
						if (count > last_str_count) {
							/* We found something we haven't returned earlier, let's return it. */
							last_str_count = count++;
							return_value.str = commands[i].command;
							break;
						}
						count++;
					}
				}
				break;
			}
			if (!return_value.str && count) {
				/* We checked the whole list and found something, but not
				 * anything we haven't returned earlier. Let's start from the beginning again. */
				last_str_count = -1;
			}
		}
		last_type = return_value.type;
	} else {
		have_last_complete = 0;
		*last_complete = '\0';
		last_str_count = -1;
	}
	return return_value;
}
void do_tab_complete(text_message *input) {
	text_field *tf = input_widget->widget_info;
	struct compl_str completed = tab_complete(input, tf->cursor);
	if (completed.str != NULL) {
		size_t len;
		size_t i;
		/* Find the length of the data we're removing */
		if (completed.type == NAME) {
			const char *last_space = strmrchr(input->data, input->data + tf->cursor, ' ');
			/* Name is a bit special because it can be anywhere in a string,
			 * not just at the beginning like the other types. */
			len = input->data + tf->cursor - 1 - (last_space ? last_space : (input->data - 1));
		} else if (completed.type == CHANNEL) {
			len = tf->cursor - 2;
		} else {
			len = tf->cursor - 1;
		}
		/* Erase the current input word */
		for (i = tf->cursor; i <= input->len; i++) {
			input->data[i - len] = input->data[i];
		}
		input->len -= len;
		tf->cursor -= len;
		paste_in_input_field((unsigned char *)completed.str);
		input->len = strlen(input->data);
	}
}
void reset_tab_completer(void) {
	tab_complete(NULL, 0);
}
int test_for_console_command(char *text, int length) {
	int i;
	int ptr_length = length;
	char *text_ptr = text;
	// Skip leading #s
	if (*text == '#' || *text == char_cmd_str[0]) {
		*text = char_cmd_str[0];
		text++;
		length--;
	}
	//Skip leading spaces
	for (; isspace(*text); text++, length--) {}
	//Check if there's anything left of the command
	if (length <= 0 || (*text == '@' && length <= 1)) {
		return 0;
	} else {
		int cmd_len;
		/* Handle numeric shortcuts */
		if ( isdigit(text[0])) {
			if ( process_text_alias(text, length) >= 0 ) {
				return 1;
			}
		}
		/* Look for a matching command */
		for (i = 0; i < command_count; i++) {
			cmd_len = strlen(commands[i].command);
			//@tosh : permet de différencier une commande d'un alias (exemple : &alias 1 salut)
			if (strlen(text) >= cmd_len && my_strncompare(&text_ptr[1], commands[i].command, cmd_len) && (isspace(text[cmd_len]) || text[cmd_len] == '\0')) {
				/* Command matched */
				if (commands[i].callback && commands[i].callback(text + cmd_len, length - cmd_len)) {
					/* The command was handled and we don't want to send it to the server */
					return 1;
				} else {
					/* the command wants to be sent to the server */
					break;
				}
			}
		}
	}
	// on change le caractere pour les commandes afin de l'envoyer au serveur
	text--;
	*text = '#';
	send_input_text_line(text_ptr, ptr_length);
	// on remet le bon caractere pour les commandes
	*text = char_cmd_str[0];
	return 0;
}
// -------- COMMAND CALLBACKS -------- //
// Return 0 if you want the string to be sent to the server.
// the first argument passed is the input string without the command itself
// if ie. '#filter foo' is passed to test_for_console_command(), only ' foo' is passed to the callback.
/* given the command text, return the start of any parameter text */
/* e.g. find first space, then skip any spaces */
static char *getparams(char *text) {
	while (*text && !isspace(*text)) {
		text++;
	}
	while (*text && isspace(*text)) {
		text++;
	}
	return text;
}
int command_cls(char *text, int len) {
	clear_display_text_buffer();
	return 1;
}
int command_muet(void) {
	disable_sound(&no_sound);
	return 1;
}
int command_calc(char *text, int len) {
	double res;
	char str[100];
	int calcerr;
	res = calc_exp(text);
	calcerr = calc_geterror();
	switch (calcerr) {
	case CALCERR_OK:
		if (trunc(res) == res) {
			safe_snprintf(str, sizeof(str), "%s = %.0f", text, res);
		} else {
			safe_snprintf(str, sizeof(str), "%s = %.2f", text, res);
		}
		LOG_TO_CONSOLE(c_orange1, str);
		break;
	case CALCERR_SYNTAX:
		safe_snprintf(str, sizeof(str), "%s = Erreur de syntaxe", text);
		LOG_TO_CONSOLE(c_orange1, str);
		break;
	case CALCERR_DIVIDE:
		safe_snprintf(str, sizeof(str), "%s = Division par zéro", text);
		LOG_TO_CONSOLE(c_orange1, str);
		break;
	case CALCERR_MEM:
		safe_snprintf(str, sizeof(str), "%s = Erreur de mémoire", text);
		LOG_TO_CONSOLE(c_orange1, str);
		break;
	case CALCERR_XOPSYNTAX:
		safe_snprintf(str, sizeof(str), "%s = Mauvais argument pour X", text);
		LOG_TO_CONSOLE(c_orange1, str);
		break;
	case CALCERR_LOPSYNTAX:
		safe_snprintf(str, sizeof(str), "%s = Mauvais argument pour L", text);
		LOG_TO_CONSOLE(c_orange1, str);
		break;
	}
	return 1;
}
int command_markpos(char *text, int len) {
	int map_x, map_y;
	char *ptr = text;
	char msg[512];
	const char *usage = help_cmd_markpos_str;
	while (isspace(*ptr)) {
		ptr++;
	}
	if (sscanf(ptr, "%d,%d ", &map_x, &map_y) != 2) {
		LOG_TO_CONSOLE(c_red2, usage);
		return 1;
	}
	while (*ptr != ' ' && *ptr) {
		ptr++;
	}
	while (*ptr == ' ') {
		ptr++;
	}
	if (!*ptr) {
		LOG_TO_CONSOLE(c_red2, usage);
		return 1;
	}
	if (put_mark_on_position(map_x, map_y, ptr)) {
		safe_snprintf(msg, sizeof(msg), location_info_str, map_x, map_y, ptr);
		LOG_TO_CONSOLE(c_orange1, msg);
	} else {
		safe_snprintf(msg, sizeof(msg), invalid_location_str, map_x, map_y);
		LOG_TO_CONSOLE(c_red2, msg);
	}
	return 1;
}
int command_mark(char *text, int len) {
	if (strlen(text) > 1) { //check for empty marks
		char str[512];
		for (; isspace(*text); text++) {}
		if (strlen(text) > 0) {
			if (put_mark_on_current_position(text)) {
				safe_snprintf(str, sizeof(str), marked_str, text);
				LOG_TO_CONSOLE(c_orange1, str);
			}
		}
	}
	return 1;
}
int command_unmark_special(char *text, int len, int do_log) {
	int i;
	while (isspace(*text)) {
		text++;
	}
	if (*text) {
		for (i = 0; i < max_mark; i++) {
			if (my_strcompare(marks[i].text, text) && (marks[i].x != -1) && !marks[i].server_side) {
				char str[512];
				marks[i].x = marks[i].y = -1;
				if (do_log) {
					safe_snprintf(str, sizeof(str), unmarked_str, marks[i].text);
					LOG_TO_CONSOLE(c_orange1, str);
				}
				save_markings();
				load_map_marks(); // simply to compact the array and make room for new marks
				break;
			}
		}
	}
	return 1;
}
int command_unmark(char *text, int len) {
	return command_unmark_special(text, len, 1);
}
int command_mark_color(char *text, int len) {
	char str[512];
	while (isspace(*text)) {
		text++;
	}
	if (*text) {
		int r = -1, g, b;
		if (sscanf(text, "%d %d %d", &r, &g, &b) == 3) {
			if (!(r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255)) {
				r = -1;                                       //don't set color
			}
		} else {
			if (strcasecmp(text, "red") == 0) {
				r = 255;
				g = 0;
				b = 0;
			} else if (strcasecmp(text, "blue") == 0) {
				r = 0;
				g = 0;
				b = 255;
			} else if (strcasecmp(text, "green") == 0) {
				r = 0;
				g = 255;
				b = 0;
			} else if (strcasecmp(text, "yellow") == 0) {
				r = 255;
				g = 255;
				b = 0;
			} else if (strcasecmp(text, "cyan") == 0) {
				r = 0;
				g = 255;
				b = 255;
			} else if (strcasecmp(text, "magenta") == 0) {
				r = 255;
				g = 0;
				b = 255;
			} else if (strcasecmp(text, "white") == 0) {
				r = 255;
				g = 255;
				b = 255;
			}
		}
		if (r > -1) {
			//set color
			curmark_r = r;
			curmark_g = g;
			curmark_b = b;
		}
	}
	safe_snprintf(str, sizeof(str), "La couleur du marqueur choisie est (RVB): %d %d %d", curmark_r, curmark_g, curmark_b);
	LOG_TO_CONSOLE(c_orange1, str);
	return 1;
}
int command_stats(char *text, int len) {
	unsigned char protocol_name;
	protocol_name = SERVER_STATS;
	my_tcp_send(my_socket, &protocol_name, 1);
	return 1;
}
int command_time(char *text, int len) {
	unsigned char protocol_name;
	protocol_name = GET_TIME;
	my_tcp_send(my_socket, &protocol_name, 1);
	return 1;
}
int command_ping(char *text, int len) {
	Uint8 str[8];
	str[0] = PING;
	*((Uint32 *)(str + 1)) = SDL_SwapLE32(SDL_GetTicks());
	my_tcp_send(my_socket, str, 5);
	return 1;
}
int command_date(char *text, int len) {
	unsigned char protocol_name;
	protocol_name = GET_DATE;
	my_tcp_send(my_socket, &protocol_name, 1);
	return 1;
}
int command_quit(char *text, int len) {
	exit_now = 1;
	return 1;
}
int command_quitter(char *text, int len) {
	confirmation_quitter();
	return 1;
}
int command_mem(char *text, int len) {
	cache_dump_sizes(cache_system);
	return 1;
}
int command_ver(char *text, int len) {
	char str[250];
	print_version_string(str, sizeof(str));
	LOG_TO_CONSOLE(c_green1, str);
	return 1;
}
int command_ignore(char *text, int len) {
	char name[MAX_USERNAME_LENGTH];
	int i;
	Uint8 ch = '\0';
	int result;
	char ignore_type = IGN_ALL;
	char *arg;
	//on extrait les éventuels arguments
	while (isspace(*text)) {
		text++;
	}
	if ((arg = strchr(text, ' ')) != NULL) {
		*arg = 0;
		if (!strcmp(text, "tout")) {
			ignore_type = IGN_ALL;
		} else if (!strcmp(text, "mp") || !strcmp(text, "MP")) {
			ignore_type = IGN_MP;
		} else if (!strcmp(text, "canal") || !strcmp(text, "canaux")) {
			ignore_type = IGN_CANAUX;
		} else {
			char str[100] = "Mauvais argument ! Arguments valables : tout, mp ou MP, canal ou canaux.";
			LOG_TO_CONSOLE(c_red1, str);
			return 1;
		}
		text = arg + 1;
		while (isspace(*text)) {
			text++;
		}
	}
	for (i = 0; i < MAX_USERNAME_LENGTH - 1; i++) {
		ch = text[i];
		if (ch == ' ' || ch == '\0') {
			ch = '\0';
			break;
		}
		name[i] = ch;
	}
	name[i] = '\0';
	if (i >= MAX_USERNAME_LENGTH - 1 && text[i] != '\0') { // This is the max chrs of name but isn't a null terminator
		char str[100];
		safe_snprintf(str, sizeof(str), "%s %s", name_too_long, not_added_to_ignores);
		LOG_TO_CONSOLE(c_red1, str);
		return 1;
	}
	if (i < 3) {
		char str[100];
		safe_snprintf(str, sizeof(str), "%s %s", name_too_short, not_added_to_ignores);
		LOG_TO_CONSOLE(c_red1, name_too_short);
		return 1;
	}
	result = add_to_ignore_list(name, save_ignores, ignore_type);
	if (result == -1) {
		char str[100];
		safe_snprintf(str, sizeof(str), already_ignoring, name);
		LOG_TO_CONSOLE(c_red1, str);
		return 1;
	}
	if (result == -2) {
		LOG_TO_CONSOLE(c_red1, ignore_list_full);
	} else {
		char str[100];
		safe_snprintf(str, sizeof(str), added_to_ignores, name);
		LOG_TO_CONSOLE(c_green1, str);
	}
	return 1;
}
int command_filter(char *text, int len) {
	char name[256];
	char str[100];
	int i;
	Uint8 ch = '\0';
	int result;
	while (isspace(*text)) {
		text++;
	}
	for (i = 0; i < sizeof(name) - 1; i++) {
		ch = text[i];
		if (ch == '\0') {
			break;
		}
		name[i] = ch;
	}
	name[i] = '\0';
	if (i >= sizeof(name) - 1 && ch != '\0') {
		safe_snprintf(str, sizeof(str), "%s %s", word_too_long, not_added_to_filter);
		LOG_TO_CONSOLE(c_red1, str);
		return 1;
	} else if (i < 3) {
		safe_snprintf(str, sizeof(str), "%s %s", word_too_short, not_added_to_filter);
		LOG_TO_CONSOLE(c_red1, word_too_short);
		return 1;
	}
	result = add_to_filter_list(name, 1, save_ignores);
	if (result == -1) {
		safe_snprintf(str, sizeof(str), already_filtering, name);
		LOG_TO_CONSOLE(c_red1, str);
	} else if (result == -2) {
		LOG_TO_CONSOLE(c_red1, filter_list_full);
	} else {
		safe_snprintf(str, sizeof(str), added_to_filters, name);
		LOG_TO_CONSOLE(c_green1, str);
	}
	return 1;
}
int command_unignore(char *text, int len) {
	char name[MAX_USERNAME_LENGTH];
	char str[200];
	int i;
	Uint8 ch = '\0';
	int result;
	while (isspace(*text)) {
		text++;
	}
	for (i = 0; i < MAX_USERNAME_LENGTH - 1; i++) {
		ch = text[i];
		if (ch == ' ' || ch == '\0') {
			break;
		}
		name[i] = ch;
	}
	name[i] = '\0';
	if (i >= MAX_USERNAME_LENGTH - 1 && text[i] != '\0') { // This is the max chrs of name but isn't a null terminator
		safe_snprintf(str, sizeof(str), "%s %s", name_too_long, not_removed_from_ignores);
		LOG_TO_CONSOLE(c_red1, str);
		return 1;
	}
	if (i < 3) {
		safe_snprintf(str, sizeof(str), "%s %s", name_too_short, not_removed_from_filter);
		LOG_TO_CONSOLE(c_red1, str);
		return 1;
	}
	result = remove_from_ignore_list(name);
	if (result == -1) {
		safe_snprintf(str, sizeof(str), not_ignoring, name);
		LOG_TO_CONSOLE(c_red1, str);
	} else {
		safe_snprintf(str, sizeof(str), removed_from_ignores, name);
		LOG_TO_CONSOLE(c_green1, str);
	}
	return 1;
}
int command_unfilter(char *text, int len) {
	char name[64];
	char str[200];
	int i;
	Uint8 ch = '\0';
	int result;
	while (isspace(*text)) {
		text++;
	}
	for (i = 0; i < sizeof(name) - 1; i++) {
		ch = text[i];
		if (ch == '\0') {
			break;
		}
		name[i] = ch;
	}
	name[i] = '\0';
	if (i >= sizeof(name) - 1 && ch != '\0') {
		safe_snprintf(str, sizeof(str), "%s %s", word_too_long, not_removed_from_filter);
		LOG_TO_CONSOLE(c_red1, str);
		return 1;
	}
	if (i < 3) {
		safe_snprintf(str, sizeof(str), "%s %s", word_too_short, not_removed_from_filter);
		LOG_TO_CONSOLE(c_red1, str);
		return 1;
	}
	result = remove_from_filter_list(name);
	if (result == -1) {
		safe_snprintf(str, sizeof(str), not_filtering, name);
		LOG_TO_CONSOLE(c_red1, str);
	} else {
		safe_snprintf(str, sizeof(str), removed_from_filter, name);
		LOG_TO_CONSOLE(c_green1, str);
	}
	return 1;
}
int command_glinfo(const char *text, int len) {
	const char *my_string;
	size_t size = 8192, minlen;
	char *this_string = calloc(size, 1);
	my_string = (const char *)glGetString(GL_RENDERER);
	minlen = strlen(video_card_str) + strlen(my_string) + 3;
	if (size < minlen) {
		while (size < minlen) {
			size += size;
		}
		this_string = realloc(this_string, size);
	}
	safe_snprintf(this_string, size, "%s: %s", video_card_str, my_string);
	LOG_TO_CONSOLE(c_red2, this_string);
	my_string = (const char *)glGetString(GL_VENDOR);
	minlen = strlen(video_vendor_str) + strlen(my_string) + 3;
	if (size < minlen) {
		while (size < minlen) {
			size += size;
		}
		this_string = realloc(this_string, size);
	}
	safe_snprintf(this_string, size, "%s: %s", video_vendor_str, my_string);
	LOG_TO_CONSOLE(c_yellow3, this_string);
	my_string = (const char *)glGetString(GL_VERSION);
	minlen = strlen(opengl_version_str) + strlen(my_string) + 3;
	if (size < minlen) {
		while (size < minlen) {
			size += size;
		}
		this_string = realloc(this_string, size);
	}
	safe_snprintf(this_string, size, "%s: %s", opengl_version_str, my_string);
	LOG_TO_CONSOLE(c_yellow2, this_string);
	my_string = (const char *)glGetString(GL_EXTENSIONS);
	minlen = strlen(supported_extensions_str) + strlen(my_string) + 3;
	if (size < minlen) {
		while (size < minlen) {
			size += size;
		}
		this_string = realloc(this_string, size);
	}
	safe_snprintf(this_string, size, "%s: %s", supported_extensions_str, my_string);
	LOG_TO_CONSOLE(c_grey1, this_string);
	free(this_string);
	return 1;
}
/*  Display book names that match the specified string, or all if
 *  no string specified.  Highlighing the books that have been read.
 */
int knowledge_command(char *text, int len) {
	char this_string[80], count_str[60];
	char *cr;
	int num_read = 0, num_total = 0;
	int show_read = 0, show_unread = 0, show_help = 0;
	size_t i;
	char *pstr[3] = {knowledge_param_read, knowledge_param_unread, knowledge_param_total};
	size_t plen[3] = {strlen(knowledge_param_read), strlen(knowledge_param_unread), strlen(knowledge_param_total)};
	// find first space, then skip any spaces
	text = getparams(text);
	// use the short form of the params (-r -u -t) if valid and different
	if ((plen[0] > 1) && (plen[1] > 1) && (plen[2] > 1) && (pstr[0][0] == '-') && (pstr[1][0] == '-') && (pstr[2][0] == '-') && (pstr[0][1] != pstr[1][1]) && (pstr[0][1] != pstr[2][1]) && (pstr[1][1] != pstr[2][1])) {
		plen[0] = plen[1] = plen[2] = 2;
	}
	// show the help if no paramaters specified
	if (strlen(text) == 0) {
		show_help = 1;
	}
	// Look for -read, -unread or -total paramaters and vary the output appropriately
	else if (strncmp(text, knowledge_param_read, plen[0]) == 0) {
		show_read = 1;
		text = getparams(text + plen[0]);
	} else if (strncmp(text, knowledge_param_unread, plen[1]) == 0) {
		show_unread = 1;
		text = getparams(text + plen[1]);
	} else if (strncmp(text, knowledge_param_total, plen[2]) == 0) {
		show_read = show_unread = 1;
		text = getparams(text + plen[1]);
		text = getparams(text + plen[2]);
	}
	if (show_read && !show_unread) {
		LOG_TO_CONSOLE(c_green2, connaissances_acquises_str);
	} else if (show_unread && !show_read) {
		LOG_TO_CONSOLE(c_green2, connaissances_non_acquises_str);
	} else {
		LOG_TO_CONSOLE(c_green2, connaissances_str);
	}
	for (i = 0; i < KNOWLEDGE_LIST_SIZE; i++) {
		// only display books that contain the specified parameter string
		// shows all books if no string specified
		if ((strlen(knowledge_list[i].name) > 0) && (get_string_occurance(text, knowledge_list[i].name, strlen(knowledge_list[i].name), 1) != -1) && knowledge_list[i].affiche == 1) {
			// remove any trailing carrage return
			safe_strncpy(this_string, knowledge_list[i].name, sizeof(this_string));
			if ((cr = strchr(this_string, '\n')) != NULL) {
				*cr = '\0';
			}
			// highlight books that have been read
			if (knowledge_list[i].present) {
				if (show_read) {
					LOG_TO_CONSOLE(c_grey1, this_string);
				}
				++num_read;
			} else if (show_unread) {
				LOG_TO_CONSOLE(c_grey2, this_string);
			}
			++num_total;
		}
	}
	safe_snprintf(count_str, sizeof(count_str), book_count_str, num_read, num_total);
	LOG_TO_CONSOLE(c_grey1, count_str);
	// give help only if no parameters specified
	if (show_help) {
		LOG_TO_CONSOLE(c_grey1, know_help_str);
	}
	return 1;
}
int command_log_conn_data(char *text, int len) {
	if (!log_conn_data) {
		LOG_TO_CONSOLE(c_grey1, logconn_str);
		log_conn_data = 1;
	} else {
		log_conn_data = 0;
	}
	return 1;
}
// TODO: make this automatic or a better command, m is too short
int command_msg(char *text, int len) {
	int no;//, m=-1;
	// find first space, then skip any spaces
	text = getparams(text);
	if (my_strncompare(text, "tout", 4)) {
		for (no = 0; no < pm_log.ppl; no++) {
			print_message(no);
		}
	} else {
		no = atoi(text) - 1;
		if (no < pm_log.ppl && no >= 0) {
			print_message(no);
		}
	}
	return 1;
}
int command_afk(char *text, int len) {
	// find first space, then skip any spaces
	while (*text && !isspace(*text)) {
		text++;
		len--;
	}
	while (*text && isspace(*text)) {
		text++;
		len--;
	}
	if (!afk) {
		if (len > 0) {
			safe_snprintf(afk_message, sizeof(afk_message), "%.*s", len, text);
		}
		go_afk();
		last_action_time = cur_time - afk_time - 1;
	} else {
		go_ifk();
	}
	return 1;
}
int command_help(char *text, int len) {
	// help can open the Enc!
	if (auto_open_encyclopedia) {
		view_tab(&tab_help_win, &tab_help_collection_id, HELP_TAB_HELP);
	}
	// this use to return 0 - to fall thru and send it to the server
	// but the server does not handle the command and Entropy says it never did
	return 1;
}
int command_storage(char *text, int len) {
	int i;
	storage_filter[0] = '\0';
	for (i = 0; i < len; i++) {
		if (text[i] == ' ') {
			break;
		}
	}
	if (i < len) {
		int nb = len - i - 1;
		if (nb > sizeof(storage_filter) - 1) {
			nb = sizeof(storage_filter) - 1;
		}
		my_strncp(storage_filter, text + i + 1, nb + 1);
	}
	if (have_storage_list) {
		int size = strlen((char *)cached_storage_list) + 1;
		unsigned char cached_storage_copy[sizeof(cached_storage_list)];
		unsigned char *endl;
		memcpy(cached_storage_copy, cached_storage_list, size);
		endl = (unsigned char *)strchr((char *)cached_storage_copy, '\n');
		if (endl == NULL) {
			// No newline? Our cached list isn't correct.
			return 0;
		}
		if (storage_filter[0] != '\0') {
			size = filter_storage_text((char *)endl + 1, size, size);  //Note: filter from the first newline, which is where the item list starts
			size += (endl - cached_storage_copy + 1);
		}
		put_text_in_buffer(CHAT_SERVER, cached_storage_copy, size);
		return 1;
	}
	return 0;
}
int command_accept_buddy(char *text, int len) {
	/* This command is here to make sure the requests queue is up to date */
	text = getparams(text);
	/* Make sure a name is given */
	if (*text && !queue_isempty(buddy_request_queue)) {
		node_t *node = queue_front_node(buddy_request_queue);
		/* Search for the node in the queue */
		while (node != NULL) {
			if (strcasecmp(text, node->data) == 0) {
				/* This is the node we're looking for, delete it */
				queue_delete_node(buddy_request_queue, node);
				break;
			}
			node = node->next;
		}
	}
	return 0;
}
/* open the specified url in the configured browser */
static int command_open_url(char *text, int len) {
	text = getparams(text);
	if (*text) {
		open_web_link(text);
	}
	return 1;
}
/* set the command queues wait time between commands */
static int command_set_user_menu_wait_time_ms(char *text, int len) {
	text = getparams(text);
	if (*text) {
		set_command_queue_wait_time_ms(atol(text));
	} else {
		set_command_queue_wait_time_ms(0);
	}
	return 1;
}
/* parse the command parameters for a spell message, then cast it */
static int command_cast_spell(char *text, int len) {
	int index = 0;
	int valid_looking_message = 1;
	Uint8 str[30];
	/* valid messages start with the CAST_SPELL message of 39 or 0x27 */
	text = getparams(text);
	if (!*text || strstr(text, "27") == NULL) {
		valid_looking_message = 0;
	}
	/* skip past everything until the CAST_SPELL message type */
	else {
		text = strstr(text, "27");
	}
	/* while we have hex digit pairs to process */
	while (valid_looking_message && strlen(text) > 0 && index < 30) {
		int i;
		Uint8 d[2];
		while (*text == ' ') {
			text++;
		}
		if (strlen(text) < 2) {
			break;
		}
		for (i = 0; i < 2; i++) {
			d[i] = *text++;
			if (d[i] >= '0' && d[i] <= '9') {
				d[i] -= '0';
			} else if (d[i] >= 'a' && d[i] <= 'f') {
				d[i] -= 'a' - 10;
			} else if (d[i] >= 'A' && d[i] <= 'F') {
				d[i] -= 'A' - 10;
			} else {
				valid_looking_message = 0;
				break;
			}
		}
		/* store the spell message byte */
		if (valid_looking_message) {
			str[index++] = d[1] + 16 * d[0];
		}
	}
	/* if we're now at the end of the text, we have some message bytes and it looks valid */
	if (!*text && index && valid_looking_message) {
		send_spell(str, index);
	} else {
		LOG_TO_CONSOLE(c_red2, invalid_spell_string_str);
	}
	return 1;
}
/* show the last spell name and message bytes */
static int command_show_spell(char *text, int len) {
	int i;
	char out_str[128];
	char mess_str[64];
	/* trap if we have no last spell or other invalid strings */
	if (!*last_spell_name || strlen(last_spell_name) > 59 || last_spell_len > 30 || last_spell_len <= 0) {
		LOG_TO_CONSOLE(c_green2, no_spell_to_show_str);
		return 1;
	}
	/* create the message body string, each byte in hex */
	for (i = 0; i < last_spell_len; i++) {
		sprintf(&mess_str[2 * i], "%02x", last_spell_str[i]);
	}
	mess_str[last_spell_len * 2] = 0;
	safe_snprintf(out_str, sizeof(out_str), "%s %s", last_spell_name, mess_str);
	LOG_TO_CONSOLE(c_green2, out_str);
	return 1;
}
/* display or test the md5sum of the current map or the specified file */
int command_ckdata(char *text, int len) {
	const int DIGEST_LEN = 16;
	Uint8 digest[DIGEST_LEN];
	char digest_str[DIGEST_LEN * 2 + 1];
	char expected_digest_str[DIGEST_LEN * 2 + 1];
	char result_str[256];
	char filename[256];
	/* paramters are optional, first is expected checksum value, second is filename */
	/* if only a filename is specfied, we display checksum rather than do match */
	filename[0] = digest_str[0] = expected_digest_str[0] = '\0';
	text = getparams(text);
	if (*text) {
		/* if we have at least one space and the first string is of digest length, assume we matching */
		char *tempstr = safe_strcasestr(text, strlen(text), " ", 1);
		if ((tempstr != NULL) && (strlen(text) - strlen(tempstr) == DIGEST_LEN * 2)) {
			safe_strncpy2(expected_digest_str, text, DIGEST_LEN * 2 + 1, DIGEST_LEN * 2);
			/* trim leading space from filename */
			while (*tempstr == ' ') {
				tempstr++;
			}
			if (*tempstr) {
				safe_strncpy(filename, tempstr, 256);
			}
		}
		/* else we only have a filename */
		else {
			safe_strncpy(filename, text, 256);
		}
	}
	/* if no parameters default to current map elm file */
	//@tosh : On vérifie la valeur de cur_map pour éviter le plantage.
	else if (cur_map == -1) {
		LOG_TO_CONSOLE(c_red1, "Carte inconnue.");
		return 1;
	} else {
		safe_strncpy(filename, continent_maps[cur_map].name, 256);
	}
	/* calculate, display checksum if we're not matching */
	if (*filename && el_file_exists(filename) && get_file_digest(filename, digest)) {
		int i;
		for (i = 0; i < DIGEST_LEN; i++) {
			sprintf(&digest_str[2 * i], "%02x", (int)digest[i]);
		}
		digest_str[DIGEST_LEN * 2] = 0;
		if (!*expected_digest_str) {
			safe_snprintf(result_str, sizeof(result_str), "&md5sum %s %s", digest_str, filename);
			LOG_TO_CONSOLE(c_grey1, result_str);
		}
	}
	/* show help if something fails */
	else {
		LOG_TO_CONSOLE(c_red2, "md5sum : fichier invalide ou erreur de syntaxe.");
		LOG_TO_CONSOLE(c_red1, "Voir la somme md5 pour la carte actuelle :     &md5sum");
		LOG_TO_CONSOLE(c_red1, "Voir la somme pour un fichier spécifique :     &md5sum nom_fichier");
		LOG_TO_CONSOLE(c_red1, "Vérifier la somme pour un fichier spécifique : &md5sum somme_md5 nom_fichier");
		return 1;
	}
	/* if we have an expected value, compare then display an appropriate message */
	if (*expected_digest_str) {
		if (my_strcompare(digest_str, expected_digest_str)) {
			LOG_TO_CONSOLE(c_green2, "md5sum : Le fichier correspond à la somme donnée");
		} else {
			LOG_TO_CONSOLE(c_red2, "md5sum : le fichier ne correspond pas à la somme donnée");
		}
	}
	return 1;
} /* end command_ckdata() */
/* pretend the specified key has been pressed - allows user menu to trigger keypress events */
int command_keypress(char *text, int len) {
	text = getparams(text);
	if (*text) {
		Uint32 value = get_key_value(text);
		if (value) {
			do_keypress(value);
		}
	}
	return 1;
}
int save_local_data(char *text, int len) {
	save_bin_cfg();
	//Save the quickbar items
	save_fr_quickitems();
	//Save the quickbar spells
	save_quickspells();
	//Save recipes
	save_recipes();
	// save el.ini if asked
	if (write_ini_on_exit) {
		write_el_ini();
	}
	// save notepad contents if the file was loaded
	if (notepad_loaded) {
		notepad_save_file();
	}
	save_exploration_map();
	flush_counters();
	save_item_lists();
	save_channel_colors();
	LOG_TO_CONSOLE(c_green1, local_save_str);
	return 0;
}
/* show counters for this session */
static int session_counters(char *text, int len) {
	text = getparams(text);
	print_session_counters(text);
	return 1;
}
int commande_rechargement_almanakh() {
	ReloadEncyclopedia();
	view_tab(&tab_help_win, &tab_help_collection_id, HELP_TAB_ENCYCLOPEDIA);
	return 1;
}
int command_salut(char *text, int len) {
	Uint8 str[2];
	str[0] = SALUT;
	my_tcp_send(my_socket, str, 1);
	return 1;
}
int command_danse(char *text, int len) {
	Uint8 str[2];
	str[0] = DANSE;
	my_tcp_send(my_socket, str, 1);
	return 1;
}
int command_salto(char *text, int len) {
	Uint8 str[2];
	str[0] = SALTO;
	my_tcp_send(my_socket, str, 1);
	return 1;
}
int command_roue(char *text, int len) {
	Uint8 str[2];
	str[0] = ROUE;
	my_tcp_send(my_socket, str, 1);
	return 1;
}
int command_go(char *text) {
	char str[100];
	char *s = (char *)str;
	int x, y;
	const char *ct = " .,";
	char **tab = NULL;
	int i;
	safe_snprintf(str, sizeof(str), text);
	s = getparams(s);
	if (s != NULL && ct != NULL) {
		char *cs = NULL;
		size_t size = 1;
		for (i = 0; (cs = strtok(s, ct)); i++) {
			if (size <= i + 1) {
				void *tmp = NULL;
				size <<= 1;
				tmp = realloc(tab, sizeof(*tab) * size);
				if (tmp != NULL) {
					tab = tmp;
				} else {
					fprintf(stderr, "Memoire insuffisante\n");
					free(tab);
					tab = NULL;
					exit(EXIT_FAILURE);
				}
			}
			tab[i] = cs;
			s = NULL;
		}
		if (i >= 2) {
			tab[i] = NULL;
		} else {
			safe_snprintf(str, sizeof(str), "Il manque les coordonnées. La commande s'utilise de cette manière : &go x y", s);
			LOG_TO_CONSOLE(c_red1, str);
			return 1;
		}
	}
	x = atoi(tab[0]);
	y = atoi(tab[1]);
	pf_move_to_position(x, y);
	return 1;
}
int command_gomarque(char *text) {
	char str[100] = {0};
	char *s;
	int x = 0, y = 0;
	int i;
	// n'a de sens sur la carte courante : annule une éventuelle inspection
	if (inspect_map_text != 0) {
		inspect_map_text = 0;
		load_map_marks();
	}
	s = getparams(text);
	for (i = 0 ; i < max_mark ; i++) {
		if (strcmp(marks[i].text, s) == 0) {
			x = marks[i].x;
			y = marks[i].y;
			break;
		}
	}
	if (x == 0) {
		safe_snprintf(str, sizeof(str) - 1, "La marque '%s' n'a pas été trouvée", s);
		LOG_TO_CONSOLE(c_red1, str);
		return 1;
	}
	pf_move_to_position(x, y);
	return 1;
}
int command_affiche_playlist(char *text) {
	affiche_playlist();
	return 1;
}
int command_modif_playlist(char *text) {
	int i = 0;
	text = getparams(text);
	i = atoi(text);
	modif_playlist(i);
	return 1;
}
int command_raz_playlist(char *text) {
	raz_playlist();
	return 1;
}
//@tosh : commande permettant de lister les musiques contenues dans ./music/
int command_list_music(char *text) {
	DIR *music_dir = NULL;
	struct dirent *music_file = NULL;
	if ((music_dir = opendir("./music/")) == NULL) {
		LOG_ERROR("%s: Impossible d'ouvrir le repertoire \"music\".", reg_error_str);
		LOG_TO_CONSOLE(c_red2, "Impossible d'ouvrir le repertoire ./music/");
		return 1;
	}
	LOG_TO_CONSOLE(c_green1, "Liste des musiques : ");
	while ((music_file = readdir(music_dir)) != NULL) {
		if (strstr(music_file->d_name, ".ogg") != NULL) {
			LOG_TO_CONSOLE(c_red1, music_file->d_name);
		}
	}
	closedir(music_dir);
	return 1;
}
void init_commands(const char *filename) {
	FILE *fp = open_file_data(filename, "r");
	if (fp == NULL) {
		LOG_ERROR("%s: %s \"%s\": %s\n", reg_error_str, cant_open_file, filename, strerror(errno));
	} else {
		/* Read keywords from commands.lst */
		char buffer[255];
		size_t buffer_len;
		char *ptr;
		while (fgets(buffer, sizeof(buffer), fp)) {
			/* Get rid of comments */
			if ((ptr = strchr(buffer, '#')) != NULL) {
				*ptr = '\0';
			}
			buffer_len = strlen(buffer);
			/* Skip empty lines */
			if (strcmp(buffer, "\r\n") != 0 && strcmp(buffer, "\n") != 0 && strlen(buffer) > 0) {
				/* Get rid of the newline. */
				if (buffer[buffer_len - 2] == '\r' && buffer[buffer_len - 1] == '\n') {
					buffer[buffer_len - 2] = '\0';
				} else if (buffer[buffer_len - 1] == '\n') {
					buffer[buffer_len - 1] = '\0';
				}
				add_command(buffer, NULL);
			}
		}
		fclose(fp);
	}
	add_command("marker_color", &command_mark_color);
	add_command("calc", &command_calc);
	add_command("cls", &command_cls);
	add_command(cmd_markpos, &command_markpos);
	add_command(cmd_mark, &command_mark);
	add_command(cmd_unmark, &command_unmark);
	add_command(cmd_stats, &command_stats);
	add_command("ping", &command_ping);
	add_command(cmd_time, &command_time);
	add_command(cmd_date, &command_date);
	add_command("quit", &command_quit);
	add_command("exit", &command_quit);
	add_command(cmd_exit, &command_quit);
	add_command("mem", &command_mem);
	add_command("quitter", &command_quitter);
	add_command("liste_themes", &command_liste_themes);
	add_command("change_theme", &command_change_theme);
	add_command("go", &command_go);
	add_command("gomarque", &command_gomarque);
	add_command("ver", &command_ver);
	add_command("vers", &command_ver);
	add_command(cmd_ignores, &list_ignores);
	add_command(cmd_ignore, &command_ignore);
	add_command(cmd_filters, &list_filters);
	add_command(cmd_filter, &command_filter);
	add_command(cmd_unignore, &command_unignore);
	add_command(cmd_unfilter, &command_unfilter);
	add_command(cmd_glinfo, &command_glinfo);
	add_command(cmd_knowledge_short, &knowledge_command);
	add_command(cmd_knowledge, &knowledge_command);
	add_command(cmd_msg, &command_msg);
	add_command(cmd_afk, &command_afk);
	add_command("jc", &command_jlc);//since we only mess with the part after the
	add_command("couleurs_canaux", &command_channel_colors);
	add_command("qc", &command_jlc);//command, one function can do both
	add_command(help_cmd_str, &command_help);
	add_command("accept_buddy", &command_accept_buddy);
	add_command("affiche_playlist", &command_affiche_playlist);
	add_command("modif_playlist", &command_modif_playlist);
	add_command("raz_playlist", &command_raz_playlist);
	add_command("liste_musique", &command_list_music);
	add_command("muet", &command_muet);
	add_command("current_song", &display_song_name);
	add_command("find", &history_grep);
	add_command("sauve", &save_local_data);
	add_command("am_cmd", commande_rechargement_almanakh);
	add_command("salut", &command_salut);
	add_command("danse", &command_danse);
	add_command("salto", &command_salto);
	add_command("roue", &command_roue);
	add_command("url", &url_command);
	add_command("chat_to_counters", &chat_to_counters_command);
	add_command(cmd_session_counters, &session_counters);
	add_command("exp", &show_exp);
	add_command("alias", &alias_command);
	add_command("unalias", &unalias_command);
	add_command("liste_alias", &aliases_command);
	add_command("md5sum", &command_ckdata);
	add_command(cmd_reload_icons, &reload_icon_window);
	add_command(cmd_open_url, &command_open_url);
	add_command(cmd_show_spell, &command_show_spell);
	add_command(cmd_cast_spell, &command_cast_spell);
	add_command(cmd_keypress, &command_keypress);
	add_command(cmd_user_menu_wait_time_ms, &command_set_user_menu_wait_time_ms);
	command_buffer_offset = NULL;
}
// NOTE: Len = length of the buffer, not the string (Verified)
void print_version_string(char *buf, size_t len) {
	char extra[100];
	if (strlen(DEF_INFO) > 0) {
		safe_snprintf(extra, sizeof(extra), ".%d Beta %s", client_version_patch, DEF_INFO);
	} else {
		safe_snprintf(extra, sizeof(extra), " - %s", nom_version);
	}
	safe_snprintf(buf, len, game_version_str, client_version_major, client_version_minor, client_version_release, extra);
}
void new_minute_console(void) {
	if (!(real_game_minute % 60)) {
		timestamp_chat_log();
	}
	if (time_warn_h >= 0 && (time_warn_h + real_game_minute) % 60 == 0) {
		char str[75];
		safe_snprintf(str, sizeof(str), time_warn_hour_str, time_warn_h);
		LOG_TO_CONSOLE(c_purple1, str);
	}
	if (time_warn_s >= 0 && (time_warn_s + real_game_minute) % 180 == 30) {
		char str[100];
		if (time_warn_s + real_game_minute == 30) { // sunrise
			safe_snprintf(str, sizeof(str), time_warn_sunrise_str, time_warn_s);
		} else { // sunset
			safe_snprintf(str, sizeof(str), time_warn_sunset_str, time_warn_s);
		}
		LOG_TO_CONSOLE(c_purple1, str);
	}
	if (time_warn_d >= 0 && (time_warn_d + real_game_minute) % 360 == 0) {
		char str[75];
		safe_snprintf(str, sizeof(str), time_warn_day_str, time_warn_d);
		LOG_TO_CONSOLE(c_purple1, str);
	}
}
