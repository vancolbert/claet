/*
 * User can store current inventory contents into a named list. When you
 * are near an open storage, you can fetch the listed items out of
 * storage directly from the window.  The window also provides a way
 * to record inventory configurations for set tasks such
 * as making steel bars at a mine. It was hoped that you would be able
 * to fetch the complete list of items in one go just by selecting the
 * list from the menu.  However, due to concerns with macroing, this may
 * never be allowed.
 *
 * Author bluap/pjbroad December 2009, new integrated names list April 2011
 *
 * TODO New features
 * 		...
 */


#include <assert.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

//needed for notepad.h
#include <SDL.h>
#include "text.h"
#include "notepad.h"

#include "asc.h"
#include "context_menu.h"
#include "elconfig.h"
#include "errors.h"
#include "font.h"
#include "gamewin.h"
#include "gl_init.h"
#include "hud.h"
#include "init.h"
#include "io/elpathwrapper.h"
#include "items.h"
#include "item_info.h"
#include "item_lists.h"
#include "questlog.h"
#include "sound.h"
#include "storage.h"
#include "translate.h"


namespace ItemLists
{
	static void quantity_input_handler(const char *input_text, void *);
	static int display_itemlist_handler(window_info *win);
	static int click_itemlist_handler(window_info *win, int mx, int my, Uint32 flags);
	static int mouseover_itemlist_handler(window_info *win, int mx, int my);
	static int hide_itemlist_handler(window_info *win);
	static int keypress_itemlist_handler(window_info *win, int mx, int my, Uint32 key, Uint32 unikey);
	static int resize_itemlist_handler(window_info *win, int new_width, int new_height);
	static void new_list_handler(const char *input_text, void *data);
	static void rename_list_handler(const char *input_text, void *data);
	static int cm_selected_item_handler(window_info *win, int widget_id, int mx, int my, int option);
	static int cm_names_handler(window_info *win, int widget_id, int mx, int my, int option);
	static void cm_names_pre_show_handler(window_info *win, int widget_id, int mx, int my, window_info *cm_win);

	//
	//	Read a line from a file with any training "\r" removed.
	//	Perhaps some evil windows interaction put it there.....
	//
	std::istream& getline_nocr( std::istream& is, std::string& str )
	{
		std::istream &res = std::getline(is, str);
		if (!str.empty() && str[str.size() - 1] == '\r')
			str.erase(str.size() - 1);
		return res;
	}

	//	A class for an individual item list.
	//
	class List
	{
		public:
			List(void) : format_error(false) {}
			bool set(std::string save_name);
			const std::string & get_name(void) const { return name; }
			void set_name(const char *new_name) { name = new_name; }
			size_t get_num_items(void) const { return image_ids.size(); }
			int get_image_id(size_t i) const { assert(i<image_ids.size()); return image_ids[i]; }
			Uint16 get_item_id(size_t i) const { assert(i<item_ids.size()); return item_ids[i]; }
			int get_quantity(size_t i) const  { assert(i<quantities.size()); return quantities[i]; }
			void set_quantity(size_t item, int quantity) { assert(item<quantities.size()); quantities[item] = quantity; }
			void write(std::ostream & out) const;
			bool read(std::istream & in);
			void del(size_t item_index);
			void add(size_t over_item_number, int image_id, Uint16 id, int quantity);
			bool is_valid_format(void) const { return format_error; }
		private:
			std::string name;
			std::vector<int> image_ids;
			std::vector<int> quantities;
			std::vector<Uint16> item_ids;
			bool format_error;
	};


	//	Class to contain a collection of item lists.
	//
	class List_Container
	{
		public:
			List_Container(void) : active_list(0), initial_active_list(0), last_mod_time(0), loaded(false) {}
			void load(void);
			void save(void);
			bool add(const char *name);
			void del(size_t list_index);
			void rename_active(const char * name);
			void select_by_name(const char *name);
			void find_next_matching(const char *filter);
			size_t get_active(void) const { return active_list; }
			size_t size(void) const { return saved_item_lists.size(); }
			bool valid_active_list(void) const { return active_list < size(); }
			void active_next_list(void) { if (active_list + 1 < size()) active_list++; }
			void active_previous_list(void) { if (active_list > 0) active_list--; }
			const std::vector<List> & get_lists(void) const { return saved_item_lists; }
			bool set_active(size_t new_active_list)
				{ if (new_active_list >= size()) return false; active_list = new_active_list; return true; }
			void set_initial_active(size_t list_index) { initial_active_list = list_index; }
			void set_quantity(size_t item, int quantity)
				{ assert(valid_active_list()); last_mod_time = SDL_GetTicks(); return saved_item_lists[active_list].set_quantity(item, quantity); }
			void del_item(size_t i)
				{ assert(valid_active_list()); saved_item_lists[active_list].del(i); last_mod_time = SDL_GetTicks(); }
			void add_item(size_t over_item_number, int image_id, Uint16 id, int quantity)
				{ assert(valid_active_list()); saved_item_lists[active_list].add(over_item_number, image_id, id, quantity); last_mod_time = SDL_GetTicks(); }
			const List & get_list(void) const
				{ assert(valid_active_list()); return saved_item_lists[active_list]; }
			void sort_list(void)
				{ std::sort( saved_item_lists.begin(), saved_item_lists.end(), List_Container::sort_compare); };
			void check_and_timed_save(bool force);
		private:
			std::vector<List> saved_item_lists;
			static int FILE_REVISION;
			size_t active_list;
			size_t initial_active_list;
			Uint32 last_mod_time;
			bool loaded;
			static const char * filename;
			static bool sort_compare(const List &a, const List &b);
	};


	//	Simple wrapper around the quantity input dialogue
	//
	class Quantity_Input
	{
		public:
			Quantity_Input(void) { init_ipu(&ipu, -1, 200, -1, 10, 1, NULL, quantity_input_handler); }
			size_t get_list(void) const { return list; };
			size_t get_item(void) const { return item; };
			void open(int parent_id, int mx, int my, size_t list, size_t item);
			~Quantity_Input(void) { close_ipu(&ipu); }
			void close(void) { if (get_show_window(ipu.popup_win)) clear_popup_window(&ipu); }
		private:
			INPUT_POPUP ipu;
			size_t list;
			size_t item;
	};


	//	Store and lookup categories for objects.
	//
	class Category_Maps
	{
		public:
			Category_Maps(void) : must_save(false), loaded(false) {}
			void update(int image_id, Uint16 item_id, int cat_id);
			bool have_image_id(int image_id) const
				{ return cat_by_image_id.find(image_id) != cat_by_image_id.end(); }
			bool have_item_id(Uint16 item_id) const
				{ return cat_by_item_id.find(item_id) != cat_by_item_id.end(); }
			int get_cat(int image_id, Uint16 item_id);
			void save(void);
			void load(void);
		private:
			static const char *filename;
			std::map<int, int> cat_by_image_id;
			std::map<Uint16, int> cat_by_item_id;
			bool must_save;
			bool loaded;
			struct IDS { public: std::vector<int> images; std::vector<Uint16> items; };
	};


