#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef __GNUC__
 #include <dirent.h>
 #include <unistd.h>
#endif
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include "init.h"
#include "2d_objects.h"
#include "actor_scripts.h"
#include "asc.h"
#include "books.h"
#include "buddy.h"
#include "chat.h"
#include "colors.h"
#include "console.h"
#include "consolewin.h"
#include "counters.h"
#include "cursors.h"
#include "dialogues.h"
#include "draw_scene.h"
#include "e3d.h"
#include "elconfig.h"
#include "elwindows.h"
#include "encyclopedia.h"
#include "errors.h"
#include "filter.h"
#include "framebuffer.h"
#include "gamewin.h"
#include "gl_init.h"
#include "hud.h"
#include "hud_timer.h"
#include "items.h"
#include "item_lists.h"
#include "keys.h"
#include "knowledge.h"
#include "langselwin.h"
#include "lights.h"
#include "loading_win.h"
#include "loginwin.h"
#include "multiplayer.h"
#include "manufacture.h"
#include "astrology.h"
#include "mapwin.h"
#include "named_colours.h"
#include "new_actors.h"
#include "openingwin.h"
#include "particles.h"
#include "questlog.h"
#include "reflection.h"
#include "rules.h"
#include "servers.h"
#include "sound.h"
#include "spells.h"
#include "storage.h"
#include "tabs.h"
#include "textures.h"
#include "themes.h"
#include "tiles.h"
#include "timers.h"
#include "trade.h"
#include "translate.h"
#include "update.h"
#include "weather.h"
#include "eye_candy_wrapper.h"
#include "minimap.h"
#include "io/elpathwrapper.h"
#include "io/elfilewrapper.h"
#include "io/xmlcallbacks.h"
#include "sky.h"
#include "popup.h"
#include "text_aliases.h"
#include "user_menus.h"
#include "image_loading.h"
#include "io/fileutil.h"
#include "fr_quickitems.h"
#define CFG_VERSION 7   // change this when critical changes to el.cfg are made that will break it
int divers_text = 0;
int ini_file_size = 0;
int disconnected = 1;
int auto_update = 1;
int exit_now = 0;
int restart_required = 0;
int allow_restart = 1;
int poor_man = 0;
int special_effects = 0;
int isometric = 1;
int mouse_limit = 15;
int no_adjust_shadows = 0;
int clouds_shadows = 1;
int item_window_on_drop = 1;
int buddy_log_notice = 1;
char configdir[256] = "./";
char datadir[256] = "./";
char lang[10] = "en";
static int no_lang_in_config = 0;
int video_mode_set = 0;
#ifdef OSX
int emulate3buttonmouse = 0;
#endif
void read_command_line(); //from main.c
void load_knowledge_list() {
	FILE *f = NULL;
	char strLine[255];
	memset(knowledge_list, 0, sizeof(knowledge_list));
	knowledge_count = 0;
	// try the language specific knowledge list
	f = open_file_lang("knowledge.lst", "rb");
	if (f == NULL) {
		LOG_ERROR("%s: %s \"knowledge.lst\": %s\n", reg_error_str, cant_open_file, strerror(errno));
		return;
	}
	char format[64];
	snprintf(format, sizeof(format), "%%hhd %%hd %%hhd %%hhd %%%zu[^\n]", sizeof(knowledge_list->name) - 1);
	while (fgets(strLine, sizeof(strLine), f)) {
		knowledge *k = knowledge_list + knowledge_count++;
		sscanf(strLine, format, &k->type, &k->id, &k->is_stored, &k->affiche, &k->name);
	}
	fclose(f);
	// On initialise la liste des categories
	init_categories();
}
void read_config() {
	// Set our configdir
	const char *tcfg = get_path_config();
	my_strncp(configdir, tcfg, sizeof(configdir));
	if ( !read_el_ini()) {
		// oops, the file doesn't exist, give up
		const char *err_stg = "Failure reading el.ini";
		fprintf(stderr, "%s\n", err_stg);
		LOG_ERROR(err_stg);
		SDL_Quit();
		exit(1);
	}
	/* if language is not set, default to "en" but use the language selection window */
	if (strlen(lang) == 0) {
		no_lang_in_config = 0;
		safe_strncpy(lang, "fr", sizeof(lang));
		LOG_INFO("No language set so defaulting to [%s] and using language selection window", lang);
	}
#ifndef WINDOWS
	if (chdir(datadir) != 0) {
		LOG_ERROR("%s() chdir(\"%s\") failed: %s\n", __FUNCTION__, datadir, strerror(errno));
	}
#endif //!WINDOWS
	if (password_str[0]) {//We have a password
		size_t k;
		for (k = 0; k < strlen(password_str); k++) {
			display_password_str[k] = '*';
		}
		display_password_str[k] = 0;
	} else if (username_str[0]) { //We have a username but not a password...
		username_box_selected = 0;
		password_box_selected = 1;
	}
}
void read_bin_cfg() {
	FILE *f = NULL;
	bin_cfg cfg_mem;
	int i;
	const char *fname = "le.cfg";
	size_t ret;
	f = open_file_config_no_local(fname, "rb");
	if (f == NULL) {
		return;     //no config file, use defaults
	}
	memset(&cfg_mem, 0, sizeof(cfg_mem));   // make sure its clean
	ret = fread(&cfg_mem, 1, sizeof(cfg_mem), f);
	fclose(f);
	if (ret != sizeof(cfg_mem)) {
		LOG_ERROR("%s() failed to read %s\n", __FUNCTION__, fname);
		return;
	}
	//verify the version number
	if (cfg_mem.cfg_version_num != CFG_VERSION) {
		return;                                    //oops! ignore the file
	}
	//good, retrive the data
	// TODO: move window save/restore into the window handler
	items_menu_x = cfg_mem.items_menu_x;
	items_menu_y = cfg_mem.items_menu_y;
	items_menu_x_len = cfg_mem.items_menu_x_len;
	items_menu_y_len = cfg_mem.items_menu_y_len;
	ground_items_menu_x = cfg_mem.ground_items_menu_x & 0xFFFF;
	ground_items_menu_y = cfg_mem.ground_items_menu_y & 0xFFFF;
	// hack pour gérer les positions négatives
	if (ground_items_menu_x > 8000) {
		ground_items_menu_x -= 16000;
	}
	if (ground_items_menu_y > 8000) {
		ground_items_menu_y -= 16000;
	}
	ground_items_menu_x_len = cfg_mem.ground_items_menu_x >> 16;
	ground_items_menu_y_len = cfg_mem.ground_items_menu_y >> 16;
	trade_menu_x = cfg_mem.trade_menu_x;
	trade_menu_y = cfg_mem.trade_menu_y;
	sigil_menu_x = cfg_mem.sigil_menu_x;
	sigil_menu_y = cfg_mem.sigil_menu_y;
	start_mini_spells = cfg_mem.start_mini_spells;
	dialogue_menu_x = cfg_mem.dialogue_menu_x;
	dialogue_menu_y = cfg_mem.dialogue_menu_y;
	manufacture_menu_x = cfg_mem.manufacture_menu_x;
	manufacture_menu_y = cfg_mem.manufacture_menu_y;
	astrology_win_x = cfg_mem.astrology_win_x;
	astrology_win_y = cfg_mem.astrology_win_y;
	tab_stats_x = cfg_mem.tab_stats_x;
	tab_stats_y = cfg_mem.tab_stats_y;
	elconfig_menu_x = cfg_mem.elconfig_menu_x;
	elconfig_menu_y = cfg_mem.elconfig_menu_y;
	tab_help_x = cfg_mem.tab_help_x;
	tab_help_y = cfg_mem.tab_help_y;
	storage_win_x = cfg_mem.storage_win_x & 0xFFFF;
	storage_win_y = cfg_mem.storage_win_y & 0xFFFF;
	// hack pour gérer les positions négatives
	if (storage_win_x > 8000) {
		storage_win_x -= 16000;
	}
	if (storage_win_y > 8000) {
		storage_win_y -= 16000;
	}
	if ((cfg_mem.storage_win_x >> 16) && (cfg_mem.storage_win_y >> 16)) {
		storage_win_x_len = cfg_mem.storage_win_x >> 16;
		storage_win_y_len = cfg_mem.storage_win_y >> 16;
	}
	buddy_menu_x = cfg_mem.buddy_menu_x & 0xFFFF;
	buddy_menu_y = cfg_mem.buddy_menu_y & 0xFFFF;
	// hack pour gérer les positions négatives
	if (buddy_menu_x > 8000) {
		buddy_menu_x -= 16000;
	}
	if (buddy_menu_y > 8000) {
		buddy_menu_y -= 16000;
	}
	if ((cfg_mem.buddy_menu_x >> 16) && (cfg_mem.buddy_menu_y >> 16)) {
//		buddy_menu_x_len=cfg_mem.buddy_menu_x >> 16;
		buddy_menu_y_len = cfg_mem.buddy_menu_y >> 16;
	}
	questlog_menu_x = cfg_mem.questlog_win_x;
	questlog_menu_y = cfg_mem.questlog_win_y;
	minimap_win_x = cfg_mem.minimap_win_x;
	minimap_win_y = cfg_mem.minimap_win_y;
	minimap_tiles_distance = cfg_mem.minimap_zoom;
	tab_selected = cfg_mem.tab_selected;
	tab_info_x = cfg_mem.tab_info_x;
	tab_info_y = cfg_mem.tab_info_y;
	if ((quickspell_x = cfg_mem.quickspell_x) > window_width || quickspell_x < 10) {
		quickspell_x = HUD_MARGIN_X;
	}
	if ((quickspell_y = cfg_mem.quickspell_y) > window_height || quickspell_y <= 0) {
		quickspell_y = HUD_MARGIN_X;
	}
	if ((quickspells_dir = cfg_mem.quickspell_flags & 3) != HORIZONTAL) {
		quickspells_dir = VERTICAL;
	}
//	quickspells_dir       =  cfg_mem.quickspell_flags       & 1;
	quickspells_draggable = (cfg_mem.quickspell_flags >> 2) & 1;
	quickspells_on_top = (cfg_mem.quickspell_flags >> 3) & 1;
	if ((quickbar_x = cfg_mem.quickbar_x) > window_width || quickbar_x < 10) {
		quickbar_x = 34;
	}
	if ((quickbar_y = cfg_mem.quickbar_y) > window_height || quickbar_y <= 0) {
		quickbar_y = HUD_MARGIN_X;
	}
	if ((quickbar_dir = cfg_mem.quickbar_flags & 3) != HORIZONTAL) {
		quickbar_dir = VERTICAL;
	}
//	quickbar_dir       =  cfg_mem.quickbar_flags       & 1;
	quickbar_draggable = (cfg_mem.quickbar_flags >> 2) & 1;
	quickbar_on_top = (cfg_mem.quickbar_flags >> 3) & 1;
#if MAX_WATCH_STATS != 5
#error You cannot just go around changing MAX_WATCH_STATS as its used by the el.cfg file!
#endif
	for (i = 0; i < MAX_WATCH_STATS; i++) {
		watch_this_stats[i] = cfg_mem.watch_this_stats[i];
		if (watch_this_stats[i] < 0 || watch_this_stats[i] >= NUM_WATCH_STAT) {
			watch_this_stats[i] = 0;
		}
	}
	if (watch_this_stats[0] < 1 || watch_this_stats[0] >= NUM_WATCH_STAT) {
		watch_this_stats[0] = NUM_WATCH_STAT - 1;
	}
	has_accepted = cfg_mem.has_accepted_rules;
	rx = cfg_mem.camera_x;
	ry = cfg_mem.camera_y;
	rz = cfg_mem.camera_z;
	new_zoom_level = zoom_level = cfg_mem.zoom_level;
	item_lists_set_active(cfg_mem.active_item_list);
	view_health_bar = cfg_mem.view_health_bar;
	view_names = cfg_mem.view_names;
	view_hp = cfg_mem.view_hp;
	voir_pdv = cfg_mem.voir_pdv;
	voir_musique_carte = cfg_mem.voir_musique_carte;
	quantities.selected = cfg_mem.quantity_selected;
	for (i = 0; i < ITEM_EDIT_QUANT; i++) {
		if (cfg_mem.quantity[i]) {
			quantities.quantity[i].val = cfg_mem.quantity[i];
			safe_snprintf(quantities.quantity[i].str, sizeof(quantities.quantity[i].str), "%d", cfg_mem.quantity[i]);
			quantities.quantity[i].len = strlen(quantities.quantity[i].str);
		}
	}
	if (zoom_level != 0.0f) {
		resize_root_window();
	}
	have_saved_langsel = cfg_mem.have_saved_langsel;
	use_small_items_window = cfg_mem.misc_bool_options & 1;
	manual_size_items_window = (cfg_mem.misc_bool_options >> 1) & 1;
	allow_equip_swap = (cfg_mem.misc_bool_options >> 2) & 1;
	items_mix_but_all = (cfg_mem.misc_bool_options >> 3) & 1;
	items_stoall_nolastrow = (cfg_mem.misc_bool_options >> 4) & 1;
	items_dropall_nolastrow = (cfg_mem.misc_bool_options >> 5) & 1;
	autoclose_storage_dialogue = (cfg_mem.misc_bool_options >> 6) & 1;
	auto_select_storage_option = (cfg_mem.misc_bool_options >> 7) & 1;
	dialogue_copy_excludes_responses = (cfg_mem.misc_bool_options >> 8) & 1;
	items_stoall_nofirstrow = (cfg_mem.misc_bool_options >> 9) & 1;
	items_dropall_nofirstrow = (cfg_mem.misc_bool_options >> 10) & 1;
	items_auto_get_all = (cfg_mem.misc_bool_options >> 11) & 1;
	dialogue_copy_excludes_newlines = (cfg_mem.misc_bool_options >> 12) & 1;
	open_minimap_on_start = (cfg_mem.misc_bool_options >> 13) & 1;
	sort_storage_categories = (cfg_mem.misc_bool_options >> 14) & 1;
	items_list_on_left = (cfg_mem.misc_bool_options >> 17) & 1;
	disable_storage_filter = (cfg_mem.misc_bool_options >> 19) & 1;
	hud_timer_keep_state = (cfg_mem.misc_bool_options >> 20) & 1;
	cm_quickbar_enabled = cfg_mem.fr_quickitems_options & 1;
	cm_quickbar_protected = (cfg_mem.fr_quickitems_options >> 1) & 1;
	fr_quickitems_insertmode = (cfg_mem.fr_quickitems_options >> 2) & 1;
	fr_quickitems_autocompress = (cfg_mem.fr_quickitems_options >> 3) & 1;
	fr_quickitems_autocomplete = (cfg_mem.fr_quickitems_options >> 4) & 1;
	fr_quickitems_autoremove = (cfg_mem.fr_quickitems_options >> 5) & 1;
	items_stoall_nofirstrow = cfg_mem.items_all_nocolrow & 1;
	items_dropall_nofirstrow = (cfg_mem.items_all_nocolrow >> 1) & 1;
	items_stoall_nolastcol = (cfg_mem.items_all_nocolrow >> 2) & 1;
	items_dropall_nolastcol = (cfg_mem.items_all_nocolrow >> 3) & 1;
	items_stoall_nofirstcol = (cfg_mem.items_all_nocolrow >> 4) & 1;
	items_dropall_nofirstcol = (cfg_mem.items_all_nocolrow >> 5) & 1;
	allow_wheel_quantity_drag = cfg_mem.allow_wheel_quantity & 1;
	allow_wheel_quantity_edit = (cfg_mem.allow_wheel_quantity >> 1) & 1;
	cm_listrecipe_enabled = cfg_mem.manurecipes_options & 1;
	cm_manurecipe_invertwheel = (cfg_mem.manurecipes_options >> 1) & 1;
	cm_manurecipe_addnolimit = (cfg_mem.manurecipes_options >> 2) & 1;
	cm_manurecipe_wheelaffect = (cfg_mem.manurecipes_options >> 3) & 1;
	disable_storage_filter = (cfg_mem.misc_bool_options >> 4) & 1;
	set_options_user_menus(cfg_mem.user_menu_win_x, cfg_mem.user_menu_win_y, cfg_mem.user_menu_options);
	floating_counter_flags = cfg_mem.floating_counter_flags;
}
void save_bin_cfg() {
	FILE *f = NULL;
	bin_cfg cfg_mem;
	int i;
	f = open_file_config("le.cfg", "wb");
	if (f == NULL) {
		LOG_ERROR("%s: %s \"el.cfg\": %s\n", reg_error_str, cant_open_file, strerror(errno));
		return;//blah, whatever
	}
	memset(&cfg_mem, 0, sizeof(cfg_mem));   // make sure its clean
	cfg_mem.cfg_version_num = CFG_VERSION;    // set the version number
	//good, retrive the data
	/*
	   // TODO: move window save/restore into the window handler
	   cfg_mem.items_menu_x=items_menu_x;
	   cfg_mem.items_menu_y=items_menu_y;

	   cfg_mem.ground_items_menu_x=ground_items_menu_x;
	   cfg_mem.ground_items_menu_y=ground_items_menu_y;

	   cfg_mem.trade_menu_x=trade_menu_x;
	   cfg_mem.trade_menu_y=trade_menu_y;

	   cfg_mem.sigil_menu_x=sigil_menu_x;
	   cfg_mem.sigil_menu_y=sigil_menu_y;

	   cfg_mem.dialogue_menu_x=dialogue_menu_x;
	   cfg_mem.dialogue_menu_y=dialogue_menu_y;

	   cfg_mem.manufacture_menu_x=manufacture_menu_x;
	   cfg_mem.manufacture_menu_y=manufacture_menu_y;

	   cfg_mem.attrib_menu_x=attrib_menu_x;
	   cfg_mem.attrib_menu_y=attrib_menu_y;

	   cfg_mem.elconfig_menu_x=elconfig_menu_x;
	   cfg_mem.elconfig_menu_y=elconfig_menu_y;

	   cfg_mem.knowledge_menu_x=knowledge_menu_x;
	   cfg_mem.knowledge_menu_y=knowledge_menu_y;

	   cfg_mem.encyclopedia_menu_x=encyclopedia_menu_x;
	   cfg_mem.encyclopedia_menu_y=encyclopedia_menu_y;

	   cfg_mem.questlog_menu_x=questlog_menu_x;
	   cfg_mem.questlog_menu_y=questlog_menu_y;
	 */
	if (tab_help_win >= 0) {
		cfg_mem.tab_help_x = windows_list.window[tab_help_win].cur_x;
		cfg_mem.tab_help_y = windows_list.window[tab_help_win].cur_y;
	} else {
		cfg_mem.tab_help_x = tab_help_x;
		cfg_mem.tab_help_y = tab_help_y;
	}
	if (items_win >= 0) {
		cfg_mem.items_menu_x = windows_list.window[items_win].cur_x;
		cfg_mem.items_menu_y = windows_list.window[items_win].cur_y;
	} else {
		cfg_mem.items_menu_x = items_menu_x;
		cfg_mem.items_menu_y = items_menu_y;
	}
	if (items_win >= 0) {
		cfg_mem.items_menu_x_len = windows_list.window[items_win].len_x;
		cfg_mem.items_menu_y_len = windows_list.window[items_win].len_y;
	} else {
		cfg_mem.items_menu_x_len = items_menu_x_len;
		cfg_mem.items_menu_y_len = items_menu_y_len;
	}
	if (ground_items_win >= 0) {
		cfg_mem.ground_items_menu_x = windows_list.window[ground_items_win].cur_x;
		cfg_mem.ground_items_menu_y = windows_list.window[ground_items_win].cur_y;
		// hack pour gérer les positions négatives
		cfg_mem.ground_items_menu_x += 16000;
		cfg_mem.ground_items_menu_y += 16000;
		cfg_mem.ground_items_menu_x |= windows_list.window[ground_items_win].len_x << 16;
		cfg_mem.ground_items_menu_y |= windows_list.window[ground_items_win].len_y << 16;
	} else {
		cfg_mem.ground_items_menu_x = ground_items_menu_x;
		cfg_mem.ground_items_menu_y = ground_items_menu_y;
		// hack pour gérer les positions négatives
		cfg_mem.ground_items_menu_x += 16000;
		cfg_mem.ground_items_menu_y += 16000;
		cfg_mem.ground_items_menu_x |= ground_items_menu_x_len << 16;
		cfg_mem.ground_items_menu_y |= ground_items_menu_y_len << 16;
	}
	if (trade_win >= 0) {
		cfg_mem.trade_menu_x = windows_list.window[trade_win].cur_x;
		cfg_mem.trade_menu_y = windows_list.window[trade_win].cur_y;
	} else {
		cfg_mem.trade_menu_x = trade_menu_x;
		cfg_mem.trade_menu_y = trade_menu_y;
	}
	cfg_mem.start_mini_spells = start_mini_spells;
	if (sigil_win >= 0) {
		cfg_mem.sigil_menu_x = windows_list.window[sigil_win].cur_x;
		cfg_mem.sigil_menu_y = windows_list.window[sigil_win].cur_y;
	} else {
		cfg_mem.sigil_menu_x = sigil_menu_x;
		cfg_mem.sigil_menu_y = sigil_menu_y;
	}
	if (dialogue_win >= 0) {
		cfg_mem.dialogue_menu_x = windows_list.window[dialogue_win].cur_x;
		cfg_mem.dialogue_menu_y = windows_list.window[dialogue_win].cur_y;
	} else {
		cfg_mem.dialogue_menu_x = dialogue_menu_x;
		cfg_mem.dialogue_menu_y = dialogue_menu_y;
	}
	if (manufacture_win >= 0) {
		cfg_mem.manufacture_menu_x = windows_list.window[manufacture_win].cur_x;
		cfg_mem.manufacture_menu_y = windows_list.window[manufacture_win].cur_y;
	} else {
		cfg_mem.manufacture_menu_x = manufacture_menu_x;
		cfg_mem.manufacture_menu_y = manufacture_menu_y;
	}
	if (astrology_win >= 0) {
		cfg_mem.astrology_win_x = windows_list.window[astrology_win].cur_x;
		cfg_mem.astrology_win_y = windows_list.window[astrology_win].cur_y;
	} else {
		cfg_mem.astrology_win_x = astrology_win_x;
		cfg_mem.astrology_win_y = astrology_win_y;
	}
	if (elconfig_win >= 0) {
		cfg_mem.elconfig_menu_x = windows_list.window[elconfig_win].cur_x;
		cfg_mem.elconfig_menu_y = windows_list.window[elconfig_win].cur_y;
	} else {
		cfg_mem.elconfig_menu_x = elconfig_menu_x;
		cfg_mem.elconfig_menu_y = elconfig_menu_y;
	}
	if (storage_win >= 0) {
		cfg_mem.storage_win_x = windows_list.window[storage_win].cur_x;
		cfg_mem.storage_win_y = windows_list.window[storage_win].cur_y;
		// hack pour gérer les positions négatives
		cfg_mem.storage_win_x += 16000;
		cfg_mem.storage_win_y += 16000;
		cfg_mem.storage_win_x |= windows_list.window[storage_win].len_x << 16;
		cfg_mem.storage_win_y |= windows_list.window[storage_win].len_y << 16;
	} else {
		cfg_mem.storage_win_x = storage_win_x;
		cfg_mem.storage_win_y = storage_win_y;
		// hack pour gérer les positions négatives
		cfg_mem.storage_win_x += 16000;
		cfg_mem.storage_win_y += 16000;
		cfg_mem.storage_win_x |= storage_win_x_len << 16;
		cfg_mem.storage_win_y |= storage_win_y_len << 16;
	}
	if (tab_stats_win >= 0) {
		cfg_mem.tab_stats_x = windows_list.window[tab_stats_win].cur_x;
		cfg_mem.tab_stats_y = windows_list.window[tab_stats_win].cur_y;
	} else {
		cfg_mem.tab_stats_x = tab_stats_x;
		cfg_mem.tab_stats_y = tab_stats_y;
	}
	if (buddy_win >= 0) {
		cfg_mem.buddy_menu_x = windows_list.window[buddy_win].cur_x;
		cfg_mem.buddy_menu_y = windows_list.window[buddy_win].cur_y;
		// hack pour gérer les positions négatives
		cfg_mem.buddy_menu_x += 16000;
		cfg_mem.buddy_menu_y += 16000;
		cfg_mem.buddy_menu_x |= windows_list.window[buddy_win].len_x << 16;
		cfg_mem.buddy_menu_y |= windows_list.window[buddy_win].len_y << 16;
	} else {
		cfg_mem.buddy_menu_x = buddy_menu_x;
		cfg_mem.buddy_menu_y = buddy_menu_y;
		// hack pour gérer les positions négatives
		cfg_mem.buddy_menu_x += 16000;
		cfg_mem.buddy_menu_y += 16000;
		cfg_mem.buddy_menu_x |= buddy_menu_x_len << 16;
		cfg_mem.buddy_menu_y |= buddy_menu_y_len << 16;
	}
	if (questlog_win >= 0) {
		cfg_mem.questlog_win_x = windows_list.window[questlog_win].cur_x;
		cfg_mem.questlog_win_y = windows_list.window[questlog_win].cur_y;
	} else {
		cfg_mem.questlog_win_x = questlog_menu_x;
		cfg_mem.questlog_win_y = questlog_menu_y;
	}
	if (minimap_win >= 0) {
		cfg_mem.minimap_win_x = windows_list.window[minimap_win].cur_x;
		cfg_mem.minimap_win_y = windows_list.window[minimap_win].cur_y;
	} else {
		cfg_mem.minimap_win_x = minimap_win_x;
		cfg_mem.minimap_win_y = minimap_win_y;
	}
	cfg_mem.minimap_zoom = minimap_tiles_distance;
	cfg_mem.tab_selected = get_tab_selected();
	if (tab_info_win >= 0) {
		cfg_mem.tab_info_x = windows_list.window[tab_info_win].cur_x;
		cfg_mem.tab_info_y = windows_list.window[tab_info_win].cur_y;
	} else {
		cfg_mem.tab_info_x = tab_info_x;
		cfg_mem.tab_info_y = tab_info_y;
	}
	cfg_mem.view_health_bar = view_health_bar;
	cfg_mem.view_names = view_names;
	cfg_mem.view_hp = view_hp;
	cfg_mem.active_item_list = item_lists_get_active();
	cfg_mem.quantity_selected = (quantities.selected < ITEM_EDIT_QUANT)?quantities.selected :0;
	cfg_mem.voir_pdv = voir_pdv;
	cfg_mem.voir_musique_carte = voir_musique_carte;
	if (quickspell_win >= 0) {
		cfg_mem.quickspell_x = window_width - windows_list.window[quickspell_win].cur_x;
		cfg_mem.quickspell_y = windows_list.window[quickspell_win].cur_y;
	} else {
		cfg_mem.quickspell_x = quickspell_x;
		cfg_mem.quickspell_y = quickspell_y;
	}
	cfg_mem.quickspell_flags = quickspells_dir;
	cfg_mem.quickspell_flags |= quickspells_draggable << 2;
	cfg_mem.quickspell_flags |= quickspells_on_top << 3;
	if (quickbar_win >= 0) {
		cfg_mem.quickbar_x = window_width - windows_list.window[quickbar_win].cur_x;
		cfg_mem.quickbar_y = windows_list.window[quickbar_win].cur_y;
	} else {
		cfg_mem.quickbar_x = quickbar_x;
		cfg_mem.quickbar_y = quickbar_y;
	}
	cfg_mem.quickbar_flags = quickbar_dir;
	cfg_mem.quickbar_flags |= quickbar_draggable << 2;
	cfg_mem.quickbar_flags |= quickbar_on_top << 3;
	for (i = 0; i < MAX_WATCH_STATS; i++) {
		cfg_mem.watch_this_stats[i] = watch_this_stats[i];
	}
	cfg_mem.has_accepted_rules = has_accepted;
	cfg_mem.camera_x = rx;
	cfg_mem.camera_y = ry;
	cfg_mem.camera_z = rz;
	cfg_mem.zoom_level = zoom_level;
	for (i = 0; i < ITEM_EDIT_QUANT; i++) {
		cfg_mem.quantity[i] = quantities.quantity[i].val;
	}
	cfg_mem.have_saved_langsel = have_saved_langsel;
	cfg_mem.misc_bool_options = 0;
	cfg_mem.misc_bool_options |= use_small_items_window;
	cfg_mem.misc_bool_options |= manual_size_items_window << 1;
	cfg_mem.misc_bool_options |= allow_equip_swap << 2;
	cfg_mem.misc_bool_options |= items_mix_but_all << 3;
	cfg_mem.misc_bool_options |= items_stoall_nolastrow << 4;
	cfg_mem.misc_bool_options |= items_dropall_nolastrow << 5;
	cfg_mem.misc_bool_options |= autoclose_storage_dialogue << 6;
	cfg_mem.misc_bool_options |= auto_select_storage_option << 7;
	cfg_mem.misc_bool_options |= dialogue_copy_excludes_responses << 8;
	cfg_mem.misc_bool_options |= items_stoall_nofirstrow << 9;
	cfg_mem.misc_bool_options |= items_dropall_nofirstrow << 10;
	cfg_mem.misc_bool_options |= items_auto_get_all << 11;
	cfg_mem.misc_bool_options |= dialogue_copy_excludes_newlines << 12;
	cfg_mem.misc_bool_options |= open_minimap_on_start << 13;
	cfg_mem.misc_bool_options |= sort_storage_categories << 14;
	cfg_mem.misc_bool_options |= items_list_on_left << 17;
	cfg_mem.misc_bool_options |= disable_storage_filter << 19;
	cfg_mem.misc_bool_options |= hud_timer_keep_state << 20;
	cfg_mem.fr_quickitems_options = 0;
	cfg_mem.fr_quickitems_options |= cm_quickbar_enabled;
	cfg_mem.fr_quickitems_options |= cm_quickbar_protected << 1;
	cfg_mem.fr_quickitems_options |= fr_quickitems_insertmode << 2;
	cfg_mem.fr_quickitems_options |= fr_quickitems_autocompress << 3;
	cfg_mem.fr_quickitems_options |= fr_quickitems_autocomplete << 4;
	cfg_mem.fr_quickitems_options |= fr_quickitems_autoremove << 5;
	cfg_mem.allow_wheel_quantity = 0;
	cfg_mem.allow_wheel_quantity |= allow_wheel_quantity_drag;
	cfg_mem.allow_wheel_quantity |= allow_wheel_quantity_edit << 1;
	cfg_mem.items_all_nocolrow = 0;
	cfg_mem.items_all_nocolrow |= items_stoall_nofirstrow;
	cfg_mem.items_all_nocolrow |= items_dropall_nofirstrow << 1;
	cfg_mem.items_all_nocolrow |= items_stoall_nolastcol << 2;
	cfg_mem.items_all_nocolrow |= items_dropall_nolastcol << 3;
	cfg_mem.items_all_nocolrow |= items_stoall_nofirstcol << 4;
	cfg_mem.items_all_nocolrow |= items_dropall_nofirstcol << 5;
	cfg_mem.manurecipes_options = 0;
	cfg_mem.manurecipes_options |= cm_listrecipe_enabled;
	cfg_mem.manurecipes_options |= cm_manurecipe_invertwheel << 1;
	cfg_mem.manurecipes_options |= cm_manurecipe_addnolimit << 2;
	cfg_mem.manurecipes_options |= cm_manurecipe_wheelaffect << 3;
	cfg_mem.misc_bool_options |= disable_storage_filter << 4;
	get_options_user_menus(&cfg_mem.user_menu_win_x, &cfg_mem.user_menu_win_y, &cfg_mem.user_menu_options);
	cfg_mem.floating_counter_flags = floating_counter_flags;
	fwrite(&cfg_mem, sizeof(cfg_mem), 1, f);
	fclose(f);
}
void init_e3d_cache() {
	//cache_e3d= cache_init(1000, &destroy_e3d);	//TODO: autofree the name as well
	cache_e3d = cache_init("E3d cache", 1500, NULL);        //no aut- free permitted
	cache_set_compact(cache_e3d, &free_e3d_va);     // to compact, free VA arrays
	cache_set_time_limit(cache_e3d, 5 * 60 * 1000);
	cache_set_size_limit(cache_e3d, 8 * 1024 * 1024);
}
void init_stuff() {
	int seed;
	char file_name[250];
	int i;
	char config_location[300];
	const char *cfgdir;
	Uint8 str[256];
	create_tcp_out_mutex();
	if (chdir(datadir) != 0) {
		LOG_ERROR("%s() chdir(\"%s\") failed: %s\n", __FUNCTION__, datadir, strerror(errno));
	}
	init_crc_tables();
	init_zip_archives();
	// initialize the text buffers - needed early for logging
	init_text_buffers();
	load_server_list("servers.lst");
	set_server_details();
	initial_sound_init();
	// Read the config file
	read_config();
	// Parse command line options
	read_command_line();
	// all options loaded
	options_loaded();
	// Check if our datadir is valid and if not failover to ./
	file_check_datadir();
	// Here you can add zip files, like
	// add_zip_archive(datadir + "data.zip");
	xml_register_el_input_callbacks();
	// XXX FIXME (Grum): actually this should only be done when windowed
	// chat is not used (which we don't know yet at this point), but let's
	// leave it here until we're certain that the chat channel buffers are
	// never used otherwise, then move it down till after the configuration
	// is read.
	init_chat_channels();
	// load the named colours for the elgl-Colour-() functions
	init_named_colours();
	// initialize the fonts, but don't load the textures yet. Do that here
	// because the messages need the font widths.
	init_fonts();
	//OK, we have the video mode settings...
	setup_video_mode(full_screen, video_mode);
	//now you may set the video mode using the %<foo> in-game
	video_mode_set = 1;
	//Good, we should be in the right working directory - load all translatables from their files
	load_translatables();
	//if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE | SDL_INIT_EVENTTHREAD) == -1)	// experimental
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) == -1) {
		LOG_ERROR("%s: %s\n", no_sdl_str, SDL_GetError());
		fprintf(stderr, "%s: %s\n", no_sdl_str, SDL_GetError());
		SDL_Quit();
		exit(1);
	}
	init_video();
	SDL_WM_SetCaption(win_principal, "Landes Eternelles");
