// TODO some of these includes are not needed

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <libconfig.h>
#include <unistd.h>
#include <X11/extensions/Xrandr.h>
#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <stdarg.h>


typedef struct _screen_class{
  xcb_connection_t *c;
  xcb_generic_error_t **e;
  xcb_screen_t *screen;
  xcb_intern_atom_reply_t *edid_atom;
  xcb_randr_get_screen_resources_reply_t *screen_resources_reply;

  void (*update_screen)(struct _screen_class*);


}screen_class;

typedef struct _set_crtc_param{
  xcb_randr_crtc_t *crtc_p;
  int pos_x,pos_y;
  xcb_randr_mode_t *mode_id_p;
  xcb_randr_output_t *output_p;
  struct _set_crtc_param *next;

}set_crtc_param;

/*
 * Structures for loading and saving the configuration file
 */

typedef struct {
	const char **edid_val;
	int *pos_x,*pos_y,*width,*height,*widthMM,*heightMM,*res_x,*res_y;
}umon_setting_val_t;



typedef struct {
	config_setting_t *edid,*res_group,*res_x,*res_y,*pos_x,
		*pos_y,*disp_group,*disp_width,*disp_height,
		*disp_widthMM,*disp_heightMM,*mon_group,*output_group,
		*pos_group,*status,*edid_setting;
}umon_setting_t;

typedef struct _save_profile_class{
  // Inheriting classes
  screen_class *screen_t_p;

  // Variables
  umon_setting_t umon_setting;
  xcb_randr_get_crtc_info_reply_t *crtc_info_reply;
  xcb_randr_output_t *cur_output;
  config_t *config;

  // Methods
  void (*save_profile)(struct _save_profile_class *,config_setting_t *);
}save_profile_class;

typedef struct _load_class{
  // Inheriting classes
  screen_class *screen_t_p;

  // Variables
  // TODO some might be unused
  umon_setting_val_t umon_setting_val;
  xcb_randr_get_crtc_info_reply_t *crtc_info_reply;
  xcb_randr_output_t *cur_output;
  config_t *config;
  int conf_output_idx,num_out_pp;
  xcb_randr_get_output_info_reply_t *output_info_reply;
  set_crtc_param *crtc_param_head;

  // Methods
  void (*load_profile)(struct _load_class *,config_setting_t *);
}load_class;
