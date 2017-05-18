
#include "classes.h"

// Global variables
const char *CONFIG_FILE; /*!< Configuration file path name*/
int VERBOSE = 0; /*!< verbose output*/

// Some helper functions
void fetch_edid(xcb_randr_output_t *,screen_class *,char **edid_string);
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