#ifdef OSX
	// don't emulate a 3 button mouse except you still have a 1 button mouse, ALT+leftclick doesn't work with the emulation
	if (!emulate3buttonmouse) {
		SDL_putenv("SDL_HAS3BUTTONMOUSE=1");
	}
#endif
	// affichage automatique du glinfo au lancement du client
	command_glinfo(str, strlen((char *)str));
	//Init the caches here, as the loading window needs them
	cache_system_init(MAX_CACHE_SYSTEM);
	init_texture_cache();
	init_e3d_cache();
	//now load the font textures
	if (load_font_textures() != 1) {
		LOG_ERROR("%s\n", fatal_data_error);
		fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, fatal_data_error);
		SDL_Quit();
		exit(1);
	}
	CHECK_GL_ERRORS();
	// read the continent map info
	read_mapinfo();
	init_liste_themes();
	init_theme(titre_theme);
	// now create the root window
	// XXX FIXME (Grum): Maybe we should do this at a later time, after
	// we're logged in?
	create_game_root_window(window_width, window_height);
	create_console_root_window(window_width, window_height);
	create_map_root_window(window_width, window_height);
	create_login_root_window(window_width, window_height);
	//create the loading window
	create_loading_win(window_width, window_height, 0);
	show_window(loading_win);
	update_loading_win(init_opengl_str, 5);
	LOG_DEBUG("Init extensions.");
	init_gl_extensions();
	LOG_DEBUG("Init extensions done");
	// Setup the new eye candy system
	LOG_DEBUG("Init eyecandy");
	ec_init();
	LOG_DEBUG("Init eyecandy done");
	// check for invalid combinations
	check_options();
	update_loading_win(init_random_str, 4);
	seed = time(NULL);
	srand(seed);
	update_loading_win(load_ignores_str, 1);
	load_ignores();
	update_loading_win(load_filters_str, 2);
	load_filters();
	update_loading_win(load_lists_str, 2);
	load_harvestable_list();
	load_entrable_list();
	load_knowledge_list();
	update_loading_win(load_cursors_str, 5);
	load_cursors();
	build_cursors();
	change_cursor(CURSOR_ARROW);
	update_loading_win(bld_glow_str, 3);
	build_glow_color_table();
	update_loading_win(init_lists_str, 2);
	init_actors_lists();
	update_loading_win("init particles", 4);
	memset(tile_list, 0, sizeof(tile_list));
	memset(lights_list, 0, sizeof(lights_list));
	main_bbox_tree = build_bbox_tree();
	init_particles();
	update_loading_win(init_audio_str, 1);
	load_sound_config_data(SOUND_CONFIG_PATH);
	update_loading_win(init_actor_defs_str, 4);
	memset(actors_defs, 0, sizeof(actors_defs));
	LOG_DEBUG("Init actor defs");
	init_actor_defs();
	LOG_DEBUG("Init actor defs done");
	update_loading_win(load_map_tiles_str, 4);
	load_map_tiles();
	update_loading_win(init_lights_str, 4);
	//lights setup
	build_global_light_table();
	build_sun_pos_table();
	reset_material();
	LOG_DEBUG("Init lights");
	init_lights();
	LOG_DEBUG("Init done");
	disable_local_lights();
	update_loading_win(init_logs_str, 4);
	update_loading_win(read_config_str, 2);
	read_bin_cfg();
	update_loading_win(init_weather_str, 3);
	weather_init();
	build_levels_table();//for some HUD stuff
	update_loading_win(load_icons_str, 4);
	//load the necesary textures
	divers_text = load_texture_cached("./textures/divers.dds", 0);
	icons_text = load_texture_cached("textures/gamebuttons.dds", tt_gui);
	hud_text = load_texture_cached("textures/gamebuttons2.dds", tt_gui);
	update_loading_win(load_textures_str, 4);
	cons_text = load_texture_cached("textures/console.dds", tt_gui);
	update_loading_win("init item textures", 5);
	for (i = 0; i < MAX_ITEMS_TEXTURES; i++) {
		char buffer[256];
		safe_snprintf(buffer, sizeof(buffer), "textures/items%d.dds", i + 1);
		if (check_image_name(buffer, sizeof(buffer), buffer) != 0) {
			items_text[i] = load_texture_cached(buffer, tt_gui);
		}
	}
	update_loading_win("init portraits", 5);
	for (i = 0; i < MAX_PORTRAITS_TEXTURES; i++) {
		char buffer[256];
		safe_snprintf(buffer, sizeof(buffer), "textures/portraits%d.dds", i + 1);
		if (check_image_name(buffer, sizeof(buffer), buffer) != 0) {
			portraits_tex[i] = load_texture_cached(buffer, tt_gui);
		}
	}
	update_loading_win("init textures", 5);
	//Load the map legend and continent map
	legend_text = load_texture_cached("maps/legend.dds", tt_gui);
	ground_detail_text = load_texture_cached("textures/ground_detail.dds", tt_gui);
	CHECK_GL_ERRORS();
	init_login_screen();
	init_spells();
	update_loading_win(init_network_str, 5);
	if (SDLNet_Init() < 0) {
		LOG_ERROR("%s: %s\n", failed_sdl_net_init, SDLNet_GetError());
		fprintf(stderr, "%s: %s\n", failed_sdl_net_init, SDLNet_GetError());
		SDLNet_Quit();
		SDL_Quit();
		exit(2);
	}
	update_loading_win(init_timers_str, 5);
	if (SDL_InitSubSystem(SDL_INIT_TIMER) < 0) {
		LOG_ERROR("%s: %s\n", failed_sdl_timer_init, SDL_GetError());
		fprintf(stderr, "%s: %s\n", failed_sdl_timer_init, SDL_GetError());
		SDL_Quit();
		exit(1);
	}
	update_loading_win(load_encyc_str, 5);
	safe_snprintf(file_name, sizeof(file_name), "languages/%s/Encyclopedia/index.xml", lang);
	ReadXML(file_name);
	read_key_config();
	init_buddy();
	init_channel_names();
	if (auto_update) {
		init_update();
	}
	have_rules = read_rules();
	if (!have_rules) {
		LOG_ERROR(rules_not_found);
		fprintf(stderr, "%s\n", rules_not_found);
		SDL_Quit();
		exit(3);
	}
	//initiate function pointers
	init_statsinfo_array();
	init_livres();
	update_loading_win(init_display_str, 5);
	if (!disable_gamma_adjust) {
		SDL_SetGamma(gamma_var, gamma_var, gamma_var);
	}
	draw_scene_timer = SDL_AddTimer(1000 / (18 * 4), my_timer, NULL);
	misc_timer = SDL_AddTimer(500, check_misc, NULL);
	safe_snprintf(config_location, sizeof(config_location), datadir_location_str, datadir);
	LOG_TO_CONSOLE(c_green4, config_location);
	cfgdir = get_path_config();
	if (cfgdir != NULL) {
		//Realistically, if this failed, then there's not much point in continuing, but oh well...
		safe_snprintf(config_location, sizeof(config_location), config_location_str, cfgdir);
		LOG_TO_CONSOLE(c_green4, config_location);
	}
	update_loading_win(prep_op_win_str, 7);
	create_opening_root_window(window_width, window_height);
	// initialize the chat window
	if (use_windowed_chat == 2) {
		display_chat();
	}
	init_commands("commands.lst");
	init_text_aliases();
	// Try to turn the sound on now so we have it for the login window
	if (have_sound_config) {
		turn_sound_on();
	} else {
		sound_on = 0;
		turn_sound_off();
	}
	// display something
	destroy_loading_win();
	if (has_accepted) {
		show_window(opening_root_win);
		connect_to_server();
	} else {
		create_rules_root_window(window_width, window_height, opening_root_win, 15);
		show_window(rules_root_win);
	}
	if (use_frame_buffer) {
		make_reflection_framebuffer(window_width, window_height);
	}
	skybox_init_gl();
	popup_init();
	LOG_DEBUG("Init done!");
	LOG_TO_CONSOLE(c_yellow2, "ATTENTION: Ceci est une version du programme non officielle, fournie pour tester.");
	LOG_TO_CONSOLE(c_red2, "   ***   Il n'y a aucune garantie de stabilité ou exactitude.   ***");
	LOG_TO_CONSOLE(c_grey2, "En cas de problème ou si vous avez des questions, faites signaler sur le fil correspondant du forum, ou bien sur: https://github.com/vancolbert/claet");
}