	// Class for the list window
	//
	class List_Window
	{
		public:
			List_Window(void) :
				cm_selected_item_menu(CM_INIT_VALUE), cm_names_menu(CM_INIT_VALUE),
				num_show_names_list(6), names_list_height(SMALL_FONT_Y_LEN),
				win_id(-1), selected_item_number(static_cast<size_t>(-1)),
				name_under_mouse(static_cast<size_t>(-1)), clicked(false),
#ifndef WITHDRAW_LIST
				mouse_over_add_button(false), resizing(false),
#else //WITHDRAW_LIST
                mouse_over_add_button(false), mouse_over_get_button(false),
                storage_moove_cat_called(false), last_time_storage_change(0), resizing(false),
#endif //WITHDRAW_LIST
				last_quantity_selected(0), num_grid_rows(min_grid_rows()),
				last_key_time(0), last_items_list_on_left(-1), desc_str(0),
				pickup_fail_time(0) {}
			int get_id(void) const { return win_id; }
			size_t get_grid_cm(void) const { return cm_selected_item_menu; }
			static int get_grid_size(void) { return 33; };
			static int get_list_gap(void) { return 3; };
			static int min_grid_rows(void) { return 3; };
			void show(window_info *win);
			int draw(window_info *win);
#ifdef WITHDRAW_LIST
            void process_withdraw_list(int last_item);
#endif //WITHDRAW_LIST
			void new_or_rename_list(bool is_new);
			int mouseover(window_info *win, int mx, int my);
			int click(window_info *win, int mx, int my, Uint32 flags);
			size_t get_item_number(int mx, int my);
			void restore_inventory_quantity(void);
			void update_scroll_len(void);
			void reset_position(void);
			void make_active_visable(void);
			void cm_names_pre_show(void);
			int keypress(char the_key);
			void resized_name_panel(window_info *win);
			void reset_pickup_fail_time(void) { pickup_fail_time = SDL_GetTicks(); }
		private:
			void calc_num_show_names(int win_len_y);
			int get_window_pos_x(window_info *parent_win) const;
			int get_size_x(void) const { return get_grid_size()*6 + ELW_BOX_SIZE + get_list_gap(); }
			int get_size_y(void) const { return get_grid_size()*num_grid_rows + get_names_size_y(); }
			int get_names_size_y(void) const
				{ return static_cast<int>(num_show_names_list * (get_list_gap() + names_list_height) + get_list_gap()); }
			size_t cm_selected_item_menu;
			size_t cm_names_menu;
			int num_show_names_list;
			float names_list_height;
			int win_id;
			size_t selected_item_number;
			size_t name_under_mouse;
			bool clicked;
			bool mouse_over_add_button;
			int add_button_x;
			int add_button_y;
#ifdef WITHDRAW_LIST
            List withdraw_list_item;
            bool mouse_over_get_button;
            bool storage_moove_cat_called;
            Uint32 last_time_storage_change;
#endif //WITHDRAW_LIST
			bool resizing;
			int last_quantity_selected;
			INPUT_POPUP ipu_item_list_name;
			int names_scroll_id;
			std::vector<const char *>help_str;
			int num_grid_rows;
			char filter[20];
			Uint32 last_key_time;
			int last_items_list_on_left;
			const char *desc_str;
			Uint32 pickup_fail_time;
	};


	//	Class for static objects to avoid destructor issues
	//  Will be created after and destructed before global statics
	class Vars
	{
		public:
			static Quantity_Input * quantity_input(void)
				{ static Quantity_Input qi; return &qi; }
			static Category_Maps * cat_maps(void)
				{ static Category_Maps cm; return &cm; }
			static List_Container * lists(void)
				{ static List_Container lc; return &lc; }
			static List_Window * win(void)
				{ static List_Window lw; return &lw; }
	};


	// Set the name, ids and quantities for a new list.
	// If there is nothing in the inventory, then return
	// value is false, the caller should delete the object.
	//
	bool List::set(std::string save_name)
	{
		name = save_name;
		for (size_t i=0; i<ITEM_NUM_ITEMS-ITEM_NUM_WEAR; i++)
			if (item_list[i].quantity > 0)
			{
				bool stacked_item = false;
				if (item_list[i].id != unset_item_uid)
					for (size_t j=0; j<item_ids.size(); j++)
						if (item_list[i].id == item_ids[j])
						{
							quantities[j]++;
							stacked_item = true;
							break;
						}
				if (!stacked_item)
				{
					image_ids.push_back(item_list[i].image_id);
					item_ids.push_back(item_list[i].id);
					quantities.push_back(item_list[i].quantity);
				}
			}
		if (quantities.empty())
			return false;
		return true;
	}


	//	Write the list to the specified stream.
	//  The name, ids and quantities are on separate lines.
	//
	void List::write(std::ostream & out) const
	{
		out << name << std::endl;
		for (size_t i=0; out && i<image_ids.size(); i++)
			out << image_ids[i] << " ";
		out << std::endl;
		for (size_t i=0; out && i<quantities.size(); i++)
			out << quantities[i] << " ";
		out << std::endl;
		for (size_t i=0; out && i<item_ids.size(); i++)
			out << item_ids[i] << " ";
		out << std::endl;
	}


	//	Read an itemlist from the specified input stream
	//	If an error occurs the function will return false;
	//	the caller should delete the object.
	//
	bool List::read(std::istream & in)
	{
		std::string name_line, image_id_line, cnt_line, item_uid_line;

		// each part is on a separate line, but allow empty lines
		while (getline_nocr(in, name_line) && name_line.empty());
		while (getline_nocr(in, image_id_line) && image_id_line.empty());
		while (getline_nocr(in, cnt_line) && cnt_line.empty());
		while (getline_nocr(in, item_uid_line) && item_uid_line.empty());

		// mop up extra lines at the end of the file silently
		if (name_line.empty())
			return false;

		// a name without data is not a list!
		if (image_id_line.empty() || cnt_line.empty() || item_uid_line.empty())
		{
			LOG_ERROR("%s: %s [%s]\n", __FILE__, item_list_format_error, name_line.c_str() );
			format_error = true;
			return false;
		}

		// read each image id value
		std::istringstream ss(image_id_line);
		int value = 0;
		while (ss >> value)
			image_ids.push_back(value);

		// read each quantity value
		ss.clear();
		ss.str(cnt_line);
		value = 0;
		while (ss >> value)
			quantities.push_back(value);

		// read each item uid value
		ss.clear();
		ss.str(item_uid_line);
		Uint16 ui_value = 0;
		while (ss >> ui_value)
			item_ids.push_back(ui_value);

		// don't use a list with unequal or empty data sets
		if ((quantities.size() != image_ids.size()) || (quantities.size() != item_ids.size()) || quantities.empty())
		{
			LOG_ERROR("%s: %s name=[%s] #id=%d #cnts=%d #uid=%d\n", __FILE__, item_list_format_error, name_line.c_str(), image_ids.size(), quantities.size(), item_ids.size() );
			format_error = true;
			return false;
		}

		// read list just fine
		name = name_line;
		return true;
	}


