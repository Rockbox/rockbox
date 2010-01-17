/*
 * Various status thingies for the interpreter and interface.
 *
 */

typedef struct frotz_setup_struct {
	int attribute_assignment;	/* done */
	int attribute_testing;		/* done */
	int context_lines;		/* done */
	int object_locating;		/* done */
	int object_movement;		/* done */
	int left_margin;		/* done */
	int right_margin;		/* done */
	int ignore_errors;		/* done */
	int interpreter_number;		/* Just dumb frotz now */
	int piracy;			/* done */
	int undo_slots;			/* done */
	int expand_abbreviations;	/* done */
	int script_cols;		/* done */
	int save_quetzal;		/* done */
	int sound;			/* done */
	int err_report_mode;		/* done */
} f_setup_t;

extern f_setup_t f_setup;


typedef struct zcode_header_struct {
	zbyte h_version;
	zbyte h_config;
	zword h_release;
	zword h_resident_size;
	zword h_start_pc;
	zword h_dictionary;
	zword h_objects;
	zword h_globals;
	zword h_dynamic_size;
	zword h_flags;
	zbyte h_serial[6];
	zword h_abbreviations;
	zword h_file_size;
	zword h_checksum;
	zbyte h_interpreter_number;
	zbyte h_interpreter_version;
	zbyte h_screen_rows;
	zbyte h_screen_cols;
	zword h_screen_width;
	zword h_screen_height;
	zbyte h_font_height;
	zbyte h_font_width;
	zword h_functions_offset;
	zword h_strings_offset;
	zbyte h_default_background;
	zbyte h_default_foreground;
	zword h_terminating_keys;
	zword h_line_width;
	zbyte h_standard_high;
	zbyte h_standard_low;
	zword h_alphabet;
	zword h_extension_table;
	zbyte h_user_name[8];

	zword hx_table_size;
	zword hx_mouse_x;
	zword hx_mouse_y;
	zword hx_unicode_table;
} z_header_t;
