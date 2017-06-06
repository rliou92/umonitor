
#include "classes.h"
#include <getopt.h>

// Global variables
const char *CONFIG_FILE; /*!< Configuration file path name*/

// Some helper functions
void fetch_edid(xcb_randr_output_t * output_p, screen_class * screen_t_p,
		char **edid_string);
void for_each_output(
	void *self,
	xcb_randr_get_screen_resources_reply_t *screen_resources_reply,
	void (*callback)(void *,xcb_randr_output_t *)
);
void for_each_output_mode(
  void *self,
  xcb_randr_get_output_info_reply_t *output_info_reply,
	void (*callback)(void *,xcb_randr_mode_t *)
);
void umon_print(const char*,...);

// private prototypes
static void set_argument_flags(void);
static void print_info(void);
static void print_current_state(void);
static void start_listening(void);
static void start_load(char *profile_name);
static void start_delete_and_save(save_or_delete_t save_or_delete, char *profile_name);
static void parse_arguments(void);

// extern
extern void screen_class_constructor(screen_class *);
extern void screen_class_destructor(screen_class *);

extern void load_class_constructor(load_class **,screen_class *);
extern void load_class_destructor(load_class *);

extern void save_class_constructor(save_class **,screen_class *,
	config_t *);
extern void save_class_destructor(save_class *);

extern void autoload_constructor(autoload_class **,screen_class *,config_t *);
extern void autoload_destructor(autoload_class *);

typedef enum {
	SAVE,
	DELETE
}save_or_delete_t;