	// Delete the specified item from the list
	//
	void List::del(size_t item_index)
	{
		assert(item_index<quantities.size());
		image_ids.erase( image_ids.begin()+item_index );
		quantities.erase( quantities.begin()+item_index );
		item_ids.erase( item_ids.begin()+item_index );
	}


	//	Add a new item to a list or increase quantity if already in the list
	void List::add(size_t over_item_number, int image_id, Uint16 id, int quantity)
	{
		do_drop_item_sound();

		// Add to quanity if already in list

		// if new item has proper ID, try to use it
		if (id != unset_item_uid)
		{
			// match only against items with the same id
			for (size_t i=0; i<quantities.size(); ++i)
				if (item_ids[i] == id)
				{
					quantities[i] += quantity;
					return;
				}
			// or with items that don't have a proper id but do match the image
			for (size_t i=0; i<quantities.size(); ++i)
				if ((item_ids[i] == unset_item_uid) && (image_ids[i] == image_id))
				{
					quantities[i] += quantity;
					return;
				}
		}
		// if the new items does not have a proper id, just look for a matching image id
		else
		{
			for (size_t i=0; i<quantities.size(); ++i)
				if (image_ids[i] == image_id)
				{
					quantities[i] += quantity;
					return;
				}
		}

		// otherwise insert the new item or add it to the end

		if (over_item_number < get_num_items())
		{
			image_ids.insert(image_ids.begin()+over_item_number, image_id);
			item_ids.insert(item_ids.begin()+over_item_number, id);
			quantities.insert(quantities.begin()+over_item_number, quantity);
		}
		else
		{
			image_ids.push_back(image_id);
			item_ids.push_back(id);
			quantities.push_back(quantity);
		}
	}


	const char * Category_Maps::filename = "item_categories.txt";

	//	If we don't already have the objects category, store it now.
	//
	void Category_Maps::update(int image_id, Uint16 item_id, int cat_id)
	{
		if (!loaded)
			load();
		if ((item_id != unset_item_uid) && !have_item_id(item_id))
		{
			//std::cout << "Storing item id " << item_id << " cat " << cat_id << std::endl;
			cat_by_item_id[item_id] = cat_id;
			must_save = true;
		}
		if (!have_image_id(image_id))
		{
			//std::cout << "Storing image id " << image_id << " cat " << cat_id << std::endl;
			cat_by_image_id[image_id] = cat_id;
			must_save = true;
		}
	}


	//	Return the category for an item/image id, or -1 for not found.
	//
	int Category_Maps::get_cat(int image_id, Uint16 item_id)
	{
		if ((item_id != unset_item_uid) && have_item_id(item_id))
		{
			//std::cout << "Retrieving by item id " << item_id << std::endl;
			return cat_by_item_id[item_id];
		}
		else if (have_image_id(image_id))
		{
			//std::cout << "Retrieving by image id " << image_id << std::endl;
			return cat_by_image_id[image_id];
		}
		else
			return -1;
	}


	//	Save the object/category mappings.
	//	Grouped for each category to save space and allow easy image/item id mixing
	//
	void Category_Maps::save(void)
	{
		if (!must_save)
			return;

		std::string fullpath = get_path_config() + std::string(filename);
		std::ofstream out(fullpath.c_str());
		if (!out)
		{
			LOG_ERROR("%s: %s [%s]\n", __FILE__, item_list_save_error_str, fullpath.c_str() );
			LOG_TO_CONSOLE(c_red2, item_list_save_error_str);
			return;
		}

		// store the ids grouped by category
		std::map<int,IDS> ids_in_cat;
		for (std::map<int,int>::const_iterator i=cat_by_image_id.begin(); i!=cat_by_image_id.end(); ++i)
			ids_in_cat[i->second].images.push_back(i->first);
		for (std::map<Uint16,int>::const_iterator i=cat_by_item_id.begin(); i!=cat_by_item_id.end(); ++i)
			ids_in_cat[i->second].items.push_back(i->first);

		// step though each category, writing ids
		for (std::map<int,IDS>::const_iterator i=ids_in_cat.begin(); i!=ids_in_cat.end(); ++i)
		{
			// write category id
			out << i->first << std::endl;
			// write the number of image ids, then the values
			out << i->second.images.size() << " ";
			for (std::vector<int>::const_iterator j=i->second.images.begin(); j!=i->second.images.end(); ++j)
				out << *j << " ";
			out << std::endl;
			// write the number of item ids, then the values
			out << i->second.items.size() << " ";
			for (std::vector<Uint16>::const_iterator j=i->second.items.begin(); j!=i->second.items.end(); ++j)
				out << *j << " ";
			out << std::endl << std::endl;
		}

		must_save = false;
	}


	//	Load the object/category mappings.
	//
	void Category_Maps::load(void)
	{
		loaded = true;
		cat_by_image_id.clear();
		cat_by_item_id.clear();

		std::string fullpath = get_path_config() + std::string(filename);
		std::ifstream in(fullpath.c_str());
		if (!in)
			return;

		while (!in.eof())
		{
			// read the info, image_id and item_id lines
			std::string info_line, image_id_line, item_id_line;
			while (getline_nocr(in, info_line) && info_line.empty());
			getline_nocr(in, image_id_line);
			getline_nocr(in, item_id_line);
			if (info_line.empty())
				break;

			// read the category
			std::istringstream ss(info_line);
			int category = -1;
			ss >> category;

			// read and count the image id values and store in the map
			ss.clear();
			ss.str(image_id_line);
			int value = 0;
			int actual_num_image_ids = 0;
			int expected_num_image_ids = 0;
			ss >> expected_num_image_ids;
			while (ss >> value)
			{
				cat_by_image_id[value] = category;
				actual_num_image_ids++;
			}

			// read and count the item id values and store in the map
			ss.clear();
			ss.str(item_id_line);
			Uint16 ui_value = 0;
			int actual_num_item_ids = 0;
			int expected_num_item_ids = 0;
			ss >> expected_num_item_ids;
			while (ss >> ui_value)
			{
				cat_by_item_id[ui_value] = category;
				actual_num_item_ids++;
			}

			// check for format errors and end now if something detected
			if ((category<0) || (actual_num_image_ids != expected_num_image_ids) || (actual_num_item_ids != expected_num_item_ids))
			{
				LOG_TO_CONSOLE(c_red2, item_list_cat_format_error_str);
				LOG_ERROR("%s: %s cat=%d expected/actual image=%d/%d item %d/%d\n",
					__FILE__, item_list_cat_format_error_str, category,
					expected_num_image_ids, actual_num_image_ids,
					expected_num_item_ids, actual_num_item_ids );
				break;
			}
		}

		must_save = false;
	}


	//	Open the input quantity window
	//
	void Quantity_Input::open(int parent_id, int mx, int my, size_t list, size_t item)
	{
		this->list = list;
		this->item = item;
		ipu.x = mx;
		ipu.y = my;
		ipu.parent = parent_id;
		ipu.data = static_cast<void *>(this);
		display_popup_win(&ipu, item_list_quantity_str);
	}


