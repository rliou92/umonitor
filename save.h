#include "classes.h"



void save_profile_class_constructor(save_profile_class *,screen_class *,
  config_t *);

// Public methods
static void save_profile(save_profile_class *,config_setting_t *);

// Private methods
static void output_info_to_config(save_profile_class *);
static void find_res_to_config(void *,xcb_randr_mode_t *);
static void disabled_to_config(save_profile_class *);
static void check_output_status(void *,xcb_randr_output_t *);

// Extern functions
extern void for_each_output(
	void *self,
	xcb_randr_get_screen_resources_reply_t *screen_resources_reply,
	void (*callback)(void *,xcb_randr_output_t *)
);
extern void for_each_output_mode(
  void *self,
  xcb_randr_get_output_info_reply_t *output_info_reply,
	void (*callback)(void *,xcb_randr_mode_t *)
);
extern void fetch_edid(xcb_randr_output_t *,screen_class *,char **edid_string);

extern const char *const CONFIG_FILE;
extern int VERBOSE;
