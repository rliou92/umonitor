#include "classes.h"

extern void for_each_output_mode(
	void *self,
	xcb_randr_get_output_info_reply_t *output_info_reply,
	void (*callback)(void *,xcb_randr_mode_t *)
);
extern void for_each_output(
	void *self,
	xcb_randr_get_screen_resources_reply_t *screen_resources_reply,
	void (*callback)(void *,xcb_randr_output_t *)
);

void load_class_constructor(load_class *self,
  screen_class *screen_t,config_t *config);
static void load_profile(load_class *self,config_setting_t *profile_group);
static void match_with_config(void *self_void,xcb_randr_output_t *output_p);
static void find_mode_id(load_class *self);
static void load_config_val(load_class *);
static xcb_randr_crtc_t find_available_crtc(load_class *,int);

extern void fetch_edid(xcb_randr_output_t *,screen_class *,char **edid_string);
extern int VERBOSE;