	//	Once a new quantity has been entered, set the value in the list
	//
	static void quantity_input_handler(const char *input_text, void *data)
	{
		assert(data != NULL);
		Quantity_Input *input = static_cast<Quantity_Input *>(data);
		if ((Vars::lists()->get_active() != input->get_list()) ||
			(input->get_item() >= Vars::lists()->get_list().get_num_items()))
			return;
		int quantity;
		std::istringstream ss(input_text);
		ss >> quantity;
		if (quantity > 0)
			Vars::lists()->set_quantity(input->get_item(), quantity);
	}


	int List_Container::FILE_REVISION = 2;
	const char * List_Container::filename = "item_lists.txt";

	//  Save the item lists to a file in players config directory
	//
	void List_Container::save(void)
	{
		if (!loaded || saved_item_lists.empty())
			return;
		std::string fullpath = get_path_config() + std::string(filename);
		std::ofstream out(fullpath.c_str());
		if (!out)
		{
			LOG_ERROR("%s: %s [%s]\n", __FILE__, item_list_save_error_str, fullpath.c_str() );
			LOG_TO_CONSOLE(c_red2, item_list_save_error_str);
			return;
		}
		out << FILE_REVISION << std::endl << std::endl;
		for (size_t i=0; i<saved_item_lists.size(); ++i)
		{
			saved_item_lists[i].write(out);
			out << std::endl;
		}
		out.close();
		last_mod_time = 0;
	}


	//  Load the item lists from the file in players config directory
	//
	void List_Container::load(void)
	{
		loaded = true;
		saved_item_lists.clear();
		std::string fullpath = get_path_config() + std::string(filename);
		std::ifstream in(fullpath.c_str());
		if (!in)
			return;
		int revision;
		in >> revision;
		if (revision != FILE_REVISION)
		{
			LOG_ERROR("%s: %s [%s]\n", __FILE__, item_list_version_error_str, fullpath.c_str() );
			LOG_TO_CONSOLE(c_red2, item_list_version_error_str);
			return;
		}
		bool logged_error = false;
		while (!in.eof())
		{
			saved_item_lists.push_back(List());
			if (!saved_item_lists.back().read(in))
			{
				if ((saved_item_lists.back().is_valid_format()) && !logged_error)
				{
					LOG_TO_CONSOLE(c_red2, item_list_format_error);
					logged_error = true;
				}
				saved_item_lists.pop_back();
			}
		}
		in.close();
		sort_list();
		set_active(initial_active_list);
	}


	//	Add a new list
	//
	bool List_Container::add(const char *name)
	{
		saved_item_lists.push_back(List());
		if (saved_item_lists.back().set(name))
		{
			sort_list();
			save();
			select_by_name(name);
			return true;
		}
		saved_item_lists.pop_back();
		return false;
	}

	//	Find the first list with the specified name - best we can simply do
	//
	void List_Container::select_by_name(const char *name)
	{
		for (size_t i=0; i<saved_item_lists.size(); ++i)
			if (saved_item_lists[i].get_name() == std::string(name))
			{
				active_list = i;
				break;
			}
	}


	//	Find the next list after the current active who's name contains the
	//	filter string - then make it the active_list.
	//
	void List_Container::find_next_matching(const char *filter)
	{
		for (size_t i=active_list+1; i<active_list+size(); ++i)
		{
			size_t check = i % size();
			std::string lowercase(saved_item_lists[check].get_name());
			std::transform(lowercase.begin(), lowercase.end(), lowercase.begin(), tolower);
			if (lowercase.find(filter, 0) != std::string::npos)
			{
				active_list = check;
				break;
			}
		}
	}


	//	Delete the specified list
	//
	void List_Container::del(size_t list_index)
	{
		assert(list_index < size());
		saved_item_lists.erase(saved_item_lists.begin()+list_index);
		save();
		if (active_list && active_list >= size())
			active_list = size()-1;
	}


	//	Rename the active list
	//
	void List_Container::rename_active(const char *name)
	{
		if (active_list >= size())
			return;
		saved_item_lists[active_list].set_name(name);
		sort_list();
		select_by_name(name);
		save();
	}


	// Used by the sort algorithm to alphabetically compare two list names, case insensitive
	//
	bool List_Container::sort_compare(const List &a, const List &b)
	{
		std::string alower(a.get_name());
		std::transform(alower.begin(), alower.end(), alower.begin(), tolower);
		std::string blower(b.get_name());
		std::transform(blower.begin(), blower.end(), blower.begin(), tolower);
		return alower < blower;
	}

	// Check if we need to do a delayed save, then do if required
	//
	void List_Container::check_and_timed_save(bool force)
	{
		if (!last_mod_time)
			return;
		if (force || (last_mod_time && SDL_GetTicks() > last_mod_time + 5000))
			save();
	}


	//	Calculate the number of item list names show - depends on window height
	//	and number of grid rows shown
	//
	void List_Window::calc_num_show_names(int win_len_y = -1)
	{
		// get the actual window height if not specfified
		if (win_len_y < 0)
		{
			if ((win_id < 0) || (win_id >= windows_list.num_windows))
				return;
			win_len_y = (&windows_list.window[win_id])->len_y;
		}
		if (win_len_y > window_height-HUD_MARGIN_Y)
			win_len_y = window_height-HUD_MARGIN_Y;
		num_show_names_list = static_cast<int>
			((win_len_y - get_grid_size()*num_grid_rows) / (get_list_gap() + names_list_height));
	}


	//	The size available to the names list has changed so resize/move elements.
	//
	void List_Window::resized_name_panel(window_info *win)
	{
		calc_num_show_names();
		cm_remove_regions(win_id);
		cm_add_region(cm_names_menu, win_id, 0, get_size_y()-get_names_size_y(), get_size_y(), get_names_size_y());
		widget_resize(win_id, names_scroll_id, ELW_BOX_SIZE, get_names_size_y()-ELW_BOX_SIZE);
		widget_move(win_id, names_scroll_id, win->len_x-ELW_BOX_SIZE, get_grid_size()*num_grid_rows);
		make_active_visable();
		update_scroll_len();
	}


	//	Create the window or just toggle its open/closed state.
	//
	void List_Window::show(window_info *win)
	{
		if (win_id < 0 )
		{
			ItemLists::Vars::lists()->load();
			ItemLists::Vars::cat_maps()->load();
			filter[0] = '\0';

			calc_num_show_names(get_grid_size()*6+110);
			add_button_x = static_cast<int>(get_size_x() - DEFAULT_FONT_X_LEN*2);
			add_button_y = get_grid_size();

			win_id = create_window(item_list_preview_title, win->window_id, 0, get_window_pos_x(win), 0, get_size_x(), get_size_y(), ELW_WIN_DEFAULT|ELW_RESIZEABLE);
			set_window_handler(win_id, ELW_HANDLER_DISPLAY, (int (*)())&display_itemlist_handler );
			set_window_handler(win_id, ELW_HANDLER_CLICK, (int (*)())&click_itemlist_handler );
			set_window_handler(win_id, ELW_HANDLER_MOUSEOVER, (int (*)())&mouseover_itemlist_handler );
			set_window_handler(win_id, ELW_HANDLER_HIDE, (int (*)())&hide_itemlist_handler );
			set_window_handler(win_id, ELW_HANDLER_KEYPRESS, (int (*)())&keypress_itemlist_handler );
			set_window_handler(win_id, ELW_HANDLER_RESIZE, (int (*)())&resize_itemlist_handler );
			set_window_min_size(win_id, get_size_x(), get_size_y());

			cm_selected_item_menu = cm_create(cm_item_list_selected_str, cm_selected_item_handler);
			cm_names_menu = cm_create(cm_item_list_names_str, cm_names_handler);
			cm_set_pre_show_handler(cm_names_menu, cm_names_pre_show_handler);
			cm_add_region(cm_names_menu, win_id, 0, get_size_y()-get_names_size_y(), get_size_y(), get_names_size_y());

			names_scroll_id = vscrollbar_add_extended(win_id, 1, NULL,
				get_size_x()-ELW_BOX_SIZE, get_grid_size()*num_grid_rows, ELW_BOX_SIZE, get_names_size_y()-ELW_BOX_SIZE, 0,
				1.0, 0.77f, 0.57f, 0.39f, 0, 1, Vars::lists()->size()-num_show_names_list);

			init_ipu(&ipu_item_list_name, -1, -1, -1, 1, 1, NULL, NULL);

			make_active_visable();
		}
		else
		{
			toggle_window(win_id);
			make_active_visable();
			close_ipu(&ipu_item_list_name);
			Vars::quantity_input()->close();
		}
	}


	// Draw the item list window
	//
	int List_Window::draw(window_info *win)
	{
		Vars::lists()->check_and_timed_save(false);

		// if resizing wait until we stop
		if (win->resized)
			resizing = true;
		// once we stop, snap the window size to fix nicely
		else if (resizing)
		{
			calc_num_show_names(win->len_y);
			resizing = false;
			resize_window (win->window_id, get_size_x(), get_size_y());
		}

		// check if we need to change the number of grid rows shown
		int new_num_grid_rows = min_grid_rows();
		if (Vars::lists()->valid_active_list())
			new_num_grid_rows = std::max(static_cast<size_t>(new_num_grid_rows), (Vars::lists()->get_list().get_num_items() +5) / 6);
		if (num_grid_rows != new_num_grid_rows)
		{
			num_grid_rows = new_num_grid_rows;
			resized_name_panel(win);
		}

		// if the left/right position flag has changed, restore the window to its default location
		if (last_items_list_on_left != items_list_on_left)
		{
			if (last_items_list_on_left != -1)
				reset_position();
			last_items_list_on_left = items_list_on_left;
		}

		glEnable(GL_TEXTURE_2D);

		// draw the images
		if (Vars::lists()->valid_active_list())
		{
			glColor3f(1.0f,1.0f,1.0f);
			for(size_t i=0; i<Vars::lists()->get_list().get_num_items() && i<static_cast<size_t>(6*num_grid_rows); i++)
			{
				int x_start, y_start;
				x_start = get_grid_size() * (i%6) + 1;
				y_start = get_grid_size() * (i/6);
				draw_item(Vars::lists()->get_list().get_image_id(i), x_start, y_start, get_grid_size());
			}
		}

		size_t help_lines_shown = 0;

		if (desc_str)
		{
			show_help(desc_str, 0, static_cast<int>(0.5 + win->len_y + 10 + SMALL_FONT_Y_LEN * help_lines_shown++));
			desc_str = 0;
		}

		// Display any name search text
		if (strlen(filter))
		{
			if (SDL_GetTicks() > (last_key_time+5000))
			{
				filter[0] = '\0';
				last_key_time = 0;
			}
			else
			{
				std::string tmp = std::string(item_list_find_str) + std::string("[") + std::string(filter) + std::string("]");
				show_help(tmp.c_str(), 0, static_cast<int>(0.5 + win->len_y + 10 + SMALL_FONT_Y_LEN * help_lines_shown++));
			}
		}

		// draw mouse over window help text
		if (show_help_text)
		{
			if (!resizing)
				for (size_t i=0; i<help_str.size(); ++i)
					show_help(help_str[i], 0, static_cast<int>(0.5 + win->len_y + 10 + SMALL_FONT_Y_LEN * help_lines_shown++));
			help_str.clear();
		}

		glDisable(GL_TEXTURE_2D);

		// draw the item grid
		glColor3f(0.77f,0.57f,0.39f);
		rendergrid(6, num_grid_rows, 0, 0, get_grid_size(), get_grid_size());

		// if an object is selected, draw a green grid around it
		if (Vars::lists()->valid_active_list() && (quantities.selected == ITEM_EDIT_QUANT) && (selected_item_number < Vars::lists()->get_list().get_num_items()))
		{
			int x_start = selected_item_number%6 * get_grid_size();
			int y_start = static_cast<int>(selected_item_number/6) * get_grid_size();
			if ((SDL_GetTicks() - pickup_fail_time) < 250)
				glColor3f(0.8f,0.2f,0.2f);
			else
				glColor3f(0.0f, 1.0f, 0.3f);
			rendergrid(1, 1, x_start, y_start, get_grid_size(), get_grid_size());
			rendergrid(1, 1, x_start-1, y_start-1, get_grid_size()+2, get_grid_size()+2);
		}

		glEnable(GL_TEXTURE_2D);

		// draw the quantities over everything else so they always show
		if (Vars::lists()->valid_active_list())
		{
			glColor3f(1.0f,1.0f,1.0f);
			char str[80];
			for(size_t i=0; i<Vars::lists()->get_list().get_num_items() && i<static_cast<size_t>(6*num_grid_rows); i++)
			{
				int x_start, y_start, y_end;
				x_start = get_grid_size() * (i%6) + 1;
				y_start = get_grid_size() * (i/6);
				y_end = y_start + get_grid_size() - 1;
				safe_snprintf(str, sizeof(str), "%i", Vars::lists()->get_list().get_quantity(i));
				draw_string_small_shadowed(x_start, (i&1)?(y_end-15):(y_end-27), (unsigned char*)str, 1,1.0f,1.0f,1.0f, 0.0f, 0.0f, 0.0f);
			}
		}

#ifdef WITHDRAW_LIST
		// Drawn the new list button (+) with highlight when mouse over
		if (mouse_over_add_button)
			glColor3f(0.99f,0.77f,0.55f);
		else
			glColor3f(0.77f,0.57f,0.39f);
		draw_string_zoomed(add_button_x, add_button_y-17, (unsigned const char*)"+", 1, 2.0);

        // Drawn the new list button (>) with highlight when mouse over
        if (mouse_over_get_button)
            glColor3f(0.99f,0.77f,0.55f);
        else
            glColor3f(0.77f,0.57f,0.39f);
        draw_string_zoomed(add_button_x, add_button_y+17, (unsigned const char*)">", 1, 2.0);
#else
        // Drawn the new list button (+) with highlight when mouse over
		if (mouse_over_add_button)
			glColor3f(0.99f,0.77f,0.55f);
		else
			glColor3f(0.77f,0.57f,0.39f);
		draw_string_zoomed(add_button_x, add_button_y, (unsigned const char*)"+", 1, 2.0);

#endif //WITHDRAW_LIST

		// draw the item list names
		glColor3f(1.0f,1.0f,1.0f);
		int pos_y = get_grid_size()*num_grid_rows + get_list_gap();
		int num_shown = 0;
		const int top_entry = vscrollbar_get_pos (win_id, names_scroll_id);
		const std::vector<List> lists = Vars::lists()->get_lists();
		const int hl_width = static_cast<int>(win->len_x-ELW_BOX_SIZE-3);
		const int hl_height = static_cast<int>(names_list_height + get_list_gap());
		const size_t disp_chars = static_cast<size_t>((win->len_x-ELW_BOX_SIZE-2*get_list_gap()) / SMALL_FONT_X_LEN);
		for (size_t i = top_entry; i<lists.size() && num_shown<num_show_names_list; ++i)
		{
			if (i==Vars::lists()->get_active())
				draw_highlight(1, static_cast<int>(pos_y-get_list_gap()/2), hl_width, hl_height, 1);
			else if (i==name_under_mouse)
				draw_highlight(1, static_cast<int>(pos_y-get_list_gap()/2), hl_width, hl_height, 0);
			glColor3f(1.0f,1.0f,1.0f);
			if (lists[i].get_name().size() > disp_chars)
			{
				std::string todisp = lists[i].get_name().substr(0,disp_chars);
				draw_string_small(get_list_gap(), pos_y, reinterpret_cast<const unsigned char*>(todisp.c_str()), 1);
				if (i==name_under_mouse)
					show_help(lists[i].get_name().c_str(), 0, static_cast<int>(0.5 + win->len_y + 10 + SMALL_FONT_Y_LEN * help_lines_shown));
			}
			else
				draw_string_small(get_list_gap(), pos_y, reinterpret_cast<const unsigned char*>(lists[i].get_name().c_str()), 1);
			pos_y += static_cast<int>(names_list_height + get_list_gap());
			num_shown++;
		}

		if (clicked && (name_under_mouse < lists.size()))
		{
			do_click_sound();
			Vars::lists()->set_active(name_under_mouse);
		}

		if (clicked && mouse_over_add_button)
		{
			do_click_sound();
			new_or_rename_list(true);
		}
		name_under_mouse = static_cast<size_t>(-1);
#ifndef WITHDRAW_LIST
		mouse_over_add_button = clicked = false;
#else //WITHDRAW_LIST
        mouse_over_add_button = false;

        if (clicked && mouse_over_get_button && Vars::lists()->valid_active_list())
        {
            do_click_sound();
            if(Vars::lists()->get_list().get_num_items() > 0 && withdraw_list_item.get_num_items() <= 0)
            {
                //on copie la liste active dans une liste qui nous servira de buffer
                for(size_t i=0; i<Vars::lists()->get_list().get_num_items(); i++)
                {
                    withdraw_list_item.add(i,Vars::lists()->get_list().get_image_id(i),Vars::lists()->get_list().get_item_id(i),Vars::lists()->get_list().get_quantity(i));
                }

            }
        }

        if(withdraw_list_item.get_num_items() == 1)
            process_withdraw_list(1);
        else if(withdraw_list_item.get_num_items() > 0)
            process_withdraw_list(0);

        name_under_mouse = static_cast<size_t>(-1);
        mouse_over_get_button = clicked = false;
#endif //WITHDRAW_LIST

#ifdef OPENGL_TRACE
CHECK_GL_ERRORS();
#endif //OPENGL_TRACE
		return 1;
	}

#ifdef WITHDRAW_LIST

    // Sequences pour tranférer la liste d'objet dans l'inventaire
    // si la liste ne contient plus qu'un item last_item=1
    //
	void List_Window::process_withdraw_list(int last_item)
	{
	    Uint32 current_time = SDL_GetTicks();
	    //int min_time_between_withdraw;
        int qte_item = withdraw_list_item.get_quantity(0);
        //min_time_between_withdraw = 100;
        if(min_time_between_withdraw < 50) min_time_between_withdraw = 50;

        if(!storage_moove_cat_called && (current_time-min_time_between_withdraw)>last_time_storage_change){
            int image_id = withdraw_list_item.get_image_id(0);
            Uint16 item_id = withdraw_list_item.get_item_id(0);
            int cat_id = Vars::cat_maps()->get_cat(image_id, item_id);

            pickup_storage_item(image_id,item_id,cat_id);
            storage_moove_cat_called = true;
            last_time_storage_change = current_time;
        }else if(storage_moove_cat_called && (current_time-min_time_between_withdraw)>last_time_storage_change){

            if((current_time-min_time_between_withdraw)>pickup_fail_time)
                withdraw_active_storage_item(qte_item);

            withdraw_list_item.del(0);
            storage_moove_cat_called = false;
            last_time_storage_change = current_time;

            if(last_item == 1){
                storage_item_dragged = item_dragged = -1;
            }
	    }
	}
#endif //WITHDRAW_LIST

	//	Prompt for the new list name, starting the process of adding a new list
	//
	void List_Window::new_or_rename_list(bool is_new)
	{
		if ((win_id < 0) || (win_id >= windows_list.num_windows))
			return;
		window_info *win = &windows_list.window[win_id];
		close_ipu(&ipu_item_list_name);
		init_ipu(&ipu_item_list_name, win_id, 310, 100, 25, 1, NULL, (is_new) ?new_list_handler :rename_list_handler);
		ipu_item_list_name.x = (win->len_x - ipu_item_list_name.popup_x_len) / 2;
		ipu_item_list_name.y = (get_grid_size()*num_grid_rows - ipu_item_list_name.popup_y_len) / 2;
		display_popup_win(&ipu_item_list_name, (is_new) ?item_list_name_str : item_list_rename_str );
	}


	//	The mouse is over the window
	//
	int List_Window::mouseover(window_info *win, int mx, int my)
	{
		if ((my < 0) || (cm_window_shown()!=CM_INIT_VALUE))
			return 0;

		if (Vars::lists()->valid_active_list() &&
			mx>=0 && mx<(get_grid_size()*6) && my>=0 && my<(get_grid_size()*num_grid_rows))
		{
			size_t item_number = get_item_number(mx, my);
			if (item_number < Vars::lists()->get_list().get_num_items())
			{
				Uint16 item_id = Vars::lists()->get_list().get_item_id(item_number);
				int image_id = Vars::lists()->get_list().get_image_id(item_number);
				if (show_item_desc_text && item_info_available() && (get_item_count(item_id, image_id) == 1))
					desc_str = get_item_description(item_id, image_id);
				help_str.push_back(item_list_pickup_help_str);
				help_str.push_back(item_list_use_help_str);
				help_str.push_back(item_list_edit_help_str);
			}
			if ((storage_item_dragged != -1) || (item_dragged != -1))
				help_str.push_back(item_list_add_help_str);
			else
				help_str.push_back(item_list_drag_help_str);
		}

#ifdef WITHDRAW_LIST
		// check if over the add list button
		if (my>add_button_y-17 && my<(add_button_y-17+2*DEFAULT_FONT_Y_LEN) && mx>add_button_x && mx<win->len_x)
		{
			help_str.push_back(item_list_create_help_str);
			mouse_over_add_button = true;
		}

		// check if over the get list button
        if (my>add_button_y+17 && my<(add_button_y+17+2*DEFAULT_FONT_Y_LEN) && mx>add_button_x && mx<win->len_x)
        {
            //help_str.push_back(item_list_create_help_str);
            mouse_over_get_button = true;
        }
#else //WITHDRAW_LIST
		// check if over the add list button
		if (my>add_button_y && my<(add_button_y+2*DEFAULT_FONT_Y_LEN) && mx>add_button_x && mx<win->len_x)
		{
			help_str.push_back(item_list_create_help_str);
			mouse_over_add_button = true;
		}
#endif //WITHDRAW_LIST
		// check if over the list names and get which name
		int start_names = get_grid_size()*num_grid_rows;
		if ((my > start_names) && (my < (start_names+get_names_size_y())))
			name_under_mouse = vscrollbar_get_pos (win_id, names_scroll_id) +
				static_cast<int>((my - start_names - get_list_gap()/2) / (get_list_gap() + names_list_height));

		// name list context help
		if ((my > start_names) && (my < (start_names+get_names_size_y())))
		{
			help_str.push_back(cm_help_options_str);
			if (!strlen(filter))
				help_str.push_back(item_list_find_help_str);
		}

		return 0;
	}


	//	Get the item number of the object under the mouse
	//
	size_t List_Window::get_item_number(int mx, int my)
	{
		if (!Vars::lists()->valid_active_list())
			return 0;
		size_t num_items = Vars::lists()->get_list().get_num_items();
		if ((my >= get_grid_size()*num_grid_rows) || (mx >= get_grid_size()*6))
			return num_items;
		size_t list_index = 6 * static_cast<int>(my/get_grid_size()) + mx/get_grid_size();
		if (list_index < num_items)
			return list_index;
		return num_items;
	}

	//	Handle mouse clicks in the window
	//
	int List_Window::click(window_info *win, int mx, int my, Uint32 flags)
	{
		if (my < 0) // don't respond here to title bar being clicked
			return 0;

		if (flags & ELW_LEFT_MOUSE)
			clicked = true;

		if (!Vars::lists()->valid_active_list())
			return 1;

		// hide and clear any quantity input widow
		Vars::quantity_input()->close();

		size_t last_selected = selected_item_number;
		size_t num_items = Vars::lists()->get_list().get_num_items();
		bool was_dragging = ((storage_item_dragged != -1) || (item_dragged != -1));
		size_t over_item_number = Vars::win()->get_item_number(mx, my);

		// If dragging item and ctrl+left-click on window, add item to list
		if ((flags & ELW_LEFT_MOUSE) && (flags & ELW_CTRL) && was_dragging)
		{
			if (storage_item_dragged != -1)
				Vars::lists()->add_item(over_item_number, storage_items[storage_item_dragged].image_id, storage_items[storage_item_dragged].id, item_quantity);
			else if (item_dragged != -1)
				Vars::lists()->add_item(over_item_number, item_list[item_dragged].image_id, item_list[item_dragged].id, item_quantity);
			return 1;
		}

		// ctrl+right-click on a selected item opens the edit menu
		if ((flags & ELW_RIGHT_MOUSE) && (flags & ELW_CTRL) && (over_item_number<num_items))
		{
			cm_show_direct(Vars::win()->get_grid_cm(), win->window_id, -1);
			storage_item_dragged = item_dragged = -1;
			return 1;
		}

		restore_inventory_quantity();

		// wheel mouse up/down scrolls
		if ((flags & ELW_WHEEL_UP ) || (flags & ELW_WHEEL_DOWN ))
		{
			// change the active list
			if (my<get_grid_size()*num_grid_rows)
			{
				if (flags & ELW_WHEEL_UP)
					Vars::lists()->active_previous_list();
				else if (flags & ELW_WHEEL_DOWN)
					Vars::lists()->active_next_list();
				make_active_visable();
			}
			// scroll the names
			else
			{
				if (flags&ELW_WHEEL_UP)
					vscrollbar_scroll_up(win->window_id, names_scroll_id);
				else if(flags&ELW_WHEEL_DOWN)
					vscrollbar_scroll_down(win->window_id, names_scroll_id);
			}
			return 1;
		}

		// see if we can use the item quantity or take items from storage
		if ((flags & ELW_RIGHT_MOUSE) || (flags & ELW_LEFT_MOUSE))
		{
			if ((over_item_number!=last_selected) && (over_item_number < num_items))
			{
				selected_item_number = over_item_number;
				last_quantity_selected = quantities.selected;
				quantities.selected = ITEM_EDIT_QUANT;
				item_quantity = quantities.quantity[ITEM_EDIT_QUANT].val = Vars::lists()->get_list().get_quantity(selected_item_number);
				if (flags & ELW_RIGHT_MOUSE)
					do_click_sound();
				if (flags & ELW_LEFT_MOUSE)
				{
#ifdef ENGLISH
					// randomly close the window
					if (!(SDL_GetTicks() & 63))
					{
						hide_window(Vars::win()->get_id());
						set_shown_string(c_red2, item_list_magic_str);
						return 0;
					}
#endif //ENGLISH
					storage_item_dragged = item_dragged = -1;
					int image_id = Vars::lists()->get_list().get_image_id(selected_item_number);
					Uint16 item_id = Vars::lists()->get_list().get_item_id(selected_item_number);
					int cat_id = Vars::cat_maps()->get_cat(image_id, item_id);
					if (cat_id != -1)
						pickup_storage_item(image_id, item_id, cat_id);
					else
					{
						do_alert1_sound();
						reset_pickup_fail_time();
						static bool first_fail = true;
						if (first_fail)
						{
							first_fail = false;
							LOG_TO_CONSOLE(c_red1, item_list_learn_cat_str);
						}
					}
				}
			}
			else
				storage_item_dragged = item_dragged = -1;
		}

		return 1;
	}


	//	Switch back to a previous item quantity on the main window.
	//
	void List_Window::restore_inventory_quantity(void)
	{
		if (quantities.selected == ITEM_EDIT_QUANT)
		{
			quantities.selected = last_quantity_selected;
			item_quantity=quantities.quantity[quantities.selected].val;
		}
		selected_item_number = static_cast<size_t>(-1);
	}


	//	Move the names list to make the select list visible
	//
	void List_Window::make_active_visable(void)
	{
		const size_t top_entry = vscrollbar_get_pos (win_id, names_scroll_id);
		int new_pos = top_entry;
		if (Vars::lists()->get_active()<top_entry)
			new_pos = Vars::lists()->get_active();
		else if (Vars::lists()->get_active()>=(top_entry+num_show_names_list))
			new_pos = Vars::lists()->get_active() - (num_show_names_list - 1);
		else
			return;
		vscrollbar_set_pos(win_id, names_scroll_id, new_pos);
	}


	//	Recalculate the scroll bar length
	//
	void List_Window::update_scroll_len(void)
	{
		if (win_id>=0)
			vscrollbar_set_bar_len(win_id, names_scroll_id, Vars::lists()->size()-num_show_names_list);
	}


	//	Place to the preferred side.
	//
	int List_Window::get_window_pos_x(window_info *parent_win) const
	{
		if (!parent_win)
			return 0;
		if (items_list_on_left)
			return -5 - get_size_x();
		return parent_win->len_x + 5;
	}


	//	Move the window back to the default poistion
	//
	void List_Window::reset_position(void)
	{
		if ((win_id>=0) && (win_id<windows_list.num_windows))
		{
			window_info *list_win = &windows_list.window[win_id];
			if (list_win && (list_win->pos_id>=0) && (list_win->pos_id<windows_list.num_windows))
			{
				window_info *parent_win = &windows_list.window[list_win->pos_id];
				if (parent_win)
				{
					int pos_x = get_window_pos_x(parent_win);
					move_window(win_id, list_win->pos_id, list_win->pos_loc, parent_win->pos_x + pos_x, parent_win->pos_y);
				}
			}
		}
	}


	//	Key presses in the window used for a search string
	//
	int List_Window::keypress(char the_key)
	{
		last_key_time = SDL_GetTicks();
		if (the_key == SDLK_ESCAPE)
		{
			filter[0] = '\0';
			last_key_time = 0;
			return 1;
		}
		if (string_input(filter, sizeof(filter), the_key) || (the_key == SDLK_RETURN))
		{
			if (strlen(filter))
			{
				Vars::lists()->find_next_matching(filter);
				Vars::win()->make_active_visable();
			}
			return 1;
		}
		return 0;
	}


	//	Enable/disable names conext menu options
	//
	void List_Window::cm_names_pre_show(void)
	{
		int no_active = (Vars::lists()->valid_active_list()) ?0 :1;
		cm_grey_line(cm_names_menu, 1, no_active);
		cm_grey_line(cm_names_menu, 3, no_active);
	}


	//	Call the names context menu pre show handler
	//
	static void cm_names_pre_show_handler(window_info *win, int widget_id, int mx, int my, window_info *cm_win)
	{
		Vars::win()->cm_names_pre_show();
	}


	//	Item list names context menu option handler
	//
	static int cm_names_handler(window_info *win, int widget_id, int mx, int my, int option)
	{
		switch (option)
		{
			case 0:
				Vars::win()->new_or_rename_list(true);
				break;
			case 1:
				if (Vars::lists()->valid_active_list())
					Vars::win()->new_or_rename_list(false);
				break;
			case 3:
				if (Vars::lists()->valid_active_list())
				{
					Vars::lists()->del(Vars::lists()->get_active());
					ItemLists::Vars::win()->update_scroll_len();
				}
				break;
			case 5:
				Vars::lists()->load();
				ItemLists::Vars::win()->update_scroll_len();
				ItemLists::Vars::win()->make_active_visable();
				break;
			default:
				return 0;
		}
		return 1;
	}


	//	Selected item context menu option handler
	//
	static int cm_selected_item_handler(window_info *win, int widget_id, int mx, int my, int option)
	{
		size_t item_under_mouse = Vars::win()->get_item_number(mx, my);
		if (!Vars::lists()->valid_active_list() || (item_under_mouse>=Vars::lists()->get_list().get_num_items()))
			return 0;

		// edit the quanity
		if (option == 0)
		{
			Vars::quantity_input()->open(win->window_id, mx, my, Vars::lists()->get_active(), item_under_mouse);
			return 1;
		}

		// delete item, removing whole list if its now empty.  Save lists in any case.
#ifdef ENGLISH
		else if (option == 2)
#else //ENGLISH
		else if (option == 1)
#endif //ENGLISH
		{
			Vars::lists()->del_item(item_under_mouse);
			if (Vars::lists()->get_list().get_num_items()==0)
			{
				Vars::lists()->del(Vars::lists()->get_active());
				Vars::win()->update_scroll_len();
			}
			return 1;
		}

		return 0;
	}


	//	Enter name input callback - when OK selected
	//
	static void new_list_handler(const char *input_text, void *data)
	{
		// if sucessful update the displayed list else tell user it failed
		if (Vars::lists()->add(input_text))
		{
			Vars::win()->update_scroll_len();
			Vars::win()->make_active_visable();
		}
		else
			LOG_TO_CONSOLE(c_red1, item_list_empty_list_str);
	}


	//	Enter new name input callback - when OK selected
	//
	static void rename_list_handler(const char *input_text, void *data)
	{
		Vars::lists()->rename_active(input_text);
		Vars::win()->make_active_visable();
	}


	//  Draw the window
	//
	static int display_itemlist_handler(window_info *win)
	{
		return Vars::win()->draw(win);
	}


	//	Handle mouse clicks in the window
	//
	static int click_itemlist_handler(window_info *win, int mx, int my, Uint32 flags)
	{
		return Vars::win()->click(win, mx, my, flags);
	}


	//	Record mouse over the window so the draw handler can show help text
	//
	static int mouseover_itemlist_handler(window_info *win, int mx, int my)
	{
		return Vars::win()->mouseover(win, mx, my);
	}


	//  Called when the window is hidden, undo any quantity setting, do pending save.
	//
	static int hide_itemlist_handler(window_info *win)
	{
		Vars::win()->restore_inventory_quantity();
		Vars::lists()->check_and_timed_save(true);
		return 1;
	}

	static int keypress_itemlist_handler(window_info *win, int mx, int my, Uint32 key, Uint32 unikey)
	{
		char keychar = tolower(key_to_char(unikey));
		if ((keychar == '`') || (key & ELW_CTRL) || (key & ELW_ALT))
			return 0;
		return Vars::win()->keypress(keychar);
	}

	static int resize_itemlist_handler(window_info *win, int new_width, int new_height)
	{
		Vars::win()->resized_name_panel(win);
		return 0;
	}

} // end ItemLists namespace




//	Interface for the outside world
//
extern "C"
{
	void toggle_items_list_window(window_info *win)
		{ ItemLists::Vars::win()->show(win); }

	void update_category_maps(int image_id, Uint16 item_id, int cat_id)
		{ ItemLists::Vars::cat_maps()->update(image_id, item_id, cat_id); }

	void save_item_lists(void)
	{
		ItemLists::Vars::lists()->save();
		ItemLists::Vars::cat_maps()->save();
	}

	unsigned int item_lists_get_active(void)
	{
		return static_cast<unsigned int>(ItemLists::Vars::lists()->get_active());
	}

	void item_lists_set_active(unsigned int active_list)
	{
		ItemLists::Vars::lists()->set_initial_active(static_cast<size_t>(active_list));
	}

	void item_lists_reset_pickup_fail_time(void)
	{
		ItemLists::Vars::win()->reset_pickup_fail_time();
	}

#ifdef WITHDRAW_LIST
    int min_time_between_withdraw;
#endif //WITHDRAW_LIST
}
